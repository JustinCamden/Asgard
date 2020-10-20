// Copyright © 2020 Justin Camden All Rights Reserved

#include "AsgardVRMovementComponent.h"

UAsgardVRMovementComponent::UAsgardVRMovementComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{

}

void UAsgardVRMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	AsgardMovementMode = (EAsgardMovementMode)((uint8)MovementMode);
	VRReplicatedMovementMode = (EVRConjoinedMovementModes)((uint8)MovementMode);
}

void UAsgardVRMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	AsgardMovementMode = (EAsgardMovementMode)((uint8)MovementMode);
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
	return;
}
