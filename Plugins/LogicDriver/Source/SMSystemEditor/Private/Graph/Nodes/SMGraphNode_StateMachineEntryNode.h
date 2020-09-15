// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "SMStateMachine.h"
#include "SMGraphNode_Base.h"
#include "RootNodes/SMGraphK2Node_RuntimeNodeContainer.h"
#include "SMGraphNode_StateMachineEntryNode.generated.h"


/** Created for normal state machine UEdGraphs. */
UCLASS(MinimalAPI)
class USMGraphNode_StateMachineEntryNode : public USMGraphNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "State Machines")
	FSMStateMachine StateMachineNode;

	/** Allow more than one initial state. Setting this to false will clear all but one initial state. */
	UPROPERTY(EditAnywhere, Category = "Parallel States")
	bool bAllowParallelEntryStates;
	
	//~ Begin UEdGraphNode Interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual void PostPasteNode() override;
	virtual bool CanUserDeleteNode() const override { return false; }
	virtual bool CanDuplicateNode() const override { return false; }
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End UEdGraphNode Interface
};

/** Created by compiler for nested state machine entry points. */
UCLASS(MinimalAPI)
class USMGraphK2Node_StateMachineEntryNode : public USMGraphK2Node_RuntimeNodeContainer
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "State Machines")
	FSMStateMachine StateMachineNode;

	virtual FSMNode_Base* GetRunTimeNode()  override { return &StateMachineNode; }

	//~ Begin UEdGraphNode Interface
	virtual void AllocateDefaultPins() override;
	//~ End UEdGraphNode Interface
};
