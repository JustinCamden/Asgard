// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphNodeUtils.h"
#include "SMGraphK2Node_Base.h"
#include "SMGraphK2Node_StateMachineNode.generated.h"


class USMGraph;
class USMGraphK2;

UCLASS(MinimalAPI)
class USMGraphK2Node_StateMachineNode : public USMGraphK2Node_Base
{
	GENERATED_UCLASS_BODY()

public:
	//~ Begin UK2Node Interface
	void AllocateDefaultPins() override;
	void OnRenameNode(const FString& NewName) override;
	void PostPlacedNewNode() override;
	void PostPasteNode() override;
	void DestroyNode() override;
	TSharedPtr<class INameValidatorInterface> MakeNameValidator() const override;
	FText GetMenuCategory() const override;
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	UObject* GetJumpTargetForDoubleClick() const override;
	/** Limit blueprints this shows up in. */
	bool IsActionFilteredOut(class FBlueprintActionFilter const& Filter) override;
	bool IsCompatibleWithGraph(UEdGraph const* Graph) const override;
	bool IsNodePure() const override;
	/** Required to show up in BP right click context menu. */
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	//~ End UK2Node Interface

	// USMGraphK2Node_Base
	bool CanCollapseNode() const override { return false; }
	bool CanCollapseToFunctionOrMacro() const override { return false; }
	// ~USMGraphK2Node_Base

	FString GetStateMachineName() const;
	USMGraph* GetStateMachineGraph() const { return BoundGraph; }
	USMGraphK2* GetTopLevelStateMachineGraph() const;
protected:
	UPROPERTY()
	class USMGraph* BoundGraph;

	/** Constructing FText strings can be costly, so we cache the node's title */
	FNodeTextCache CachedFullTitle;
};
