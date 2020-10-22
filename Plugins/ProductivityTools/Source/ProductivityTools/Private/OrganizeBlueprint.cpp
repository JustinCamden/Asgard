// Copyright (c) 2019 Isara Technologies. All Rights Reserved.

#include "OrganizeBlueprint.h"
#include "ProductivityToolsSettings.h"

#include "K2Node_Event.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_Knot.h"
#include "EdGraphNode_Comment.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"

#include "Kismet2/BlueprintEditorUtils.h"
#include "ScopedTransaction.h"
#include "UnrealEd.h"

#define LOCTEXT_NAMESPACE "FProductivityToolsModule"

TMap<UBlueprint*, FBlueprintEditor*> FOrganizeBlueprint::BlueprintEditors = TMap<UBlueprint*, FBlueprintEditor*>();

TWeakPtr<SGraphEditor> FOrganizeBlueprint::GraphEditor = nullptr;

UEdGraph* FOrganizeBlueprint::CurrentGraph = nullptr;

TArray<FNodeInfos> FOrganizeBlueprint::NodesInfos = TArray<FNodeInfos>();
TArray<FPinInfos> FOrganizeBlueprint::PinsInfos = TArray<FPinInfos>();

TArray<FPinPair> FOrganizeBlueprint::PinPairsStraightened = TArray<FPinPair>();

TArray<UEdGraphNode*> FOrganizeBlueprint::GraphNodes = TArray<UEdGraphNode*>();
TArray<UEdGraphNode*> FOrganizeBlueprint::DiscoveredNodes = TArray<UEdGraphNode*>();

TMap<UEdGraphPin*, TSharedPtr<SGraphPin>> FOrganizeBlueprint::PinMap = TMap<UEdGraphPin*, TSharedPtr<SGraphPin>>();
TMap<UEdGraphNode*, TSharedPtr<SGraphNode>> FOrganizeBlueprint::NodeMap = TMap<UEdGraphNode*, TSharedPtr<SGraphNode>>();

void FOrganizeBlueprint::OrganizeBlueprint(UBlueprint* Blueprint)
{
	// Create a transaction (in order to make it possible to undo/redo the organization)
	const FScopedTransaction Transaction(LOCTEXT("OrganizeBlueprintAction", "Organize Blueprint"));

	// Empty the saved position and size of nodes and pins
	NodesInfos = TArray<FNodeInfos>();
	PinsInfos = TArray<FPinInfos>();
	PinPairsStraightened = TArray<FPinPair>();

	// Retrieve the blueprint editor for this blueprint
	FBlueprintEditor* BlueprintEditor = BlueprintEditors[Blueprint];

	// Retrieve the graph currently opened, we will organize this graph only
	CurrentGraph = BlueprintEditor->GetFocusedGraph();

	if (CurrentGraph == nullptr)
	{
		// There is no graph currently opened and focused
		return;
	}

	// Retrieve all the nodes of the graph to use later
	GraphNodes = CurrentGraph->Nodes;

	// Prevent NodesInfos and PinsInfos arrays to reallocate memory by providing enough memory (keep enough place for all nodes and pins in the graph)
	// This way, these arrays won't change addresses and pointers to this array will stay valid even when new elements are added to the array	
	NodesInfos.Empty(GraphNodes.Num() * 2);
	int32 PinCount = 0;
	for (UEdGraphNode* GraphNode : GraphNodes) {
		PinCount += GraphNode->Pins.Num();
	}
	PinsInfos.Empty(PinCount * 2);
		
	// Retrieve event nodes and begin function nodes
	// They will be the start for the node's recursive discovery
	TArray<UEdGraphNode*> StartingNodes = TArray<UEdGraphNode*>();

	TArray<UK2Node_Event*> EventNodes;
	CurrentGraph->GetNodesOfClass(EventNodes);
	for (UK2Node_Event* Node : EventNodes)
	{
		StartingNodes.Add((UEdGraphNode*)Node);
	}

	TArray<UK2Node_FunctionEntry*> FunctionNodes;
	CurrentGraph->GetNodesOfClass(FunctionNodes);
	for (UK2Node_FunctionEntry* Node : FunctionNodes)
	{
		StartingNodes.Add((UEdGraphNode*)Node);
	}

	// Open the graph in editor (the only way I found to retrieve the widget SGraphEditor used to retrieve node sizes)
	GraphEditor = BlueprintEditor->OpenGraphAndBringToFront(CurrentGraph);

	// For each event node and function begin node in the graph
	for (UEdGraphNode* Node : StartingNodes)
	{
		// Do a Depth First Search on these nodes using exec links between these nodes
		DiscoverNodes(Node, /*bCreateRerouteNodes=*/ false);
	}

	// Verify if nodes need to be moved (they are overlaping or too close to each other)
	// If so, move them and verify again until no nodes need to be moved
	int32 LoopCount = 0;
	while (MoveNodes(GraphNodes))
	{
		// In case of infinite loop... It should never happen but we never know
		if (LoopCount >= 100)
		{
			// After 100 iterations, it is very likely that the nodes are not finding any good position
			// There is no point continuing to move the nodes
			break;
		}
		LoopCount++;
	}

	for (UEdGraphNode* Node : StartingNodes)
	{
		// Another Depth First Search
		// this time, allow the creation/modification/removing of reroute nodes
		DiscoverNodes(Node, /*bCreateRerouteNodes=*/ true);
	}

	for (UEdGraphNode* Node : StartingNodes)
	{
		// Last Depth First Search on these nodes
		// A last check to make sure everything is well straightened
		DiscoverNodes(Node, /*bCreateRerouteNodes=*/ false);
	}

	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);	
}

void FOrganizeBlueprint::DiscoverNodes(UEdGraphNode* StartingNode, bool bCreateRerouteNodes = true)
{
	DiscoveredNodes = TArray<UEdGraphNode*>();
	DiscoverNodes(StartingNode, DiscoveredNodes, bCreateRerouteNodes);
}

void FOrganizeBlueprint::DiscoverNodes(UEdGraphNode* ParentNode, TArray<UEdGraphNode*>& discoveredNodes, bool bCreateRerouteNodes = true)
{
	// Add the current node to the discovered nodes
	discoveredNodes.Add(ParentNode);

	FNodeInfos* ParentNodeInfos = RetrieveNodeInfos(ParentNode);

	// We go through every pin of the current node
	for (UEdGraphPin* Pin : ParentNode->GetAllPins())
	{
		FRemoveNodeHelper RemoveHelper = FRemoveNodeHelper();
		
		ParentNodeInfos = RetrieveNodeInfos(ParentNode, Pin);

		// For every other pin linked to this pin
		for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
		{
			// Retrieve the node of the linked pin
			UEdGraphNode* LinkedNode = LinkedPin->GetOwningNode();

			FNodeInfos* LinkedNodeInfos = RetrieveNodeInfos(LinkedNode, LinkedPin);

			ParentNodeInfos = RetrieveNodeInfos(ParentNode, Pin);

			// Verify if the nodes are well ordered, 
			// For exemple, nodes linked to the input of another one should be placed on its left
			HandleHorizontalOrdering(ParentNodeInfos, LinkedNodeInfos);

			ParentNodeInfos = RetrieveNodeInfos(ParentNode, Pin);

			// Check and straighten the link between the nodes if needed
			HandleStraightening(ParentNodeInfos, LinkedNodeInfos);

			ParentNodeInfos = RetrieveNodeInfos(ParentNode, Pin);

			// Align nodes to "parallel" nodes (nodes linked to a common node will be aligned with each other)
			HandleParallelNodes(LinkedNodeInfos);

			// Handle variable nodes
			HandleVariableNodes(ParentNodeInfos, LinkedNodeInfos);

			// Verify the placement of reroute nodes (if they are needed and/or they need to be moved)
			if (bCreateRerouteNodes)
			{
				HandleReroutes(ParentNodeInfos, LinkedNodeInfos, RemoveHelper);
			}

			// If we did not visit the node owning the linked pin
			if (!discoveredNodes.Contains(LinkedNode))
			{
				// Start a discovery from this node
				DiscoverNodes(LinkedNode, discoveredNodes, bCreateRerouteNodes);
			}
		}
		// Remove useless nodes if any and remake links which were connected to these nodes
		HandleRemoving(RemoveHelper);
	}
}

void FOrganizeBlueprint::HandleReroutes(FNodeInfos* ParentNodeInfos, FNodeInfos* LinkedNodeInfos, FRemoveNodeHelper& RemoveHelper)
{
	TArray<FPinInfos*> PreviousPinsInfos = TArray<FPinInfos*>();
	TArray<FPinInfos*> FollowingPinsInfos = TArray<FPinInfos*>();

	TArray<FNodeInfos*> PreviousNodesInfos = TArray<FNodeInfos*>();
	TArray<FNodeInfos*> RerouteNodes = TArray<FNodeInfos*>();
	TArray<FNodeInfos*> FollowingNodesInfos = TArray<FNodeInfos*>();

	FNodeInfos* PreviousNodeInfos = nullptr;
	FNodeInfos* RerouteNodeInfos = nullptr;
	FNodeInfos* RerouteNode2Infos = nullptr;
	FNodeInfos* FollowingNodeInfos = nullptr;

	// If we need to move the reroute nodes to find them a good position
	bool bMoveRerouteNode = false;

	// Whether there already is a reroute between the two nodes
	bool bExistingRerouteNode = false;

	// Define previous variable declarations to handle the reroutes
	if (UK2Node_Knot* KnotNode = Cast<UK2Node_Knot>(LinkedNodeInfos->Node))
	{
		if (UK2Node_Knot* KnotNode2 = Cast<UK2Node_Knot>(ParentNodeInfos->Node))
		{

		}
		else
		{
			// Case where parent node is a non-reroute node linked to a reroute node
			if (LinkedNodeInfos->Node->GetPinAt(1)->LinkedTo.Num() > 0)
			{
				// Retrieve the pins connected by the reroute nodes
				PreviousPinsInfos = GetLinkedPinsInfosIgnoringReroute(LinkedNodeInfos->Node->GetPinAt(0), RerouteNodes);
				FollowingPinsInfos = GetLinkedPinsInfosIgnoringReroute(LinkedNodeInfos->Node->GetPinAt(1), RerouteNodes);

				// Retrieve the nodes from the pins
				for (FPinInfos* PreviousPinInfos : PreviousPinsInfos)
				{
					FNodeInfos* NodeInfos = RetrieveNodeInfos(PreviousPinInfos->Pin->GetOwningNode(), PreviousPinInfos->Pin);
					PreviousNodesInfos.Add(NodeInfos);
				}

				for (FPinInfos* FollowingPinInfos : FollowingPinsInfos)
				{
					FNodeInfos* NodeInfos = RetrieveNodeInfos(FollowingPinInfos->Pin->GetOwningNode(), FollowingPinInfos->Pin);
					FollowingNodesInfos.Add(NodeInfos);
				}

				RerouteNodes.Add(LinkedNodeInfos);

				PreviousNodeInfos = ParentNodeInfos;
				RerouteNodeInfos = LinkedNodeInfos;

				if (FollowingPinsInfos.Num() > 0)
				{
					FollowingNodeInfos = RetrieveNodeInfos(FollowingPinsInfos[0]->Pin->GetOwningNode(), FollowingPinsInfos[0]->Pin);
				}

				bMoveRerouteNode = true;
				bExistingRerouteNode = true;
			}
		}
	}
	else
	{
		if (UK2Node_Knot* KnotNode2 = Cast<UK2Node_Knot>(ParentNodeInfos->Node))
		{
			// Case where parent node is a reroute node linked to a non-reroute node
			if (ParentNodeInfos->Node->GetPinAt(0)->LinkedTo.Num() > 0)
			{
				// Retrieve the pins connected by the reroute nodes
				PreviousPinsInfos = GetLinkedPinsInfosIgnoringReroute(ParentNodeInfos->Node->GetPinAt(0), RerouteNodes);
				FollowingPinsInfos = GetLinkedPinsInfosIgnoringReroute(ParentNodeInfos->Node->GetPinAt(1), RerouteNodes);

				// Retrieve the nodes from these pins
				for (FPinInfos* PreviousPinInfos : PreviousPinsInfos)
				{
					FNodeInfos* NodeInfos = RetrieveNodeInfos(PreviousPinInfos->Pin->GetOwningNode(), PreviousPinInfos->Pin);
					PreviousNodesInfos.Add(NodeInfos);
				}
				for (FPinInfos* FollowingPinInfos : FollowingPinsInfos)
				{
					FNodeInfos* NodeInfos = RetrieveNodeInfos(FollowingPinInfos->Pin->GetOwningNode(), FollowingPinInfos->Pin);
					FollowingNodesInfos.Add(NodeInfos);
				}

				RerouteNodes.Add(ParentNodeInfos);

				if (PreviousPinsInfos.Num() > 0)
				{
					PreviousNodeInfos = RetrieveNodeInfos(PreviousPinsInfos[0]->Pin->GetOwningNode(), PreviousPinsInfos[0]->Pin);
				}
				RerouteNodeInfos = ParentNodeInfos;
				FollowingNodeInfos = LinkedNodeInfos;

				bMoveRerouteNode = true;
				bExistingRerouteNode = true;
			}
		}
		else
		{
			// Case where it is a link between to non-reroute nodes
			if (ParentNodeInfos->PinInfos->Pin->Direction == EEdGraphPinDirection::EGPD_Output)
			{
				// Retrieve the pins connected by the reroute nodes
				PreviousPinsInfos = TArray<FPinInfos*>({ ParentNodeInfos->PinInfos });
				FollowingPinsInfos = TArray<FPinInfos*>({ LinkedNodeInfos->PinInfos });

				// Retrieve the nodes from these pins
				for (FPinInfos* PreviousPinInfos : PreviousPinsInfos)
				{
					FNodeInfos* NodeInfos = RetrieveNodeInfos(PreviousPinInfos->Pin->GetOwningNode(), PreviousPinInfos->Pin);
					PreviousNodesInfos.Add(NodeInfos);
				}

				for (FPinInfos* FollowingPinInfos : FollowingPinsInfos)
				{
					FNodeInfos* NodeInfos = RetrieveNodeInfos(FollowingPinInfos->Pin->GetOwningNode(), FollowingPinInfos->Pin);
					FollowingNodesInfos.Add(NodeInfos);
				}

				PreviousNodeInfos = ParentNodeInfos;
				FollowingNodeInfos = LinkedNodeInfos;

				// if we can't make a link without adding a reroute node
				if (GetOverlappingNodes(PreviousNodesInfos, FollowingNodeInfos).Num() > 0)
				{
					// Create a reroute node
					UEdGraphNode* RerouteNode = (UEdGraphNode*)CreateRerouteNode(TArray<FPinInfos*>({ PreviousNodeInfos->PinInfos }), FollowingNodeInfos->PinInfos,
						FVector2D(PreviousNodeInfos->PosMax.X, PreviousNodeInfos->Pos.Y + PreviousNodeInfos->PinInfos->NodeOffset.Y));

					RerouteNodeInfos = RetrieveNodeInfos(RerouteNode, RerouteNode->GetPinAt(0));
					RerouteNodes.Add(RerouteNodeInfos);

					bMoveRerouteNode = true;
				}
			}
		}
	}

	// Handle reroute nodes for this link
	// This condition ensure we handle the reroute node creation from left to right
	if (bMoveRerouteNode && ParentNodeInfos->PinInfos->Pin->Direction == EEdGraphPinDirection::EGPD_Output)
	{
		// If we can't make a link without reroute nodes
		// (if it overlaps another node when we try to link without reroute nodes)
		TArray<UEdGraphNode*> OverlappingNodes = GetOverlappingNodes(PreviousNodesInfos, FollowingNodeInfos);
		if (OverlappingNodes.Num() > 0)
		{
			// If there is currently more than one reroute node for this link
			// (only handle up to 2 reroute nodes for the moment)
			if (RerouteNodes.Num() > 1)
			{
				FNodeInfos* RerouteToKeep = nullptr;

				// Verify if we can delete some reroute nodes
				// For each reroute node it will check if there will be nodes overlapping the links if we only keep this one reroute
				for (FNodeInfos* RerouteNode : RerouteNodes)
				{
					AttachPin(RerouteNode, RerouteNode->Node->GetPinAt(1));
					TArray<UEdGraphNode*> OverlappingNodesAfter = GetOverlappingNodes(RerouteNode, FollowingNodeInfos);
					if (OverlappingNodesAfter.Num() == 0)
					{
						AttachPin(RerouteNode, RerouteNode->Node->GetPinAt(0));
						TArray<UEdGraphNode*> OverlappingNodesBefore = GetOverlappingNodes(PreviousNodesInfos, RerouteNode);
						if (OverlappingNodesBefore.Num() == 0)
						{
							// Keep this reroute and delete all other
							RerouteToKeep = RerouteNode;

							// Saves the nodes to remove later
							for (FNodeInfos* RerouteNode2 : RerouteNodes)
							{
								if (RerouteNode2 != RerouteToKeep)
								{
									RemoveHelper.NodesToRemove.Add(RerouteNode2->Node);
								}
							}

							// Saves the links to make later
							for (FPinInfos* PreviousPinInfos : PreviousPinsInfos)
							{
								RemoveHelper.LinksToMake.Add(FPinPair(PreviousPinInfos->Pin, RerouteToKeep->Node->GetPinAt(0)));
							}
							for (FPinInfos* FollowingPinInfos : FollowingPinsInfos)
							{
								RemoveHelper.LinksToMake.Add(FPinPair(RerouteToKeep->Node->GetPinAt(1), FollowingPinInfos->Pin));
							}

							break;
						}
					}
				}
				// This means we still have 2 or more reroute nodes to handle
				if (RerouteToKeep == nullptr)
				{
					// Move the reroutes on the X axis to try to avoid any overlapping node
					FNodeInfos* RerouteNode1 = RerouteNodes[0];
					FNodeInfos* RerouteNode2 = RerouteNodes[1];
		
					// We check for each link (3 in total) if there is any node in the way
					// If there is, move the corresponding reroute nodes around it

					TArray<UEdGraphNode*> OverlappingNodesAfter = GetOverlappingNodes(RerouteNode2, FollowingNodeInfos);
					if (OverlappingNodesAfter.Num() != 0)
					{
						for (UEdGraphNode* NodeAfter : OverlappingNodesAfter)
						{
							FNodeInfos* NodeAfterInfos = RetrieveNodeInfos(NodeAfter);
							RerouteNode2->SetPosX(NodeAfterInfos->PosMax.X - RerouteNode2->Size.X);
						}
					}

					TArray<UEdGraphNode*> OverlappingNodesBefore = GetOverlappingNodes(PreviousNodesInfos, RerouteNode1);
					if (OverlappingNodesBefore.Num() != 0)
					{
						for (UEdGraphNode* NodeBefore : OverlappingNodesBefore)
						{
							FNodeInfos* NodeBeforeInfos = RetrieveNodeInfos(NodeBefore);
							RerouteNode1->SetPosX(NodeBeforeInfos->Pos.X);
						}
					}

					TArray<UEdGraphNode*> OverlappingNodesMiddle = GetOverlappingNodes(RerouteNode1, RerouteNode2);
					if (OverlappingNodesMiddle.Num() != 0)
					{
						for (UEdGraphNode* NodeMiddle : OverlappingNodesMiddle)
						{
							FNodeInfos* NodeMiddleInfos = RetrieveNodeInfos(NodeMiddle);
							RerouteNode1->SetPosY(NodeMiddleInfos->Pos.Y - RerouteNode1->Size.Y);
							RerouteNode2->SetPosY(NodeMiddleInfos->Pos.Y - RerouteNode2->Size.Y);
						}
					}
				}
			}
			// If there is currently 1 or less reroute nodes for this link
			else
			{
				// Try to add and/or move a reroute node correctly
				bool bReroutesHandled = HandleReroutePlacement(PreviousNodesInfos, RerouteNodeInfos, FollowingNodeInfos);

				// If it failed to position the reroute node correctly
				// (meaning it is not possible to make the link not overlap any node with only one reroute)
				if (!bReroutesHandled)
				{
					for (UEdGraphNode* OverlappingNode : OverlappingNodes)
					{
						FNodeInfos* OverlappingNodeInfos = RetrieveNodeInfos(OverlappingNode);

						if (RerouteNode2Infos == nullptr)
						{
							AttachPin(RerouteNodeInfos, RerouteNodeInfos->Node->GetPinAt(0));

							// Create a second reroute node to go under the overlapping node
							UK2Node_Knot* RerouteNode2 = CreateRerouteNode(PreviousPinsInfos, RerouteNodeInfos->PinInfos,
								FVector2D(OverlappingNodeInfos->Pos.X, OverlappingNodeInfos->Pos.Y - RerouteNodeInfos->Size.Y));

							// Align the first reroute node with the one we just created
							RerouteNodeInfos->SetPos(OverlappingNodeInfos->PosMax.X - RerouteNodeInfos->Size.X, RerouteNode2->NodePosY);

							RerouteNode2Infos = RetrieveNodeInfos((UEdGraphNode*)RerouteNode2, RerouteNode2->GetOutputPin());
							RerouteNodes.Add(RerouteNode2Infos);
						}
						// If we already created a second reroute node here during the discovery
						// (it can happen because the deletions and the links making are not made instantly to not mess with the node discovery)
						else
						{
							// Align the existing reroute node with the first reroute node
							RerouteNode2Infos->SetPos(OverlappingNodeInfos->Pos.X, OverlappingNodeInfos->Pos.Y - SETTINGS->NodesMinDistance);
							RerouteNodeInfos->SetPos(OverlappingNodeInfos->PosMax.X - RerouteNodeInfos->Size.X, OverlappingNodeInfos->Pos.Y - SETTINGS->NodesMinDistance);
						}

						// Try to position the second reroute node
						bool ReroutesHandled2 = HandleReroutePlacement(RerouteNode2Infos, RerouteNodeInfos, FollowingNodeInfos);
						if (ReroutesHandled2)
						{
							// The reroute nodes are well positioned
							break;
						}
						// If it failed to position the second reroute node without overlapping other nodes
						else
						{
							// Try to position it above the overlapping node
							RerouteNode2Infos->SetPos(OverlappingNodeInfos->Pos.X, OverlappingNodeInfos->PosMax.Y + SETTINGS->NodesMinDistance);
							RerouteNodeInfos->SetPos(OverlappingNodeInfos->PosMax.X - RerouteNodeInfos->Size.X, OverlappingNodeInfos->PosMax.Y + SETTINGS->NodesMinDistance);
							bool ReroutesHandled3 = HandleReroutePlacement(RerouteNode2Infos, RerouteNodeInfos, FollowingNodeInfos);
							if (ReroutesHandled3)
							{
								// The reroute nodes are well positioned
								break;
							}
							// If it failed again,
							// It will try to go around another overlapping node in the following iteration of the "for" loop
						}
					}
				}
			}
		}
		// If we can make a link without reroute nodes
		else
		{
			// Make sure reroute nodes have been found
			if (bExistingRerouteNode)
			{
				// Preparing to remove all reroute nodes between the two nodes
				for (FNodeInfos* KnotNodeInfos : RerouteNodes)
				{
					RemoveHelper.NodesToRemove.Add(KnotNodeInfos->Node);
				}

				// Save all the links to make after removing the reroute nodes
				for (FPinInfos* PreviousPinInfos : PreviousPinsInfos)
				{
					for (FPinInfos* FollowingPinInfos : FollowingPinsInfos)
					{
						RemoveHelper.LinksToMake.Add(FPinPair(PreviousPinInfos->Pin, FollowingPinInfos->Pin));
					}
				}
			}
		}
	}
}

void FOrganizeBlueprint::HandleStraightening(FNodeInfos* ParentNodeInfos, FNodeInfos* LinkedNodeInfos)
{
	// If it's a connection to a reroute node
	if (UK2Node_Knot* RerouteNode = Cast<UK2Node_Knot>(LinkedNodeInfos->Node))
	{
		// We need to straighten the link to this reroute

		FNodeInfos* RerouteNodeInfos = LinkedNodeInfos;

		UEdGraphNode* OtherNode = nullptr;
		FNodeInfos* OtherNodeInfos = nullptr;

		FVector ParentNodePos;
		FVector OtherNodePos;

		bool CanStraighten = false;

		// If there is a reroute node, this means links from both sides of the reroute nodes should not be straighten
		// Check if there is another node linked to the other side of the reroute node
		if (RerouteNodeInfos->PinInfos->Pin->Direction == EEdGraphPinDirection::EGPD_Input)
		{
			if (RerouteNode->GetOutputPin()->LinkedTo.Num() > 0)
			{
				// Retrieving infos about the node of the other side
				OtherNode = RerouteNode->GetOutputPin()->LinkedTo[0]->GetOwningNode();
				OtherNodeInfos = RetrieveNodeInfos(OtherNode, RerouteNode->GetPinAt(1)->LinkedTo[0]);

				// Saving the position of pins linked to both sides of the reroute
				ParentNodePos = FVector(ParentNodeInfos->PosMax.X, ParentNodeInfos->Pos.Y + ParentNodeInfos->PinInfos->NodeOffset.Y, 0);
				OtherNodePos = FVector(OtherNodeInfos->Pos.X, OtherNodeInfos->Pos.Y + OtherNodeInfos->PinInfos->NodeOffset.Y, 0);
				CanStraighten = true;
			}
		}
		else if (RerouteNodeInfos->PinInfos->Pin->Direction == EEdGraphPinDirection::EGPD_Output)
		{
			if (RerouteNode->GetInputPin()->LinkedTo.Num() > 0)
			{
				// Retrieving infos about the node of the other side
				OtherNode = RerouteNode->GetInputPin()->LinkedTo[0]->GetOwningNode();
				OtherNodeInfos = RetrieveNodeInfos(OtherNode, RerouteNode->GetPinAt(0)->LinkedTo[0]);

				// Saving the position of pins linked to both sides of the reroute
				ParentNodePos = FVector(ParentNodeInfos->Pos.X, ParentNodeInfos->Pos.Y + ParentNodeInfos->PinInfos->NodeOffset.Y, 0);
				OtherNodePos = FVector(OtherNodeInfos->PosMax.X, OtherNodeInfos->Pos.Y + OtherNodeInfos->PinInfos->NodeOffset.Y, 0);
				CanStraighten = true;
			}
		}

		// If there is a linked node the other other way around the reroute node
		if (CanStraighten)
		{
			FVector NewReroutePos;
			// If the reroute node is far from the other node on the Y axis
			if (FMath::Abs(RerouteNodeInfos->Pos.Y - (OtherNodeInfos->Pos.Y + OtherNodeInfos->PinInfos->NodeOffset.Y)) > 20)
			{
				// The reroute will be straightened to the parent node
				NewReroutePos = FVector(RerouteNodeInfos->Pos.X, ParentNodeInfos->Pos.Y + ParentNodeInfos->PinInfos->NodeOffset.Y - RerouteNodeInfos->PinInfos->NodeOffset.Y, 0);
			}
			else
			{
				// The reroute will be straightened to the node from the other side of the reroute
				NewReroutePos = FVector(RerouteNodeInfos->Pos.X, OtherNodeInfos->Pos.Y + OtherNodeInfos->PinInfos->NodeOffset.Y - RerouteNodeInfos->PinInfos->NodeOffset.Y, 0);
			}

			TArray<UEdGraphNode*> NodesToExclude = TArray<UEdGraphNode*>();
			NodesToExclude.Add(ParentNodeInfos->Node);
			NodesToExclude.Add(LinkedNodeInfos->Node);
			NodesToExclude.Add(OtherNodeInfos->Node);
			// Check if it does not overlap nodes with the new reroute position
			TArray<UEdGraphNode*> OverlappingNodes = GetOverlappingNodesWithLine(ParentNodePos, NewReroutePos, NodesToExclude);
			OverlappingNodes.Append(GetOverlappingNodesWithLine(NewReroutePos, OtherNodePos, NodesToExclude));

			// If no nodes will overlap the links to the reroute node with its new position
			if (OverlappingNodes.Num() == 0)
			{
				// Straighten the link by moving the reroute node
				RerouteNodeInfos->SetPosY(NewReroutePos.Y);
				PinPairsStraightened.Add({ ParentNodeInfos->PinInfos->Pin, LinkedNodeInfos->PinInfos->Pin });
			}
		}
	}
	else
	{
		if (UK2Node_Knot* RerouteNode2 = Cast<UK2Node_Knot>(ParentNodeInfos->Node))
		{

		}
		// if it's a connection between two non-reroute nodes
		else
		{
			// Check if the connection between the two nodes need to be straighten
			// (For example in cases where there are several links to the same node, we should not straighten them all)
			if (IsPinStraightenable(ParentNodeInfos->PinInfos->Pin, LinkedNodeInfos->PinInfos->Pin) 
				&& IsPinStraightenable(LinkedNodeInfos->PinInfos->Pin, ParentNodeInfos->PinInfos->Pin))
			{
				// Straighten the link by moving the two nodes on the Y axis
				Straighten(LinkedNodeInfos, ParentNodeInfos);

				// if some nodes overlaps this straighten link, move the nodes out of the way
				TArray<UEdGraphNode*> OverlappingNodes = GetOverlappingNodes(ParentNodeInfos, LinkedNodeInfos);
				if (OverlappingNodes.Num() > 0)
				{
					for (UEdGraphNode* OverlappingNode : OverlappingNodes)
					{
						// Moving the overlapping node out of the way
						FNodeInfos* OverNodeInfos = RetrieveNodeInfos(OverlappingNode);
						float OverNodeYCenter = (OverNodeInfos->PosMax.Y + OverNodeInfos->Pos.Y) / 2;
						if (OverNodeYCenter > ParentNodeInfos->Pos.Y + ParentNodeInfos->PinInfos->NodeOffset.Y)
						{
							// Move the node to the bottom to avoid the link
							OverNodeInfos->SetPosY(ParentNodeInfos->Pos.Y + ParentNodeInfos->PinInfos->NodeOffset.Y + SETTINGS->NodesMinDistance);
						}
						else
						{
							// Move the node to the top to avoid the link
							OverNodeInfos->SetPosY(ParentNodeInfos->Pos.Y + ParentNodeInfos->PinInfos->NodeOffset.Y - OverNodeInfos->Size.Y - SETTINGS->NodesMinDistance);
						}
					}
				}
			}
		}
	}
}

void FOrganizeBlueprint::Straighten(FNodeInfos* NodeToStraighten, FNodeInfos* NodeToStraightenTo)
{
	// Move the node of Y axis to align the pins
	NodeToStraighten->SetPosY(NodeToStraightenTo->Pos.Y + NodeToStraightenTo->PinInfos->NodeOffset.Y - NodeToStraighten->PinInfos->NodeOffset.Y);

	// Save the link between the two pins as straightened
	PinPairsStraightened.Add({ NodeToStraightenTo->PinInfos->Pin, NodeToStraighten->PinInfos->Pin });
}

UK2Node_Knot* FOrganizeBlueprint::CreateRerouteNode(TArray<FPinInfos*> PreviousPinsInfos, FPinInfos* FollowingPinInfos, FVector2D ReroutePos)
{
	// Set the current graph as modified so that we can undo the modification made to it (added a reroute node)
	CurrentGraph->Modify();

	// Create the reroute node
	FGraphNodeCreator<UK2Node_Knot> NodeCreator(*CurrentGraph);
	UK2Node_Knot* RerouteNode = NodeCreator.CreateNode();
	RerouteNode->NodePosX = ReroutePos.X;
	RerouteNode->NodePosY = ReroutePos.Y;
	NodeCreator.Finalize();
	
	// Retrieve and set the pin type of the newly created reroute
	//FName LinkCategory = FollowingPinInfos->Pin->PinType.PinCategory;
	FEdGraphPinType LinkType = FollowingPinInfos->Pin->PinType;
	RerouteNode->GetInputPin()->PinType = LinkType;
	RerouteNode->GetOutputPin()->PinType = LinkType;
	//const UEdGraphNode::FCreatePinParams& PinParams = UEdGraphNode::FCreatePinParams();
	//RerouteNode->GetInputPin()->PinType = FEdGraphPinType(LinkCategory, NAME_None, nullptr, PinParams.ContainerType, PinParams.bIsReference, PinParams.ValueTerminalType);
	//RerouteNode->GetOutputPin()->PinType = FEdGraphPinType(LinkCategory, NAME_None, nullptr, PinParams.ContainerType, PinParams.bIsReference, PinParams.ValueTerminalType);

	// Remake all links with the new reroute
	TArray<FPinPair> NewLinks = TArray<FPinPair>();
	for (FPinInfos* PreviousPinInfos : PreviousPinsInfos)
	{
		FollowingPinInfos->Pin->BreakLinkTo(PreviousPinInfos->Pin);
		NewLinks.Add({ RerouteNode->GetInputPin(), PreviousPinInfos->Pin });
	}
	NewLinks.Add({ RerouteNode->GetOutputPin(), FollowingPinInfos->Pin });

	MakeLinks(NewLinks);

	return RerouteNode;
}

TArray<UEdGraphNode*> FOrganizeBlueprint::GetOverlappingNodes(TArray<FNodeInfos*> PreviousNodesInfos, FNodeInfos* FollowingNodeInfos)
{
	TArray<UEdGraphNode*> OverlappingNodes = TArray<UEdGraphNode*>();
	TArray<UEdGraphNode*> NodesToExclude = TArray<UEdGraphNode*>();

	for (FNodeInfos* PreviousNodeInfos : PreviousNodesInfos)
	{
		FVector LineStart = FVector(PreviousNodeInfos->PosMax.X + 1, PreviousNodeInfos->Pos.Y + PreviousNodeInfos->PinInfos->NodeOffset.Y, 0);
		FVector LineEnd = FVector(FollowingNodeInfos->Pos.X - 1, FollowingNodeInfos->Pos.Y + FollowingNodeInfos->PinInfos->NodeOffset.Y, 0);
		NodesToExclude.Add(PreviousNodeInfos->Node);
		NodesToExclude.Add(FollowingNodeInfos->Node);
		OverlappingNodes.Append(GetOverlappingNodesWithLine(LineStart, LineEnd, NodesToExclude));
		NodesToExclude.Empty();
	}

	return OverlappingNodes;
}

TArray<UEdGraphNode*> FOrganizeBlueprint::GetOverlappingNodes(FNodeInfos* PreviousNodeInfos, FNodeInfos* RerouteNodeInfos, FNodeInfos* FollowingNodeInfos)
{
	FVector LineStart = FVector(PreviousNodeInfos->PosMax.X + 1, PreviousNodeInfos->Pos.Y + PreviousNodeInfos->PinInfos->NodeOffset.Y, 0);
	FVector LineEnd = FVector(RerouteNodeInfos->Pos.X, RerouteNodeInfos->Pos.Y, 0);
	FVector Line2Start = FVector(RerouteNodeInfos->Pos.X, RerouteNodeInfos->Pos.Y, 0);
	FVector Line2End = FVector(FollowingNodeInfos->Pos.X - 1, FollowingNodeInfos->Pos.Y + FollowingNodeInfos->PinInfos->NodeOffset.Y, 0);
	TArray<UEdGraphNode*> NodesToExclude = TArray<UEdGraphNode*>();
	NodesToExclude.Add(PreviousNodeInfos->Node);
	NodesToExclude.Add(RerouteNodeInfos->Node);
	NodesToExclude.Add(FollowingNodeInfos->Node);

	TArray<UEdGraphNode*> OverlappingNodes = GetOverlappingNodesWithLine(LineStart, LineEnd, NodesToExclude);
	OverlappingNodes.Append(GetOverlappingNodesWithLine(Line2Start, Line2End, NodesToExclude));

	return OverlappingNodes;
}

TArray<UEdGraphNode*> FOrganizeBlueprint::GetOverlappingNodes(FNodeInfos* PreviousNodeInfos, FNodeInfos* FollowingNodeInfos)
{
	FVector LineStart = FVector(PreviousNodeInfos->PosMax.X + 1, PreviousNodeInfos->Pos.Y + PreviousNodeInfos->PinInfos->NodeOffset.Y, 0);
	FVector LineEnd = FVector(FollowingNodeInfos->Pos.X - 1, FollowingNodeInfos->Pos.Y + FollowingNodeInfos->PinInfos->NodeOffset.Y, 0);

	TArray<UEdGraphNode*> NodesToExclude = TArray<UEdGraphNode*>();
	NodesToExclude.Add(PreviousNodeInfos->Node);
	NodesToExclude.Add(FollowingNodeInfos->Node);

	return GetOverlappingNodesWithLine(LineStart, LineEnd, NodesToExclude);
}

TArray<UEdGraphNode*> FOrganizeBlueprint::GetOverlappingNodesWithLine(FVector LineStart, FVector LineEnd, TArray<UEdGraphNode*> NodesToExclude)
{
	TArray<UEdGraphNode*> OverlappingNodes = TArray<UEdGraphNode*>();

	FVector StartToEnd = LineEnd - LineStart;

	// For each node in the graph
	for (UEdGraphNode* Node : GraphNodes)
	{
		if (UK2Node_Knot* RerouteNode = Cast<UK2Node_Knot>(Node))
		{
			continue;
		}

		if (UEdGraphNode_Comment* CommentNode = Cast<UEdGraphNode_Comment>(Node))
		{
			continue;
		}

		// If we do not have to ignore the node
		if (!NodesToExclude.Contains(Node))
		{
			FVector2D NodeSize = GetNodeSize(*GraphEditor.Pin().Get(), Node);
			// A box representing the node position
			FBox NodeBox = FBox(
				FVector(Node->NodePosX, Node->NodePosY, 0),
				FVector(Node->NodePosX + NodeSize.X, Node->NodePosY + NodeSize.Y, 0)
			);

			// Check if the straight line overlap the node
			if (FMath::LineBoxIntersection(NodeBox, LineStart, LineEnd, StartToEnd))
			{
				OverlappingNodes.Add(Node);
			}
		}
	}
	return OverlappingNodes;
}


TArray<UEdGraphNode*> FOrganizeBlueprint::GetOverlappingNodesWithLine(FVector LineStart, FVector LineEnd)
{
	return GetOverlappingNodesWithLine(LineStart, LineEnd, TArray<UEdGraphNode*>());
}

TArray<FPinInfos*>  FOrganizeBlueprint::GetLinkedPinsInfosIgnoringReroute(UEdGraphPin* Pin, TArray<FNodeInfos*>& IgnoredReroutesInfos)
{
	TArray<FPinInfos*> pinsInfos = TArray<FPinInfos*>();

	for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
	{
		UEdGraphNode* LinkedNode = LinkedPin->GetOwningNode();

		// Check if the linked node is a reroute node
		if (UK2Node_Knot* RerouteNode = Cast<UK2Node_Knot>(LinkedNode))
		{	
			IgnoredReroutesInfos.Add(RetrieveNodeInfos(LinkedNode));

			// Call recursively the function from this reroute and add the result to the list
			if (LinkedPin->Direction == EEdGraphPinDirection::EGPD_Input)
			{
				pinsInfos.Append(GetLinkedPinsInfosIgnoringReroute(RerouteNode->GetOutputPin(), IgnoredReroutesInfos));
			}
			else if (LinkedPin->Direction == EEdGraphPinDirection::EGPD_Output)
			{
				pinsInfos.Append(GetLinkedPinsInfosIgnoringReroute(RerouteNode->GetInputPin(), IgnoredReroutesInfos));
			}
		}
		else
		{
			// If the linked node is not a reroute, add it to the list
			pinsInfos.Add(RetrievePinInfos(LinkedPin));
		}
	}
	
	return pinsInfos;
}

TArray<FPinInfos*>  FOrganizeBlueprint::GetLinkedPinsInfosIgnoringReroute(UEdGraphPin* Pin)
{
	TArray<FNodeInfos*> IgnoredReroutesInfos = TArray<FNodeInfos*>();
	return GetLinkedPinsInfosIgnoringReroute(Pin, IgnoredReroutesInfos);
}

bool FOrganizeBlueprint::MoveNodes(TArray<UEdGraphNode*>& NodesToMove)
{
	bool bNodesMoved = false;

	// For each node
	for (UEdGraphNode* Node : NodesToMove)
	{
		if (UEdGraphNode_Comment* CommentNode = Cast<UEdGraphNode_Comment>(Node))
		{
			continue;
		}

		FNodeInfos* NodeInfos = RetrieveNodeInfos(Node);
		
		// Compare it to each other node
		for (UEdGraphNode* Node2 : NodesToMove)
		{
			if (UEdGraphNode_Comment* CommentNode = Cast<UEdGraphNode_Comment>(Node2))
			{
				continue;
			}

			// If they are not the same node
			if (Node != Node2)
			{
				FNodeInfos* Node2Infos = RetrieveNodeInfos(Node2);

				// If a X axis position has already been defined for this node,
				// Skip moving this node
				if (!Node2Infos->PriorityX)
				{
					// Verify if the nodes are overlaping on Y axis
					if (NodeInfos->PosMax.Y >= Node2Infos->Pos.Y && NodeInfos->PosMax.Y <= Node2Infos->PosMax.Y
						|| NodeInfos->Pos.Y <= Node2Infos->PosMax.Y && NodeInfos->Pos.Y >= Node2Infos->Pos.Y
						|| Node2Infos->PosMax.Y >= NodeInfos->Pos.Y && Node2Infos->PosMax.Y <= NodeInfos->PosMax.Y
						|| Node2Infos->Pos.Y <= NodeInfos->PosMax.Y && Node2Infos->Pos.Y >= NodeInfos->Pos.Y)
					{
						// Case :		[-----]		Node
						//			  [---------]	Node2
						if (NodeInfos->Pos.X >= Node2Infos->Pos.X && NodeInfos->Pos.X <= Node2Infos->PosMax.X
							&& NodeInfos->PosMax.X >= Node2Infos->Pos.X && NodeInfos->PosMax.X <= Node2Infos->PosMax.X)
						{
							if ((NodeInfos->Pos.X - Node2Infos->Pos.X) < (Node2Infos->PosMax.X - NodeInfos->PosMax.X))
							{
								// Move Node2 to the right of Node1
								Node2Infos->SetPosX(NodeInfos->PosMax.X + SETTINGS->NodesMinDistance);

								bNodesMoved = true;
								// Allow to move a node only once (if the node need to move again, it will move in next function call)
								break;
							}
							else
							{
								// Move Node2 to the left of Node1
								Node2Infos->SetPosX(NodeInfos->Pos.X - Node2Infos->Size.X - SETTINGS->NodesMinDistance);

								bNodesMoved = true;
								// Allow to move a node only once (if the node need to move again, it will move in next function call)
								break;
							}
						}
						// Case :		[-----]		Node
						//				  [-----]	Node2
						else if (NodeInfos->PosMax.X >= Node2Infos->Pos.X && NodeInfos->PosMax.X <= Node2Infos->PosMax.X
							// Or if the two nodes are too close
							|| NodeInfos->PosMax.X < Node2Infos->Pos.X && NodeInfos->PosMax.X + SETTINGS->NodesMinDistance > Node2Infos->Pos.X)
						{
							// Move Node2 to the right of Node1
							Node2Infos->SetPosX(NodeInfos->PosMax.X + SETTINGS->NodesMinDistance + 1);
 
							bNodesMoved = true;
							// Allow to move a node only once (if the node need to move again, it will move in next function call)
							break;
						}
						// Case :		  [-----]	Node
						//				[-----]		Node2
						else if (NodeInfos->Pos.X <= Node2Infos->PosMax.X && NodeInfos->Pos.X >= Node2Infos->Pos.X
							// Or if the two nodes are too close
							|| NodeInfos->Pos.X > Node2Infos->PosMax.X && NodeInfos->Pos.X - SETTINGS->NodesMinDistance < Node2Infos->PosMax.X)
						{
							// Move Node2 to the left of Node1
							Node2Infos->SetPosX(NodeInfos->Pos.X - Node2Infos->Size.X - SETTINGS->NodesMinDistance - 1);

							bNodesMoved = true;
							// Allow to move a node only once (if the node need to move again, it will move in next function call)
							break;
						}
						// Case :	  [---------]	Node
						//				[-----]		Node2
						else if (Node2Infos->PosMax.X >= NodeInfos->Pos.X && Node2Infos->PosMax.X <= NodeInfos->PosMax.X
							&& Node2Infos->Pos.X <= NodeInfos->PosMax.X && Node2Infos->Pos.X >= NodeInfos->Pos.X)
						{
							if ( (Node2Infos->Pos.X - NodeInfos->Pos.X) < (NodeInfos->PosMax.X - Node2Infos->PosMax.X) )
							{
								// Move Node2 to the left of Node1
								Node2Infos->SetPosX(NodeInfos->Pos.X - Node2Infos->Size.X - SETTINGS->NodesMinDistance);
							}
							else
							{
								// Move Node2 to the right of Node1
								Node2Infos->SetPosX(NodeInfos->PosMax.X + SETTINGS->NodesMinDistance);
							}
							bNodesMoved = true;
							// Allow to move a node only once (if the node need to move again, it will move in next function call)
							break;
						}
					}
				}
			}
		}
	}
	return bNodesMoved;
}

bool FOrganizeBlueprint::HandleReroutePlacement(FNodeInfos* PreviousNodeInfos, FNodeInfos* RerouteNodeInfos, FNodeInfos* FollowingNodeInfos)
{
	// Whether or not the wire is going up to link the two pins
	bool bLinkGoingUp = PreviousNodeInfos->Pos.Y + PreviousNodeInfos->PinInfos->NodeOffset.Y > FollowingNodeInfos->Pos.Y + FollowingNodeInfos->PinInfos->NodeOffset.Y;
	
	// Whether or not the wire is straightened
	bool bLinkStraighten = PreviousNodeInfos->Pos.Y + PreviousNodeInfos->PinInfos->NodeOffset.Y == FollowingNodeInfos->Pos.Y + FollowingNodeInfos->PinInfos->NodeOffset.Y;

	// Save the current position of the reroute node
	FVector2D RerouteLastPos = RerouteNodeInfos->Pos;

	// Retrieve nodes in the way of the link
	TArray<UEdGraphNode*> OverlappingNodes = GetOverlappingNodes(PreviousNodeInfos, RerouteNodeInfos, FollowingNodeInfos);

	int32 TryOffset = 0;

	// While we do not find a good position for the reroute node
	while (OverlappingNodes.Num() > 0)
	{
		float PosX = 0;
		float PosY = 0;

		for (UEdGraphNode* OverlappingNode : OverlappingNodes)
		{	
			FNodeInfos* OverlappingNodeInfos = RetrieveNodeInfos(OverlappingNode);

			// Check if the reroute node has been positioned way too far (the offset kept being increased because no good position were found)
			if ((OverlappingNodeInfos->PosMax.X - RerouteNodeInfos->Size.X + TryOffset > FollowingNodeInfos->Pos.X
				|| OverlappingNodeInfos->Pos.X - TryOffset < PreviousNodeInfos->Pos.X) && TryOffset > 10)
			{
				// Put the reroute node back to its previous position
				RerouteNodeInfos->SetPos(RerouteLastPos.X, RerouteLastPos.Y);

				// It failed to find a good position (this means one reroute node may not be enough)
				return false;
			}

			// If the node is overlapping a straighten link
			if (bLinkStraighten)
			{
				TryOffset++;
			}
			// If the node is not overlapping a straighten link
			else
			{
				if (bLinkGoingUp)
				{
					// Move the reroute node so it goes under the overlapping node to reach the upper pin
					PosX = OverlappingNodeInfos->PosMax.X - RerouteNodeInfos->Size.X + TryOffset;
					PosY = PreviousNodeInfos->Pos.Y + PreviousNodeInfos->PinInfos->NodeOffset.Y - RerouteNodeInfos->PinInfos->NodeOffset.Y/* - 14*/;
				}
				else
				{
					// Move the reroute node so it goes under the overlapping node to reach the lower pin
					PosX = OverlappingNodeInfos->Pos.X + TryOffset;
					PosY = FollowingNodeInfos->Pos.Y + FollowingNodeInfos->PinInfos->NodeOffset.Y - RerouteNodeInfos->PinInfos->NodeOffset.Y/* - 14*/ /*+ TryOffset*/;
				}
				RerouteNodeInfos->SetPos(PosX, PosY);

				TArray<UEdGraphNode*> OverlappingNodes2 = GetOverlappingNodes(PreviousNodeInfos, RerouteNodeInfos, FollowingNodeInfos);
				if (OverlappingNodes2.Num() > 0)
				{
					if (bLinkGoingUp)
					{
						// Move the reroute node so it goes above the overlapping node to reach the upper pin
						PosX = OverlappingNodeInfos->Pos.X + TryOffset;
						PosY = FollowingNodeInfos->Pos.Y + FollowingNodeInfos->PinInfos->NodeOffset.Y - RerouteNodeInfos->PinInfos->NodeOffset.Y/* - 14*/ /*- TryOffset*/;
					}
					else
					{
						// Move the reroute node so it goes above the overlapping node to reach the lower pin
						PosX = OverlappingNodeInfos->PosMax.X - RerouteNodeInfos->Size.X + TryOffset;
						PosY = PreviousNodeInfos->Pos.Y + PreviousNodeInfos->PinInfos->NodeOffset.Y - RerouteNodeInfos->PinInfos->NodeOffset.Y/* - 14*/;
					}
					RerouteNodeInfos->SetPos(PosX, PosY);

					// If there is no more overlapping nodes
					OverlappingNodes2 = GetOverlappingNodes(PreviousNodeInfos, RerouteNodeInfos, FollowingNodeInfos);			
					if (!(OverlappingNodes2.Num() > 0))
					{
						TryOffset = 0;
						break;
						
					}
					else
					{
						// If there is still a node in the way, try with an increased offset
						TryOffset++;
					}
				}
				// If there's no more overlapping nodes
				else
				{
					TryOffset = 0;
					break;
				}
			}
		}
		// Update the current overlapping nodes list (in order to check the while loop condition)
		OverlappingNodes = GetOverlappingNodes(PreviousNodeInfos, RerouteNodeInfos, FollowingNodeInfos);
	}
	return true;
}

bool FOrganizeBlueprint::HandleReroutePlacement(TArray<FNodeInfos*> PreviousNodesInfos, FNodeInfos* RerouteNodeInfos, FNodeInfos* FollowingNodeInfos)
{
	for (FNodeInfos* PreviousNodeInfos : PreviousNodesInfos)
	{
		if (!HandleReroutePlacement(PreviousNodeInfos, RerouteNodeInfos, FollowingNodeInfos))
		{
			return false;
		}
	}
	return true;
}

bool FOrganizeBlueprint::IsPinStraightenable(UEdGraphPin* Pin, UEdGraphPin* OtherPin)
{
	bool IsExec = Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec;

	return IsPinStraightenable(Pin, OtherPin, IsExec);
}

bool FOrganizeBlueprint::IsPinStraightenable(UEdGraphPin* Pin, UEdGraphPin* OtherPin, bool bExec)
{
	UEdGraphNode* Node = Pin->GetOwningNode();

	// for exec pin
	if (bExec)
	{
		for (int32 i = 0; i < Node->GetAllPins().Num(); i++)
		{
			// for each pin of the same direction in the node
			if (Pin->Direction == Node->GetPinAt(i)->Direction)
			{
				if (Node->GetPinAt(i)->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
				{
					if (Node->GetPinAt(i)->LinkedTo.Num() > 0)
					{
						if (Node->GetPinAt(i) == Pin)
						{
							// If the pin is the first exec linked of this direction
							// The pin is straightenable
							return true;
						}
						else
						{
							// If there is a prior exec linked pin of the same direction
							// The pin should not be straighten
							return false;
						}
					}
				}
			}
		}
	}
	// for data pin
	else
	{
		for (int32 i = 0; i < Node->GetAllPins().Num(); i++)
		{
			if (Pin->Direction == Node->GetPinAt(i)->Direction)
			{
				if (Node->GetPinAt(i)->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
				{
					if (Node->GetPinAt(i)->LinkedTo.Num() > 1)
					{
						// If there is a prior exec pin linked to at least 2 other pins
						// The pin should not be straighten
						return false;
					}
					else if (Node->GetPinAt(i)->LinkedTo.Num() == 1)
					{
						for (UEdGraphPin* LinkedPin : Node->GetPinAt(i)->LinkedTo)
						{
							if (IsPinPairStraightened(Node->GetPinAt(i), LinkedPin))
							{
								// If a prior exec pin has a straighten link
								// The pin should not be straighten
								return false;
							}
						}
					}
				}
				else
				{
					if (Node->GetPinAt(i) == Pin)
					{
						for (UEdGraphPin* LinkedPin : Node->GetPinAt(i)->LinkedTo)
						{
							
							if (IsPinPairStraightened(Node->GetPinAt(i), LinkedPin) && LinkedPin != OtherPin)
							{
								// If a prior wire coming from the pin is already straightened
								// The pin should not be straighten
								return false;
							}
						}
						// If the pin is the first to be linked and do not have any straightened wire
						return true;
					}
					else
					{
						if (Node->GetPinAt(i)->LinkedTo.Num() > 1)
						{
							for (UEdGraphPin* LinkedPin : Node->GetPinAt(i)->LinkedTo)
							{
								if (IsPinPairStraightened(Node->GetPinAt(i), LinkedPin))
								{
									// If a prior data pin has a straighten link
									// The pin should not be straighten
									return false;
								}
							}
						}
						else if (Node->GetPinAt(i)->LinkedTo.Num() == 1)
						{
							// If a prior data pin is linked
							// The pin should not be straighten
							return false;
						}
					}
				}
			}
		}
	}
	// Reaching this means the specified pin is not linked to anything
	return false;
}

bool FOrganizeBlueprint::IsPinPairStraightened(UEdGraphPin* Pin1, UEdGraphPin* Pin2)
{
	FPinPair PinPair1 = FPinPair(Pin1, Pin2);
	FPinPair PinPair2 = FPinPair(Pin2, Pin1);

	return PinPairsStraightened.Contains(PinPair1) || PinPairsStraightened.Contains(PinPair2);
}

void FOrganizeBlueprint::HandleVariableNodes(FNodeInfos* ParentNodeInfos, FNodeInfos* LinkedNodeInfos)
{
	if (LinkedNodeInfos->IsVariableNode())
	{
		if (LinkedNodeInfos->PinInfos->Pin->LinkedTo.Num() == 1)
		{		
			//Straighten(LinkedNodeInfos, ParentNodeInfos);
			if (LinkedNodeInfos->PinInfos->Pin->Direction == EEdGraphPinDirection::EGPD_Output) {
				LinkedNodeInfos->SetPosX(ParentNodeInfos->Pos.X - LinkedNodeInfos->Size.X - SETTINGS->NodesMinDistance);
			}
		}	
	}
}

void FOrganizeBlueprint::HandleParallelNodes(FNodeInfos* NodeInfos)
{
	// If it's not a connection to a reroute node
	if (UK2Node_Knot* RerouteNode = Cast<UK2Node_Knot>(NodeInfos->Node))
	{
		return;
	}
	
	// Only consider parallel nodes with exec pins
	// (nodes with data pins coming to the same node should not be aligned)
	if (NodeInfos->PinInfos->Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
	{
		TArray<FPinInfos*> ParallelLinkedPinsInfos = TArray<FPinInfos*>();
		// Verify if the node is at least well ordered vertically with other nodes with links coming from common node

		// Ony consider parallel nodes linked to a common node if the common node is placed before
		if (NodeInfos->PinInfos->Pin->Direction == EEdGraphPinDirection::EGPD_Input)
		{
			// Retrieve the parallel pins (pins linked to the common node)
			ParallelLinkedPinsInfos = GetParallelPinsInfos(NodeInfos->PinInfos->Pin);
		}

		int32 MinYPos = NodeInfos->Pos.Y;
		int32 XPos = NodeInfos->Pos.X;
		// For each parallel pin
		for (FPinInfos* ParallelLinkedPinInfos : ParallelLinkedPinsInfos)
		{
			// Retrieve the parallel node from the pin
			UEdGraphNode* ParallelNode = ParallelLinkedPinInfos->Pin->GetOwningNode();

			// Check if the node is not the same as the one we want to align 
			// (it can happen if there is multiple links between two nodes)
			if (ParallelNode != NodeInfos->Node)
			{
				FNodeInfos* ParallelNodeInfos = RetrieveNodeInfos(ParallelNode, ParallelLinkedPinInfos->Pin);

				int32 YPos = ParallelNodeInfos->PosMax.Y;

				// Retrieve the lowest parallel node position in the graph
				if (YPos + SETTINGS->NodesMinDistance > MinYPos)
				{
					MinYPos = YPos;
				}

				// Retrieve the X position of the parallel node
				XPos = ParallelNodeInfos->Pos.X;
			}
		}

		if (MinYPos != NodeInfos->Pos.Y)
		{
			// Move the node lower to other parallel nodes
			NodeInfos->SetPosY(MinYPos + SETTINGS->NodesMinDistance);
		}

		if (NodeInfos->Pos.X != XPos)
		{
			// Align the node with the other parallel nodes
			NodeInfos->SetPosX(XPos);
		}

		if (ParallelLinkedPinsInfos.Num() > 0)
		{
			// If the node must be aligned, set this X position as priority
			NodeInfos->PriorityX = true;
		}
	}
}

TArray<FPinInfos*> FOrganizeBlueprint::GetParallelPinsInfos(UEdGraphPin* Pin)
{
	TArray<FPinInfos*> ParallelLinkedPinsInfos = TArray<FPinInfos*>();
	
	// For each pin linked to the specified pin (ignoring reroute nodes)
	for (FPinInfos* LinkedPinInfos : GetLinkedPinsInfosIgnoringReroute(Pin))
	{	
		// Retrieve the "common node" from which it will retrieve the parallel pins
		UEdGraphNode* CommonNode = LinkedPinInfos->Pin->GetOwningNode();

		for(int32 i = 0; i < CommonNode->GetAllPins().Num(); i++)
		{
			UEdGraphPin* ParallelPin = CommonNode->GetPinAt(i);
			if (ParallelPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
			{
				if (ParallelPin->Direction == LinkedPinInfos->Pin->Direction)
				{
					// For all exec pins of the same direction of the specified pin
					if (ParallelPin != LinkedPinInfos->Pin)
					{
						// Retrieve the pins linked to the common node
						ParallelLinkedPinsInfos.Append(GetLinkedPinsInfosIgnoringReroute(ParallelPin));
					}
					else
					{
						// Allow to not retrieve parallel pins from nodes which are linked below the node's specified pin
						// (because pins are iterated in index order)
						break;
					}
				}
			}
		}
	}
	return ParallelLinkedPinsInfos;
}

void FOrganizeBlueprint::HandleHorizontalOrdering(FNodeInfos* ParentNodeInfos, FNodeInfos* LinkedNodeInfos)
{
	// Excluding moving nodes around a reroute node
	if (UK2Node_Knot* RerouteNode = Cast<UK2Node_Knot>(ParentNodeInfos->Node))
	{
		return;
	}

	if (ParentNodeInfos->PinInfos->Pin->Direction == EEdGraphPinDirection::EGPD_Output && LinkedNodeInfos->PinInfos->Pin->Direction == EEdGraphPinDirection::EGPD_Input)
	{
		if (ParentNodeInfos->PosMax.X >= LinkedNodeInfos->Pos.X - SETTINGS->NodesMinDistance)
		{
			// Move the node to the left of its following node
			LinkedNodeInfos->SetPosX(ParentNodeInfos->PosMax.X + SETTINGS->NodesMinDistance);
		}
	}
	else if (ParentNodeInfos->PinInfos->Pin->Direction == EEdGraphPinDirection::EGPD_Input && LinkedNodeInfos->PinInfos->Pin->Direction == EEdGraphPinDirection::EGPD_Output)
	{
		if (ParentNodeInfos->Pos.X <= LinkedNodeInfos->PosMax.X + SETTINGS->NodesMinDistance)
		{
			// Move the node to the right of its previous node
			LinkedNodeInfos->SetPosX(ParentNodeInfos->Pos.X - LinkedNodeInfos->Size.X - SETTINGS->NodesMinDistance);
		}
	}
}

void FOrganizeBlueprint::HandleRemoving(FRemoveNodeHelper& RemoveHelper)
{
	if (RemoveHelper.NodesToRemove.Num() > 0)
	{
		// Remove the previously saved reroute nodes for real
		for (UEdGraphNode* NodeToRemove : RemoveHelper.NodesToRemove)
		{
			CurrentGraph->RemoveNode(NodeToRemove);
		}
		// Now makes the previously saved links
		MakeLinks(RemoveHelper.LinksToMake);
	}
}

FVector2D FOrganizeBlueprint::GetNodeSize(const SGraphEditor& graphEditor, const UEdGraphNode* Node)
{
	FSlateRect Rect;
	if (graphEditor.GetBoundsForNode(Node, Rect, 0.f))
	{
		// Retrieve the size using the graph editor widget
		return FVector2D(Rect.Right - Rect.Left, Rect.Bottom - Rect.Top);
	}
	// NodeWidth and NodeHeight are only useful when the node is resizable
	return FVector2D(Node->NodeWidth, Node->NodeHeight);
}

FNodeInfos* FOrganizeBlueprint::RetrieveNodeInfos(UEdGraphNode* Node)
{
	for (int32 i = 0; i < NodesInfos.Num(); i++)
	{
		if (NodesInfos[i].Node == Node)
		{
			// if we already created infos for this node, retrieve them
			NodesInfos[i].UpdatePos();
			return &NodesInfos[i];
		}
	}
	// else, create new infos for this node
	NodesInfos.Add(FNodeInfos(Node));
	return &NodesInfos.Last();
}

FNodeInfos* FOrganizeBlueprint::RetrieveNodeInfos(UEdGraphNode* Node, UEdGraphPin* Pin)
{
	for (int32 i = 0; i < NodesInfos.Num(); i++)
	{
		if (NodesInfos[i].Node == Node)
		{
			// if we already created infos for this node, retrieve them
			NodesInfos[i].UpdatePos();
			AttachPin(&NodesInfos[i], Pin);
			return &NodesInfos[i];
		}
	}
	// else, create new infos for this node
	NodesInfos.Add(FNodeInfos(Node, Pin));
	return &NodesInfos.Last();
}

FPinInfos* FOrganizeBlueprint::RetrievePinInfos(UEdGraphPin* Pin)
{
	//return RetrievePinInfos(Pin, NodeMap[Pin->GetOwningNode()]);

	for (int32 i = 0; i < PinsInfos.Num(); i++)
	{
		if (PinsInfos[i].Pin == Pin)
		{
			// if we already created infos for this pin, retrieve them
			return &PinsInfos[i];
		}
	}
	// else, create new infos for this pin
	PinsInfos.Add(FPinInfos(Pin));
	return &PinsInfos.Last();
}

FPinInfos* FOrganizeBlueprint::RetrievePinInfos(UEdGraphPin* Pin, TSharedPtr<SGraphNode> NodeWidget)
{
	for (int32 i = 0; i < PinsInfos.Num(); i++)
	{
		if (PinsInfos[i].Pin == Pin)
		{
			// if we already created infos for this node, retrieve them
			return &PinsInfos[i];
		}
	}
	// else, create new infos for this pin using specified node widget
	PinsInfos.Add(FPinInfos(Pin, NodeWidget));
	return &PinsInfos.Last();
}

void FOrganizeBlueprint::AttachPin(FNodeInfos* NodeInfos, UEdGraphPin* Pin)
{	
	// Set the pin infos for this node
	NodeInfos->PinInfos = RetrievePinInfos(Pin, NodeInfos->Widget);
}

void FOrganizeBlueprint::MakeLinks(TArray<FPinPair> LinksToMake)
{
	// For each pair of pin
	for (FPinPair Link : LinksToMake)
	{
		// Set the pins as modified to allow to undo the making of this link
		Link.Pin1->Modify();
		Link.Pin2->Modify();

		Link.Pin1->MakeLinkTo(Link.Pin2);
	}
}

FNodeInfos::FNodeInfos(UEdGraphNode* Node)
{
	this->Node = Node;
	Pos = FVector2D(Node->NodePosX, Node->NodePosY);
	Size = FOrganizeBlueprint::GetNodeSize(*FOrganizeBlueprint::GraphEditor.Pin().Get(), Node);
	PosMax = FVector2D(Pos.X + Size.X, Pos.Y + Size.Y);
	PriorityX = false;
	if (FOrganizeBlueprint::NodeMap.Contains(Node))
	{
		// Retrieve node widget if it has been drawn already
		Widget = FOrganizeBlueprint::NodeMap[Node];
	}
}

FNodeInfos::FNodeInfos(UEdGraphNode* Node, UEdGraphPin* Pin)
{
	this->Node = Node;
	Pos = FVector2D(Node->NodePosX, Node->NodePosY);
	Size = FOrganizeBlueprint::GetNodeSize(*FOrganizeBlueprint::GraphEditor.Pin().Get(), Node);
	PosMax = FVector2D(Pos.X + Size.X, Pos.Y + Size.Y);
	PriorityX = false;
	if (FOrganizeBlueprint::NodeMap.Contains(Node))
	{
		// Retrieve node widget if it has been drawn already
		Widget = FOrganizeBlueprint::NodeMap[Node];
		// Retrieve pin infos from this node widget
		PinInfos = FOrganizeBlueprint::RetrievePinInfos(Pin, Widget);
	}
	else
	{
		// If the node widget has not been drawn yet, try retrieving the pin
		// (it is very unlikely to have the pin drawn without its owning node being drawn as well)
		PinInfos = FOrganizeBlueprint::RetrievePinInfos(Pin);
	}
}


bool FNodeInfos::IsVariableNode()
{
	int32 OutputPinCount = 0;
	for (UEdGraphPin* Pin : Node->Pins)
	{
		// If there is a input pin with a name different than "self", it is not a variable node
		// (because nodes without visible input pins still have a input pin named "self")
		if (Pin->Direction == EEdGraphPinDirection::EGPD_Input && Pin->GetName() != "self")
		{
			return false;
		}
		// If there is more than one output pin, the node is not a variable node
		else if (Pin->Direction == EEdGraphPinDirection::EGPD_Output)
		{
			if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
			{
				return false;
			}
			OutputPinCount++;
			if (OutputPinCount >= 2)
			{
				return false;
			}
		}
	}
	return true;
}

void FNodeInfos::SetPosX(float X)
{
	Pos.X = FMath::RoundHalfFromZero(X);
	UpdatePos();
}

void FNodeInfos::SetPosY(float Y)
{
	Pos.Y = FMath::RoundHalfFromZero(Y);
	UpdatePos();
}

void FNodeInfos::SetPos(float X, float Y)
{
	SetPosX(X);
	SetPosY(Y);
}

void FNodeInfos::UpdatePos()
{
	// Set the node as modified to allow to undo the change of position for this node
	Node->Modify();
	Node->NodePosX = Pos.X;
	Node->NodePosY = Pos.Y;
	PosMax = FVector2D(Pos.X + Size.X, Pos.Y + Size.Y);
	
}


FPinInfos::FPinInfos(UEdGraphPin* Pin)
{
	this->Pin = Pin;
	if (FOrganizeBlueprint::PinMap.Contains(Pin))
	{
		// Retrieve pin widget if it has been drawn already
		Widget = FOrganizeBlueprint::PinMap[Pin];
		NodeOffset = Widget->GetNodeOffset();
	}
	else
	{
		// Default offset if the pin widget is not found (7.5 is the Y offset for reroute node's pins)
		NodeOffset = FVector2D(0, 7.5f);
	}
}

FPinInfos::FPinInfos(UEdGraphPin* Pin, TSharedPtr<SGraphNode> NodeWidget)
{
	this->Pin = Pin;
	if (NodeWidget.IsValid())
	{
		// Retrieve pin widget from the node widget
		Widget = NodeWidget.Get()->FindWidgetForPin(Pin);
		if (Widget.IsValid())
		{
			NodeOffset = Widget->GetNodeOffset();
		}
		else
		{
			// Default offset if the pin widget is not found (7.5 is the Y offset for reroute node's pins)
			NodeOffset = FVector2D(0, 7.5f);
		}
	}
	else
	{
		// Default offset if the pin widget is not found (7.5 is the Y offset for reroute node's pins)
		NodeOffset = FVector2D(0, 7.5f);
	}
}

FPinPair::FPinPair(UEdGraphPin* Pin1, UEdGraphPin* Pin2)
{
	this->Pin1 = Pin1;
	this->Pin2 = Pin2;
}

FRemoveNodeHelper::FRemoveNodeHelper()
{
	this->NodesToRemove = TArray<UEdGraphNode*>();
	this->LinksToMake = TArray<FPinPair>();
}

#undef LOCTEXT_NAMESPACE


