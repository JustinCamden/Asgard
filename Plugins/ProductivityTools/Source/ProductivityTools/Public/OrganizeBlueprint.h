// Copyright (c) 2019 Isara Technologies. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "BlueprintGraphPanelPinFactory.h"
#include "BlueprintEditor.h"

#include "KismetPins/SGraphPinExec.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Knot.h"
#include "NodeFactory.h"

/** The class used to contains various informations about a pin */
class FPinInfos
{
public:
	/** Constructor retrieving the pin widget from the Pin Map (widgets retrieved in the pin factory) */
	FPinInfos(UEdGraphPin* Pin);

	/** Constructor retrieving the pin widget from the widget of the owning node */
	FPinInfos(UEdGraphPin* Pin, TSharedPtr<SGraphNode> Widget);

	/** The pin the infos belong to */
	UEdGraphPin* Pin;

	/** The pin position inside of its node */
	FVector2D NodeOffset;

	/** The widget of the pin */
	TSharedPtr<SGraphPin> Widget;
};

/** The class used to contains various informations about a node */
class FNodeInfos
{
public:
	/** Constructor */
	FNodeInfos(UEdGraphNode* Node);

	/** Constructor assigning pin infos as well for this node*/
	FNodeInfos(UEdGraphNode* Node, UEdGraphPin* Pin);

	/** The node the infos belong to */
	UEdGraphNode* Node;

	/** The current position of the node (position is taken at the top left corner of the node) */
	FVector2D Pos;

	/** The current position of the node (taken at the bottom right corner of the node) */
	FVector2D PosMax;

	/** The size of the node */
	FVector2D Size;

	/** The infos of the pin currently associated with the node
	* This is the pin which will be used by actions requiring pin's infos as well as node's (like straightening) */
	FPinInfos* PinInfos;

	/** Wether or not the current X position of the node should be a priority over other nodes positions
	(this basically means other nodes overlapping with this node will be moved away instead of moving away this one) */
	bool PriorityX;

	/** The widget of the node */
	TSharedPtr<SGraphNode> Widget;

	/** Check if the node is a "variable node" (node without input pins and with only one output pin) */
	bool IsVariableNode();

	/** Set the X position of the node */
	void SetPosX(float X);

	/** Set the Y position of the node */
	void SetPosY(float Y);

	/** Set the X and Y position of the node */
	void SetPos(float X, float Y);

	/** Update the position of the UEdGraphNode object from the FNodeInfos position
	* (It is automatically called when using the node infos setters)*/
	void UpdatePos();
};

class FPinPair
{
public:
	FPinPair(UEdGraphPin* Pin1, UEdGraphPin* Pin2);

	UEdGraphPin* Pin1;
	UEdGraphPin* Pin2;

	bool operator==(const FPinPair& PinPair) const {
		return PinPair.Pin1 == Pin1 && PinPair.Pin2 == Pin2;
	}
};

/** Class for containing nodes to remove and links to make after removing them */
class FRemoveNodeHelper
{
public:
	FRemoveNodeHelper();

	/** The list of reroute node which need to be removed */
	TArray<UEdGraphNode*> NodesToRemove;

	/** The list of pin pair which need to be linked */
	TArray<FPinPair> LinksToMake;
};

/** The class which hold the OrganizeBlueprint related functions */
class FOrganizeBlueprint
{
public:
	/**
	* Function called by the Organize button in the Blueprint Editor toolbar
	* Organize nodes in the blueprint graph
	*
	* @param	Blueprint		The Blueprint to organize
	*/
	static void OrganizeBlueprint(UBlueprint* Blueprint);

	/**
	* Do a Depth First Search on the specified node using exec links between these nodes
	* Once a node is visited, it straightens the exec link between it and its parent if needed
	* It also aligns nodes if they are linked to the same node
	*
	* @param	ParentNode				The Blueprint to organize
	* @param	DiscoveredNodes			The visited nodes during the search
	*/
	static void DiscoverNodes(UEdGraphNode* ParentNode, TArray<UEdGraphNode*>& DiscoveredNodes, bool bCreateRerouteNodes);
	static void DiscoverNodes(UEdGraphNode* ParentNode, bool bCreateRerouteNodes);

	/**
	* Verify if the specified nodes need to be moved (they are overlaping or too close to each other)
	* If they do need to be moved, it moves them away
	* Each node can only be moved once per call, so it is very likely to need to call the function multiple times
	* until all the nodes are set
	*
	* @param	NodesToMove			The Blueprint to organize
	*
	* @return	Whether a node has been moved by the call
	*/
	static bool MoveNodes(TArray<UEdGraphNode*>& NodesToMove);

	/**
	* Retrieve the size of the specified node in the graph
	*
	* @param	GraphEditor			The graph editor widget in which the node is drawn
	* @param	Node				The node to retrieve its size
	*
	* @return	A Vector with the height and the length of the node
	*/
	static FVector2D GetNodeSize(const SGraphEditor& GraphEditor, const UEdGraphNode* Node);

	/**
	* Straighten the link between the two specified nodes
	*
	* @param	NodeToStraighten		The node to move in order to straighten the link
	* @param	NodeToStraightenTo		The other node from which the link is connected
	*/
	static void Straighten(FNodeInfos* NodeToStraighten, FNodeInfos* NodeToStraightenTo);

	/**
	* Create and link a reroute node the specified pins
	*
	* @param	PreviousPinsInfos		The pins which should be linked to the reroute's input
	* @param	FollowingPinInfos		The pin which should be linked to the reroute's output
	* @param	ReroutePos				Where the created reroute should be positioned
	*
	* @return	The created reroute node
	*/
	static UK2Node_Knot* CreateRerouteNode(TArray<FPinInfos*> PreviousPinsInfos, FPinInfos* FollowingPinInfos, FVector2D ReroutePos);

	/**
	* Retrieve nodes overlapping the link between the specified nodes
	*
	* @param	PreviousNodeInfos		The node from which the link start
	* @param	FollowingNodeInfos		The node from which the link end
	*
	* @return	An array of nodes overlapping the link
	*/
	static TArray<UEdGraphNode*> GetOverlappingNodes(FNodeInfos* PreviousNodeInfos, FNodeInfos* FollowingNodeInfos);

	/**
	* Retrieve nodes overlapping the links between the specified nodes
	* Check for nodes overlapping links with several starting nodes
	*
	* @param	PreviousNodesInfos		The nodes from which the links start
	* @param	FollowingNodeInfos		The node from which the links end
	*
	* @return An array of nodes overlapping the links
	*/
	static TArray<UEdGraphNode*> GetOverlappingNodes(TArray<FNodeInfos*> PreviousNodesInfos, FNodeInfos* FollowingNodeInfos);

	/**
	* Retrieve nodes overlapping the link between the specified nodes
	* Check for nodes overlapping a link with a reroute
	*
	* @param	PreviousNodeInfos		The node from which the link start
	* @param	RerouteNodeInfos		The node rerouting the link
	* @param	FollowingNodeInfos		The node from which the link end
	*
	* @return	An array of nodes overlapping the link
	*/
	static TArray<UEdGraphNode*> GetOverlappingNodes(FNodeInfos* PreviousNodeInfos, FNodeInfos* RerouteNodeInfos, FNodeInfos* FollowingNodeInfos);

	/**
	* Retrieve nodes overlapping a straight line between the two specified positions
	*
	* @param	LineStart			The starting position of the line
	* @param	LineEnd				The ending positon of the line
	* @param	NodesToExclude		The array of nodes to exclude from the overlapping
	*
	* @return	An array of nodes overlapping the line
	*/
	static TArray<UEdGraphNode*> GetOverlappingNodesWithLine(FVector LineStart, FVector LineEnd, TArray<UEdGraphNode*> NodesToExclude);
	static TArray<UEdGraphNode*> GetOverlappingNodesWithLine(FVector LineStart, FVector LineEnd);

	/**
	* Try to position a specified reroute node between two nodes in order to avoid potential overlapping nodes
	* Returns false if it couldn't find a way above or under any nodes in the way, in this case a second reroute node may be needed
	*
	* @param	PreviousNodeInfos		The node from which the link start
	* @param	RerouteNodeInfos		The node rerouting the link
	* @param	FollowingNodeInfos		The node from which the link end
	*
	* @return	true if a good position have been found for the reroute node
	*/
	static bool HandleReroutePlacement(FNodeInfos* PreviousNodeInfos, FNodeInfos* RerouteNodeInfos, FNodeInfos* FollowingNodeInfos);

	/**
	* Try to position a specified reroute node between several starting nodes and an ending node in order to avoid potential overlapping nodes
	* Returns false if it couldn't find a way above or under any nodes in the way, in this case a second reroute node may be needed
	*
	* @param	PreviousNodesInfos		The nodes from which the links start
	* @param	RerouteNodeInfos		The node rerouting the link
	* @param	FollowingNodeInfos		The node from which the links end
	*
	* @return	true if a good position have been found for the reroute node
	*/
	static bool HandleReroutePlacement(TArray<FNodeInfos*> PreviousNodesInfos, FNodeInfos* RerouteNodeInfos, FNodeInfos* FollowingNodeInfos);

	/**
	* Create reroute nodes, move or delete them between the two specified nodes
	*
	* @param	ParentNodeInfos		The node from which the link start
	* @param	LinkedNodeInfos		The node from which the link end
	*/
	static void HandleReroutes(FNodeInfos* ParentNodeInfos, FNodeInfos* LinkedNodeInfos, FRemoveNodeHelper& RemoveHelper);

	/**
	* This function will straighten the link between the two specified nodes if needed
	* The straightening will be made by either changing the linked node position or moving an existing reroute node
	*
	* @param	ParentNodeInfos		The node from which the link start
	* @param	LinkedNodeInfos		The node from which the link end
	*/
	static void HandleStraightening(FNodeInfos* ParentNodeInfos, FNodeInfos* LinkedNodeInfos);

	/**
	* Retrieve the pins linked to the specified pin ignoring possible reroute nodes on this link
	* This function is called recursively on reroute nodes, this way it keep "skipping" reroute nodes until a pin to a non-reroute node is found
	*
	* @param	Pin					The pin to retrieve the linked pins from
	* @param	IgnoredReroutes		An array of all reroutes found and ignored during the process
	*
	* @return	An array of pins linked to the specified pin
	*/
	static TArray<FPinInfos*> GetLinkedPinsInfosIgnoringReroute(UEdGraphPin* Pin, TArray<FNodeInfos*>& IgnoredReroutes);
	static TArray<FPinInfos*> GetLinkedPinsInfosIgnoringReroute(UEdGraphPin* Pin);

	/**
	* Check if we should straighten the link from the specified pin by looking at other pins of the owning node and their links
	*
	* @param	Pin				The pin to check if it should be straighten or not
	* @param	OtherPin		The pin to the other side of the link (only used to make sure we do not compare with the link we want to check)
	* @param	bExec			Whether or not the pin is an exec pin
	*
	* @return	true if the pin should be straightened
	*/
	static bool IsPinStraightenable(UEdGraphPin* Pin, UEdGraphPin* OtherPin);
	static bool IsPinStraightenable(UEdGraphPin* Pin, UEdGraphPin* OtherPin, bool bExec);

	/**
	* Move and straighten the specified node if it is a variable node (node without input pins and with only one output pin)
	*
	* @param	ParentNodeInfos		The parent node of the variable node (used to straighten the node)
	* @param	LinkedNodeInfos		The variable node to move and straighten
	*/
	static void HandleVariableNodes(FNodeInfos* ParentNodeInfos, FNodeInfos* LinkedNodeInfos);

	/**
	* Align the specified node with nodes linked to the same node as the specified node
	*
	* @param	NodeInfos		The node to align
	*/
	static void HandleParallelNodes(FNodeInfos* NodeInfos);

	/**
	* Retrieve pins considered as "parallel pins" of the specified pin
	* Parallel pins are pins connected to the pins of the same node
	* This function will only returns pins connected to common pins situated above the common pin linked to the specified pin
	*
	* @param	Pin		The pin to retrieve the parallel pins from
	*
	* @return	An array of parallel pins infos
	*/
	static TArray<FPinInfos*> GetParallelPinsInfos(UEdGraphPin* Pin);

	/**
	* Move the specified linked node to the right or the left of the specified parent node depending on the way they are linked
	* For exemple, if a node is linked to the input of another node but is placed on the right of it, this function will move the node to the left of the other node
	*
	* @param	ParentNodeInfos		The node to check the ordering with
	* @param	LinkedNodeInfos		The node which will be moved if needed
	*/
	static void HandleHorizontalOrdering(FNodeInfos* ParentNodeInfos, FNodeInfos* LinkedNodeInfos);

	/**
	* Check if reroute nodes are not needed between the two specified nodes
	* If they are not needed, it removes them
	*
	* @param	PreviousNodeInfos		The node from which the link start
	* @param	FollowingNodeInfos		The node from which the link end
	*
	* @return	true if it have set nodes to remove, false if it could not remove nodes
	*/
	static bool RemoveRerouteIfPossible(FNodeInfos* PreviousNodeInfos, FNodeInfos* FollowingNodeInfos);

	/**
	* Remove specified nodes and make new links after the deletion
	*
	* @param	RemoveHelper		Container with the nodes to remove and the links to make
	*/
	static void HandleRemoving(FRemoveNodeHelper& RemoveHelper);

	/** A Map associating each pin with its widget */
	static TMap<UEdGraphPin*, TSharedPtr<SGraphPin>> PinMap;

	/** A Map associating each node with its widget */
	static TMap<UEdGraphNode*, TSharedPtr<SGraphNode>> NodeMap;

	/** The instance of the blueprint editor opened for each blueprint */
	static TMap<UBlueprint*, FBlueprintEditor*> BlueprintEditors;

	/** The widget of the currently opened graph
	* It is used to retrieve the node's widgets size */
	static TWeakPtr<SGraphEditor> GraphEditor;

	/** The current graph we are organizing */
	static UEdGraph* CurrentGraph;

	/** All the node infos currently created during this organization */
	static TArray<FNodeInfos> NodesInfos;

	/** All the pin infos currently created during this organization */
	static TArray<FPinInfos> PinsInfos;

	/** All pin pairs that are connected by a straighten link */
	static TArray<FPinPair> PinPairsStraightened;

	/**
	* This function returns true if the link between the two specified pins have been straightened
	*
	* @param	Pin1		One of the two pins to check the straighten link from
	* @param	Pin2		One of the two pins to check the straighten link from
	*
	* @return	true if the link between the specified pins have been straightened
	*/
	static bool IsPinPairStraightened(UEdGraphPin* Pin1, UEdGraphPin* Pin2);

	/**
	* Retrieve infos of the specified node from the Node Map if it have been created before,
	* If not, create the infos for this node and add it to the map
	*
	* @param	Node		The node to retrieve the infos from
	*/
	static FNodeInfos* RetrieveNodeInfos(UEdGraphNode* Node);

	/**
	* Retrieve infos of the specified node from the Node Map if it have been created before,
	* If not, create the infos for this node and add it to the map
	* Retrieve infos for the specified pin as well and attach it to the node
	*
	* @param	Node		The node to retrieve the infos from
	* @param	Pin			The pin to retrieve the infos from and to attach to the node
	*/
	static FNodeInfos* RetrieveNodeInfos(UEdGraphNode* Node, UEdGraphPin* Pin);

	/**
	* Retrieve infos of the specified pin from the Pin Map if it have been created before,
	* If not, create the infos for this pin using the pin factory widgets and add it to the map
	*
	* @param	Pin		The pin to retrieve the infos from
	*/
	static FPinInfos* RetrievePinInfos(UEdGraphPin* Pin);

	/**
	* Retrieve infos of the specified pin from the Pin Map if it have been created before,
	* If not, create the infos for this pin using the specified node widget and add it to the map
	*
	* @param	Pin				The pin to retrieve the infos from
	* @param	NodeWidget		The node widget to use to create the pin infos if needed
	*/
	static FPinInfos* RetrievePinInfos(UEdGraphPin* Pin, TSharedPtr<SGraphNode> NodeWidget);

	/**
	* Attach the specified pin to the specified node infos
	*
	* @param	NodeInfos		The node infos to attach the pin to
	* @param	Pin				The pin to attach to the node infos
	*/
	static void AttachPin(FNodeInfos* NodeInfos, UEdGraphPin* Pin);

	/**
	* Make links between the specified pins
	*
	* @param	LinksToMake		The pairs of pin to link
	*/
	static void MakeLinks(TArray<FPinPair> LinksToMake);

	/** All the nodes in the current graph */
	static TArray<UEdGraphNode*> GraphNodes;

	static TArray<UEdGraphNode*> DiscoveredNodes;
};

/** The Class needed to retrieve the node widgets */
class FCustomNodeFactory : public FGraphPanelNodeFactory
{
private:
	static TSharedPtr<FCustomNodeFactory> Instance;

	static UEdGraphNode* LastNode;

	FCustomNodeFactory()
	{

	}

public:

	static TSharedPtr<FCustomNodeFactory> GetInstance()
	{
		return Instance;
	}

private:

	/**
	* This function is called each time a node needs to be drawn in the graph
	* If it returns a widget (SGraphNode), it will be used for drawing the node
	* If it returns nullptr, the engine will use the default way to draw the node
	* We need this function to retrieve the widgets of the nodes without modifying the way pins are drawn
	* The node's widgets will be used to retrieve the pins widget of this node
	* The pin's widgets are needed to know the offset position of the pin in the node in order to well straighten the links
	*/
	virtual TSharedPtr<class SGraphNode> CreateNode(class UEdGraphNode* Node) const override
	{
		// The trick is to call the "CreateNodeWidget" function in FNodeFactory
		// (this is the very function that calls the one we are in now)

		// For each node, it will go this way :
		// - CreateNodeWidget(1) is called , it checks for registered factories and try to create a node with this factory
		// - CreateNode(1) is called, set static variable "LastNode" as the current node and call CreateNodeWidget again
		// - CreateNodeWidget(2) is called and call CreateNode(2) for this factory again
		// - In CreateNode(2), this time, we check the node with the one we saved in the static variable earlier, if they are the same we return nullptr
		// - Because CreateNode(2) returned nullptr, CreateNodeWidget(2) creates the engine node widget for this node and returns it
		// - CreateNode(1) finally retrieves the widget returned and returns it as well for CreateNodeWidget(1) after saving the widget for us to use

		if (LastNode == nullptr)
		{
			LastNode = Node;
		}
		else if (LastNode == Node)
		{
			// If this function returns "nullptr" the node will be drawn with the defaults engine functions
			return nullptr;
		}

		// Create the widget for the pin
		// We unregister then register again to prevent "this" from being called by CreateNodeWidget
		FEdGraphUtilities::UnregisterVisualNodeFactory(Instance);
		TSharedPtr<SGraphNode> NodeWidget = FNodeFactory::CreateNodeWidget(Node);
		FEdGraphUtilities::RegisterVisualNodeFactory(Instance);

		// Set static attribute to nullptr for the following nodes to be drawn
		LastNode = nullptr;

		// Associate the pin with its widget in order to retrieve the widget later
		FOrganizeBlueprint::NodeMap.Add(Node, NodeWidget);

		return NodeWidget;
	}
};

TSharedPtr<FCustomNodeFactory> FCustomNodeFactory::Instance = MakeShareable(new FCustomNodeFactory());
UEdGraphNode* FCustomNodeFactory::LastNode = nullptr;

/** The Class needed to retrieve the pins widgets */
class FCustomPinFactory : public FGraphPanelPinFactory
{
private:
	static UEdGraphPin* LastPin;

	/**
	* This function is called each time a pin needs to be drawn in the graph
	* We need this function to retrieve the widgets of the pins (in order to retrieve the pin offset in a node) without modifying the way pins are drawn
	* Widgets retrieved this way are only used in cases where we want to retrieve infos of a pin without having retrieved the infos of the node owning this pin before
	*/
	virtual TSharedPtr<class SGraphPin> CreatePin(class UEdGraphPin* InPin) const override
	{
		// It works the same as the Custom Node Factory but with pins

		if (LastPin == nullptr)
		{
			LastPin = InPin;
		}
		else if (LastPin == InPin)
		{
			// If this function returns "nullptr" the pin will be drawn with the defaults engine functions
			return nullptr;
		}

		// Create the widget for the pin
		TSharedPtr<SGraphPin> PinWidget = FNodeFactory::CreatePinWidget(InPin);

		// Set static attribute to nullptr for the following pins to be drawn
		LastPin = nullptr;

		// Associate the pin with its widget in order to retrieve the widget later
		FOrganizeBlueprint::PinMap.Add(InPin, PinWidget);

		return PinWidget;
	}
};

UEdGraphPin* FCustomPinFactory::LastPin = nullptr;