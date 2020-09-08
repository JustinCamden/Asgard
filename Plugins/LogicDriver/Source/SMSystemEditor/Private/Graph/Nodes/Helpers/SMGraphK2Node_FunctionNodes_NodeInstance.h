// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "SMGraphK2Node_FunctionNodes.h"
#include "SMGraphK2Node_FunctionNodes_NodeInstance.generated.h"

UCLASS(MinimalAPI)
class USMGraphK2Node_FunctionNode_NodeInstance : public USMGraphK2Node_FunctionNode
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	FText GetMenuCategory() const override;
	bool IsCompatibleWithGraph(UEdGraph const* Graph) const override;
	FText GetTooltipText() const override;
	void AllocateDefaultPins() override;
	//~ UEdGraphNode

	// USMGraphK2Node_RuntimeNodeReference
	void PreCompileValidate(FCompilerResultsLog& MessageLog) override;
	bool HandlesOwnExpansion() const override { return true; }
	void CustomExpandNode(FSMKismetCompilerContext& CompilerContext, USMGraphK2Node_RuntimeNodeContainer* RuntimeNodeContainer, FProperty* NodeProperty) override;
	// ~USMGraphK2Node_RuntimeNodeReference

	// USMGraphK2Node_FunctionNode
	/** Creates a function node and wires execution pins. The self pin can be null and will be used from the auto created cast node. */
	bool ExpandAndWireStandardFunction(UFunction* Function, UEdGraphPin* SelfPin, FSMKismetCompilerContext& CompilerContext, USMGraphK2Node_RuntimeNodeContainer* RuntimeNodeContainer, FProperty* NodeProperty) override;
	// ~USMGraphK2Node_FunctionNode
	
	UPROPERTY()
	TSubclassOf<UObject> NodeInstanceClass;
};

////////////////////////
/// Node Base Classes
////////////////////////

UCLASS(MinimalAPI)
class USMGraphK2Node_StateInstance_Base : public USMGraphK2Node_FunctionNode_NodeInstance
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	bool IsCompatibleWithGraph(UEdGraph const* Graph) const override;
	// ~UEdGraphNode
};

UCLASS(MinimalAPI)
class USMGraphK2Node_TransitionInstance_Base : public USMGraphK2Node_FunctionNode_NodeInstance
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	bool IsCompatibleWithGraph(UEdGraph const* Graph) const override;
	// ~UEdGraphNode
};

UCLASS(MinimalAPI)
class USMGraphK2Node_ConduitInstance_Base : public USMGraphK2Node_FunctionNode_NodeInstance
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	bool IsCompatibleWithGraph(UEdGraph const* Graph) const override;
	// ~UEdGraphNode
};

////////////////////////
/// Usable Node Classes
////////////////////////

UCLASS(MinimalAPI)
class USMGraphK2Node_StateInstance_Begin : public USMGraphK2Node_StateInstance_Base
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	//~ UEdGraphNode

	// USMGraphK2Node_RuntimeNodeReference
	void CustomExpandNode(FSMKismetCompilerContext& CompilerContext, USMGraphK2Node_RuntimeNodeContainer* RuntimeNodeContainer, FProperty* NodeProperty) override;
	// ~USMGraphK2Node_RuntimeNodeReference
};

UCLASS(MinimalAPI)
class USMGraphK2Node_StateInstance_Update : public USMGraphK2Node_StateInstance_Base
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	void AllocateDefaultPins() override;
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	//~ UEdGraphNode

	// USMGraphK2Node_RuntimeNodeReference
	void CustomExpandNode(FSMKismetCompilerContext& CompilerContext, USMGraphK2Node_RuntimeNodeContainer* RuntimeNodeContainer, FProperty* NodeProperty) override;
	// ~USMGraphK2Node_RuntimeNodeReference
};

UCLASS(MinimalAPI)
class USMGraphK2Node_StateInstance_End : public USMGraphK2Node_StateInstance_Base
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	//~ UEdGraphNode

	// USMGraphK2Node_RuntimeNodeReference
	void CustomExpandNode(FSMKismetCompilerContext& CompilerContext, USMGraphK2Node_RuntimeNodeContainer* RuntimeNodeContainer, FProperty* NodeProperty) override;
	// ~USMGraphK2Node_RuntimeNodeReference
};

UCLASS(MinimalAPI)
class USMGraphK2Node_StateInstance_StateMachineStart : public USMGraphK2Node_StateInstance_Base
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	//~ UEdGraphNode

	// USMGraphK2Node_RuntimeNodeReference
	void CustomExpandNode(FSMKismetCompilerContext& CompilerContext, USMGraphK2Node_RuntimeNodeContainer* RuntimeNodeContainer, FProperty* NodeProperty) override;
	// ~USMGraphK2Node_RuntimeNodeReference
};

UCLASS(MinimalAPI)
class USMGraphK2Node_StateInstance_StateMachineStop : public USMGraphK2Node_StateInstance_Base
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	//~ UEdGraphNode

	// USMGraphK2Node_RuntimeNodeReference
	void CustomExpandNode(FSMKismetCompilerContext& CompilerContext, USMGraphK2Node_RuntimeNodeContainer* RuntimeNodeContainer, FProperty* NodeProperty) override;
	// ~USMGraphK2Node_RuntimeNodeReference
};

UCLASS(MinimalAPI)
class USMGraphK2Node_TransitionInstance_CanEnterTransition : public USMGraphK2Node_TransitionInstance_Base
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	void AllocateDefaultPins() override;
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	bool IsNodePure() const override { return true; }
	//~ UEdGraphNode

	// USMGraphK2Node_RuntimeNodeReference
	void CustomExpandNode(FSMKismetCompilerContext& CompilerContext, USMGraphK2Node_RuntimeNodeContainer* RuntimeNodeContainer, FProperty* NodeProperty) override;
	// ~USMGraphK2Node_RuntimeNodeReference
};

UCLASS(MinimalAPI)
class USMGraphK2Node_TransitionInstance_OnTransitionTaken : public USMGraphK2Node_TransitionInstance_Base
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	//~ UEdGraphNode

	// USMGraphK2Node_RuntimeNodeReference
	void CustomExpandNode(FSMKismetCompilerContext& CompilerContext, USMGraphK2Node_RuntimeNodeContainer* RuntimeNodeContainer, FProperty* NodeProperty) override;
	// ~USMGraphK2Node_RuntimeNodeReference
};

UCLASS(MinimalAPI)
class USMGraphK2Node_TransitionInstance_OnTransitionInitialized : public USMGraphK2Node_TransitionInstance_Base
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	//~ UEdGraphNode

	// USMGraphK2Node_RuntimeNodeReference
	void CustomExpandNode(FSMKismetCompilerContext& CompilerContext, USMGraphK2Node_RuntimeNodeContainer* RuntimeNodeContainer, FProperty* NodeProperty) override;
	// ~USMGraphK2Node_RuntimeNodeReference
};

UCLASS(MinimalAPI)
class USMGraphK2Node_TransitionInstance_OnTransitionShutdown : public USMGraphK2Node_TransitionInstance_Base
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	//~ UEdGraphNode

	// USMGraphK2Node_RuntimeNodeReference
	void CustomExpandNode(FSMKismetCompilerContext& CompilerContext, USMGraphK2Node_RuntimeNodeContainer* RuntimeNodeContainer, FProperty* NodeProperty) override;
	// ~USMGraphK2Node_RuntimeNodeReference
};

UCLASS(MinimalAPI)
class USMGraphK2Node_ConduitInstance_CanEnterTransition : public USMGraphK2Node_ConduitInstance_Base
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	void AllocateDefaultPins() override;
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	bool IsNodePure() const override { return true; }
	//~ UEdGraphNode

	// USMGraphK2Node_RuntimeNodeReference
	void CustomExpandNode(FSMKismetCompilerContext& CompilerContext, USMGraphK2Node_RuntimeNodeContainer* RuntimeNodeContainer, FProperty* NodeProperty) override;
	// ~USMGraphK2Node_RuntimeNodeReference
};

UCLASS(MinimalAPI)
class USMGraphK2Node_ConduitInstance_OnConduitEntered : public USMGraphK2Node_ConduitInstance_Base
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	//~ UEdGraphNode

	// USMGraphK2Node_RuntimeNodeReference
	void CustomExpandNode(FSMKismetCompilerContext& CompilerContext, USMGraphK2Node_RuntimeNodeContainer* RuntimeNodeContainer, FProperty* NodeProperty) override;
	// ~USMGraphK2Node_RuntimeNodeReference
};

UCLASS(MinimalAPI)
class USMGraphK2Node_ConduitInstance_OnConduitInitialized : public USMGraphK2Node_ConduitInstance_Base
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	//~ UEdGraphNode

	// USMGraphK2Node_RuntimeNodeReference
	void CustomExpandNode(FSMKismetCompilerContext& CompilerContext, USMGraphK2Node_RuntimeNodeContainer* RuntimeNodeContainer, FProperty* NodeProperty) override;
	// ~USMGraphK2Node_RuntimeNodeReference
};

UCLASS(MinimalAPI)
class USMGraphK2Node_ConduitInstance_OnConduitShutdown : public USMGraphK2Node_ConduitInstance_Base
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	//~ UEdGraphNode

	// USMGraphK2Node_RuntimeNodeReference
	void CustomExpandNode(FSMKismetCompilerContext& CompilerContext, USMGraphK2Node_RuntimeNodeContainer* RuntimeNodeContainer, FProperty* NodeProperty) override;
	// ~USMGraphK2Node_RuntimeNodeReference
};