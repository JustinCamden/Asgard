// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#include "SMGraphK2Node_TransitionInitializedNode.h"
#include "EdGraph/EdGraph.h"
#include "Graph/Schema/SMGraphK2Schema.h"
#include "Graph/SMTransitionGraph.h"
#include "Graph/SMConduitGraph.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "SMBlueprintEditorUtils.h"
#include "SMBlueprint.h"


#define LOCTEXT_NAMESPACE "SMTransitionInitializedNode"

USMGraphK2Node_TransitionInitializedNode::USMGraphK2Node_TransitionInitializedNode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAllowMoreThanOneNode = true;
}

void USMGraphK2Node_TransitionInitializedNode::AllocateDefaultPins()
{
	CreatePin(EGPD_Output, USMGraphK2Schema::PC_Exec, UEdGraphSchema_K2::PN_Then);
}

void USMGraphK2Node_TransitionInitializedNode::PostPlacedNewNode()
{
	RuntimeNodeGuid = GetRuntimeContainer()->GetRunTimeNodeChecked()->GetNodeGuid();
}

FText USMGraphK2Node_TransitionInitializedNode::GetMenuCategory() const
{
	return FText::FromString(STATE_MACHINE_HELPER_CATEGORY);
}

FText USMGraphK2Node_TransitionInitializedNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	const bool IsConduit = Cast<USMConduitGraph>(FSMBlueprintEditorUtils::FindTopLevelOwningGraph(GetGraph())) != nullptr;
	
	if ((TitleType == ENodeTitleType::MenuTitle || TitleType == ENodeTitleType::ListView))
	{
		return IsConduit ? LOCTEXT("AddConduitInitializedEvent", "Add Event On Conduit Initialized") : LOCTEXT("AddTransitionInitializedEvent", "Add Event On Transition Initialized");
	}

	return FText::FromString(IsConduit ? TEXT("On Conduit Initialized") : TEXT("On Transition Initialized"));
}

FText USMGraphK2Node_TransitionInitializedNode::GetTooltipText() const
{
	return LOCTEXT("TransitionInitializedNodeTooltip", "Called when the state leading to this node is entered.");
}

void USMGraphK2Node_TransitionInitializedNode::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();

	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);
		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

bool USMGraphK2Node_TransitionInitializedNode::IsActionFilteredOut(FBlueprintActionFilter const& Filter)
{
	for (UBlueprint* Blueprint : Filter.Context.Blueprints)
	{
		if (!Cast<USMBlueprint>(Blueprint))
		{
			return true;
		}
	}

	for (UEdGraph* Graph : Filter.Context.Graphs)
	{
		// Only works on transition and conduit graphs.
		if (!Graph->IsA<USMTransitionGraph>() && !Graph->IsA<USMConduitGraph>())
		{
			return true;
		}
	}

	return false;
}

bool USMGraphK2Node_TransitionInitializedNode::IsCompatibleWithGraph(UEdGraph const* Graph) const
{
	return Graph->IsA<USMTransitionGraph>() || Graph->IsA<USMConduitGraph>();
}

#undef LOCTEXT_NAMESPACE
