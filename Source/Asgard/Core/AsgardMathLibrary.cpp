// Copyright © 2020 Justin Camden All Rights Reserved
                                        
#include "AsgardMathLibrary.h"


float UAsgardMathLibrary::AngleBetweenVectorsRadians(const FVector& ANormalized, const FVector& BNormalized)
{
	return FMath::Acos(ANormalized | BNormalized);
}

float UAsgardMathLibrary::AngleBetweenVectors(const FVector& ANormalized, const FVector& BNormalized)
{
	return FMath::RadiansToDegrees(FMath::Acos(ANormalized | BNormalized));
}

FVector UAsgardMathLibrary::RotateVectorTowardsVector(const FVector& StartNormalized, const FVector& TargetNormalized, float RotationSpeed, float DeltaTime, float MaxErrorAngle)
{
	float ErrorAngle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(StartNormalized, TargetNormalized)));
	ErrorAngle = FMath::Min(FMath::FInterpConstantTo(ErrorAngle, 0.0f, DeltaTime, RotationSpeed), MaxErrorAngle);
	return (ErrorAngle > 0.0f ? TargetNormalized.RotateAngleAxis(ErrorAngle, FVector::CrossProduct(TargetNormalized, StartNormalized)) : TargetNormalized);
}
