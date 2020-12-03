// Copyright © 2020 Justin Camden All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "AsgardCollisionProfiles.generated.h"

/**
 * Container for quick references to collision profiles
 */
UCLASS()
class ASGARD_API UAsgardCollisionProfiles : public UObject
{
	GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Asgard|CollisionProfiles")
    static const FName VRRoot();
};

FORCEINLINE FName const UAsgardCollisionProfiles::VRRoot()
{
    return FName(TEXT("VRRoot"));
}
