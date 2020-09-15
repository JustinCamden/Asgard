// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#include "SMPropertyGraph.h"
#include "Utilities/SMBlueprintEditorUtils.h"
#include "ScopedTransaction.h"


USMPropertyGraph::USMPropertyGraph(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), ResultNode(nullptr), TempGraphProperty(nullptr)
{
	bAllowDeletion = false;
}

void USMPropertyGraph::RefreshProperty(bool bModify)
{
	// This can be set if the graph was deleted then the action undone.
	ClearFlags(RF_Transient);

	if (ResultNode)
	{
		ResultNode->SetPinValueFromPropertyDefaults();
	}
	
	if (bModify)
	{
		UBlueprint* Blueprint = FSMBlueprintEditorUtils::FindBlueprintForGraphChecked(this);
		FSMBlueprintEditorUtils::ConditionallyCompileBlueprint(Blueprint);
	}
	
	if(ResultNode)
	{
		ResultNode->ReconstructNode();
	}
}

void USMPropertyGraph::ResetGraph()
{
	Modify();

	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(this);
	// Clear out existing nodes since this graph supports reconstruction without deletion.
	TArray<UEdGraphNode*> NodesToDelete = Nodes;
	for (UEdGraphNode* Node : NodesToDelete)
	{
		// Don't delete the main result node.
		if (Node == ResultNode)
		{
			continue;
		}

		FBlueprintEditorUtils::RemoveNode(Blueprint, Node, true);
	}

	// Recreate any existing default nodes except the main node saved above.
	GetSchema()->CreateDefaultNodesForGraph(*this);
	ResultNode->OwningGraphNode->InitPropertyGraphNodes(this, ResultNode->GetPropertyNode());
}

void USMPropertyGraph::SetUsingGraphToEdit(bool bValue)
{
	Modify();
	bEditable = bValue;
}

void USMPropertyGraph::ToggleGraphPropertyEdit()
{
	SetUsingGraphToEdit(!IsGraphBeingUsedToEdit());
	// Forces details panel to update.
	FSMBlueprintEditorUtils::ConditionallyCompileBlueprint(FSMBlueprintEditorUtils::FindBlueprintForGraphChecked(this));
}

void USMPropertyGraph::SetPropertyOnGraph(FProperty* Property)
{
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "SetObject", "Set Object"));

	Modify();
	ResetGraph();

	const FName VariableName = Property->GetFName();

	// TODO: Error message if variable doesn't exist.
	FBPVariableDescription Variable;
	UBlueprint* Blueprint = FSMBlueprintEditorUtils::FindBlueprintForGraphChecked(this);
	if (FSMBlueprintEditorUtils::TryGetVariableByName(Blueprint, VariableName, Variable) ||
		FSMBlueprintEditorUtils::GetPropertyForVariable(Blueprint, VariableName))
	{
		UEdGraphPin* ResultPin = ResultNode->GetResultPinChecked();
		FSMBlueprintEditorUtils::PlacePropertyOnGraph(this, Property, ResultPin, nullptr, 50);
	}
}

void USMPropertyGraph::SetFunctionOnGraph(UFunction* Function)
{
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "SetObject", "Set Object"));

	Modify();
	ResetGraph();

	UEdGraphPin* ResultPin = ResultNode->GetResultPinChecked();
	FSMBlueprintEditorUtils::PlaceFunctionOnGraph(this, Function, ResultPin, nullptr, nullptr, 50);
}

void USMPropertyGraph::PruneDisconnectedNodes()
{
	if(ResultNode)
	{
		UBlueprint* Blueprint = FSMBlueprintEditorUtils::FindBlueprintForGraphChecked(this);
		bool bChanged = false;
		
		TSet<UEdGraphNode*> ConnectedNodes;
		FSMBlueprintEditorUtils::GetAllConnectedNodes(ResultNode, EGPD_Input, ConnectedNodes);

		TArray<UEdGraphNode*> AllNodes;
		FSMBlueprintEditorUtils::GetAllNodesOfClassNested<UEdGraphNode>(this, AllNodes);

		for(UEdGraphNode* Node : AllNodes)
		{
			if(!ConnectedNodes.Contains(Node))
			{
				RemoveNode(Node);
				bChanged = true;
			}
		}

		if(bChanged)
		{
			FSMBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
		}
	}
}
