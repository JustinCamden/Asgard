// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Graph/Nodes/SMGraphK2Node_Base.h"
#include "SMGraphK2Node_RootNode.generated.h"


UCLASS()
class SMSYSTEMEDITOR_API USMGraphK2Node_RootNode : public USMGraphK2Node_Base
{
	GENERATED_UCLASS_BODY()

	//~ Begin UEdGraphNode Interface
	bool CanUserDeleteNode() const override { return false; }
	bool CanDuplicateNode() const override { return false; }
	void PostPasteNode() override;
	void DestroyNode() override;
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	FLinearColor GetNodeTitleColor() const override;
	FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	//~ End UEdGraphNode Interface

	// USMGraphK2Node_Base
	bool CanCollapseNode() const override { return true; }
	bool CanCollapseToFunctionOrMacro() const override { return false; }
	// ~USMGraphK2Node_Base

	// If this node is in the process of being destroyed.
	bool bIsBeingDestroyed;

	// If this node can be placed more than once on the same graph.
	bool bAllowMoreThanOneNode;
};
