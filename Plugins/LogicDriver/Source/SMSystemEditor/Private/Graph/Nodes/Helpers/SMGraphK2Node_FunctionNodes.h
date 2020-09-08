// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "Graph/Nodes/RootNodes/SMGraphK2Node_RuntimeNodeContainer.h"
#include "SMGraphK2Node_FunctionNodes.generated.h"


UCLASS(MinimalAPI)
class USMGraphK2Node_FunctionNode : public USMGraphK2Node_RuntimeNodeReference
{
public:
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

	virtual bool ExpandAndWireStandardFunction(UFunction* Function, UEdGraphPin* SelfPin, FSMKismetCompilerContext& CompilerContext, USMGraphK2Node_RuntimeNodeContainer* RuntimeNodeContainer, FProperty* NodeProperty);
};

UCLASS(MinimalAPI)
class USMGraphK2Node_FunctionNode_StateMachineRef : public USMGraphK2Node_FunctionNode
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	bool IsCompatibleWithGraph(UEdGraph const* Graph) const override;
	//~ UEdGraphNode

	// USMGraphK2Node_RuntimeNodeReference
	bool HandlesOwnExpansion() const override { return true; }
	void CustomExpandNode(FSMKismetCompilerContext& CompilerContext, USMGraphK2Node_RuntimeNodeContainer* RuntimeNodeContainer, FProperty* NodeProperty) override;
	// ~USMGraphK2Node_RuntimeNodeReference
};

UCLASS(MinimalAPI)
class USMGraphK2Node_StateMachineRef_Start : public USMGraphK2Node_FunctionNode_StateMachineRef
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	void AllocateDefaultPins() override;
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FText GetTooltipText() const override;
	//~ UEdGraphNode

	// USMGraphK2Node_RuntimeNodeReference
	void CustomExpandNode(FSMKismetCompilerContext& CompilerContext, USMGraphK2Node_RuntimeNodeContainer* RuntimeNodeContainer, FProperty* NodeProperty) override;
	// ~USMGraphK2Node_RuntimeNodeReference
};

UCLASS(MinimalAPI)
class USMGraphK2Node_StateMachineRef_Update : public USMGraphK2Node_FunctionNode_StateMachineRef
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	void AllocateDefaultPins() override;
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FText GetTooltipText() const override;
	//~ UEdGraphNode

	// USMGraphK2Node_RuntimeNodeReference
	void CustomExpandNode(FSMKismetCompilerContext& CompilerContext, USMGraphK2Node_RuntimeNodeContainer* RuntimeNodeContainer, FProperty* NodeProperty) override;
	// ~USMGraphK2Node_RuntimeNodeReference
};

UCLASS(MinimalAPI)
class USMGraphK2Node_StateMachineRef_Stop : public USMGraphK2Node_FunctionNode_StateMachineRef
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	void AllocateDefaultPins() override;
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FText GetTooltipText() const override;
	//~ UEdGraphNode

	// USMGraphK2Node_RuntimeNodeReference
	void CustomExpandNode(FSMKismetCompilerContext& CompilerContext, USMGraphK2Node_RuntimeNodeContainer* RuntimeNodeContainer, FProperty* NodeProperty) override;
	// ~USMGraphK2Node_RuntimeNodeReference
};

UENUM(BlueprintType)
enum ESMDelegateOwner
{
	SMDO_This			UMETA(DisplayName = "This"),
	SMDO_Context		UMETA(DisplayName = "Context")
};

UCLASS(MinimalAPI)
class USMGraphK2Node_FunctionNode_TransitionEvent : public USMGraphK2Node_FunctionNode
{
	GENERATED_UCLASS_BODY()
	
	UPROPERTY()
	FName DelegatePropertyName;
	
	UPROPERTY()
	TSubclassOf<UObject> DelegateOwnerClass;

	UPROPERTY()
	TEnumAsByte<ESMDelegateOwner> DelegateOwnerInstance;
	
	UPROPERTY()
	FMemberReference EventReference;
	
	// UEdGraphNode
	void AllocateDefaultPins() override;
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	bool IsCompatibleWithGraph(UEdGraph const* Graph) const override;
	bool IsActionFilteredOut(FBlueprintActionFilter const& Filter) override;
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	bool CanUserDeleteNode() const override { return false; }
	bool CanDuplicateNode() const override { return false; }
	bool HasExternalDependencies(TArray<UStruct*>* OptionalOutput) const override;
	void ReconstructNode() override;
	//~ UEdGraphNode

	// USMGraphK2Node_Base
	void PostCompileValidate(FCompilerResultsLog& MessageLog) override;
	// ~USMGraphK2Node_Base
	
	// USMGraphK2Node_RuntimeNodeReference
	bool HandlesOwnExpansion() const override { return true; }
	void CustomExpandNode(FSMKismetCompilerContext& CompilerContext, USMGraphK2Node_RuntimeNodeContainer* RuntimeNodeContainer, FProperty* NodeProperty) override;
	// ~USMGraphK2Node_RuntimeNodeReference

	void SetEventReferenceFromDelegate(FMulticastDelegateProperty* Delegate, ESMDelegateOwner InstanceType);
	
	UFunction* GetDelegateFunction() const;
	void UpdateNodeFromFunction();
};