// Copyright © 2020 Justin Camden All Rights Reserved

#include "AsgardSplineLibrary.h"
#include "Runtime/Engine/Classes/Components/SplineMeshComponent.h"

void UAsgardSplineLibrary::CalculateSplineSegmentNumAndLength(
	const USplineComponent* Spline,
	int32& OutNumSegments,
	float& OutSegmentLength,
	float IdealSegmentLength)
{
	checkf(Spline != nullptr, TEXT("CalculateSplineSegmentCountAndLength failed! Spline was null."));

	IdealSegmentLength = FMath::Clamp(IdealSegmentLength, 1.0F, 100000.0F);
	OutNumSegments = (Spline->GetSplineLength() / IdealSegmentLength);
	OutSegmentLength = (Spline->GetSplineLength() / OutNumSegments);
}

void UAsgardSplineLibrary::CalculateSplineSegmentStartAndEnd(
	const class USplineComponent* Spline,
	FVector& StartLocation,
	FVector& StartTangent,
	FVector& EndLocation,
	FVector& EndTangent,
	const int32 SegmentIndex,
	const float SegmentLength,
	const ESplineCoordinateSpace::Type CoordinateSpace)
{
	checkf(Spline != nullptr, TEXT("CalculateSplineSegmentStartAndEnd failed! Spline was null."));

	StartLocation = Spline->GetLocationAtDistanceAlongSpline(SegmentIndex * SegmentLength, CoordinateSpace);
	EndLocation = Spline->GetLocationAtDistanceAlongSpline((SegmentIndex + 1) * SegmentLength, CoordinateSpace);

	FVector Tan1 = Spline->GetTangentAtDistanceAlongSpline(SegmentIndex * SegmentLength, CoordinateSpace);
	StartTangent = Tan1.GetSafeNormal() * SegmentLength;

	FVector Tan2 = Spline->GetTangentAtDistanceAlongSpline((SegmentIndex + 1) * SegmentLength, CoordinateSpace);
	EndTangent = Tan2.GetSafeNormal() * SegmentLength;
}

float UAsgardSplineLibrary::CalculateSplineUpRotation(
	const class USplineComponent* Spline,
	const int32 SegmentIndex,
	const float SegmentLength,
	const ESplineCoordinateSpace::Type CoordinateSpace)
{
	checkf(Spline != nullptr, TEXT("CalculateSplineUpRotation failed! Spline was null."));

	FVector Tan = Spline->GetTangentAtDistanceAlongSpline((SegmentIndex + 1) * SegmentLength, CoordinateSpace);
	FVector Crossed1 = FVector::CrossProduct(Tan.GetSafeNormal(), Spline->GetUpVectorAtDistanceAlongSpline(SegmentIndex * SegmentLength, CoordinateSpace));
	FVector Crossed2 = FVector::CrossProduct(Tan.GetSafeNormal(), Spline->GetUpVectorAtDistanceAlongSpline((SegmentIndex + 1) * SegmentLength, CoordinateSpace));
	FVector Crossed3 = FVector::CrossProduct(Crossed1, Crossed2).GetSafeNormal();
	float Dot1 = FVector::DotProduct(Crossed1.GetSafeNormal(), Crossed2.GetSafeNormal());
	float Dot2 = FVector::DotProduct(Crossed3.GetSafeNormal(), Tan);
	return ((FMath::Sign(Dot2)) * (-1) * (FMath::Acos(Dot1)));
}

void UAsgardSplineLibrary::MatchSplineMeshToSpline(
	const int& SegmentIndex, 
	const float& SegmentLength, 
	const USplineComponent* Spline, 
	USplineMeshComponent* SplineMesh, 
	const FVector2D& StartScale, 
	const FVector2D& EndScale, 
	float Roll,
	bool UpdateMesh)
{
	checkf(Spline != nullptr, TEXT("MatchSplineMeshToSpline failed! Spline was null."));
	checkf(SplineMesh != nullptr, TEXT("MatchSplineMeshToSpline failed! SplineMesh was null."));

	FVector StartLocation;
	FVector StartTangent;
	FVector EndLocation;
	FVector EndTangent;
	CalculateSplineSegmentStartAndEnd(Spline, StartLocation, StartTangent, EndLocation, EndTangent, SegmentIndex, SegmentLength, ESplineCoordinateSpace::Local);
	SplineMesh->SetStartAndEnd(StartLocation, StartTangent, EndLocation, EndTangent, false);

	FVector UpDirection = Spline->GetUpVectorAtDistanceAlongSpline(SegmentIndex * SegmentLength, ESplineCoordinateSpace::World);
	FTransform OwnerTransform = Spline->GetOwner()->GetActorTransform();
	UpDirection = OwnerTransform.InverseTransformVectorNoScale(UpDirection);
	SplineMesh->SetSplineUpDir(UpDirection, true);

	float Rotation = CalculateSplineUpRotation(Spline, SegmentIndex, SegmentLength);

	Roll = FMath::DegreesToRadians(Roll);
	Rotation = Rotation + Roll;
	SplineMesh->SetStartRoll(FMath::DegreesToRadians(Roll), false);
	SplineMesh->SetEndRoll(Rotation, false);

	SplineMesh->SetStartScale(StartScale, false);
	SplineMesh->SetEndScale(EndScale, false);

	if (UpdateMesh)
	{
		SplineMesh->UpdateMesh();
	}

	return;
}

void UAsgardSplineLibrary::BuildOffsetSpline(
	const USplineComponent* BaseSpline, 
	USplineComponent* OffsetSpline, 
	const float RotFromUp, 
	const float OffsetDist)
{
	checkf(BaseSpline != nullptr, TEXT("BuildOffsetSpline failed! BaseSpline was null."));
	checkf(OffsetSpline != nullptr, TEXT("BuildOffsetSpline failed! OffsetSpline was null."));

	ESplineCoordinateSpace::Type CoordinateSpace = ESplineCoordinateSpace::World;
	OffsetSpline->ClearSplinePoints(true);
	int32 LastIdx = BaseSpline->GetNumberOfSplinePoints() - 1;

	FVector UpVectorScaled;
	FVector TanAtPoint;
	FVector OffsetVector;
	FVector PointLocation;

	for (int Idx = 0; Idx <= LastIdx; Idx++)
	{
		UpVectorScaled = OffsetDist * (BaseSpline->GetUpVectorAtSplinePoint(Idx, CoordinateSpace));
		TanAtPoint = (BaseSpline->GetTangentAtSplinePoint(Idx, CoordinateSpace));
		OffsetVector = UpVectorScaled.RotateAngleAxis(RotFromUp, TanAtPoint.GetSafeNormal());
		PointLocation = OffsetVector + (BaseSpline->GetLocationAtSplinePoint(Idx, CoordinateSpace));
		OffsetSpline->AddSplinePointAtIndex(PointLocation, Idx, CoordinateSpace, false);

		OffsetSpline->SetTangentAtSplinePoint(Idx, TanAtPoint, CoordinateSpace, false);

		OffsetSpline->SetSplinePointType(Idx, BaseSpline->GetSplinePointType(Idx), false);
	}

	OffsetSpline->UpdateSpline();

	// Fix tangents
	FVector ArriveTan;
	FVector LeaveTan;
	FVector BaseTan;
	int32 ArriveIndex;
	int32 ArriveLoopingIdx;
	int32 LeaveLoopingIdx;
	for (int32 OuterIdx = 0; OuterIdx <= 2; OuterIdx++)
	{
		for (int InnerIdx = 0; InnerIdx <= LastIdx; InnerIdx++)
		{
			BaseTan = BaseSpline->GetTangentAtSplinePoint(InnerIdx, CoordinateSpace);

			ArriveIndex = FMath::Clamp(int32(InnerIdx - 1), 0, int32(LastIdx + 1));
			ArriveLoopingIdx = ((InnerIdx - 1) < 0 ? LastIdx : 0);
			ArriveTan = BaseTan * ((OffsetSpline->GetDistanceAlongSplineAtSplinePoint(InnerIdx) -
									(OffsetSpline->GetDistanceAlongSplineAtSplinePoint(ArriveIndex) +
									OffsetSpline->GetDistanceAlongSplineAtSplinePoint(ArriveLoopingIdx))) /
									(BaseSpline->GetDistanceAlongSplineAtSplinePoint(InnerIdx) -
									(BaseSpline->GetDistanceAlongSplineAtSplinePoint(ArriveIndex) +
									BaseSpline->GetDistanceAlongSplineAtSplinePoint(ArriveLoopingIdx))));

			LeaveLoopingIdx = (LastIdx == InnerIdx ? 1 : 0);
			LeaveTan = BaseTan * ((OffsetSpline->GetDistanceAlongSplineAtSplinePoint(InnerIdx) -
									(OffsetSpline->GetDistanceAlongSplineAtSplinePoint(InnerIdx + 1) +
									(OffsetSpline->GetSplineLength() * LeaveLoopingIdx))) /
									(BaseSpline->GetDistanceAlongSplineAtSplinePoint(InnerIdx) -
									(BaseSpline->GetDistanceAlongSplineAtSplinePoint(InnerIdx + 1) +
									(BaseSpline->GetSplineLength() * LeaveLoopingIdx))));

			OffsetSpline->SetTangentsAtSplinePoint(InnerIdx, ArriveTan, LeaveTan, CoordinateSpace, false);
		}
		OffsetSpline->UpdateSpline();
	}
}

void UAsgardSplineLibrary::BuildCorrectedSpline(
	const USplineComponent* BaseSpline, 
	const USplineComponent* OffsetSpline, 
	USplineComponent* CorrectedSpline, 
	const float IdealSegmentLength)
{
	checkf(BaseSpline != nullptr, TEXT("BuildCorrectedSpline failed! BaseSpline was null."));
	checkf(OffsetSpline != nullptr, TEXT("BuildCorrectedSpline failed! BaseSpline was null."));
	checkf(CorrectedSpline != nullptr, TEXT("BuildCorrectedSpline failed! BaseSpline was null."));

	int32 NumSplineSegments;
	float SplineSegmentLength;
	CalculateSplineSegmentNumAndLength(BaseSpline, NumSplineSegments, SplineSegmentLength, IdealSegmentLength);
	CorrectedSpline->ClearSplinePoints(true);
	int32 LastIdx = NumSplineSegments + (BaseSpline->IsClosedLoop() ? -1 : 0);
	FVector Location;
	FVector LocationOffset;
	FVector Tangent;
	ESplineCoordinateSpace::Type CoordinateSpace = ESplineCoordinateSpace::World;
	for (int32 Idx = 0; Idx <= LastIdx; Idx++)
	{
		Location = BaseSpline->GetLocationAtDistanceAlongSpline(Idx * SplineSegmentLength, CoordinateSpace);
		CorrectedSpline->AddSplinePointAtIndex(Location, Idx, CoordinateSpace, false);

		LocationOffset = OffsetSpline->FindLocationClosestToWorldLocation(Location, CoordinateSpace);
		CorrectedSpline->SetUpVectorAtSplinePoint(Idx, (LocationOffset - Location).GetSafeNormal(), CoordinateSpace, false);

		Tangent = SplineSegmentLength * ((BaseSpline->GetTangentAtDistanceAlongSpline(Idx * SplineSegmentLength, CoordinateSpace)).GetSafeNormal());
		CorrectedSpline->SetTangentAtSplinePoint(Idx, Tangent, CoordinateSpace, false);
	}
	CorrectedSpline->UpdateSpline();
}

void UAsgardSplineLibrary::FixSplineTwist(
	int32& OutNumSegments, 
	float& OutSegmentLength, 
	const USplineComponent* BaseSpline, 
	USplineComponent* OffsetSpline, 
	USplineComponent* CorrectedSpline, 
	const float OffsetRotFromUp,
	const float OffsetDist,
	const float IdealSegmentLength)
{
	checkf(BaseSpline != nullptr, TEXT("FixSplineTwist failed! BaseSpline was null."));
	checkf(OffsetSpline != nullptr, TEXT("FixSplineTwist failed! BaseSpline was null."));
	checkf(CorrectedSpline != nullptr, TEXT("FixSplineTwist failed! BaseSpline was null."));

	ESplineCoordinateSpace::Type CoordinateSpace = ESplineCoordinateSpace::World;
	OffsetSpline->ClearSplinePoints(true);
	int32 LastIdx = BaseSpline->GetNumberOfSplinePoints() - 1;

	FVector UpVectorScaled;
	FVector TanAtPoint;
	FVector OffsetVector;
	FVector PointLocation;

	for (int Idx = 0; Idx <= LastIdx; Idx++)
	{
		UpVectorScaled = OffsetDist * (BaseSpline->GetUpVectorAtSplinePoint(Idx, CoordinateSpace));
		TanAtPoint = (BaseSpline->GetTangentAtSplinePoint(Idx, CoordinateSpace));
		OffsetVector = UpVectorScaled.RotateAngleAxis(OffsetRotFromUp, TanAtPoint.GetSafeNormal());
		PointLocation = OffsetVector + (BaseSpline->GetLocationAtSplinePoint(Idx, CoordinateSpace));
		OffsetSpline->AddSplinePointAtIndex(PointLocation, Idx, CoordinateSpace, false);

		OffsetSpline->SetTangentAtSplinePoint(Idx, TanAtPoint, CoordinateSpace, false);

		OffsetSpline->SetSplinePointType(Idx, BaseSpline->GetSplinePointType(Idx), false);
	}

	OffsetSpline->UpdateSpline();

	// Fix tangents
	FVector ArriveTan;
	FVector LeaveTan;
	FVector BaseTan;
	int32 ArriveIndex;
	int32 ArriveLoopingIdx;
	int32 LeaveLoopingIdx;
	for (int32 OuterIdx = 0; OuterIdx <= 2; OuterIdx++)
	{
		for (int InnerIdx = 0; InnerIdx <= LastIdx; InnerIdx++)
		{
			BaseTan = BaseSpline->GetTangentAtSplinePoint(InnerIdx, CoordinateSpace);

			ArriveIndex = FMath::Clamp(int32(InnerIdx - 1), 0, int32(LastIdx + 1));
			ArriveLoopingIdx = ((InnerIdx - 1) < 0 ? LastIdx : 0);
			ArriveTan = BaseTan * ((OffsetSpline->GetDistanceAlongSplineAtSplinePoint(InnerIdx) -
									(OffsetSpline->GetDistanceAlongSplineAtSplinePoint(ArriveIndex) +
									OffsetSpline->GetDistanceAlongSplineAtSplinePoint(ArriveLoopingIdx))) /
									(BaseSpline->GetDistanceAlongSplineAtSplinePoint(InnerIdx) -
									(BaseSpline->GetDistanceAlongSplineAtSplinePoint(ArriveIndex) +
									BaseSpline->GetDistanceAlongSplineAtSplinePoint(ArriveLoopingIdx))));

			LeaveLoopingIdx = (LastIdx == InnerIdx ? 1 : 0);
			LeaveTan = BaseTan * ((OffsetSpline->GetDistanceAlongSplineAtSplinePoint(InnerIdx) -
									(OffsetSpline->GetDistanceAlongSplineAtSplinePoint(InnerIdx + 1) +
									(OffsetSpline->GetSplineLength() * LeaveLoopingIdx))) /
									(BaseSpline->GetDistanceAlongSplineAtSplinePoint(InnerIdx) -
									(BaseSpline->GetDistanceAlongSplineAtSplinePoint(InnerIdx + 1) +
									(BaseSpline->GetSplineLength() * LeaveLoopingIdx))));

			OffsetSpline->SetTangentsAtSplinePoint(InnerIdx, ArriveTan, LeaveTan, CoordinateSpace, false);
		}
		OffsetSpline->UpdateSpline();
	}

	CalculateSplineSegmentNumAndLength(BaseSpline, OutNumSegments, OutSegmentLength, IdealSegmentLength);
	CorrectedSpline->ClearSplinePoints(true);
	LastIdx = OutNumSegments + (BaseSpline->IsClosedLoop() ? -1 : 0);
	FVector Location;
	FVector LocationOffset;
	FVector Tangent;
	for (int32 Idx = 0; Idx <= LastIdx; Idx++)
	{
		Location = BaseSpline->GetLocationAtDistanceAlongSpline(Idx * OutSegmentLength, CoordinateSpace);
		CorrectedSpline->AddSplinePointAtIndex(Location, Idx, CoordinateSpace, false);

		LocationOffset = OffsetSpline->FindLocationClosestToWorldLocation(Location, CoordinateSpace);
		CorrectedSpline->SetUpVectorAtSplinePoint(Idx, (LocationOffset - Location).GetSafeNormal(), CoordinateSpace, false);

		Tangent = OutSegmentLength* ((BaseSpline->GetTangentAtDistanceAlongSpline(Idx * OutSegmentLength, CoordinateSpace)).GetSafeNormal());
		CorrectedSpline->SetTangentAtSplinePoint(Idx, Tangent, CoordinateSpace, false);
	}
	CorrectedSpline->UpdateSpline();
}
