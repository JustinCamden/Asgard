// Copyright © 2020 Justin Camden All Rights Reserved
                                        
#include "AsgardMathLibrary.h"

FVector UAsgardMathLibrary::RotateVectorTowardsVector(const FVector& StartNormalized, const FVector& TargetNormalized, float RotationSpeed, float DeltaTime, float MaxErrorAngle)
{
	float ErrorAngle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(StartNormalized, TargetNormalized)));
	ErrorAngle = FMath::Min(FMath::FInterpConstantTo(ErrorAngle, 0.0f, DeltaTime, RotationSpeed), MaxErrorAngle);
	return (ErrorAngle > 0.0f ? TargetNormalized.RotateAngleAxis(ErrorAngle, FVector::CrossProduct(TargetNormalized, StartNormalized)) : TargetNormalized);
}

FTransform UAsgardMathLibrary::InvertTransformHandedness(const FTransform& TransformToInvert)
{
	// Location
	FVector InvertedY = FVector(1.0f, -1.0f, 1.0f);
	return FTransform(InvertRotationHandedness(TransformToInvert.GetRotation().Rotator()), 
		InvertedY * TransformToInvert.GetLocation(), 
		TransformToInvert.GetScale3D());
}
