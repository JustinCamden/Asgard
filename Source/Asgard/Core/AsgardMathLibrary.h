// Copyright © 2020 Justin Camden All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AsgardMathLibrary.generated.h"

/** Library for commonly used math functions. */
UCLASS()
class ASGARD_API UAsgardMathLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/*
	* Returns the angle between two vectors in radians.
	* Assumes the inputs are normalized.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Asgard|MathLibrary")
	static float AngleBetweenVectorsRadians(const FVector& ANormalized, const FVector& BNormalized);

	/*
	* Returns the angle between two vectors in degrees.
	* Assumes the inputs are normalized.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Asgard|MathLibrary")
	float AngleBetweenVectors(const FVector& ANormalized, const FVector& BNormalized);

	/**
	* Rotates a unit vector towards another vector at a speed of RotationSpeed in degrees.
	* Assumes both are normalized.
	* @param MaxErrorAngle If > 0, then vector never be more than this many degrees away from the target
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Asgard|MathLibrary")
	static FVector RotateVectorTowardsVector(const FVector& StartNormalized, const FVector& TargetNormalized, float RotationSpeed, float DeltaTime, float MaxErrorAngle);

	/** Inverts the handedness of a rotation, changing left handed to right handed and vice versa. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Asgard|MathLibrary")
	static FRotator InvertRotationHandedness(const FRotator& RotationToInvert);

	/** Inverts the handedness of a transform, changing left handed to right handed and vice versa. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Asgard|MathLibrary")
	static FTransform InvertTransformHandedness(const FTransform& TransformToInvert);

	/** Inverts the handedness of a transform, changing left handed to right handed and vice versa. Does not affect scale. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Asgard|MathLibrary")
	static FTransform InvertTransformHandednessNoScale(const FTransform& TransformToInvert);
};

FORCEINLINE float UAsgardMathLibrary::AngleBetweenVectorsRadians(const FVector& ANormalized, const FVector& BNormalized)
{
	return FMath::Acos(ANormalized | BNormalized);
}

FORCEINLINE float UAsgardMathLibrary::AngleBetweenVectors(const FVector& ANormalized, const FVector& BNormalized)
{
	return FMath::RadiansToDegrees(FMath::Acos(ANormalized | BNormalized));
}

FORCEINLINE FRotator UAsgardMathLibrary::InvertRotationHandedness(const FRotator& RotationToInvert)
{
	return FRotator(RotationToInvert.Pitch, RotationToInvert.Yaw * -1.0f, RotationToInvert.Roll * -1.0f);
}