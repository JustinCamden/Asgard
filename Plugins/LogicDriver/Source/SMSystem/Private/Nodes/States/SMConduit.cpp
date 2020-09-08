// Copyright Recursoft LLC 2019-2020. All Rights Reserved.

#include "SMConduit.h"
#include "SMConduitInstance.h"

FSMConduit::FSMConduit() : Super(), bCanEnterTransition(false), bEvalWithTransitions(false),
                           bCheckedForTransitions(false)
{
}

void FSMConduit::Initialize(UObject* Instance)
{
	Super::Initialize(Instance);
	
	for (FSMExposedFunctionHandler& FunctionHandler : TransitionInitializedGraphEvaluators)
	{
		FunctionHandler.Initialize(Instance);
	}
	for (FSMExposedFunctionHandler& FunctionHandler : TransitionShutdownGraphEvaluators)
	{
		FunctionHandler.Initialize(Instance);
	}

	ConduitEnteredGraphEvaluator.Initialize(Instance);
}

void FSMConduit::Reset()
{
	Super::Reset();

	for (FSMExposedFunctionHandler& FunctionHandler : TransitionInitializedGraphEvaluators)
	{
		FunctionHandler.Reset();
	}
	for (FSMExposedFunctionHandler& FunctionHandler : TransitionShutdownGraphEvaluators)
	{
		FunctionHandler.Reset();
	}

	ConduitEnteredGraphEvaluator.Reset();
}

bool FSMConduit::IsNodeInstanceClassCompatible(UClass* NewNodeInstanceClass) const
{
	return NewNodeInstanceClass && NewNodeInstanceClass->IsChildOf<USMConduitInstance>();
}

UClass* FSMConduit::GetDefaultNodeInstanceClass() const
{
	return USMConduitInstance::StaticClass();
}

bool FSMConduit::StartState()
{
	const bool bResult = Super::StartState();

	ConduitEnteredGraphEvaluator.Execute();
	
	if(USMConduitInstance* ConduitInstance = Cast<USMConduitInstance>(GetNodeInstance()))
	{
		ConduitInstance->OnStateBegin();
	}

	return bResult;
}

bool FSMConduit::UpdateState(float DeltaSeconds)
{
	const bool bResult = Super::UpdateState(DeltaSeconds);

	if (USMConduitInstance* ConduitInstance = Cast<USMConduitInstance>(GetNodeInstance()))
	{
		ConduitInstance->OnStateUpdate(DeltaSeconds);
	}

	return bResult;
}

bool FSMConduit::EndState(float DeltaSeconds, const FSMTransition* TransitionToTake)
{
	const bool bResult = Super::EndState(DeltaSeconds, TransitionToTake);

	if (USMConduitInstance* ConduitInstance = Cast<USMConduitInstance>(GetNodeInstance()))
	{
		ConduitInstance->OnStateEnd();
	}

	return bResult;
}

bool FSMConduit::GetValidTransition(TArray<TArray<FSMTransition*>>& Transitions)
{
	if (bCheckedForTransitions)
	{
		return false;
	}
	
	// First check that the conduit passes.
	Execute();

	if(!bCanEnterTransition)
	{
		return false;
	}

	bCheckedForTransitions = true;
	
	// Passes, find the best transition.
	const bool bResult = Super::GetValidTransition(Transitions);
	
	bCheckedForTransitions = false;
	return bResult;
}

void FSMConduit::EnterConduitWithTransition()
{
	if (IsConfiguredAsTransition())
	{
		SetActive(true);
		ConduitEnteredGraphEvaluator.Execute();
		SetActive(false);
	}
}
