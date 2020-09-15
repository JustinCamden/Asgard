// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "EdGraphUtilities.h"


struct FSMGraphPanelNodeFactory : public FGraphPanelNodeFactory
{
public:
	virtual TSharedPtr<class SGraphNode> CreateNode(class UEdGraphNode* Node) const override;
};

struct FSMGraphPinFactory : public FGraphPanelPinFactory
{
public:
	virtual TSharedPtr<class SGraphPin> CreatePin(class UEdGraphPin* Pin) const override;
};

struct FSMGraphPinConnectionFactory : public FGraphPanelPinConnectionFactory
{
public:
	virtual class FConnectionDrawingPolicy* CreateConnectionPolicy(const class UEdGraphSchema* Schema, int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const class FSlateRect& InClippingRect, class FSlateWindowElementList& InDrawElements, class UEdGraph* InGraphObj) const override;
};