// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "SMGraphK2Node_RuntimeNodeContainer.h"
#include "SMState.h"
#include "SMGraphK2Node_StateEntryNode.generated.h"


UCLASS(MinimalAPI)
class USMGraphK2Node_StateEntryNode : public USMGraphK2Node_RuntimeNodeContainer
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = "State Machines")
	FSMState StateNode;

	//~ Begin UEdGraphNode Interface
	void AllocateDefaultPins() override;
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FText GetTooltipText() const override;
	bool IsCompatibleWithGraph(UEdGraph const* Graph) const override;
	//~ End UEdGraphNode Interface

	FSMNode_Base* GetRunTimeNode()  override { return &StateNode; }
};
