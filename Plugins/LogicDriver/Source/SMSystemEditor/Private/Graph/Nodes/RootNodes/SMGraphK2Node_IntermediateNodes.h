// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "Graph/Nodes/SMGraphNode_StateMachineEntryNode.h"
#include "SMGraphK2Node_IntermediateNodes.generated.h"


/**
 * State Start override for intermediate graphs.
 */
UCLASS(MinimalAPI)
class USMGraphK2Node_IntermediateEntryNode : public USMGraphK2Node_StateMachineEntryNode
{
	GENERATED_UCLASS_BODY()

	//~ Begin UEdGraphNode Interface
	void AllocateDefaultPins() override;
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FText GetTooltipText() const override;
	bool IsCompatibleWithGraph(UEdGraph const* Graph) const override;
	//~ End UEdGraphNode Interface
};

/**
 * This blueprint's root State machine start entry point
 */
UCLASS(MinimalAPI)
class USMGraphK2Node_IntermediateStateMachineStartNode : public USMGraphK2Node_RuntimeNodeReference
{
	GENERATED_UCLASS_BODY()

	//~ Begin UEdGraphNode Interface
	bool CanUserDeleteNode() const override { return true; }
	void AllocateDefaultPins() override;
	ERedirectType DoPinsMatchForReconstruction(const UEdGraphPin* NewPin, int32 NewPinIndex, const UEdGraphPin* OldPin, int32 OldPinIndex) const override;
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FText GetTooltipText() const override;
	FText GetMenuCategory() const override;
	bool IsCompatibleWithGraph(UEdGraph const* Graph) const override;
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	bool IsActionFilteredOut(class FBlueprintActionFilter const& Filter) override;
	void PostPlacedNewNode() override;
	//~ End UEdGraphNode Interface
};

/**
 * When the blueprint's root state machine stops.
 */
UCLASS(MinimalAPI)
class USMGraphK2Node_IntermediateStateMachineStopNode : public USMGraphK2Node_RuntimeNodeReference
{
	GENERATED_UCLASS_BODY()

	//~ Begin UEdGraphNode Interface
	bool CanUserDeleteNode() const override { return true; }
	void AllocateDefaultPins() override;
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FText GetTooltipText() const override;
	FText GetMenuCategory() const override;
	bool IsCompatibleWithGraph(UEdGraph const* Graph) const override;
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	bool IsActionFilteredOut(class FBlueprintActionFilter const& Filter) override;
	void PostPlacedNewNode() override;
	//~ End UEdGraphNode Interface
};