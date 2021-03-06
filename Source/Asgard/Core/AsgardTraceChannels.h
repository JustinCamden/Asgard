﻿// Copyright © 2020 Justin Camden All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Runtime/Engine/Classes/Engine/EngineTypes.h"
#include "AsgardTraceChannels.generated.h"

/**
 * 
 */
UCLASS()
class ASGARD_API UAsgardTraceChannels : public UObject
{
	GENERATED_BODY()
public:
    static const TEnumAsByte<ECollisionChannel> VRTeleportTrace;
};