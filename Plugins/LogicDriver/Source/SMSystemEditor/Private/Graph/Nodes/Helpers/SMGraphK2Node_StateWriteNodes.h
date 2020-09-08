// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "Graph/Nodes/RootNodes/SMGraphK2Node_RuntimeNodeContainer.h"
#include "SMGraphK2Node_StateWriteNodes.generated.h"


UCLASS(MinimalAPI)
class USMGraphK2Node_StateWriteNode : public USMGraphK2Node_RuntimeNodeReference
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	void PostPlacedNewNode() override;
	void PostPasteNode() override;
	bool IsCompatibleWithGraph(UEdGraph const* Graph) const override;
	bool CanUserDeleteNode() const override { return true; }
	bool CanDuplicateNode() const override { return true; }
	bool IsNodePure() const override { return false; }
	FText GetMenuCategory() const override;
	bool IsActionFilteredOut(class FBlueprintActionFilter const& Filter) override;
	//~ UEdGraphNode

	// USMGraphK2Node_Base
	bool CanCollapseNode() const override { return true; }
	bool CanCollapseToFunctionOrMacro() const override { return true; }
	UEdGraphPin* GetInputPin() const override;
	//~ End UEdGraphNode Interface
};


UCLASS(MinimalAPI)
class USMGraphK2Node_StateWriteNode_CanEvaluate : public USMGraphK2Node_StateWriteNode
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	void AllocateDefaultPins() override;
	bool IsCompatibleWithGraph(UEdGraph const* Graph) const override;
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FText GetTooltipText() const override;
	//~ UEdGraphNode
};


UCLASS(MinimalAPI)
class USMGraphK2Node_StateWriteNode_CanEvaluateFromEvent : public USMGraphK2Node_StateWriteNode
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	void AllocateDefaultPins() override;
	bool IsCompatibleWithGraph(UEdGraph const* Graph) const override;
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FText GetTooltipText() const override;
	//~ UEdGraphNode
};


UCLASS(MinimalAPI)
class USMGraphK2Node_StateWriteNode_TransitionEventReturn : public USMGraphK2Node_StateWriteNode
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = "Transition Event")
	bool bEventTriggersUpdate;
	
	// UEdGraphNode
	void AllocateDefaultPins() override;
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	bool IsCompatibleWithGraph(UEdGraph const* Graph) const override;
	bool IsActionFilteredOut(FBlueprintActionFilter const& Filter) override;
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	bool CanUserDeleteNode() const override { return true; }
	bool CanDuplicateNode() const override { return true; }
	bool ShouldShowNodeProperties() const override { return true; }
	bool DrawNodeAsExit() const override { return true; }
	//~ UEdGraphNode

	// USMGraphK2Node_RuntimeNodeReference
	bool HandlesOwnExpansion() const override { return true; }
	void CustomExpandNode(FSMKismetCompilerContext& CompilerContext, USMGraphK2Node_RuntimeNodeContainer* RuntimeNodeContainer, FProperty* NodeProperty) override;
	// ~USMGraphK2Node_RuntimeNodeReference

};