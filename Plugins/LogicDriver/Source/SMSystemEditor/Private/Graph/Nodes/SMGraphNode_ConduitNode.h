// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "SMConduitInstance.h"
#include "SMGraphNode_StateNode.h"
#include "SMGraphNode_ConduitNode.generated.h"


class USMGraphNode_TransitionEdge;
class USMGraph;

UCLASS(MinimalAPI)
class USMGraphNode_ConduitNode : public USMGraphNode_StateNodeBase
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, NoClear, Category = "Class", meta = (BlueprintBaseOnly))
	TSubclassOf<USMConduitInstance> ConduitClass;

	/**
	 * @deprecated Set on the node template instead.
	 */
	UPROPERTY()
	bool bEvalWithTransitions_DEPRECATED;
	
	// UEdGraphNode
	void AllocateDefaultPins() override;
	FText GetTooltipText() const override;
	void AutowireNewNode(UEdGraphPin* FromPin) override;
	void PostPlacedNewNode() override;
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ UEdGraphNode

	// USMGraphNode_Base
	void ImportDeprecatedProperties() override;
	void PlaceDefaultInstanceNodes() override;
	UClass* GetNodeClass() const override { return ConduitClass; }
	void SetNodeClass(UClass* Class) override;
	FName GetFriendlyNodeName() const override { return "Conduit"; }
	void SetRuntimeDefaults(FSMState_Base& State) const override;
	// ~USMGraphNode_Base

	/** If this conduit should be configured to evaluate with transitions. */
	bool ShouldEvalWithTransitions() const;

protected:
	FLinearColor Internal_GetBackgroundColor() const override;
};
