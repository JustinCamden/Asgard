// Copyright © 2020 Justin Camden All Rights Reserved

#include "AsgardLashComponent.h"
#include "Runtime/Engine/Classes/Engine/World.h"

// Stat cycles
DECLARE_CYCLE_STAT(TEXT("AsgardLash SimulateLash"), STAT_ASGARD_LashSimulateLash, STATGROUP_ASGARD_Lash);
DECLARE_CYCLE_STAT(TEXT("AsgardLash Velocity"), STAT_ASGARD_LashVelocity, STATGROUP_ASGARD_Lash);
DECLARE_CYCLE_STAT(TEXT("AsgardLash Constraints"), STAT_ASGARD_LashConstraints, STATGROUP_ASGARD_Lash);
DECLARE_CYCLE_STAT(TEXT("AsgardLash Detection"), STAT_ASGARD_LashDetection, STATGROUP_ASGARD_Lash);
DECLARE_CYCLE_STAT(TEXT("AsgardLash Collision"), STAT_ASGARD_LashCollision, STATGROUP_ASGARD_Lash);

// Console variable setup so we can enable and disable debugging from the console
// Draw detection debug
static TAutoConsoleVariable<int32> CVarAsgardLashDrawDebug(
	TEXT("Asgard.LashDrawDebug"),
	0,
	TEXT("Whether to enable Lash debug drawing.\n")
	TEXT("0: Disabled, 1: Enabled"),
	ECVF_Scalability | ECVF_RenderThreadSafe);
static const auto LashDrawDebug = IConsoleManager::Get().FindConsoleVariable(TEXT("Asgard.LashDrawDebug"));

// Macros for debug builds
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
#include "DrawDebugHelpers.h"
#define DRAW_LASH()	if (LashDrawDebug->GetInt() || bDebugDrawLash) { for (FVector& CurrPoint : LashPointLocations) { DrawDebugSphere(GetWorld(), CurrPoint, CollisionRadius, 16, FColor::Magenta, false, -1.0f, 0, 0.5f); } }
#else
#define DRAW_LASH()	/* nothing */
#endif


// Sets default values for this component's properties
UAsgardLashComponent::UAsgardLashComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	MaxLashSegments = 20;
	LashSegmentMaxLength = 10.0f;
	LashGrowthMinSegmentLength = 7.5f;
	LashShrinkMaxSegmentLength = 0.1f;
	LashCollisionMinSegmentLength = 1.0f;
	LashShrinkSpeedNotExtended = 600.0f;
	LashShrinkSpeedBlocked = 200.0f;
	PhysicsStepsPerSecond = 300.0f;
	ChildPointCorrectionWeight = 0.5125f;
	Damping = FVector(5.0f, 5.0f, 5.0f);
	Gravity = FVector(0.0f, 0.0f, -20.0f);
	CollisionRadius = 2.5f;
	CollisionChannel = ECollisionChannel::ECC_WorldStatic;
	SlidingVelocityCorrectionWeight = 0.5f;
	bAutoIgnoreOwner = true;
	CollisionSkinWidth = 0.1f;
	AttachMaxStretchDistance = 25.0f;
	BlockdPointCorrectionWeight = 1.0f;
}

void UAsgardLashComponent::AttachLashEndToComponent(USceneComponent* AttachToComponent)
{
	if (AttachToComponent)
	{
		AttachedToActor = AttachToComponent->GetOwner();
		AttachedToComponent = AttachToComponent;
	}

	return;
}

void UAsgardLashComponent::DetachLashEndFromComponent()
{
	AttachedToActor = nullptr;
	AttachedToComponent = nullptr;
}

void UAsgardLashComponent::SetLashExtended(bool bNewExtended)
{
	if (bNewExtended)
	{
		bLashExtended = true;
		LashFirstSegmentMaxLength = LashSegmentMaxLength;
	}
	else
	{
		if (NumLashSegments > 0)
		{
			LashFirstSegmentMaxLength = (LashPointLocations[1] - LashPointLocations[0]).Size();
		}
		bLashExtended = false;
	}
}

// Called when the game starts
void UAsgardLashComponent::BeginPlay()
{
	Super::BeginPlay();

	// Reserve space for the lash points
	LashPointSimulationStartLocations.Reserve(MaxLashSegments);
	LashPointLocations.Reserve(MaxLashSegments);
	LashPointFrameStartLocations.Reserve(MaxLashSegments);
	LashPointBlockingComponents.Reserve(MaxLashSegments);

	// Set the first point to be equal to the location of the component
	FVector ComponentLocation = GetComponentLocation();
	LashPointSimulationStartLocations.Add(ComponentLocation);
	LashPointLocations.Add(ComponentLocation);
	LashPointFrameStartLocations.Add(ComponentLocation);
	LashPointBlockingComponents.Add(nullptr);

	// Calculate initial variables
	LashFirstSegmentMaxLength = LashSegmentMaxLength;
}

void UAsgardLashComponent::AddLashSegmentAtEnd()
{
	FVector NewPoint = LashPointSimulationStartLocations.Last();
	LashPointSimulationStartLocations.Emplace(NewPoint);
	LashPointFrameStartLocations.Emplace(NewPoint);
	NewPoint = LashPointLocations.Last();
	LashPointLocations.Emplace(NewPoint);
	LashPointBlockingComponents.Add(nullptr);
	NumLashSegments++;

	return;
}

void UAsgardLashComponent::RemoveLashSegmentFromFront()
{
	checkf(NumLashSegments > 0, TEXT("ERROR: NumLashPoints was <= 0. (AsgardLashComponent, RemoveLashPointFromEnd, %s"), * GetNameSafe(this));
	LashPointSimulationStartLocations.RemoveAt(1, 1, false);
	LashPointLocations.RemoveAt(1, 1, false);
	LashPointFrameStartLocations.RemoveAt(1, 1, false);
	LashPointBlockingComponents.RemoveAt(1, 1, false);
	NumLashSegments--;

	return;
}

void UAsgardLashComponent::RemoveLashSegmentAtIndex(int32 Idx)
{
	checkf(NumLashSegments >= Idx, TEXT("ERROR: NumLashPoints was < %f. (AsgardLashComponent, RemoveLashPointFromEnd, %s"), Idx, *GetNameSafe(this));
	LashPointSimulationStartLocations.RemoveAt(Idx, 1, false);
	LashPointLocations.RemoveAt(Idx, 1, false);
	LashPointFrameStartLocations.RemoveAt(Idx, 1, false);
	LashPointBlockingComponents.RemoveAt(Idx, 1, false);
	NumLashSegments--;
}

bool UAsgardLashComponent::ShrinkLash(float DeltaTime, float ShrinkSpeed)
{
	// Condense any existing lash segments that are close to each other
	for (int32 Idx = 1; Idx < NumLashSegments; Idx++)
	{
		if ((LashPointLocations[Idx + 1] - LashPointLocations[Idx]).SizeSquared() < (LashShrinkMaxSegmentLength * LashShrinkMaxSegmentLength))
		{
			RemoveLashSegmentAtIndex(Idx);
			Idx--;
		}
	}

	// Decrease the maximum length of the last segment
	LashFirstSegmentMaxLength = FMath::FInterpConstantTo(LashFirstSegmentMaxLength, 0.0f, DeltaTime, ShrinkSpeed);

	// If the first segment is short enough, remove it
	if (LashFirstSegmentMaxLength < LashShrinkMaxSegmentLength)
	{
		RemoveLashSegmentFromFront();

		// Update the length of the last segment if more segments remain
		if (NumLashSegments > 0)
		{
			LashFirstSegmentMaxLength = (LashPointLocations[1] - LashPointLocations[0]).Size();
		}
		else
		{
			return true;
		}
	}
	return false;
}

void UAsgardLashComponent::ApplyVelocityToLashPoints(float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_ASGARD_LashVelocity);

	for (uint8 Idx = 1; Idx <= NumLashSegments; Idx++)
	{
		// Calculate the velocity
		FVector& CurrentLocation = LashPointLocations[Idx];
		FVector& LastLocation = LashPointSimulationStartLocations[Idx];
		FVector Velocity = CurrentLocation - LastLocation;
		Velocity.X = FMath::Lerp(Velocity.X, 0.0f, Damping.X * DeltaTime);
		Velocity.Y = FMath::Lerp(Velocity.Y, 0.0f, Damping.Y * DeltaTime);
		Velocity.Z = FMath::Lerp(Velocity.Z, 0.0f, Damping.Z * DeltaTime);

		// Update the last and current locations
		LastLocation = CurrentLocation;
		CurrentLocation = CurrentLocation + Velocity + (Gravity * DeltaTime);
	}

	return;
}

void UAsgardLashComponent::ApplyConstraintsToLashPointsFromFront()
{
	SCOPE_CYCLE_COUNTER(STAT_ASGARD_LashConstraints);

	// The first segment is special because it is constrained to the root of the chain, aka the component location
	// Begin by cashing the variables
	const FVector& FirstPoint = LashPointLocations[0];
	FVector& SecondPoint = LashPointLocations[1];
	FVector ToSecondPoint = SecondPoint - FirstPoint;
	float FirstSegmentLength = ToSecondPoint.SizeSquared();

	// If physics and constraint simulation has lengthened the first segment, cap the size at the max segment length
	if (FirstSegmentLength > LashFirstSegmentMaxLength * LashFirstSegmentMaxLength)
	{
		ToSecondPoint = ToSecondPoint / (FMath::Sqrt(FirstSegmentLength));
		SecondPoint = FirstPoint + (ToSecondPoint * LashFirstSegmentMaxLength);
	}

	// Solve constraints from start to end to give the lash a responsive feeling
	for (int32 Idx = 1; Idx < NumLashSegments; Idx++)
	{
		// Cache variables
		FVector& CurrentPoint = LashPointLocations[Idx];
		FVector& NextPoint = LashPointLocations[Idx + 1];
		FVector ToNextPoint = NextPoint - CurrentPoint;
		float DistToNextPoint = ToNextPoint.SizeSquared();

		// If the current point is too far from the next point
		if (DistToNextPoint > LashSegmentMaxLength * LashSegmentMaxLength)
		{
			// Normalize direction to next point
			DistToNextPoint = FMath::Sqrt(DistToNextPoint);
			ToNextPoint = ToNextPoint / DistToNextPoint;

			// Scale the distance according to the error
			DistToNextPoint -= LashSegmentMaxLength;
			ToNextPoint = ToNextPoint * DistToNextPoint;

			// Apply the correction to the current and next points
			float NextPointCorrectionWeight = (LashPointBlockingComponents[Idx + 1] ? BlockdPointCorrectionWeight : ChildPointCorrectionWeight);
			CurrentPoint = CurrentPoint + (ToNextPoint * (1.0f - NextPointCorrectionWeight));
			NextPoint = NextPoint + (ToNextPoint * -NextPointCorrectionWeight);
		}
	}

	return;
}

void UAsgardLashComponent::ApplyConstraintsToLashPointsFromBack()
{
	SCOPE_CYCLE_COUNTER(STAT_ASGARD_LashConstraints);

	// If the end is attached to a component
	int32 LastIdx = NumLashSegments;
	if (AttachedToComponent)
	{
		// Calculate the designated offset from the attached component
		FVector NewAttachedLocation = AttachedToComponent->GetComponentTransform().TransformPositionNoScale(AttachOffset);

		// If the lash would stretch too far, detach
		if (AttachMaxStretchDistance > 0.0f 
			&& (NewAttachedLocation - LashPointLocations[NumLashSegments - 1]).SizeSquared() > FMath::Square(LashSegmentMaxLength + AttachMaxStretchDistance))
		{
			DetachLashEndFromComponent();
		}

		// Otherwise set the last point to the designated offset from the attached component
		else
		{
			LashPointLocations[LastIdx] = NewAttachedLocation;

			// Update the next point according to constraints
			FVector& NextPoint = LashPointLocations[LastIdx - 1];
			FVector ToNextPoint = NextPoint - NewAttachedLocation;
			float DistToNextPoint = ToNextPoint.SizeSquared();

			// If the current point is too far from the next point
			if (DistToNextPoint > LashSegmentMaxLength * LashSegmentMaxLength)
			{
				ToNextPoint = ToNextPoint / (FMath::Sqrt(DistToNextPoint));
				NextPoint = NewAttachedLocation + (ToNextPoint * LashSegmentMaxLength);
			}


			LastIdx--;
		}
	}

	// Solve constraints from end to start to give the lash a weighty feeling
	for (int32 Idx = LastIdx; Idx > 1; Idx--)
	{
		// Cache variables
		FVector& CurrentPoint = LashPointLocations[Idx];
		FVector& NextPoint = LashPointLocations[Idx - 1];
		FVector ToNextPoint = NextPoint - CurrentPoint;
		float DistToNextPoint = ToNextPoint.SizeSquared();

		// If the current point is too far from the next point
		if (DistToNextPoint > LashSegmentMaxLength* LashSegmentMaxLength)
		{
			// Normalize direction to next point
			DistToNextPoint = FMath::Sqrt(DistToNextPoint);
			ToNextPoint = ToNextPoint / DistToNextPoint;

			// Scale the distance according to the error
			DistToNextPoint -= LashSegmentMaxLength;
			ToNextPoint = ToNextPoint * DistToNextPoint;

			// Apply the correction to the current and next points
			float CurrentPointCorrectionWeight = (LashPointBlockingComponents[Idx] ? BlockdPointCorrectionWeight : ChildPointCorrectionWeight);
			CurrentPoint = CurrentPoint + (ToNextPoint * CurrentPointCorrectionWeight);
			NextPoint = NextPoint + (ToNextPoint * -(1.0f - CurrentPointCorrectionWeight));
		}
	}

	// The first segment is special because it is constrained to the root of the chain, aka the component location
	// It can also shrink while the chain is inactive
	// Begin by cashing the variables
	const FVector& FirstPoint = LashPointLocations[0];
	FVector& SecondPoint = LashPointLocations[1];
	FVector ToSecondPoint = SecondPoint - FirstPoint;
	float FirstSegmentLength = ToSecondPoint.SizeSquared();

	// If physics and constraint simulation has lengthened the first segment, cap the size at the max segment length
	if (FirstSegmentLength > LashFirstSegmentMaxLength * LashFirstSegmentMaxLength)
	{
		ToSecondPoint = ToSecondPoint / (FMath::Sqrt(FirstSegmentLength));
		SecondPoint = FirstPoint + (ToSecondPoint * LashFirstSegmentMaxLength);
	}

	return;
}

void UAsgardLashComponent::DetectActorsInLashSegments()
{
	SCOPE_CYCLE_COUNTER(STAT_ASGARD_LashDetection);

	// Initialize variables for overlap test
	UWorld* World = GetWorld();
	TArray<FOverlapResult> TotalOverlaps;
	FCollisionQueryParams Params;
	Params.AddIgnoredActors(IgnoredActors);
	if (bAutoIgnoreOwner)
	{
		Params.AddIgnoredActors(TArray<AActor*> {GetOwner()});
	}
	if (AttachedToActor)
	{
		Params.AddIgnoredActors(TArray<AActor*> {AttachedToActor});
	}
	Params.TraceTag = FName("AsgardLashOverlapTest");
	FCollisionResponseParams ResponseParams;

	// Perform an overlap check for each lash segment
	for (int32 Idx = 1; Idx <= NumLashSegments; Idx++)
	{
		TArray<FOverlapResult> OutOverlaps;

		// Overlap from previous point
		FVector& CurrentPoint = LashPointLocations[Idx];
		FVector& PreviousPoint = LashPointLocations[Idx - 1];
		FVector OverlapDirection = CurrentPoint - PreviousPoint;
		FCollisionShape CollisionShape = FCollisionShape::MakeCapsule(FVector(CollisionRadius, CollisionRadius, OverlapDirection.Size()));
		World->OverlapMultiByChannel(
			OutOverlaps,
			(CurrentPoint + PreviousPoint) * 0.5f,
			FRotationMatrix::MakeFromZ(OverlapDirection).ToQuat(),
			CollisionChannel,
			CollisionShape,
			Params,
			ResponseParams);
		TotalOverlaps.Append(OutOverlaps);
		OutOverlaps.Reset();

		// Overlap from last location
		FVector& FrameStartLocation = LashPointFrameStartLocations[Idx];
		OverlapDirection = CurrentPoint - FrameStartLocation;
		CollisionShape.SetCapsule(FVector(CollisionRadius, CollisionRadius, OverlapDirection.Size()));
		World->OverlapMultiByChannel(
			OutOverlaps,
			(CurrentPoint + FrameStartLocation) * 0.5f,
			FRotationMatrix::MakeFromZ(OverlapDirection).ToQuat(),
			CollisionChannel,
			CollisionShape,
			Params,
			ResponseParams);
		TotalOverlaps.Append(OutOverlaps);
	}

	// If we detect nothing, clear the list of detected components
	if (ComponentsInLash.Num() > 0 && (TotalOverlaps.Num() <= 0 ))
	{
		for (UPrimitiveComponent* LostComponent : ComponentsInLash)
		{
			OnComponentExitLash.Broadcast(LostComponent);
		}
		for (AActor* LostActor : ActorsInLash)
		{
			OnActorExitLash.Broadcast(LostActor);
		}
		ComponentsInLash.Reset();
		ActorsInLash.Reset();

	}

	// Otherwise, process the results normally
	else
	{
		TSet<AActor*> LostActors = ActorsInLash;
		TSet<UPrimitiveComponent*> LostComponents = ComponentsInLash;

		for (FOverlapResult& Overlap : TotalOverlaps)
		{
			UPrimitiveComponent* DetectedComponent = Overlap.Component.Get();
			if (DetectedComponent)
			{
				AActor* DetectedActor = Overlap.Actor.Get();
				bool bBlockingHit = Overlap.bBlockingHit;

				// Add the actor to the detected list
				bool bAlreadyDetected = false;
				ActorsInLash.Add(DetectedActor, &bAlreadyDetected);

				// If it's a new actor, broadcast the on detected event for it and its detected component
				if (!bAlreadyDetected)
				{
					ComponentsInLash.Add(DetectedComponent);
					OnActorEnterLash.Broadcast(DetectedActor, DetectedComponent, bBlockingHit);
					OnComponentEnterLash.Broadcast(DetectedComponent, bBlockingHit);
				}

				// If the actor is already detected, check if the component is as well
				else
				{
					// Remove detected actor from lost list if appropriate
					LostActors.Remove(DetectedActor);
					ComponentsInLash.Add(DetectedComponent, &bAlreadyDetected);

					// If so, broadcast the appropriate event
					if (!bAlreadyDetected)
					{
						OnComponentEnterLash.Broadcast(DetectedComponent, bBlockingHit);
					}
					else
					{
						// Remove detected component from lost list if appropriate
						LostComponents.Remove(DetectedComponent);
					}
				}
			}
		}

		// Clear components and actors that are no longer detected
		for (UPrimitiveComponent* LostComponent : LostComponents)
		{
			ComponentsInLash.Remove(LostComponent);
			OnComponentExitLash.Broadcast(LostComponent);
		}
		for (AActor* LostActor : LostActors)
		{
			ActorsInLash.Remove(LostActor);
			OnActorExitLash.Broadcast(LostActor);
		}
	}

	return;
}

void UAsgardLashComponent::CollideWithActorsInLashSegments(float DeltaTime, float PhysicsStepTime)
{
	SCOPE_CYCLE_COUNTER(STAT_ASGARD_LashCollision);

	// Initialize variables for trace
	UWorld* World = GetWorld();
	TArray<FHitResult> TotalHits;
	FCollisionQueryParams Params;
	Params.AddIgnoredActors(IgnoredActors);
	if (bAutoIgnoreOwner)
	{
		Params.AddIgnoredActors(TArray<AActor*> {GetOwner()});
	}
	Params.TraceTag = FName("AsgardLashCollisionTest");
	FCollisionResponseParams ResponseParams;
	FCollisionShape CollisionShape = FCollisionShape::MakeSphere(CollisionRadius);
	FQuat CollisionRotation = FQuat();

	// For each lash segment
	bLashBlocked = false;
	for (int32 Idx = 1; Idx <= NumLashSegments; Idx++)
	{
		UPrimitiveComponent* BlockingComponent = nullptr;

		// Trace from the frame start position to the current position
		TArray<FHitResult> HitsFromFrameStart;
		FVector& FrameStart = LashPointFrameStartLocations[Idx];
		FVector& CurrentPoint = LashPointLocations[Idx];
		bool bHitFromFrameStart = World->SweepMultiByChannel(
			HitsFromFrameStart,
			FrameStart,
			CurrentPoint,
			CollisionRotation,
			CollisionChannel,
			CollisionShape,
			Params,
			ResponseParams);

		// If hit from frame start and did not start penetrating, correct the point to the hit location offset by the skinwidth
		FVector FrameStartTraceEndpoint;
		FHitResult* FromFrameStartHit = nullptr;
		if (bHitFromFrameStart)
		{
			FromFrameStartHit = &HitsFromFrameStart.Last();
			if (!FromFrameStartHit->bStartPenetrating)
			{
				FrameStartTraceEndpoint = FromFrameStartHit->Location + (FromFrameStartHit->Normal * (FromFrameStartHit->PenetrationDepth + CollisionSkinWidth));
			}
			else
			{
				FrameStartTraceEndpoint = CurrentPoint;
			}
		}
		else
		{
			FrameStartTraceEndpoint = CurrentPoint;
		}

		// Trace from the previous segment
		TArray<FHitResult> HitsFromPrevious;
		FVector& PreviousPoint = LashPointLocations[Idx - 1];
		bool bHitFromPrevious = World->SweepMultiByChannel(
			HitsFromPrevious,
			PreviousPoint,
			FrameStartTraceEndpoint,
			CollisionRotation,
			CollisionChannel,
			CollisionShape,
			Params,
			ResponseParams);

		// If hit from the previous segment
		FHitResult* FromPreviousHit = nullptr;
		if (bHitFromPrevious)
		{
			// If started penetrating, resize the lash to break at this point, and end the loop
			FromPreviousHit = &HitsFromPrevious.Last();
			if (FromPreviousHit->bStartPenetrating)
			{
				LashPointLocations.SetNum(Idx, false);
				LashPointSimulationStartLocations.SetNum(Idx, false);
				LashPointFrameStartLocations.SetNum(Idx, false);
				LashPointBlockingComponents.SetNum(Idx, false);
				LashPointBlockingComponents[Idx - 1] = FromPreviousHit->Component.Get();
				NumLashSegments = Idx - 1;
				TotalHits.Add(*FromPreviousHit);
				break;
			}

			// Otherwise, offset the current point from the hit location by the skin width
			else
			{
				CurrentPoint = FromPreviousHit->Location + (FromPreviousHit->Normal * (FromPreviousHit->PenetrationDepth + CollisionSkinWidth));
				BlockingComponent = FromPreviousHit->Component.Get();

				// Impart velocity from parent and gravity along the impact normal
				// so that the point doesn't stick to the object it is colliding against
				FVector& LastPoint = LashPointSimulationStartLocations[Idx];
				FVector Correction = (PreviousPoint - LashPointSimulationStartLocations[Idx - 1]);
				Correction += (Gravity * PhysicsStepTime);
				LastPoint = CurrentPoint - (Correction * (1.0f - ChildPointCorrectionWeight));
				Correction = FVector::VectorPlaneProject(Correction, FromPreviousHit->ImpactNormal).GetSafeNormal() * Correction.Size() * ChildPointCorrectionWeight;
				CurrentPoint = CurrentPoint + Correction;
			}
		}

		// Process hits and overlaps frome the previous point
		TotalHits.Append(HitsFromPrevious);

		// If no blocking hits occured, 
		// process hits and overlaps from the frame start location
		if (!bHitFromFrameStart && !bHitFromPrevious)
		{
			LashPointBlockingComponents[Idx] = nullptr;
			TotalHits.Append(HitsFromFrameStart);
		}

		// If blocking hits occured
		else
		{
			bLashBlocked = true;

			// If hit from frame start but not hit from previous, 
			// process hits and overlaps from frame start location,
			// and use the hit from the the frame start location for the blocking component
			if (!bHitFromPrevious)
			{
				CurrentPoint = FrameStartTraceEndpoint;
				TotalHits.Append(HitsFromFrameStart);
				BlockingComponent = FromFrameStartHit->Component.Get();


				// Old sliding code

				// Impart velocity from parent and gravity along the impact normal
				// so that the point doesn't stick to the object it is colliding against
				//FVector& LastPoint = LashPointSimulationStartLocations[Idx];
				//FVector Correction = (PreviousPoint - LashPointSimulationStartLocations[Idx - 1]);
				//Correction += (Gravity * PhysicsStepTime);
				//LastPoint = CurrentPoint - (Correction * (1.0f - SlidingVelocityCorrectionWeight));
				//Correction = FVector::VectorPlaneProject(Correction, FromFrameStartHit->ImpactNormal).GetSafeNormal() * Correction.Size() * SlidingVelocityCorrectionWeight;
				//CurrentPoint = CurrentPoint + Correction;

				LashPointSimulationStartLocations[Idx] = CurrentPoint;
			}

			// If hit from the previous point
			// and current point nearly equals the frame start trace endpoint,
			// process hits and overlaps from the frame start point
			else if (CurrentPoint.Equals(FrameStartTraceEndpoint, CollisionSkinWidth))
			{
				TotalHits.Append(HitsFromPrevious);
			}

			// If the previous and next points are close enough for there to be one lash segment, remove the current point
			if ((PreviousPoint - CurrentPoint).SizeSquared() < (LashCollisionMinSegmentLength * LashCollisionMinSegmentLength)
				|| Idx < NumLashSegments && (LashPointLocations[Idx + 1] - PreviousPoint).SizeSquared() < (LashSegmentMaxLength * LashCollisionMinSegmentLength))
			{
				LashPointBlockingComponents[Idx - 1] = BlockingComponent;
				RemoveLashSegmentAtIndex(Idx);

				break;
			}

			// Update the blocking component
			LashPointBlockingComponents[Idx] = BlockingComponent;
		}
	}

	// If we detect nothing, clear the list of detected components
	if (ComponentsInLash.Num() > 0 && (TotalHits.Num() <= 0))
	{
		for (UPrimitiveComponent* LostComponent : ComponentsInLash)
		{
			OnComponentExitLash.Broadcast(LostComponent);
		}
		for (AActor* LostActor : ActorsInLash)
		{
			OnActorExitLash.Broadcast(LostActor);
		}
		ComponentsInLash.Reset();
		ActorsInLash.Reset();
	}

	// Otherwise, process the results normally
	else
	{
		TSet<AActor*> LostActors = ActorsInLash;
		TSet<UPrimitiveComponent*> LostComponents = ComponentsInLash;

		for (FHitResult& Hit : TotalHits)
		{
			UPrimitiveComponent* DetectedComponent = Hit.Component.Get();
			if (DetectedComponent)
			{
				AActor* DetectedActor = Hit.Actor.Get();
				bool bBlockingHit = Hit.bBlockingHit;

				// Add the actor to the detected list
				bool bAlreadyDetected = false;
				ActorsInLash.Add(DetectedActor, &bAlreadyDetected);

				// If it's a new actor, broadcast the on detected event for it and its detected component
				if (!bAlreadyDetected)
				{
					ComponentsInLash.Add(DetectedComponent);
					OnActorEnterLash.Broadcast(DetectedActor, DetectedComponent, bBlockingHit);
					OnComponentEnterLash.Broadcast(DetectedComponent, bBlockingHit);
				}

				// If the actor is already detected, check if the component is as well
				else
				{
					// Remove detected actor from lost list if appropriate
					LostActors.Remove(DetectedActor);
					ComponentsInLash.Add(DetectedComponent, &bAlreadyDetected);

					// If so, broadcast the appropriate event
					if (!bAlreadyDetected)
					{
						OnComponentEnterLash.Broadcast(DetectedComponent, bBlockingHit);
					}
					else
					{
						// Remove detected component from lost list if appropriate
						LostComponents.Remove(DetectedComponent);
					}
				}
			}
		}

		// Clear components and actors that are no longer detected
		for (UPrimitiveComponent* LostComponent : LostComponents)
		{
			ComponentsInLash.Remove(LostComponent);
			OnComponentExitLash.Broadcast(LostComponent);
		}
		for (AActor* LostActor : LostActors)
		{
			ActorsInLash.Remove(LostActor);
			OnActorExitLash.Broadcast(LostActor);
		}
	}

	return;
}


// Called every frame
void UAsgardLashComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	// Update origin position
	LashPointFrameStartLocations = LashPointLocations;
	FVector& LashOrigin = LashPointLocations[0];
	LashPointSimulationStartLocations[0] = LashOrigin;
	LashOrigin = GetComponentLocation();

	// If there are lash segments extended
	if (NumLashSegments > 0)
	{
		SCOPE_CYCLE_COUNTER(STAT_ASGARD_LashSimulateLash);

		// Calculate the number of physics steps
		float PhysicsStepTime = 1.0f / PhysicsStepsPerSecond;
		PhysicsStepRemainder += DeltaTime;
		int32 NumPhysicsSteps = (int32)(PhysicsStepRemainder / PhysicsStepTime);
		PhysicsStepRemainder -= (float)NumPhysicsSteps * PhysicsStepTime;

		// If extended and not blocked
		if (bLashExtended && !bLashBlocked)
		{
			// For each physics steps
			for (int32 Idx = 0; Idx < NumPhysicsSteps; Idx++)
			{
				// If more lash segments can be added and the last segment is of the minimum length, then add a segment
				if (NumLashSegments < MaxLashSegments
					&& LashPointBlockingComponents[NumLashSegments] == nullptr
					&& (LashPointLocations[NumLashSegments] - LashPointLocations[NumLashSegments - 1]).SizeSquared() >= LashGrowthMinSegmentLength * LashGrowthMinSegmentLength)
				{
					AddLashSegmentAtEnd();
				}

				// Update point velocities
				ApplyVelocityToLashPoints(PhysicsStepTime);

				// If the last segment is attached to a component, apply constraints from the back to make the lash feel weighty
				if (AttachedToComponent)
				{
					ApplyConstraintsToLashPointsFromBack();
				}

				// Otherwise, apply constrainst from the front so that it feels responsive
				else
				{
					ApplyConstraintsToLashPointsFromFront();
				}

			}
		}

		// If not extended or blocked
		else
		{
			float ShrinkSpeed = (bLashExtended ? LashShrinkSpeedBlocked : LashShrinkSpeedNotExtended);

			// For each physic step while the lash has not fully shrunk
			for (int32 Idx = 0; Idx < NumPhysicsSteps; Idx++)
			{
				// Shrink the lash
				if (ShrinkLash(PhysicsStepTime, ShrinkSpeed))
				{
					break;
				}
				else
				{
					// If the lash has still not fully shrink, update point velocities and constraints
					ApplyVelocityToLashPoints(PhysicsStepTime);
					ApplyConstraintsToLashPointsFromBack();

					// If the last segment is attached to a component, apply constraints from the back to make the lash feel weighty
					if (AttachedToComponent)
					{
						ApplyConstraintsToLashPointsFromBack();
					}

					// Otherwise, apply constrainst from the front so that it feels responsive
					else
					{
						ApplyConstraintsToLashPointsFromFront();
					}
				}
			}
		}

		// If collision is enabled, run the appropriate collision checks
		if (bEnableCollision && NumLashSegments > 0)
		{
			if (bBlockedByObjects)
			{
				CollideWithActorsInLashSegments(DeltaTime, PhysicsStepTime);
			}
			else
			{
				DetectActorsInLashSegments();
			}
		}

		// Else, if objects are detected, clear them
		else if (ComponentsInLash.Num() > 0)
		{
			for (UPrimitiveComponent* LostComponent : ComponentsInLash)
			{
				OnComponentExitLash.Broadcast(LostComponent);
			}
			for (AActor* LostActor : ActorsInLash)
			{
				OnActorExitLash.Broadcast(LostActor);
			}
			ComponentsInLash.Reset();
			ActorsInLash.Reset();
		}
	}

	// Otherwise, if the lash is active
	else if (bLashExtended)
	{
		// Add a segment
		AddLashSegmentAtEnd();
	}

	DRAW_LASH();

	return;
}
