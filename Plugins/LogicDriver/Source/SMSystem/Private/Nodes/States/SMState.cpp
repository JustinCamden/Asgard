// Copyright Recursoft LLC 2019-2020. All Rights Reserved.

#include "SMState.h"
#include "SMConduit.h"
#include "SMTransition.h"
#include "SMStateInstance.h"
#include "SMUtils.h"

#define GRAPH_PROPERTY_EVAL_ON_START 0
#define GRAPH_PROPERTY_EVAL_ON_UPDATE 1
#define GRAPH_PROPERTY_EVAL_ON_END 2
#define GRAPH_PROPERTY_EVAL_ON_ROOT_SM_START 3
#define GRAPH_PROPERTY_EVAL_ON_ROOT_SM_STOP 4

void FSMState_Base::UpdateReadStates()
{
	Super::UpdateReadStates();
	bIsInEndState = IsEndState();
	bHasUpdated = HasUpdated();
	TimeInState = GetActiveTime();
}

void FSMState_Base::ResetReadStates()
{
	bHasUpdated = false;
	bIsInEndState = false;
	TimeInState = 0.f;
}

FSMState_Base::FSMState_Base() : Super(), bIsRootNode(false), bAlwaysUpdate(false), bEvalTransitionsOnStart(false),
bDisableTickTransitionEvaluation(false), bStayActiveOnStateChange(false), bAllowParallelReentry(false),
PreviousEnteredState(nullptr), bReenteredByParallelState(false), NextTransition(nullptr)
{
	ResetReadStates();
}

void FSMState_Base::Initialize(UObject* Instance)
{
	Super::Initialize(Instance);
	OnRootStateMachineStartedGraphEvaluator.Initialize(Instance);
	OnRootStateMachineStoppedGraphEvaluator.Initialize(Instance);
	
	ResetReadStates();

	OutgoingTransitions.Sort([](const FSMTransition& lhs, const FSMTransition& rhs)
	{
		return lhs.Priority < rhs.Priority;
	});

	IncomingTransitions.Sort([](const FSMTransition& lhs, const FSMTransition& rhs)
	{
		return lhs.Priority < rhs.Priority;
	});
}

void FSMState_Base::Reset()
{
	Super::Reset();
	OnRootStateMachineStartedGraphEvaluator.Reset();
	OnRootStateMachineStoppedGraphEvaluator.Reset();
	ResetReadStates();
}

bool FSMState_Base::IsNodeInstanceClassCompatible(UClass* NewNodeInstanceClass) const
{
	return NewNodeInstanceClass && NewNodeInstanceClass->IsChildOf<USMStateInstance>();
}

UClass* FSMState_Base::GetDefaultNodeInstanceClass() const
{
	return USMStateInstance::StaticClass();
}

void FSMState_Base::GetAllTransitionChains(TArray<FSMTransition*>& OutTransitions) const
{
	for (FSMTransition* Transition : OutgoingTransitions)
	{
		Transition->GetConnectedTransitions(OutTransitions);
	}
}

bool FSMState_Base::StartState()
{
	NextTransition = nullptr;
	
	ResetReadStates();

	if(IsActive() && (!HasBeenReenteredFromParallelState() || !bAllowParallelReentry))
	{
		return false;
	}

	if (CanExecuteGraphProperties(GRAPH_PROPERTY_EVAL_ON_START))
	{
		ExecuteGraphProperties();
	}

	SetActive(true);
	
	if (USMStateInstance_Base* StateInstance = Cast<USMStateInstance_Base>(NodeInstance))
	{
		StateInstance->OnStateBeginEvent.Broadcast(StateInstance);
	}
	
	InitializeTransitions();
	
	return true;
}

bool FSMState_Base::UpdateState(float DeltaSeconds)
{
	if(!IsActive())
	{
		return false;
	}

	TimeInState += DeltaSeconds;
	UpdateReadStates();
	if (CanExecuteGraphProperties(GRAPH_PROPERTY_EVAL_ON_UPDATE))
	{
		ExecuteGraphProperties();
	}

	if (USMStateInstance_Base* StateInstance = Cast<USMStateInstance_Base>(NodeInstance))
	{
		StateInstance->OnStateUpdateEvent.Broadcast(StateInstance, DeltaSeconds);
	}
	
	return bHasUpdated = true;
}

bool FSMState_Base::EndState(float DeltaSeconds, const FSMTransition* TransitionToTake)
{
	if (!IsActive())
	{
		return false;
	}

	SetTransitionToTake(TransitionToTake);

	if(bAlwaysUpdate && !HasUpdated())
	{
		UpdateState(DeltaSeconds);
	}

	UpdateReadStates();
	if (CanExecuteGraphProperties(GRAPH_PROPERTY_EVAL_ON_END))
	{
		ExecuteGraphProperties();
	}
	
	if (USMStateInstance_Base* StateInstance = Cast<USMStateInstance_Base>(NodeInstance))
	{
		StateInstance->OnStateEndEvent.Broadcast(StateInstance);
	}
	
	SetActive(false);
	ShutdownTransitions();

	return true;
}

void FSMState_Base::OnStartedByInstance(USMInstance* Instance)
{
	// Only execute if allowed and if it's this owning instance starting it.
	// This means reference nodes won't initialize until their owning blueprint is started.
	if (CanExecuteLogic() && Instance == GetOwningInstance())
	{
		UpdateReadStates();
		if (CanExecuteGraphProperties(GRAPH_PROPERTY_EVAL_ON_ROOT_SM_START))
		{
			ExecuteGraphProperties();
		}
		OnRootStateMachineStartedGraphEvaluator.Execute();
	}
}

void FSMState_Base::OnStoppedByInstance(USMInstance* Instance)
{
	// Only execute if allowed and if it's this owning instance.
	if (CanExecuteLogic() && Instance == GetOwningInstance())
	{
		UpdateReadStates();
		if (CanExecuteGraphProperties(GRAPH_PROPERTY_EVAL_ON_ROOT_SM_STOP))
		{
			ExecuteGraphProperties();
		}
		OnRootStateMachineStoppedGraphEvaluator.Execute();
	}
}

bool FSMState_Base::GetValidTransition(TArray<TArray<FSMTransition*>>& Transitions)
{
	for(FSMTransition* Transition : OutgoingTransitions)
	{
		TArray<FSMTransition*> Chain;
		if(Transition->CanTransition(Chain))
		{
			Transitions.Add(Chain);

			// Check if blocking or not.
			if (IsConduit() || !Transition->bRunParallel)
			{
				return true;
			}
		}
	}

	return Transitions.Num() > 0;
}

bool FSMState_Base::IsEndState() const
{
	for(FSMTransition* Transition : OutgoingTransitions)
	{
		// Look for at least one valid transition.
		if(!Transition->bAlwaysFalse)
		{
			return false;
		}
	}

	return true;
}

bool FSMState_Base::IsInEndState() const
{
	return IsEndState();
}

bool FSMState_Base::HasUpdated() const
{
	return bHasUpdated;
}

void FSMState_Base::SetCanExecuteLogic(bool bValue)
{
	bCanExecuteLogic = bValue;
}

bool FSMState_Base::CanExecuteGraphProperties(uint32 OnEvent) const
{
	if (USMStateInstance_Base* StateInstance = Cast<USMStateInstance_Base>(GetNodeInstance()))
	{
		if (!StateInstance->bAutoEvalExposedProperties)
		{
			return false;
		}

		switch (OnEvent)
		{
		case GRAPH_PROPERTY_EVAL_ON_START:
			return StateInstance->bEvalGraphsOnStart;
		case GRAPH_PROPERTY_EVAL_ON_UPDATE:
			return StateInstance->bEvalGraphsOnUpdate;
		case GRAPH_PROPERTY_EVAL_ON_END:
			return StateInstance->bEvalGraphsOnEnd;
		case GRAPH_PROPERTY_EVAL_ON_ROOT_SM_START:
			return StateInstance->bEvalGraphsOnRootStateMachineStart;
		case GRAPH_PROPERTY_EVAL_ON_ROOT_SM_STOP:
			return StateInstance->bEvalGraphsOnRootStateMachineStop;
		}
	}

	return false;
}

bool FSMState_Base::CanEvaluateTransitionsOnTick() const
{
	if (bDisableTickTransitionEvaluation)
	{
		/* Check if any immediate outgoing transition has just completed from an event before returning false. */
		for (FSMTransition* Transition : OutgoingTransitions)
		{
			if (Transition->bCanEnterTransitionFromEvent)
			{
				return true;
			}
		}
	}

	return !bDisableTickTransitionEvaluation;
}

void FSMState_Base::SetTransitionToTake(const FSMTransition* Transition)
{
	NextTransition = Transition;
}

void FSMState_Base::SetPreviousEnteredState(FSMState_Base* PreviousState)
{
	PreviousEnteredState = PreviousState;
}

void FSMState_Base::NotifyOfParallelReentry(bool bValue)
{
	bReenteredByParallelState = bValue;
}

void FSMState_Base::AddOutgoingTransition(FSMTransition* Transition)
{
	OutgoingTransitions.AddUnique(Transition);
}

void FSMState_Base::AddIncomingTransition(FSMTransition* Transition)
{
	IncomingTransitions.AddUnique(Transition);
}

void FSMState_Base::InitializeTransitions()
{
	TArray<FSMTransition*> AllTransitions;
	GetAllTransitionChains(AllTransitions);
	
	for(FSMTransition* Transition : AllTransitions)
	{
		USMUtils::ExecuteGraphFunctions(Transition->TransitionInitializedGraphEvaluators);
		
		if (Transition->GetToState()->IsConduit())
		{
			USMUtils::ExecuteGraphFunctions(((FSMConduit*)Transition->GetToState())->TransitionInitializedGraphEvaluators);
		}
	}
}

void FSMState_Base::ShutdownTransitions()
{
	TArray<FSMTransition*> AllTransitions;
	GetAllTransitionChains(AllTransitions);

	for (FSMTransition* Transition : AllTransitions)
	{
		USMUtils::ExecuteGraphFunctions(Transition->TransitionShutdownGraphEvaluators);

		if (Transition->GetToState()->IsConduit())
		{
			USMUtils::ExecuteGraphFunctions(((FSMConduit*)Transition->GetToState())->TransitionShutdownGraphEvaluators);
		}
	}
}


FSMState::FSMState() : Super()
{
}

void FSMState::Initialize(UObject* Instance)
{
	Super::Initialize(Instance);

	UpdateStateGraphEvaluator.Initialize(Instance);
	EndStateGraphEvaluator.Initialize(Instance);
}

void FSMState::Reset()
{
	Super::Reset();
	UpdateStateGraphEvaluator.Reset();
	EndStateGraphEvaluator.Reset();
}

bool FSMState::StartState()
{
	if(!Super::StartState())
	{
		return false;
	}

	if (CanExecuteLogic())
	{
		Execute();
	}

	return true;
}

bool FSMState::UpdateState(float DeltaSeconds)
{
	if (!Super::UpdateState(DeltaSeconds))
	{
		return false;
	}

	if (CanExecuteLogic())
	{
		UpdateStateGraphEvaluator.Execute((void*)&DeltaSeconds);
	}

	return true;
}

bool FSMState::EndState(float DeltaSeconds, const FSMTransition* TransitionToTake)
{
	if (!Super::EndState(DeltaSeconds, TransitionToTake))
	{
		return false;
	}

	if (CanExecuteLogic())
	{
		EndStateGraphEvaluator.Execute();
	}

	return true;
}