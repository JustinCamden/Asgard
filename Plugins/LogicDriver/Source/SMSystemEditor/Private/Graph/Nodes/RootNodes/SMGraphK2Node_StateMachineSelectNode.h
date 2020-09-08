// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "SMGraphK2Node_RootNode.h"
#include "SMGraphK2Node_StateMachineSelectNode.generated.h"


UCLASS(MinimalAPI)
class USMGraphK2Node_StateMachineSelectNode : public USMGraphK2Node_RootNode
{
	GENERATED_UCLASS_BODY()

	//~ Begin UEdGraphNode Interface
	void AllocateDefaultPins() override;
	FLinearColor GetNodeTitleColor() const override;
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FText GetTooltipText() const override;
	bool IsNodePure() const override { return true; }
	bool IsCompatibleWithGraph(UEdGraph const* Graph) const override;
	//~ End UEdGraphNode Interface

	// USMGraphK2Node_Base
	bool CanCollapseNode() const override { return false; }
	bool CanCollapseToFunctionOrMacro() const override { return false; }
	// ~USMGraphK2Node_Base
};
