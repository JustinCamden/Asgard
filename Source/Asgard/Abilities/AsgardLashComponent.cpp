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
#define DRAW_LASH()	if (LashDrawDebug->GetInt() || bDebugDrawLash) { for (FVector& CurrPoint : LashPointCurrentLocations) { DrawDebugSphere(GetWorld(), CurrPoint, CollisionRadius, 16, FColor::Magenta, false, -1.0f, 0, 0.5f); } }
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
	LashShrinkSpeed = 100.0f;
	PhysicsStepsPerSecond = 300.0f;
	ChildPointCorrectionWeight = 0.4875f;
	Damping = FVector(5.0f, 5.0f, 5.0f);
	Gravity = FVector(0.0f, 0.0f, -20.0f);
	CollisionRadius = 2.5f;
	CollisionChannel = ECollisionChannel::ECC_WorldStatic;
	bAutoIgnoreOwner = true;
	CollisionSkinWidth = 0.01f;
}

// Called when the game starts
void UAsgardLashComponent::BeginPlay()
{
	Super::BeginPlay();

	// Reserve space for the lash points
	LashPointLastLocations.Reserve(MaxLashSegments);
	LashPointCurrentLocations.Reserve(MaxLashSegments);
	LashPointFrameStartLocations.Reserve(MaxLashSegments);
	LashPointBlockingComponents.Reserve(MaxLashSegments);

	// Set the first point to be equal to the location of the component
	FVector ComponentLocation = GetComponentLocation();
	LashPointLastLocations.Add(ComponentLocation);
	LashPointCurrentLocations.Add(ComponentLocation);
	LashPointFrameStartLocations.Add(ComponentLocation);
	LashPointBlockingComponents.Add(nullptr);
}

void UAsgardLashComponent::AddLashSegmentAtEnd()
{
	FVector NewPoint = LashPointLastLocations.Last();
	LashPointLastLocations.Emplace(NewPoint);
	LashPointFrameStartLocations.Emplace(NewPoint);
	NewPoint = LashPointCurrentLocations.Last();
	LashPointCurrentLocations.Emplace(NewPoint);
	LashPointBlockingComponents.Add(nullptr);
	NumLashSegments++;

	return;
}

void UAsgardLashComponent::RemoveLashSegmentFromFront()
{
	checkf(NumLashSegments > 0, TEXT("ERROR: NumLashPoints was <= 0. (AsgardLashComponent, RemoveLashPointFromEnd, %s"), * GetNameSafe(this));
	LashPointLastLocations.RemoveAt(1, 1, false);
	LashPointCurrentLocations.RemoveAt(1, 1, false);
	LashPointFrameStartLocations.RemoveAt(1, 1, false);
	LashPointBlockingComponents.RemoveAt(1, 1, false);
	NumLashSegments--;

	return;
}

bool UAsgardLashComponent::ShrinkLash(float DeltaTime)
{
	// Shrink the first segment
	const FVector& FirstPoint = LashPointCurrentLocations[0];
	FVector& SecondPoint = LashPointCurrentLocations[1];
	SecondPoint = FMath::VInterpConstantTo(SecondPoint, FirstPoint, DeltaTime, LashShrinkSpeed);

	// If the first segment is short, enough remove it
	float DeltaMagSquared = (SecondPoint - FirstPoint).SizeSquared();
	if (DeltaMagSquared <= LashShrinkMaxSegmentLength * LashShrinkMaxSegmentLength)
	{
		RemoveLashSegmentFromFront();

		// Update the length of the last segment if more segments remain
		if (NumLashSegments > 0)
		{
			LashShrinFirstSegmentLastLength = (LashPointCurrentLocations[1] - FirstPoint).Size();
			return false;
		}
		else
		{
			return true;
		}
	}

	// Otherwise, update the length of the last segment
	else
	{
		LashShrinFirstSegmentLastLength = FMath::Sqrt(DeltaMagSquared);
		return false;
	}
}

void UAsgardLashComponent::ApplyVelocityToLashPoints(float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_ASGARD_LashVelocity);

	for (uint8 Idx = 1; Idx <= NumLashSegments; Idx++)
	{
		// Calculate the velocity
		FVector& CurrentLocation = LashPointCurrentLocations[Idx];
		FVector& LastLocation = LashPointLastLocations[Idx];
		FVector Velocity = CurrentLocation - LastLocation;
		Velocity = FMath::Lerp(Velocity, Velocity - (Damping * Velocity), DeltaTime);

		// Update the last and current locations
		LastLocation = CurrentLocation;
		CurrentLocation = CurrentLocation + Velocity + (Gravity * DeltaTime);
	}

	return;
}

void UAsgardLashComponent::ApplyConstraintsToLashPointsFromFront()
{
	SCOPE_CYCLE_COUNTER(STAT_ASGARD_LashConstraints);

	// Solve constraints from the end to give the lash a weighty feely
	for (int32 Idx = NumLashSegments; Idx > 1; Idx--)
	{
		// Cache variables
		FVector& CurrentPoint = LashPointCurrentLocations[Idx];
		FVector& NextPoint = LashPointCurrentLocations[Idx - 1];
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
			CurrentPoint = CurrentPoint + (ToNextPoint * ChildPointCorrectionWeight);
			NextPoint = NextPoint + (ToNextPoint * -(1.0f - ChildPointCorrectionWeight));
		}
	}

	// The first segment is special because it is constrained to the root of the chain, aka the component
	// It can also shrink while the chain is inactive
	// Begin by cashing the variables
	const FVector& FirstPoint = LashPointCurrentLocations[0];
	FVector& SecondPoint = LashPointCurrentLocations[1];
	FVector ToSecondPoint = SecondPoint - FirstPoint;
	float FirstSegmentLength = ToSecondPoint.SizeSquared();

	// If physics and constraint simulation has lengthened the first segment
	if (FirstSegmentLength > LashSegmentMaxLength * LashSegmentMaxLength)
	{
		ToSecondPoint = ToSecondPoint / (FMath::Sqrt(FirstSegmentLength));
		SecondPoint = FirstPoint + (ToSecondPoint * LashSegmentMaxLength);
	}

	return;
}

void UAsgardLashComponent::ApplyConstraintsToLashPointsFromBack()
{
	SCOPE_CYCLE_COUNTER(STAT_ASGARD_LashConstraints);

	// Solve constraints from the end to give the lash a weighty feely
	for (int32 Idx = NumLashSegments; Idx > 1; Idx--)
	{
		// Cache variables
		FVector& CurrentPoint = LashPointCurrentLocations[Idx];
		FVector& NextPoint = LashPointCurrentLocations[Idx - 1];
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
			CurrentPoint = CurrentPoint + (ToNextPoint *  (1.0f - ChildPointCorrectionWeight));
			NextPoint = NextPoint + (ToNextPoint * -ChildPointCorrectionWeight);
		}
	}

	// The first segment is special because it is constrained to the root of the chain, aka the component
	// Don't worry about shrinking while applying constrainst from the back
	// Begin by cashing the variables
	const FVector& FirstPoint = LashPointCurrentLocations[0];
	FVector& SecondPoint = LashPointCurrentLocations[1];
	FVector ToSecondPoint = SecondPoint - FirstPoint;
	float FirstSegmentLength = ToSecondPoint.SizeSquared();

	// If physics and constraint simulation has lengthened the first segment
	if (FirstSegmentLength > LashShrinFirstSegmentLastLength * LashShrinFirstSegmentLastLength)
	{
		ToSecondPoint = ToSecondPoint / (FMath::Sqrt(FirstSegmentLength));
		SecondPoint = FirstPoint + (ToSecondPoint * LashShrinFirstSegmentLastLength);
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
	Params.TraceTag = FName("AsgardLashOverlapTest");
	FCollisionResponseParams ResponseParams;

	// Perform an overlap check for each lash segment
	for (int32 Idx = 1; Idx <= NumLashSegments; Idx++)
	{
		TArray<FOverlapResult> OutOverlaps;

		// Overlap from previous point
		FVector& CurrentPoint = LashPointCurrentLocations[Idx];
		FVector& PreviousPoint = LashPointCurrentLocations[Idx - 1];
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
	for (int32 Idx = 1; Idx <= NumLashSegments; Idx++)
	{
		UPrimitiveComponent* BlockingComponent = nullptr;

		// Trace from the frame start position to the current position
		TArray<FHitResult> HitsFromFrameStart;
		FVector& FrameStart = LashPointFrameStartLocations[Idx];
		FVector& CurrentPoint = LashPointCurrentLocations[Idx];
		bool bHitFromFrameStart = World->SweepMultiByChannel(
			HitsFromFrameStart,
			FrameStart,
			CurrentPoint,
			CollisionRotation,
			CollisionChannel,
			CollisionShape,
			Params,
			ResponseParams);

		// If hit from frame start and did not start penetrating, correct the correct point to the hit location offset by the skinwidth
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
		FVector& PreviousPoint = LashPointCurrentLocations[Idx - 1];
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
				LashPointCurrentLocations.SetNum(Idx, false);
				LashPointLastLocations.SetNum(Idx, false);
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
			// If hit from frame start but not hit from previous, 
			// process hits and overlaps from frame start location,
			// and use the hit from the the frame start location for the blocking component
			if (!bHitFromPrevious)
			{
				CurrentPoint = FrameStartTraceEndpoint;
				TotalHits.Append(HitsFromFrameStart);
				BlockingComponent = FromFrameStartHit->Component.Get();
			}

			// If hit from the previous point
			// and current point nearly equals the frame start trace endpoint,
			// process hits and overlaps from the frame start point
			else if (CurrentPoint.Equals(FrameStartTraceEndpoint, CollisionSkinWidth))
			{
				TotalHits.Append(HitsFromPrevious);
			}

			// If the current point is too close to the previous point,
			// resize the lash to break at this point, and end the loop
			float CutOffDist = 1.0f;
			FVector ToPrevious = PreviousPoint - CurrentPoint;
			float ToPreviousSizeSquared = ToPrevious.SizeSquared();
			if (ToPreviousSizeSquared < CutOffDist * CutOffDist)
			{
				LashPointCurrentLocations.SetNum(Idx, false);
				LashPointLastLocations.SetNum(Idx, false);
				LashPointFrameStartLocations.SetNum(Idx, false);
				LashPointBlockingComponents.SetNum(Idx, false);
				LashPointBlockingComponents[Idx - 1] = BlockingComponent;
				NumLashSegments = Idx - 1;
				break;
			}

			// Otherwise, update the blocking component
			LashPointBlockingComponents[Idx] = BlockingComponent;
			
			// Update the last position for more accurate velocity calculations
			LashPointLastLocations[Idx] = CurrentPoint;
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
	LashPointFrameStartLocations = LashPointCurrentLocations;
	FVector& LashOrigin = LashPointCurrentLocations[0];
	LashPointLastLocations[0] = LashOrigin;
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

		// If active
		if (bLashExtended)
		{
			// For each physics steps
			for (int32 Idx = 0; Idx < NumPhysicsSteps; Idx++)
			{
				// If more lash segments can be added and the last segment is of the minimum length, then add a segment
				if (NumLashSegments < MaxLashSegments
					&& LashPointBlockingComponents[NumLashSegments] == nullptr
					&& (LashPointCurrentLocations[NumLashSegments] - LashPointCurrentLocations[NumLashSegments - 1]).SizeSquared() >= LashGrowthMinSegmentLength * LashGrowthMinSegmentLength)
				{
					AddLashSegmentAtEnd();
				}

				// Update point velocities and contrainst
				ApplyVelocityToLashPoints(PhysicsStepTime);
				ApplyConstraintsToLashPointsFromFront();
			}
		}
		// If not active
		else
		{
			// For	` each physic step while the lash has not fully shrunk
			for (int32 Idx = 0; Idx < NumPhysicsSteps; Idx++)
			{
				// Shrink the lash
				if (ShrinkLash(PhysicsStepTime))
				{
					break;
				}
				else
				{
					// If the lash has still not fully shrink, update point velocities and constraints
					ApplyVelocityToLashPoints(PhysicsStepTime);
					ApplyConstraintsToLashPointsFromBack();
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
