  ﻿// Copyright © 2020 Justin Camden All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AsgardMathLibrary.generated.h"

/**
 * Library for commonly used math functions.
 */
UCLASS()
class ASGARD_API UAsgardMathLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/*
	* Returns the angle between two vectors in radians.
	* Assumes the inputs are normalized.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = AsgardMathLibrary)
	static float AngleBetweenVectorsRadians(const FVector& ANormalized, const FVector& BNormalized);

	/*
	* Returns the angle between two vectors in degrees.
	* Assumes the inputs are normalized.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = AsgardMathLibrary)
	static float AngleBetweenVectors(const FVector& ANormalized, const FVector& BNormalized);

	/**
	* Rotates a unit vector towards another vector at a speed of RotationSpeed in degrees.
	* Assumes both are normalized.
	* @param MaxErrorAngle If > 0, then vector never be more than this many degrees away from the target
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = AsgardMathLibrary)
	static FVector RotateVectorTowardsVector(const FVector& StartNormalized, const FVector& TargetNormalized, float RotationSpeed, float DeltaTime, float MaxErrorAngle);

};
