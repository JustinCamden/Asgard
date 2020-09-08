// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "SGraphNode.h"
#include "SGraphPreviewer.h"


class USMGraphNode_ConduitNode;
class USMGraphNode_StateNodeBase;

class SGraphNode_StateNode : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SGraphNode_StateNode) : _ContentPadding(FMargin(4.f, 0.f, 4.f, 0.f)) {}
	SLATE_ARGUMENT(FMargin, ContentPadding)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, USMGraphNode_StateNodeBase* InNode);

	// SGraphNode interface
	void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	void UpdateGraphNode() override;
	void CreatePinWidgets() override;
	void AddPin(const TSharedRef<SGraphPin>& PinToAdd) override;
	TSharedPtr<SToolTip> GetComplexTooltip() override;
	TArray<FOverlayWidgetInfo> GetOverlayWidgets(bool bSelected, const FVector2D& WidgetSize) const override;
	FReply OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override;
	void RequestRenameOnSpawn() override;
	FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	// End of SGraphNode interface

	// SNodePanel::SNode interface
	void GetNodeInfoPopups(FNodeInfoContext* Context, TArray<FGraphInformationPopupInfo>& Popups) const override;
	// End of SNodePanel::SNode interface
protected:
	virtual TSharedPtr<SVerticalBox> CreateContentBox();
	virtual FSlateColor GetBorderBackgroundColor() const;
	virtual const FSlateBrush* GetNameIcon() const;
	virtual TSharedRef<SVerticalBox> BuildComplexTooltip();
	virtual UEdGraph* GetGraphToUseForTooltip() const;

protected:
	TSharedPtr<SGraphPreviewer> GraphPreviewer;
	TSharedPtr<SWidget> AnyStateImpactWidget;
	TMap<TSharedPtr<class SSMGraphProperty_Base>, class USMGraphK2Node_PropertyNode_Base*> PropertyWidgets;
	FMargin ContentPadding;
	const int32 OverlayWidgetPadding = 25;
};

class SGraphNode_ConduitNode : public SGraphNode_StateNode
{
public:
	SLATE_BEGIN_ARGS(SGraphNode_ConduitNode) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, USMGraphNode_ConduitNode* InNode);

protected:
	const FSlateBrush* GetNameIcon() const override;
};
