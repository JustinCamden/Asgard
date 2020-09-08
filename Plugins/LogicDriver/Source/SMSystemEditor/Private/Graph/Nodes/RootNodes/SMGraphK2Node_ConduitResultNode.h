// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "SMConduit.h"
#include "SMGraphK2Node_TransitionResultNode.h"
#include "SMGraphK2Node_ConduitResultNode.generated.h"


UCLASS(MinimalAPI)
class USMGraphK2Node_ConduitResultNode : public USMGraphK2Node_TransitionResultNode
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = "State Machines")
	FSMConduit ConduitNode;

	//~ Begin UEdGraphNode Interface
	void AllocateDefaultPins() override;
	FLinearColor GetNodeTitleColor() const override;
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FText GetTooltipText() const override;
	bool IsNodePure() const override { return true; }
	bool IsCompatibleWithGraph(UEdGraph const* Graph) const override;
	//~ End UEdGraphNode Interface

	FSMNode_Base* GetRunTimeNode() override { return &ConduitNode; }
};
