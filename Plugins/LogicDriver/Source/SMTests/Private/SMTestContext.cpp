// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#include "SMTestContext.h"

float USMTestContext::GreaterThanTest = 5;

void USMStateTestInstance::OnStateBeginEventFunc(USMStateInstance_Base* Instance)
{
	check(Instance == this);
	StateBeginEventHit.Increase();
}

void USMStateTestInstance::OnStateUpdateEventFunc(USMStateInstance_Base* Instance, float DeltaSeconds)
{
	check(Instance == this);
	StateUpdateEventHit.Increase();
}

void USMStateTestInstance::OnStateEndEventFunc(USMStateInstance_Base* Instance)
{
	check(Instance == this);
	StateEndEventHit.Increase();
}

void USMStateTestInstance::OnStateBegin_Implementation()
{
	ExposedInt++;
	StateBeginHit.Increase();
}

void USMStateTestInstance::OnStateUpdate_Implementation(float DeltaSeconds)
{
	StateUpdateHit.Increase();
}

void USMStateTestInstance::OnStateEnd_Implementation()
{
	StateEndHit.Increase();
}

void USMStateTestInstance::OnRootStateMachineStart_Implementation()
{
	StateMachineStartHit.Increase();
	OnStateBeginEvent.AddDynamic(this, &USMStateTestInstance::OnStateBeginEventFunc);
	OnStateUpdateEvent.AddDynamic(this, &USMStateTestInstance::OnStateUpdateEventFunc);
	OnStateEndEvent.AddDynamic(this, &USMStateTestInstance::OnStateEndEventFunc);
}

void USMStateTestInstance::OnRootStateMachineStop_Implementation()
{
	OnStateBeginEvent.RemoveDynamic(this, &USMStateTestInstance::OnStateBeginEventFunc);
	OnStateUpdateEvent.RemoveDynamic(this, &USMStateTestInstance::OnStateUpdateEventFunc);
	OnStateEndEvent.RemoveDynamic(this, &USMStateTestInstance::OnStateEndEventFunc);
	StateMachineStopHit.Increase();
}

void USMStateMachineTestInstance::OnStateBegin_Implementation()
{
	StateBeginHit.Increase();
}

void USMStateMachineTestInstance::OnStateUpdate_Implementation(float DeltaSeconds)
{
	StateUpdateHit.Increase();
}

void USMStateMachineTestInstance::OnStateEnd_Implementation()
{
	StateEndHit.Increase();
}

void USMTransitionTestInstance::OnTransitionEnteredEventFunc(USMTransitionInstance* TransitionInstance)
{
	check(TransitionInstance);
	TransitionEnteredEventHit.Increase();
}

void USMTransitionTestInstance::OnTransitionInitialized_Implementation()
{
	TransitionInitializedHit.Increase();
	OnTransitionEnteredEvent.AddDynamic(this, &USMTransitionTestInstance::OnTransitionEnteredEventFunc);
}

void USMTransitionTestInstance::OnTransitionShutdown_Implementation()
{
	TransitionShutdownHit.Increase();
	//OnTransitionEnteredEvent.RemoveDynamic(this, &USMTransitionTestInstance::OnTransitionEnteredEventFunc); Can't remove because this will fire before TransitionEntered.
}

USMStateMachineTestComponent::USMStateMachineTestComponent(class FObjectInitializer const& ObjectInitializer) : Super(ObjectInitializer)
{
}

void USMTextGraphState::OnStateBegin_Implementation()
{
	TextGraph.Execute();
	EvaluatedText = TextGraph.Result;
}

void USMStateMachineTestComponent::SetStateMachineClass(UClass* NewClass)
{
	StateMachineClass = NewClass;
}

void USMStateMachineTestComponent::ClearTemplateInstance()
{
	InstanceTemplate = nullptr;
}

void USMStateMachineTestComponent::SetAllowTick(bool bAllowOverride, bool bCanEverTick)
{
	bOverrideTick_DEPRECATED = bAllowOverride;
	bCanEverTick_DEPRECATED = bCanEverTick;
}

void USMStateMachineTestComponent::SetTickInterval(bool bAllowOverride, float TickInterval)
{
	bOverrideTickInterval_DEPRECATED = bAllowOverride;
	TickInterval_DEPRECATED = TickInterval;
}

void USMStateMachineTestComponent::ImportDeprecatedProperties_Public()
{
	ImportDeprecatedProperties();
}
