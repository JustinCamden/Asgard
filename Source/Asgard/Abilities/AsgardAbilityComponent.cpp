// Copyright © 2020 Justin Camden All Rights Reserved

#include "AsgardAbilityComponent.h"

// Sets default values for this component's properties
UAsgardAbilityComponent::UAsgardAbilityComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

void UAsgardAbilityComponent::ActivateAbility()
{
	bIsAbilityActive = true;
	OnAbilityActivated();
	NotifyAbilityActivated.Broadcast();
}

void UAsgardAbilityComponent::DeactivateAbility()
{
	bIsAbilityActive = false;
	OnAbilityDeactivated();
	NotifyAbilityDeactivated.Broadcast();
}

