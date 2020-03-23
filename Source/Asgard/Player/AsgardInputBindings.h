// Copyright © 2020 Justin Camden All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "AsgardInputBindings.generated.h"

/**
 * Container for quick references to input binding
 */
UCLASS()
class ASGARD_API UAsgardInputBindings : public UObject
{
	GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category = AsgardInputBindings)
    static const FName MoveForwardAxis();

    UFUNCTION(BlueprintCallable, Category = AsgardInputBindings)
    static const FName MoveRightAxis();
};

FORCEINLINE FName const UAsgardInputBindings::MoveForwardAxis()
{
    return FName(TEXT("MoveForwardAxis"));
}

FORCEINLINE FName const UAsgardInputBindings::MoveRightAxis()
{
    return FName(TEXT("MoveRightAxis"));
}