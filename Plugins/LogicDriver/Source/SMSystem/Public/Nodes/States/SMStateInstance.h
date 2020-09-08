// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "SMNodeInstance.h"
#include "SMNode_Info.h"
#include "SMStateInstance.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStateBeginSignature, class USMStateInstance_Base*, StateInstance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStateUpdateSignature, class USMStateInstance_Base*, StateInstance, float, DeltaSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStateEndSignature, class USMStateInstance_Base*, StateInstance);

/**
 * The abstract base class for all state type nodes including state machine nodes and conduits.
 */
UCLASS(Abstract, Blueprintable, BlueprintType, classGroup = "State Machine", hideCategories = (SMStateInstance_Base), meta = (DisplayName = "State Class Base"))
class SMSYSTEM_API USMStateInstance_Base : public USMNodeInstance
{
	GENERATED_BODY()

public:
	USMStateInstance_Base();
	
	/** Called when the state is started. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Logic Driver|Node Instance")
	void OnStateBegin();

	/**
	 * Called when the state is updated.
	 *
	 * @param DeltaSeconds Time delta in seconds from the last update.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Logic Driver|Node Instance")
	void OnStateUpdate(float DeltaSeconds);

	/** Called when the state is ending. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Logic Driver|Node Instance")
	void OnStateEnd();

	/** Called when the immediate owning state machine blueprint is starting. If this is part of a reference
	 * then it will be called when the reference starts. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Logic Driver|Node Instance")
	void OnRootStateMachineStart();

	/** Called when the immediate owning state machine blueprint is stopping. If this is part of a reference
	 * then it will be called when the reference stops. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Logic Driver|Node Instance")
	void OnRootStateMachineStop();
	
	/** Return read only information about the owning state. */
	UFUNCTION(BlueprintCallable, Category = "Logic Driver|Node Instance")
	void GetStateInfo(FSMStateInfo& State) const;

	/** If this state is an end state. */
	bool IsInEndState() const override;

	/** Checks if this state is a state machine. */
	UFUNCTION(BlueprintCallable, Category = "Logic Driver|Node Instance")
	bool IsStateMachine() const;

	/** Force set the active flag of this state. This call is replicated and can be called from the server or from a client that is not a simulated proxy.
	 * @param bValue True activates the state, false deactivates the state.
	 */
	UFUNCTION(BlueprintCallable, Category = "Logic Driver|Node Instance")
	void SetActive(bool bValue);
	
	/**
	 * Signals to the owning state machine to process transition evaluation.
	 * This is similar to calling Update on the owner root state machine, however state update logic (Tick) won't execute.
	 * All transitions are evaluated as normal starting from the root state machine down.
	 * Depending on super state transitions it's possible the state machine this state is part of may exit.
	 */
	UFUNCTION(BlueprintCallable, Category = "Logic Driver|Node Instance")
	void EvaluateTransitions();
	
	/**
	 * Return all outgoing transition instances.
	 * @param Transitions The outgoing transitions.
	 * @param bExcludeAlwaysFalse Skip over transitions that can't ever be true.
	 * @return True if any transitions exist.
	 */
	UFUNCTION(BlueprintCallable, Category = "Logic Driver|Node Instance")
	bool GetOutgoingTransitions(TArray<class USMTransitionInstance*>& Transitions, bool bExcludeAlwaysFalse = true) const;

	/**
	 * Return all incoming transition instances.
	 * @param Transitions The incoming transitions.
	 * @param bExcludeAlwaysFalse Skip over transitions that can't ever be true.
	 * @return True if any transitions exist.
	 */
	UFUNCTION(BlueprintCallable, Category = "Logic Driver|Node Instance")
	bool GetIncomingTransitions(TArray<class USMTransitionInstance*>& Transitions, bool bExcludeAlwaysFalse = true) const;
	
	/** The transition this state will be taking. Generally only valid on EndState and if this state isn't an end state. May be null. */
	UFUNCTION(BlueprintCallable, Category = "Logic Driver|Node Instance")
	class USMTransitionInstance* GetTransitionToTake() const;

	/**
	 * Forcibly move to the next state providing this state is active and a transition is directly connecting the states.
	 * @param NextStateInstance The state node instance to switch to.
	 * @param bRequireTransitionToPass Will evaluate the transition and only switch states if it passes.
	 *
	 * @return True if the active state was switched.
	 */
	UFUNCTION(BlueprintCallable, Category = "Logic Driver|Node Instance")
	bool SwitchToLinkedState(USMStateInstance_Base* NextStateInstance, bool bRequireTransitionToPass = true);

	/**
	 * Return the next connected state given a transition index.
	 * @param Index The array index of the transition. If transitions have manual priorities they should correlate with the index value.
	 *
	 * @return The connected state or nullptr.
	 */
	UFUNCTION(BlueprintCallable, Category = "Logic Driver|Node Instance")
	USMStateInstance_Base* GetNextStateByTransitionIndex(int32 Index) const;
	
	/**
	 * Recursively search connected nodes for nodes matching the given type.
	 * @param OutNodes All found nodes.
	 * @param NodeClass The class type to search for.
	 * @param bIncludeChildren Include children of type NodeClass or require an exact match.
	 * @param StopIfTypeIsNot The search is broken when a node's type is not found in this list. Leave empty to never break the search.
	 */
	void GetAllNodesOfType(TArray<USMNodeInstance*>& OutNodes, TSubclassOf<USMNodeInstance> NodeClass, bool bIncludeChildren = true, const TArray<UClass*>& StopIfTypeIsNot = TArray<UClass*>()) const;

	/**
	 * Should graph properties evaluate on state start.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Graph Properties", AdvancedDisplay, meta = (InstancedTemplate, HideOnNode, EditCondition = "bAutoEvalExposedProperties", DisplayName = "Auto Eval on Start"))
	bool bEvalGraphsOnStart;

	/**
	 * Should graph properties evaluate on state update.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Graph Properties", AdvancedDisplay, meta = (InstancedTemplate, HideOnNode, EditCondition = "bAutoEvalExposedProperties", DisplayName = "Auto Eval on Update"))
	bool bEvalGraphsOnUpdate;

	/**
	 * Should graph properties evaluate on state end.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Graph Properties", AdvancedDisplay, meta = (InstancedTemplate, HideOnNode, EditCondition = "bAutoEvalExposedProperties", DisplayName = "Auto Eval on End"))
	bool bEvalGraphsOnEnd;

	/**
	 * Should graph properties evaluate when the root state machine starts.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Graph Properties", AdvancedDisplay, meta = (InstancedTemplate, HideOnNode, EditCondition = "bAutoEvalExposedProperties", DisplayName = "Auto Eval on Root State Machine Start"))
	bool bEvalGraphsOnRootStateMachineStart;

	/**
	 * Should graph properties evaluate when the root state machine stops.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Graph Properties", AdvancedDisplay, meta = (InstancedTemplate, HideOnNode, EditCondition = "bAutoEvalExposedProperties", DisplayName = "Auto Eval on Root State Machine Stop"))
	bool bEvalGraphsOnRootStateMachineStop;
	
protected:
	/* Override in native classes to implement. Never call these directly. */
	
	virtual void OnStateBegin_Implementation() {}
	virtual void OnStateUpdate_Implementation(float DeltaSeconds) {}
	virtual void OnStateEnd_Implementation() {}
	virtual void OnRootStateMachineStart_Implementation() {}
	virtual void OnRootStateMachineStop_Implementation() {}
	
#if WITH_EDITORONLY_DATA
public:
	const FLinearColor& GetEndStateColor() const { return NodeEndStateColor; }
	bool ShouldDisplayNameWidget() const { return bDisplayNameWidget; }
	const FSMStateConnectionValidator& GetAllowedConnections() const { return ConnectionRules; }
	
	/** Override to register native classes with the context menu. Default behavior only allows blueprints. */
	virtual bool IsRegisteredWithContextMenu() const;
	virtual bool HideFromContextMenuIfRulesFail() const { return bHideFromContextMenuIfRulesFail; }
protected:
	/** Allows this node to be displayed in the graph context menu when placing nodes. */
	UPROPERTY(EditDefaultsOnly, Category = "General", AdvancedDisplay, meta = (InstancedTemplate))
	bool bRegisterWithContextMenu;
	
	/** Restrict placement unless rules can be verified. Ex: If the rule says this can only be connected from a state node, then this won't show
	 * in the context menu unless dragging from a state node. */
	UPROPERTY(EditDefaultsOnly, Category = "Behavior", AdvancedDisplay, meta = (InstancedTemplate, EditCondition = "bRegisterWithContextMenu"))
	bool bHideFromContextMenuIfRulesFail;
	
	/** The color this node should be when it is an end state. */
	UPROPERTY(EditDefaultsOnly, Category = "Color", meta = (EditCondition = "bUseCustomColors", DisplayPriority = 2))
	FLinearColor NodeEndStateColor;

	/** Whether the name of this state should be visible on the node. It can still be changed in the details panel or by renaming the state graph. */
	UPROPERTY(EditDefaultsOnly, Category = "Display", meta = (InstancedTemplate))
	bool bDisplayNameWidget;
	
	/** Define what types of connections are allowed. Default is all. */
	UPROPERTY(EditDefaultsOnly, Category = "Behavior", meta = (InstancedTemplate, ShowOnlyInnerProperties))
	FSMStateConnectionValidator ConnectionRules;
#endif

public:
	/**
	 * Always update the state at least once before ending.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "State")
	bool bAlwaysUpdate;

	/**
	 * Prevents conditional transitions for this state from being evaluated on Tick.
	 * This is good to use if the transitions leading out of the state are event based
	 * or if you are manually calling EvaluateTransitions from a state instance.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "State")
	bool bDisableTickTransitionEvaluation;

	/** Sets all current and future transitions from this state to run in parallel. Conduit nodes are not supported. */
	UPROPERTY(EditDefaultsOnly, Category = "Parallel States")
	bool bDefaultToParallel;
	
	/** If this state can be reentered from a parallel state if this state is already active. This will not call On State End but will call On State Begin. */
	UPROPERTY(EditDefaultsOnly, Category = "Parallel States")
	bool bAllowParallelReentry;

	/** If the state should remain active even after a transition is taken from this state. */
	UPROPERTY(EditDefaultsOnly, Category = "Parallel States")
	bool bStayActiveOnStateChange;
	
	/**
	 * Allows transitions to be evaluated in the same tick as Start State.
	 * Normally transitions are evaluated on the second tick.
	 * This can be chained with other nodes that have this checked making it
	   possible to evaluate multiple nodes and transitions in a single tick.
	 
	 * When using this consider performance implications and any potential
	   infinite loops such as if you are using a self transition on this state.

	 * Individual transitions can modify this behavior with bCanEvalWithStartState.
	 */
	UPROPERTY(EditDefaultsOnly, AdvancedDisplay, Category = "State")
	bool bEvalTransitionsOnStart;

	/**
	 * Prevents the `Any State` node from adding transitions to this node.
	 * This can be useful for maintaining end states.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "State")
	bool bExcludeFromAnyState;

	/** Called right before the state has started. */
	UPROPERTY(BlueprintAssignable, Category = "Logic Driver|Node Instance")
	FOnStateBeginSignature OnStateBeginEvent;

	/** Called before the state has updated. */
	UPROPERTY(BlueprintAssignable, Category = "Logic Driver|Node Instance")
	FOnStateUpdateSignature OnStateUpdateEvent;

	/** Called before the state has ended. */
	UPROPERTY(BlueprintAssignable, Category = "Logic Driver|Node Instance")
	FOnStateEndSignature OnStateEndEvent;
};

/**
 * The base class for state nodes.
 */
UCLASS(Blueprintable, BlueprintType, classGroup = "State Machine", hideCategories = (SMStateInstance), meta = (DisplayName = "State Class"))
class SMSYSTEM_API USMStateInstance : public USMStateInstance_Base
{
	GENERATED_BODY()

public:
	USMStateInstance();

};

