// Copyright © 2020 Justin Camden All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Runtime/Engine/Classes/Components/SplineComponent.h"
#include "AsgardSplineLibrary.generated.h"

/**
 * Library of functions for working with splines.
 */
UCLASS()
class ASGARD_API UAsgardSplineLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	/** Input a spline and ideal subdivision length to get number of subdivisions and calculated length. */
	UFUNCTION(BlueprintCallable, Category = "Asgard|SplineLibrary")
	static void CalculateSplineSegmentNumAndLength(
		const USplineComponent* Spline,
		int32& OutNumSegments,
		float& OutSegmentLength,
		float IdealSegmentLength = 100.0f);

	/** Input a spline with segment index and section length to calculate start and end locations and tangents.*/
	UFUNCTION(BlueprintCallable, Category = "Asgard|SplineLibrary")
		static void CalculateSplineSegmentStartAndEnd(
			const USplineComponent* Spline,
			FVector& StartLocation,
			FVector& StartTangent,
			FVector& EndLocation,
			FVector& EndTangent,
			const int32 SegmentIndex,
			const float SegmentLength = 100.0f,
			const ESplineCoordinateSpace::Type CoordinateSpace = ESplineCoordinateSpace::Local);

	/** Input a spline to calculate the rotation of a spline segment from the current 'Up Vector' to its end location. */
	UFUNCTION(BlueprintPure, Category = "Asgard|SplineLibrary")
		static float CalculateSplineUpRotation(
			const USplineComponent* Spline,
			const int32 SegmentIndex,
			const float SegmentLength = 100.0f,
			const ESplineCoordinateSpace::Type CoordinateSpace = ESplineCoordinateSpace::World);

	/** Input a spline and splinemesh to set its start and end along with twisting */
	UFUNCTION(BlueprintCallable, Category = "Asgard|SplineLibrary")
		static void MatchSplineMeshToSpline(
			const int& SegmentIndex,
			const float& SegmentLength,
			const USplineComponent* Spline,
			USplineMeshComponent* SplineMesh,
			const FVector2D& StartScale,
			const FVector2D& EndScale,
			float Roll = 0.0f,
			bool UpdateMesh = false);

	/** Input a Spline to offset with offset distance and rotation from the spline's up vector at each point. */
	UFUNCTION(BlueprintCallable, Category = "Asgard|SplineLibrary")
		static void BuildOffsetSpline(
			const USplineComponent* BaseSpline,
			USplineComponent* OffsetSpline,
			const float RotFromUp = 0.0f,
			const float OffsetDist = 30.0f);

	/*
	* Builds a corrected spline using a base spline and offset spline. 
	* Outputs the length and count of each spline segment, base on the ideal length.
	* Final spline should be heavily subdivided to avoid twisting.
	*/
	UFUNCTION(BlueprintCallable, Category = "Asgard|SplineLibrary")
		static void BuildCorrectedSpline(
			const USplineComponent* BaseSpline,
			const USplineComponent* OffsetSpline,
			USplineComponent* CorrectedSpline,
			const float IdealSegmentLength = 100.0f);

	/*
	* Builds a an offset and corrected spline using a base spline and offset spline. 
	* Outputs the length and count of each spline segment, base on the ideal length.
	* Final spline should be heavily subdivided to avoid twisting.
	*/
	UFUNCTION(BlueprintCallable, Category = "Asgard|SplineLibrary")
		static void FixSplineTwist(
			int32& OutNumSegments,
			float& OutSegmentLength,
			const USplineComponent* BaseSpline,
			USplineComponent* OffsetSpline,
			USplineComponent* CorrectedSpline,
			const float OffsetRotFromUp,
			const float OffsetDist = 30.0f,
			const float IdealSegmentLength = 100.0f);
};
