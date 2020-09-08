// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateColor.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWidget.h"
#include "SNodePanel.h"
#include "SGraphNode.h"


class SToolTip;
class USMGraphNode_TransitionEdge;

class SGraphNode_TransitionEdge : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SGraphNode_TransitionEdge){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, USMGraphNode_TransitionEdge* InNode);
	// SGraphNode interface
	void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	void MoveTo(const FVector2D& NewPosition, FNodeSet& NodeFilter) override;
	bool RequiresSecondPassLayout() const override;
	void PerformSecondPassLayout(const TMap< UObject*, TSharedRef<SNode> >& NodeToWidgetLookup) const override;
	void UpdateGraphNode() override;
	TSharedPtr<SToolTip> GetComplexTooltip() override;
	// ~SGraphNode interface

	// SWidget interface
	void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	void OnMouseLeave(const FPointerEvent& MouseEvent) override;
	// End of SWidget interface

	// Calculate position for multiple nodes to be placed between a start and end point, by providing this nodes index and max expected nodes 
	void PositionBetweenTwoNodesWithOffset(const FGeometry& StartGeom, const FGeometry& EndGeom, int32 NodeIndex, int32 MaxNodes) const;

protected:
	FSlateColor GetEdgeColor() const;
	const FSlateBrush* GetIcon() const;

};
