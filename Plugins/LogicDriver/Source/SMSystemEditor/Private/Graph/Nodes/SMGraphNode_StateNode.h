// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "SMGraphNode_Base.h"
#include "SMState.h"
#include "SMStateInstance.h"
#include "SMGraphNode_StateNode.generated.h"


/** Base class required as states and conduits branch separately from a common source. */
UCLASS()
class SMSYSTEMEDITOR_API USMGraphNode_StateNodeBase : public USMGraphNode_Base
{
	GENERATED_UCLASS_BODY()

	/**
	 * @deprecated Set on the node template instead.
	 */
	UPROPERTY()
	bool bAlwaysUpdate_DEPRECATED;

	/**
	 * @deprecated Set on the node template instead.
	 */
	UPROPERTY()
	bool bDisableTickTransitionEvaluation_DEPRECATED;

	/**
	 * @deprecated Set on the node template instead.
	 */
	UPROPERTY()
	bool bEvalTransitionsOnStart_DEPRECATED;

	/**
	 * @deprecated Set on the node template instead.
	 */
	UPROPERTY()
	bool bExcludeFromAnyState_DEPRECATED;
	
	/**
	 * Set by the editor and read by the schema to allow self transitions.
	 * We don't want to drag / drop self transitions because a single pin click will
	 * trigger them. There doesn't seem to be an ideal way for the schema to detect
	 * mouse movement to prevent single clicks when in CanCreateConnection,
	 * so we're relying on a context menu.
	 */
	bool bCanTransitionToSelf;

	// UEdGraphNode
	void AllocateDefaultPins() override;
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	void AutowireNewNode(UEdGraphPin* FromPin) override;
	/** Called after playing a new node -- will create new blue print graph. */
	void PostPlacedNewNode() override;
	void PostPasteNode() override;
	void DestroyNode() override;
	TSharedPtr<class INameValidatorInterface> MakeNameValidator() const override;
	void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	// ~UEdGraphNode

	// USMGraphNode
	void ImportDeprecatedProperties() override;
	// ~USMGraphNode
	
	/** Copy configuration settings to the runtime node. */
	virtual void SetRuntimeDefaults(FSMState_Base& State) const;
	virtual FString GetStateName() const;

	/**
	 * Checks if there are no outbound transitions.
	 * @param bCheckAnyState Checks if an any state will prevent this from being an end state.
	 */
	virtual bool IsEndState(bool bCheckAnyState = true) const;

	/** Checks if there are any connections to this node. Does not count self. */
	virtual bool HasInputConnections() const;

	/** Checks if there are any connections from this node. */
	virtual bool HasOutputConnections() const;

	/** If transitions are supposed to run in parallel. */
	bool ShouldDefaultTransitionsToParallel() const;

	/** If this node shouldn't receive transitions from an Any State. */
	bool ShouldExcludeFromAnyState() const;
	
	/** Checks if there is a node connected via outgoing transition. */
	bool HasTransitionToNode(UEdGraphNode* Node) const;

	/** Checks if there is a node connected via incoming transition. */
	bool HasTransitionFromNode(UEdGraphNode* Node) const;

	/** Returns the previous node at the given input linked to index. */
	USMGraphNode_StateNodeBase* GetPreviousNode(int32 Index = 0) const;

	/** Returns the next node at the given output linked to index. */
	USMGraphNode_StateNodeBase* GetNextNode(int32 Index = 0) const;

	USMGraphNode_TransitionEdge* GetPreviousTransition(int32 Index = 0) const;

	USMGraphNode_TransitionEdge* GetNextTransition(int32 Index = 0) const;
protected:
	FLinearColor Internal_GetBackgroundColor() const override;
};

/**
 * Regular state nodes which have K2 graphs.
 */
UCLASS(MinimalAPI)
class USMGraphNode_StateNode : public USMGraphNode_StateNodeBase
{
public:
	GENERATED_UCLASS_BODY()
	
	UPROPERTY(EditAnywhere, NoClear, Category = "Class", meta = (BlueprintBaseOnly))
	TSubclassOf<USMStateInstance> StateClass;

	// UEdGraphNode
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ UEdGraphNode
	
	// USMGraphNode_Base
	void PlaceDefaultInstanceNodes() override;
	UClass* GetNodeClass() const override { return StateClass; }
	void SetNodeClass(UClass* Class) override;
	FName GetFriendlyNodeName() const override { return "State"; }
	// ~USMGraphNode_Base
};

/**
 * Nodes without a graph that just serve to transfer their transitions to all other USMGraphNode_StateNodeBase in a single SMGraph.
 */
UCLASS(MinimalAPI)
class USMGraphNode_AnyStateNode : public USMGraphNode_StateNodeBase
{
public:
	GENERATED_UCLASS_BODY()

	/** Allows the initial transitions to evaluate even when the active state is an initial state of this node.
	 * Default behavior prevents this. */
	UPROPERTY(EditAnywhere, Category = "Any State")
	bool bAllowInitialReentry;
	
	// UEdGraphNode
	void AllocateDefaultPins() override;
	void PostPlacedNewNode() override;
	void PostPasteNode() override;
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	void OnRenameNode(const FString& NewName) override;
	// ~UEdGraphNode
	
	// USMGraphNode_Base
	FName GetFriendlyNodeName() const override { return "Any State"; }
	FString GetStateName() const override;
	// ~USMGraphNode_Base

protected:
	FLinearColor Internal_GetBackgroundColor() const override;
	
protected:
	UPROPERTY()
	FText NodeName;
};
