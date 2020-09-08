// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "SMGraphNode_StateMachineStateNode.h"
#include "SMGraphNode_StateMachineParentNode.generated.h"


UCLASS()
class SMSYSTEMEDITOR_API USMGraphNode_StateMachineParentNode : public USMGraphNode_StateMachineStateNode
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Parent State Machine", NoClear, meta = (NoResetToDefault))
	TSubclassOf<class USMInstance> ParentClass;

	//~ Begin UEdGraphNode Interface
	void PostPlacedNewNode() override;
	UObject* GetJumpTargetForDoubleClick() const override;
	void JumpToDefinition() const override;
	//~ End UEdGraphNode Interface

	// USMGraphNode_StateMachineStateNode
	void CreateBoundGraph() override;
	void UpdateEditState() override;
	bool ReferenceStateMachine(USMBlueprint* OtherStateMachine, bool bRestrictCircularReference) override { return false; }
	void InitStateMachineReferenceTemplate(bool bInitialLoad) override {}
	// ~USMGraphNode_StateMachineStateNode

	void SetParentIfNull();

	/** Builds all nested graphs of parents that have been expanded. Only valid during compile after ExpandParentNodes() */
	TSet<USMGraph*> GetAllNestedExpandedParents() const;

	/** A cloned graph of the parent. Only valid during compile after ExpandParentNodes(). */
	UPROPERTY(Transient, NonTransactional)
	USMGraph* ExpandedGraph;
	
protected:
	// USMGraphNode_StateNodeBase
	FLinearColor Internal_GetBackgroundColor() const override;
	// ~USMGraphNode_StateNodeBase
};
