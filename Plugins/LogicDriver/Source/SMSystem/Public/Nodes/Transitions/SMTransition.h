// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "SMState.h"
#include "SMTransition.generated.h"


/**
 * Transitions determine when an FSM can exit one state and advance to the next.
 */
USTRUCT(BlueprintInternalUseOnly)
struct SMSYSTEM_API FSMTransition : public FSMNode_Base
{
	GENERATED_USTRUCT_BODY()

public:
//#pragma region Blueprint Graph Exposed

	/** Entry point to when a transition is taken. */
	UPROPERTY()
	FSMExposedFunctionHandler TransitionEnteredGraphEvaluator;

	/** Entry point to when a transition is first initialized. */
	UPROPERTY()
	TArray<FSMExposedFunctionHandler> TransitionInitializedGraphEvaluators;

	/** Entry point to when a transition is shutdown. */
	UPROPERTY()
	TArray<FSMExposedFunctionHandler> TransitionShutdownGraphEvaluators;

	/** Entry point to before a transition evaluates. */
	UPROPERTY()
	FSMExposedFunctionHandler TransitionPreEvaluateGraphEvaluator;

	/** Entry point to after a transition evaluates. */
	UPROPERTY()
	FSMExposedFunctionHandler TransitionPostEvaluateGraphEvaluator;

	/** Lower number means this transition is checked sooner. */
	UPROPERTY(EditAnywhere, Category = "State Machines")
	int32 Priority;

	/** Set from graph execution. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Result, meta = (AlwaysAsPin))
	bool bCanEnterTransition;

	/** Set from graph execution when updated by event. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Result, meta = (AlwaysAsPin))
	bool bCanEnterTransitionFromEvent;
	
	/** Set from graph execution or configurable from details panel. Must be true for the transition to be evaluated conditionally. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Transition)
	bool bCanEvaluate;

	/** Allows auto-binded events to evaluate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Transition)
	bool bCanEvaluateFromEvent;
	
	/**
	 * This transition will not prevent the next transition in the priority sequence from being evaluated.
	 * This allows the possibility for multiple active states.
	 */
	UPROPERTY()
	bool bRunParallel;

	/**
	 * If the transition should still evaluate if already connecting to an active state.
	 */
	UPROPERTY()
	bool bEvalIfNextStateActive;
	
	/** Guid to the state this transition is from. Kismet compiler will convert this into a state link. */
	UPROPERTY()
	FGuid FromGuid;
	
	/** Guid to the state this transition is leading to. Kismet compiler will convert this into a state link. */
	UPROPERTY()
	FGuid ToGuid;

	UPROPERTY()
	bool bAlwaysFalse;

	/** Secondary check state machine will make if a state is evaluating transitions on the same tick as Start State. */
	UPROPERTY()
	bool bCanEvalWithStartState;
	
public:
	void UpdateReadStates() override;

//#pragma endregion

public:
	FSMTransition();

	// FSMNode_Base
	void Initialize(UObject* Instance) override;
	void Reset() override;
	bool IsNodeInstanceClassCompatible(UClass* NewNodeInstanceClass) const override;
	UClass* GetDefaultNodeInstanceClass() const override;
	// ~FSMNode_Base
	
	/** Will execute any transition tunnel logic. */
	void TakeTransition();

	/** Execute the graph and return the result. Only determines if this transition passes. */
	bool DoesTransitionPass();

	/**
	 * Checks the execution tree in the event of conduits.
	 * @param Transitions All transitions that pass.
	 * @return True if a valid path exists.
	 */
	bool CanTransition(TArray<FSMTransition*>& Transitions);

	/**
	 * Retrieve all transitions in a chain. If the length is more than one that implies a transition conduit is in use.
	 * @param Transitions All transitions connected to this transition, ordered by traversal.
	 */
	void GetConnectedTransitions(TArray<FSMTransition*>& Transitions) const;

	/** If the transition is allowed to evaluate conditionally. This has to be true in order for the transition to be taken. */
	bool CanEvaluateConditionally() const;

	/* If the transition is allowed to evaluate from an event. **/
	bool CanEvaluateFromEvent() const;

	FORCEINLINE FSMState_Base* GetFromState() const { return FromState; }
	FORCEINLINE FSMState_Base* GetToState() const { return ToState; }

	/** Sets the state leading to this transition. This will update the state with this transition. */
	void SetFromState(FSMState_Base* State);
	void SetToState(FSMState_Base* State);

	/** Checks to make sure every transition is allowed to evaluate with the start state. */
	static bool CanEvaluateWithStartState(const TArray<FSMTransition*>& TransitionChain);

	/** Get the final state a transition chain will reach. Attempts to find a non-conduit first. */
	static FSMState_Base* GetFinalStateFromChain(const TArray<FSMTransition*>& TransitionChain);

	/** Checks if any transition allows evaluation if the next state is active. */
	static bool CanChainEvalIfNextStateActive(const TArray<FSMTransition*>& TransitionChain);
private:
	FSMState_Base* FromState;
	FSMState_Base* ToState;
};