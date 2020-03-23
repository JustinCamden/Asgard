// Copyright © 2020 Justin Camden All Rights Reserved


#include "AsgardPlayerMovementComponent.h"

UAsgardPlayerMovementComponent::UAsgardPlayerMovementComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	AsgardMovementMode = (EAsgardMovementMode)VRReplicatedMovementMode;
}
