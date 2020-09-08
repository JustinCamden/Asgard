// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "Graph/Nodes/SMGraphNode_Base.h"
#include "Helpers/SMGraphK2Node_FunctionNodes.h"
#include "SMTransitionInstance.h"
#include "SMGraphNode_TransitionEdge.generated.h"


struct FSMTransition;
class USMGraphNode_StateNodeBase;

UCLASS()
class SMSYSTEMEDITOR_API USMGraphNode_TransitionEdge : public USMGraphNode_Base
{
	GENERATED_UCLASS_BODY()

	/**
	 * The instance which contains the delegate during run-time.
	 * This - The state machine instance.
	 * Context - The context of the state machine.
	 */
	UPROPERTY(EditAnywhere, Category = "Event Trigger")
	TEnumAsByte<ESMDelegateOwner> DelegateOwnerInstance;

	/**
	 * The class of the instance containing the delegate.
	 */
	UPROPERTY(EditAnywhere, Category = "Event Trigger")
	TSubclassOf<UObject> DelegateOwnerClass;

	/**
	 * Available delegates.
	 */
	UPROPERTY(EditAnywhere, Category = "Event Trigger")
	FName DelegatePropertyName;
	
	/**
	 * @deprecated Set on the node template instead.
	 */
	UPROPERTY()
	int32 PriorityOrder_DEPRECATED;

	/**
	 * @deprecated Set on the node template instead.
	 */
	UPROPERTY()
	bool bCanEvaluate_DEPRECATED;

	/**
	 * @deprecated Set on the node template instead.
	 */
	UPROPERTY()
	bool bCanEvaluateFromEvent_DEPRECATED;
	
	/**
	 * @deprecated Set on the node template instead.
	 */
	UPROPERTY()
	bool bCanEvalWithStartState_DEPRECATED;
	
	UPROPERTY(EditAnywhere, NoClear, Category = "Class", meta = (BlueprintBaseOnly))
	TSubclassOf<USMTransitionInstance> TransitionClass;
	
	/** Copy configuration settings to the runtime node. */
	void SetRuntimeDefaults(FSMTransition& Transition) const;

	/** Copy configurable settings from another transition node. */
	void CopyFrom(const USMGraphNode_TransitionEdge& Transition);
	
	//~ Begin UEdGraphNode Interface
	void AllocateDefaultPins() override;
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	void PinConnectionListChanged(UEdGraphPin* Pin) override;
	void PostPlacedNewNode() override;
	void PrepareForCopying() override;
	void PostPasteNode() override;
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	void DestroyNode() override;
	bool CanDuplicateNode() const override { return true; }
	//~ End UEdGraphNode Interface

	// USMGraphNode_Base
	void ImportDeprecatedProperties() override;
	void PlaceDefaultInstanceNodes() override;
	FName GetFriendlyNodeName() const override { return "Transition"; }
	FLinearColor GetActiveBackgroundColor() const override;
	UClass* GetNodeClass() const override { return TransitionClass; }
	void SetNodeClass(UClass* Class) override;
	bool SupportsPropertyGraphs() const override { return false; }
	// ~USMGraphNode_Base
	
	FLinearColor GetTransitionColor(bool bIsHovered) const;

	float GetMaxDebugTime() const override;
	
	void CreateBoundGraph();
	void SetBoundGraph(UEdGraph* Graph);
	
	UClass* GetSelectedDelegateOwnerClass() const;
	
	void SetupDelegateDefaults();
	void InitTransitionDelegate();
	void GoToTransitionEventNode();
	
	FString GetTransitionName() const;
	void CreateConnections(USMGraphNode_StateNodeBase* Start, USMGraphNode_StateNodeBase* End);
	bool PossibleToTransition() const;

	USMGraphNode_StateNodeBase* GetStartNode() const;
	USMGraphNode_StateNodeBase* GetEndNode() const;

	bool ShouldRunParallel() const;

protected:
	FLinearColor Internal_GetBackgroundColor() const override;
	void SetDefaultsWhenPlaced();
};
