// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "SMState.h"
#include "SMConduit.generated.h"


/**
 * A conduit can either be configured to run as a state or as a transition.
 * Internally it consists of a single transition that must be true before outgoing transitions evaluate.
 */
USTRUCT(BlueprintInternalUseOnly)
struct SMSYSTEM_API FSMConduit : public FSMState_Base
{
	GENERATED_USTRUCT_BODY()

//#pragma region Blueprint Graph Exposed
	/** Set from graph execution. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Result, meta = (AlwaysAsPin))
	bool bCanEnterTransition;

	/**
	 * This conduit will be evaluated with inbound and outbound transitions.
	 * If any transition fails the entire transition fails. In that case the
	 * state leading to this conduit will not take this transition.
	 */
	UPROPERTY()
	bool bEvalWithTransitions;

	/** Entry point to when a transition is first initialized. */
	UPROPERTY()
	TArray<FSMExposedFunctionHandler> TransitionInitializedGraphEvaluators;

	/** Entry point to when a transition is shutdown. */
	UPROPERTY()
	TArray<FSMExposedFunctionHandler> TransitionShutdownGraphEvaluators;

	/** Entry point when the conduit is entered. */
	UPROPERTY()
	FSMExposedFunctionHandler ConduitEnteredGraphEvaluator;
	
//#pragma endregion

public:
	FSMConduit();

	// FSMNode_Base
	void Initialize(UObject* Instance) override;
	void Reset() override;
	bool IsNodeInstanceClassCompatible(UClass* NewNodeInstanceClass) const override;
	UClass* GetDefaultNodeInstanceClass() const override;
	// ~FSMNode_Base

	// FSMState_Base
	bool StartState() override;
	bool UpdateState(float DeltaSeconds) override;
	bool EndState(float DeltaSeconds, const FSMTransition* TransitionToTake) override;

	bool IsConduit() const override { return true; }
	
	/** Evaluate the conduit and retrieve the correct condition. */
	bool GetValidTransition(TArray<TArray<FSMTransition*>>& Transitions) override;
	// ~FSMState_Base

	/** Should this be considered an extension to a transition? */
	bool IsConfiguredAsTransition() const { return bEvalWithTransitions; }

	void EnterConduitWithTransition();
	
private:
	// True for GetValidTransitions, prevents stack overflow when looped with other transition based conduits.
	bool bCheckedForTransitions;
};
