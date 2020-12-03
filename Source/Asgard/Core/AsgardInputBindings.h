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
    UFUNCTION(BlueprintCallable, Category = "Asgard|InputBindings")
    static const FName MoveForwardAxis();

    UFUNCTION(BlueprintCallable, Category = "Asgard|InputBindings")
    static const FName MoveRightAxis();

    UFUNCTION(BlueprintCallable, Category = "Asgard|InputBindings")
    static const FName TurnForwardAxis();

    UFUNCTION(BlueprintCallable, Category = "Asgard|InputBindings")
    static const FName TurnRightAxis();

    UFUNCTION(BlueprintCallable, Category = "Asgard|InputBindings")
    static const FName FlightThrusterLeftAction();

    UFUNCTION(BlueprintCallable, Category = "Asgard|InputBindings")
    static const FName FlightThrusterRightAction();

    UFUNCTION(BlueprintCallable, Category = "Asgard|InputBindings")
    static const FName ToggleFlightAction();

    UFUNCTION(BlueprintCallable, Category = "Asgard|InputBindings")
    static const FName PrecisionTeleportAction();
};

FORCEINLINE FName const UAsgardInputBindings::MoveForwardAxis()
{
    return FName(TEXT("MoveForwardAxis"));
}

FORCEINLINE FName const UAsgardInputBindings::MoveRightAxis()
{
    return FName(TEXT("MoveRightAxis"));
}

FORCEINLINE FName const UAsgardInputBindings::TurnForwardAxis()
{
    return FName(TEXT("TurnForwardAxis"));
}

FORCEINLINE FName const UAsgardInputBindings::TurnRightAxis()
{
    return FName(TEXT("TurnRightAxis"));
}

FORCEINLINE FName const UAsgardInputBindings::ToggleFlightAction()
{
    return FName(TEXT("ToggleFlightAction"));
}

FORCEINLINE FName const UAsgardInputBindings::FlightThrusterLeftAction()
{
    return FName(TEXT("FlightThrusterLeftAction"));
}

FORCEINLINE FName const UAsgardInputBindings::FlightThrusterRightAction()
{
    return FName(TEXT("FlightThrusterRightAction"));
}

FORCEINLINE FName const UAsgardInputBindings::PrecisionTeleportAction()
{
    return FName(TEXT("PrecisionTeleportToAction"));
}
