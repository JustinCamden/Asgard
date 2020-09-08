// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "SMTransition.h"
#include "SMGraphK2Node_RuntimeNodeContainer.h"
#include "SMGraphK2Node_TransitionResultNode.generated.h"


UCLASS()
class SMSYSTEMEDITOR_API USMGraphK2Node_TransitionResultNode : public USMGraphK2Node_RuntimeNodeContainer
{
	GENERATED_UCLASS_BODY()

	static const FName EvalPinName;
	
	UPROPERTY(EditAnywhere, Category = "State Machines")
	FSMTransition TransitionNode;

	//~ Begin UEdGraphNode Interface
	void AllocateDefaultPins() override;
	FLinearColor GetNodeTitleColor() const override;
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FText GetTooltipText() const override;

	bool IsNodePure() const override { return true; }
	bool IsCompatibleWithGraph(UEdGraph const* Graph) const override;
	//~ End UEdGraphNode Interface

	FSMNode_Base* GetRunTimeNode() override { return &TransitionNode; }
	UEdGraphPin* GetTransitionEvaluationPin() const;
};
