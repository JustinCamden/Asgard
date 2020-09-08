// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "Graph/Nodes/RootNodes/SMGraphK2Node_RuntimeNodeContainer.h"
#include "Graph/Nodes/SMGraphNode_StateNode.h"
#include "SMGraphK2Node_StateReadNodes.generated.h"


UCLASS(MinimalAPI)
class USMGraphK2Node_StateReadNode : public USMGraphK2Node_RuntimeNodeReference
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	void PostPlacedNewNode() override;
	void PostPasteNode() override;
	bool IsCompatibleWithGraph(UEdGraph const* Graph) const override;
	bool CanUserDeleteNode() const override { return true; }
	bool CanDuplicateNode() const override { return true; }
	bool IsNodePure() const override { return true; }
	FText GetMenuCategory() const override;
	bool IsActionFilteredOut(class FBlueprintActionFilter const& Filter) override;
	//~ UEdGraphNode

	// USMGraphK2Node_Base
	bool CanCollapseNode() const override { return true; }
	bool CanCollapseToFunctionOrMacro() const override { return true; }
	//~ End UEdGraphNode Interface

	/** Returns either the current state or the FromState of a transition. */
	virtual FString GetMostRecentStateName() const;

	/** Returns the current transition name. Only valid if in a transition graph. */
	virtual FString GetTransitionName() const;

	virtual USMGraphNode_StateNodeBase* GetMostRecentState() const;

};


UCLASS(MinimalAPI)
class USMGraphK2Node_StateReadNode_HasStateUpdated : public USMGraphK2Node_StateReadNode
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	void AllocateDefaultPins() override;
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FText GetTooltipText() const override;
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	//~ UEdGraphNode
};


UCLASS(MinimalAPI)
class USMGraphK2Node_StateReadNode_TimeInState : public USMGraphK2Node_StateReadNode
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	void AllocateDefaultPins() override;
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FText GetTooltipText() const override;
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	//~ UEdGraphNode
};


UCLASS(MinimalAPI)
class USMGraphK2Node_StateReadNode_CanEvaluate : public USMGraphK2Node_StateReadNode
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	void AllocateDefaultPins() override;
	bool IsCompatibleWithGraph(UEdGraph const* Graph) const override;
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FText GetTooltipText() const override;
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	//~ UEdGraphNode
};


UCLASS(MinimalAPI)
class USMGraphK2Node_StateReadNode_CanEvaluateFromEvent : public USMGraphK2Node_StateReadNode
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	void AllocateDefaultPins() override;
	bool IsCompatibleWithGraph(UEdGraph const* Graph) const override;
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FText GetTooltipText() const override;
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	//~ UEdGraphNode
};


UCLASS(MinimalAPI)
class USMGraphK2Node_StateReadNode_GetStateInformation : public USMGraphK2Node_StateReadNode
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	void AllocateDefaultPins() override;
	bool IsCompatibleWithGraph(UEdGraph const* Graph) const override;
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FText GetTooltipText() const override;
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	//~ UEdGraphNode

	// USMGraphK2Node_RuntimeNodeReference
	bool HandlesOwnExpansion() const override { return true; }
	void CustomExpandNode(FSMKismetCompilerContext& CompilerContext, USMGraphK2Node_RuntimeNodeContainer* RuntimeNodeContainer, FProperty* NodeProperty) override;
	// ~USMGraphK2Node_RuntimeNodeReference
};

UCLASS(MinimalAPI)
class USMGraphK2Node_StateReadNode_GetTransitionInformation : public USMGraphK2Node_StateReadNode
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	void AllocateDefaultPins() override;
	bool IsCompatibleWithGraph(UEdGraph const* Graph) const override;
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FText GetTooltipText() const override;
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	//~ UEdGraphNode

	// USMGraphK2Node_RuntimeNodeReference
	bool HandlesOwnExpansion() const override { return true; }
	void CustomExpandNode(FSMKismetCompilerContext& CompilerContext, USMGraphK2Node_RuntimeNodeContainer* RuntimeNodeContainer, FProperty* NodeProperty) override;
	// ~USMGraphK2Node_RuntimeNodeReference
};

UCLASS(MinimalAPI)
class USMGraphK2Node_StateReadNode_GetStateMachineReference : public USMGraphK2Node_StateReadNode
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	void AllocateDefaultPins() override;
	bool IsCompatibleWithGraph(UEdGraph const* Graph) const override;
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FText GetTooltipText() const override;
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	void PostPasteNode() override;
	bool HasExternalDependencies(TArray<class UStruct*>* OptionalOutput) const override;
	FBlueprintNodeSignature GetSignature() const override;
	//~ UEdGraphNode

	// USMGraphK2Node_RuntimeNodeReference
	bool HandlesOwnExpansion() const override { return true; }
	void CustomExpandNode(FSMKismetCompilerContext& CompilerContext, USMGraphK2Node_RuntimeNodeContainer* RuntimeNodeContainer, FProperty* NodeProperty) override;
	// ~USMGraphK2Node_RuntimeNodeReference

	TSubclassOf<class UObject> GetStateMachineReferenceClass() const;

	/** The class type this is referencing. The output pin will be dynamic cast to this.
	 * When force replacing references this can cause warnings, but is present in other UE4 blueprints. 
	 */
	UPROPERTY()
	TSubclassOf<class UObject> ReferencedObject;
};


UCLASS(MinimalAPI)
class USMGraphK2Node_StateMachineReadNode : public USMGraphK2Node_StateReadNode
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	bool IsCompatibleWithGraph(UEdGraph const* Graph) const override;
	bool IsActionFilteredOut(class FBlueprintActionFilter const& Filter) override;
	void ValidateNodeDuringCompilation(FCompilerResultsLog& MessageLog) const override;
	// ~UEdGraphNode
};


UCLASS(MinimalAPI)
class USMGraphK2Node_StateMachineReadNode_InEndState : public USMGraphK2Node_StateMachineReadNode
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	void AllocateDefaultPins() override;
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FText GetTooltipText() const override;
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	//~ UEdGraphNode
};

UCLASS(MinimalAPI)
class USMGraphK2Node_StateReadNode_GetNodeInstance : public USMGraphK2Node_StateReadNode
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	void AllocateDefaultPins() override;
	bool IsCompatibleWithGraph(UEdGraph const* Graph) const override;
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FText GetTooltipText() const override;
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	void PostPasteNode() override;
	FBlueprintNodeSignature GetSignature() const override;
	//~ UEdGraphNode

	// USMGraphK2Node_RuntimeNodeReference
	bool HandlesOwnExpansion() const override { return true; }
	void CustomExpandNode(FSMKismetCompilerContext& CompilerContext, USMGraphK2Node_RuntimeNodeContainer* RuntimeNodeContainer, FProperty* NodeProperty) override;
	// ~USMGraphK2Node_RuntimeNodeReference

	void AllocatePinsForType(TSubclassOf<class UObject> TargetType);
	
	/** The class type this is referencing. The output pin will be dynamic cast to this.
	 * When force replacing references this can cause warnings, but is present in other UE4 blueprints. 
	 */
	UPROPERTY()
	TSubclassOf<class UObject> ReferencedObject;

	static void CreateAndWireExpandedNodes(UEdGraphNode* SourceNode, const TSubclassOf<UObject> Class, FSMKismetCompilerContext& CompilerContext,
		USMGraphK2Node_RuntimeNodeContainer* RuntimeNodeContainer, FProperty* NodeProperty, class UK2Node_DynamicCast** CastOutputNode);
};