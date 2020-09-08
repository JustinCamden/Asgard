// Copyright Recursoft LLC 2019-2020. All Rights Reserved.ings.

#include "SMTransition.h"
#include "SMConduit.h"
#include "SMState.h"
#include "SMTransitionInstance.h"

struct TransitionEvaluatorHelper
{
	TransitionEvaluatorHelper(FSMTransition* Transition)
	{
		TransitionPtr = Transition;
		TransitionPtr->UpdateReadStates();
		TransitionPtr->TransitionPreEvaluateGraphEvaluator.Execute();
	}
	~TransitionEvaluatorHelper()
	{
		TransitionPtr->UpdateReadStates();
		TransitionPtr->TransitionPostEvaluateGraphEvaluator.Execute();
	}

	FSMTransition* TransitionPtr;
};

void FSMTransition::UpdateReadStates()
{
	Super::UpdateReadStates();
	FSMState_Base* State = GetFromState();
	bIsInEndState = State->IsInEndState();
	bHasUpdated = State->HasUpdated();
	TimeInState = State->GetActiveTime();
}

FSMTransition::FSMTransition() : Super(), Priority(0), bCanEnterTransition(false), bCanEnterTransitionFromEvent(false),
                                 bCanEvaluate(true), bCanEvaluateFromEvent(true), bRunParallel(false),
                                 bEvalIfNextStateActive(true),
                                 bAlwaysFalse(false), bCanEvalWithStartState(true),
                                 FromState(nullptr), ToState(nullptr)
{
}

void FSMTransition::Initialize(UObject* Instance)
{
	Super::Initialize(Instance);
	
	TransitionEnteredGraphEvaluator.Initialize(Instance);
	for (FSMExposedFunctionHandler& FunctionHandler : TransitionInitializedGraphEvaluators)
	{
		FunctionHandler.Initialize(Instance);
	}
	for (FSMExposedFunctionHandler& FunctionHandler : TransitionShutdownGraphEvaluators)
	{
		FunctionHandler.Initialize(Instance);
	}
	TransitionPreEvaluateGraphEvaluator.Initialize(Instance);
	TransitionPostEvaluateGraphEvaluator.Initialize(Instance);
}

void FSMTransition::Reset()
{
	Super::Reset();
	TransitionEnteredGraphEvaluator.Reset();
	for (FSMExposedFunctionHandler& FunctionHandler : TransitionInitializedGraphEvaluators)
	{
		FunctionHandler.Reset();
	}
	for (FSMExposedFunctionHandler& FunctionHandler : TransitionShutdownGraphEvaluators)
	{
		FunctionHandler.Reset();
	}
	TransitionPreEvaluateGraphEvaluator.Reset();
	TransitionPostEvaluateGraphEvaluator.Reset();
}

bool FSMTransition::IsNodeInstanceClassCompatible(UClass* NewNodeInstanceClass) const
{
	return NewNodeInstanceClass && NewNodeInstanceClass->IsChildOf<USMTransitionInstance>();
}

UClass* FSMTransition::GetDefaultNodeInstanceClass() const
{
	return USMTransitionInstance::StaticClass();
}

void FSMTransition::TakeTransition()
{
	SetActive(true);

	if (USMTransitionInstance* TransitionInstance = Cast<USMTransitionInstance>(NodeInstance))
	{
		TransitionInstance->OnTransitionEnteredEvent.Broadcast(TransitionInstance);
	}
	
	TransitionEnteredGraphEvaluator.Execute();
	SetActive(false);

	if (GetToState()->IsConduit())
	{
		// Let the conduit know it's being entered with this transition.
		FSMConduit* Conduit = (FSMConduit*)GetToState();
		Conduit->EnterConduitWithTransition();
	}
}

bool FSMTransition::DoesTransitionPass() 
{
	FSMState_Base* NextState = GetToState();
	if ((bRunParallel && !bEvalIfNextStateActive && NextState->IsActive()) || NextState->HasBeenReenteredFromParallelState())
	{
		return false;
	}
	
	TransitionEvaluatorHelper Evaluator(this);

	if (CanEvaluateFromEvent() && bCanEnterTransitionFromEvent)
	{
		bCanEnterTransitionFromEvent = false;
		bCanEnterTransition = true;
		return true;
	}
	
	if(CanEvaluateConditionally())
	{
		Execute();
	}
	else
	{
		bCanEnterTransition = false;
	}

	return bCanEnterTransition;
}

bool FSMTransition::CanTransition(TArray<FSMTransition*>& Transitions)
{
	if (!DoesTransitionPass())
	{
		return false;
	}

	bool bSuccess = false;

	// Additional transitions that occur after this transition.
	TArray<TArray<FSMTransition*>> NextTransitions;

	FSMState_Base* NextState = GetToState();
	if (!NextState->IsConduit())
	{
		// Normal state, we're good to transition.
		bSuccess = true;
	}
	else
	{
		// This is a conduit.
		FSMConduit* Conduit = (FSMConduit*)NextState;
		if (!Conduit->IsConfiguredAsTransition())
		{
			// We can enter this conduit as a state, doesn't matter if we're stuck here.
			bSuccess = true;
		}
		else if (Conduit->GetValidTransition(NextTransitions))
		{
			bSuccess = true;
		}
		// else conduit couldn't complete a valid transition.
	}

	if (bSuccess)
	{
		Transitions.Add(const_cast<FSMTransition*>(this));
		if (NextTransitions.Num() > 0)
		{
			// Conduits will only have possible transition chain since they don't support starting parallel states.
			Transitions.Append(NextTransitions[0]);
		}
	}

	return bSuccess;
}

void FSMTransition::GetConnectedTransitions(TArray<FSMTransition*>& Transitions) const
{
	if (Transitions.Contains(this))
	{
		return;
	}
	
	Transitions.Add(const_cast<FSMTransition*>(this));

	FSMState_Base* NextState = GetToState();
	if (NextState->IsConduit())
	{
		FSMConduit* Conduit = (FSMConduit*)NextState;
		if (Conduit->IsConfiguredAsTransition())
		{
			for (FSMTransition* Transition : Conduit->GetOutgoingTransitions())
			{
				Transition->GetConnectedTransitions(Transitions);
			}
		}
	}
}

bool FSMTransition::CanEvaluateConditionally() const
{
	return bCanEvaluate;
}

bool FSMTransition::CanEvaluateFromEvent() const
{
	return bCanEvaluateFromEvent;
}

void FSMTransition::SetFromState(FSMState_Base* State)
{
	FromState = State;
	FromState->AddOutgoingTransition(this);
}

void FSMTransition::SetToState(FSMState_Base* State)
{
	ToState = State;
	ToState->AddIncomingTransition(this);
}

bool FSMTransition::CanEvaluateWithStartState(const TArray<FSMTransition*>& TransitionChain)
{
	for (FSMTransition* Transition : TransitionChain)
	{
		if (!Transition->bCanEvalWithStartState)
		{
			return false;
		}
	}

	return true;
}

FSMState_Base* FSMTransition::GetFinalStateFromChain(const TArray<FSMTransition*>& TransitionChain)
{
	FSMState_Base* FoundState = nullptr;
	for (FSMTransition* Transition : TransitionChain)
	{
		FoundState = Transition->GetToState();
		if (!FoundState->IsConduit())
		{
			break;
		}
	}

	check(FoundState);
	return FoundState;
}

bool FSMTransition::CanChainEvalIfNextStateActive(const TArray<FSMTransition*>& TransitionChain)
{
	for (FSMTransition* Transition : TransitionChain)
	{
		if (Transition->bEvalIfNextStateActive)
		{
			return true;
		}
	}

	return false;
}
