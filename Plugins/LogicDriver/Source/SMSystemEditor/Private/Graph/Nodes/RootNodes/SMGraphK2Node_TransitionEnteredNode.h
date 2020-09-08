// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "SMGraphK2Node_RuntimeNodeContainer.h"
#include "SMGraphK2Node_TransitionEnteredNode.generated.h"


UCLASS(MinimalAPI)
class USMGraphK2Node_TransitionEnteredNode : public USMGraphK2Node_RuntimeNodeReference
{
	GENERATED_UCLASS_BODY()

	//~ Begin UEdGraphNode Interface
	void AllocateDefaultPins() override;
	void PostPlacedNewNode() override;
	FText GetMenuCategory() const override;
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FText GetTooltipText() const override;
	/** Required to show up in BP right click context menu. */
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	/** Limit blueprints this shows up in. */
	bool IsActionFilteredOut(class FBlueprintActionFilter const& Filter) override;
	bool IsCompatibleWithGraph(UEdGraph const* Graph) const override;
	/** User can replace node. */
	bool CanUserDeleteNode() const override { return true; }
	//~ End UEdGraphNode Interface
};
