// Copyright © 2020 Justin Camden All Rights Reserved


#include "AsgardVRCharacter.h"
#include "AsgardVRMovementComponent.h"
#include "Asgard/Core/AsgardInputBindings.h"
#include "Asgard/Core/AsgardCollisionProfiles.h"
#include "NavigationSystem/Public/NavigationSystem.h"
#include "Runtime/Engine/Public/EngineUtils.h"


AAsgardVRCharacter::AAsgardVRCharacter(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer.SetDefaultSubobjectClass<UAsgardVRMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	//PlayerNavFilterClass = UNavigationQueryFilter::StaticClass();

	PrimaryActorTick.bCanEverTick = true;

	// Walking input
	WalkingInputAxisWeightMode = EAsgardInputAxisWeightMode::Circular;
	WalkingInputDeadZone = 0.15f;
	WalkingInputMaxZone = 0.9f;
	WalkingInputForwardMultiplier = 1.0f;
	WalkingInputStrafeMultiplier = 1.0f;
	WalkingInputBackwardMultiplier = 1.0f;
	WalkingOrientationMaxAbsPitchRadians = 0.9f;

	// Walking values
	WalkingOrientationMode = EAsgardWalkingOrientationMode::LeftController;

	// Movement permissions
	bIsWalkingEnabled = true;
	bIsFlyingEnabled = false;

	// Movement state
	bIsWalking = false;
	
	// Component references
	if (UPawnMovementComponent* MoveComp = GetMovementComponent())
	{
		AsgardMovementComponent = Cast<UAsgardVRMovementComponent>(MoveComp);
	}

	// Collision profiles
	GetCapsuleComponent()->SetCollisionProfileName(UAsgardCollisionProfiles::VRRoot());
}

void AAsgardVRCharacter::BeginPlay()
{
	// Initialize input settings
	SetWalkingOrientationMode(WalkingOrientationMode);

	// Cache the navdata
	CacheNavData();

	Super::BeginPlay();
}

void AAsgardVRCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateMovement();

	return;
}

void AAsgardVRCharacter::OnMoveForwardAxis(float Value)
{
	MoveForwardAxis = Value;

	return;
}

void AAsgardVRCharacter::OnMoveRightAxis(float Value)
{
	MoveRightAxis = Value;

	return;
}

void AAsgardVRCharacter::SetWalkingOrientationMode(const EAsgardWalkingOrientationMode NewWalkingOrientationMode)
{
	WalkingOrientationMode = NewWalkingOrientationMode;
	switch (WalkingOrientationMode)
	{
		case EAsgardWalkingOrientationMode::LeftController:
		{
			WalkingOrientationComponent = LeftMotionController;
			break;
		}
		case EAsgardWalkingOrientationMode::RightController:
		{
			WalkingOrientationComponent = RightMotionController;
			break;
		}
		case EAsgardWalkingOrientationMode::VRCamera:
		{
			WalkingOrientationComponent = VRReplicatedCamera;
			break;
		}
		case EAsgardWalkingOrientationMode::CharacterCapsule:
		{
			WalkingOrientationComponent = VRRootReference;
			break;
		}
		default:
		{
			ensureMsgf(false, TEXT("Invalid orientation mode! (Actor %s, SetWalkingOrientationMode)"), *GetNameSafe(this));
			break;
		}
	}

	return;
}

void AAsgardVRCharacter::SetFlyingOrientationMode(const EAsgardFlyingAnalogOrientationMode NewFlyingOrientationMode)
{
	FlyingAnalogOrientationMode = NewFlyingOrientationMode;
	switch (FlyingAnalogOrientationMode)
	{
		case EAsgardFlyingAnalogOrientationMode::LeftController:
		{
			FlyingAnalogOrientationComponent = LeftMotionController;
			break;
		}
		case EAsgardFlyingAnalogOrientationMode::RightController:
		{
			FlyingAnalogOrientationComponent = RightMotionController;
			break;
		}
		case EAsgardFlyingAnalogOrientationMode::VRCamera:
		{
			FlyingAnalogOrientationComponent = VRReplicatedCamera;
			break;
		}
		default:
		{
			ensureMsgf(false, TEXT("Invalid orientation mode! (Actor %s, SetWalkingOrientationMode)"), *GetNameSafe(this));
			break;
		}
	}

	return;
}

void AAsgardVRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up movement bindings
	PlayerInputComponent->BindAxis(UAsgardInputBindings::MoveForwardAxis(), this, &AAsgardVRCharacter::OnMoveForwardAxis);
	PlayerInputComponent->BindAxis(UAsgardInputBindings::MoveRightAxis(), this, &AAsgardVRCharacter::OnMoveRightAxis);

	return;
}

bool AAsgardVRCharacter::CanWalk_Implementation()
{
	return bIsWalkingEnabled;
}

bool AAsgardVRCharacter::CanFly_Implementation()
{
	return bIsFlyingEnabled;
}

bool AAsgardVRCharacter::IsAboveNavMesh(float MaxHeight, FVector& OutProjectedPoint)
{
	// Return true if we are moving on the ground
	if (VRMovementReference->IsMovingOnGround())
	{
		return true;
	}

	// Otherwise, perform a trace from the bottom of the capsule collider towards the ground
	FVector TraceOrigin = GetCapsuleComponent()->GetComponentLocation();
	FVector TraceEnd = TraceOrigin;
	TraceEnd.Z -= (MaxHeight + GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	if (GetWorld()->LineTraceSingleByProfile(HitResult, TraceOrigin, TraceEnd, UAsgardCollisionProfiles::VRRoot(), Params))
	{
		// If we hit an object, check to see if the point can be projected onto a navmesh;
		return ProjectPointToVRNavigation(HitResult.ImpactPoint, OutProjectedPoint, false);
	}

	return false;
}

void AAsgardVRCharacter::CacheNavData()
{
	UWorld* World = GetWorld();
	if (World)
	{
		for (ANavigationData* CurrNavData : TActorRange<ANavigationData>(World))
		{
			if (GetNameSafe(CurrNavData) == "RecastNavMesh-VRCharacter")
			{
				NavData = CurrNavData;
				break;
			}
		}
	}

	return;
}

bool AAsgardVRCharacter::ProjectPointToVRNavigation(const FVector& Point, FVector& OutProjectedPoint, bool bCheckIfIsOnGround)
{
	// Project the to the navigation
	UNavigationSystemV1* const NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (NavSys)
	{
		FNavLocation ProjectedNavLoc(Point);
		bool const bRetVal = NavSys->ProjectPointToNavigation(Point,
			ProjectedNavLoc,
			(NavQueryExtent.IsNearlyZero() ? INVALID_NAVEXTENT : NavQueryExtent),
			NavData,
			UNavigationQueryFilter::GetQueryFilter(*NavData, this, NavFilterClass));
		OutProjectedPoint = ProjectedNavLoc.Location;

		// If point projected successfully and we want to ensure the point is on the ground
		if (bRetVal && bCheckIfIsOnGround)
		{
			// Trace downwards and see if we hit something
			FHitResult GroundTraceHit;
			const FVector GroundTraceOrigin = OutProjectedPoint;
			const FVector GroundTraceEnd = GroundTraceOrigin + FVector(0.0f, 0.0f, -200.0f);
			FCollisionQueryParams GroundTraceParams(FName(TEXT("GroundTrace")), false, this);
			if (GetWorld()->LineTraceSingleByProfile(GroundTraceHit, GroundTraceOrigin, GroundTraceEnd, UAsgardCollisionProfiles::VRRoot(), GroundTraceParams))
			{
				// If so, return the point of impact
				OutProjectedPoint = GroundTraceHit.ImpactPoint;
				return true;
			}
			else
			{
				// Otherwise, return false
				return false;
			}
		}

		// Otherwise return the projection result
		else 
		{
			return bRetVal;
		}
	}

	return false;
}

void AAsgardVRCharacter::UpdateMovement()
{
	// Branch depending on current movement mode
	switch (AsgardMovementComponent->GetAsgardMovementMode())
	{
		case EAsgardMovementMode::C_MOVE_Walking:
		{
			// If we can walk
			if (CanWalk())
			{
				// Get the input vector depending on the walking weight mode
				FVector WalkingInput = FVector::ZeroVector;
				if (WalkingInputAxisWeightMode == EAsgardInputAxisWeightMode::Circular)
				{
					WalkingInput = CalculateCircularInputVector(
						MoveForwardAxis,
						MoveRightAxis,
						WalkingInputDeadZone,
						WalkingInputMaxZone);
				}
				else
				{
					WalkingInput = CalculateCrossInputVector(
						MoveForwardAxis,
						MoveRightAxis,
						WalkingInputDeadZone,
						WalkingInputMaxZone);
				}

				// Guard against no input
				if (FMath::IsNearlyZero(WalkingInput.SizeSquared()))
				{
					StopWalking();
					break;
				}

				// Transform the input vector from the forward of the orientation component
				FVector WalkingForward = WalkingOrientationComponent->GetForwardVector();

				// Guard against the forward vector being nearly up or down
				if (FMath::Abs(FVector::DotProduct(FVector::UpVector, WalkingForward)) > (WalkingOrientationMaxAbsPitchRadians))
				{
					StopWalking();
					break;
				}

				// Flatten the input vector
				WalkingForward.Z = 0.0f;
				WalkingForward = WalkingForward.GetSafeNormal();
				FVector WalkingRight = FVector::CrossProduct(FVector::UpVector, WalkingForward);

				// Scale the input vector by the forward, backward, and strafing multipliers
				WalkingInput *= FVector(WalkingInput.X > 0.0f ? WalkingInputForwardMultiplier : WalkingInputBackwardMultiplier, WalkingInputStrafeMultiplier, 0.0f);
				WalkingInput = (WalkingInput.X * WalkingForward) + (WalkingInput.Y * WalkingRight);

				// Guard against the input vector being nullified
				if (FMath::IsNearlyZero(WalkingInput.SizeSquared()))
				{
					StopWalking();
					break;
				}

				// Apply the movement input
				StartWalking();
				AddMovementInput(WalkingInput);
			}

			// Else, ensure we are not in a walking state
			else
			{
				StopWalking();
			}

			break;
		}
		case EAsgardMovementMode::C_MOVE_Flying:
		{
			// If we can fly
			if (CanFly())
			{
				// If 

				// Add thrust from the left and right hand thrusters
				if (bIsLeftThrusterActive)
				{
					AsgardMovementComponent->AddInputVector(LeftMotionController->GetForwardVector());
				}
				if (bIsRightThrusterActive)
				{
					AsgardMovementComponent->AddInputVector(RightMotionController->GetForwardVector());
				}
			}

			// Otherwise, change movement mode to walking
			else
			{
				AsgardMovementComponent->SetAsgardMovementMode(EAsgardMovementMode::C_MOVE_Falling);
			}

			break;
		}

		default:
		{
			ensureMsgf(false, TEXT("Invalid movement mode! (Actor %s, UpdateMovement)"), *GetNameSafe(this));
			break;
		}
	}
}

const FVector AAsgardVRCharacter::CalculateCircularInputVector(float AxisX, float AxisY, float DeadZone, float MaxZone) const
{
	ensureMsgf(DeadZone < 1.0f && DeadZone >= 0.0f, 
		TEXT("Invalid Dead Zone for calculating an input vector! (Actor %s, Value %f, CalculateCircularInputVecto)"), 
		*GetNameSafe(this), DeadZone);

	ensureMsgf(MaxZone <= 1.0f && MaxZone >= 0.0f,
		TEXT("Invalid Max Zone for calculating an input vector! (Actor %s, Value %f, CalculateCircularInputVector)"),
		*GetNameSafe(this),MaxZone);

	ensureMsgf(MaxZone > DeadZone,
		TEXT("Max Zone must be greater than Dead Zone for calculating an input vector! (Actor %s, Max Zone %f, Dead Zone %f, CalculateCircularInputVector)"),
		*GetNameSafe(this), MaxZone, DeadZone);

	// Cache the size of the input vector
	FVector AdjustedInputVector = FVector(AxisX, AxisY, 0.0f);
	float InputVectorLength = AdjustedInputVector.SizeSquared();

	ensureMsgf(InputVectorLength <= 1.0f,
		TEXT("Input Axis size was greater than 1! Inputs must be normalized! (Actor %s, Value %f, CalculateCircularInputVector)"),
		*GetNameSafe(this), InputVectorLength);

	// If the size of the input vector is greater than the deadzone
	if (InputVectorLength > DeadZone * DeadZone)
	{
		// Normalize the size of the vector to the remaining space between the deadzone and the maxzone
		AdjustedInputVector.Normalize();
		if (InputVectorLength < MaxZone * MaxZone)
		{
			InputVectorLength = FMath::Sqrt(InputVectorLength);
			AdjustedInputVector *= (InputVectorLength - DeadZone) / (MaxZone - DeadZone);
		}
		return AdjustedInputVector;
	}
	else
	{
		return FVector::ZeroVector;
	}
}

const FVector AAsgardVRCharacter::CalculateCrossInputVector(float AxisX, float AxisY, float DeadZone, float MaxZone) const
{
	ensureMsgf(DeadZone < 1.0f && DeadZone >= 0.0f,
		TEXT("Invalid Dead Zone for calculating an input vector! (Actor %s, Value %f, CalculateCrossInputVector)"),
		*GetNameSafe(this), DeadZone);

	ensureMsgf(MaxZone <= 1.0f && MaxZone >= 0.0f,
		TEXT("Invalid Max Zone for calculating an input vector! (Actor %s, Value %f, CalculateCrossInputVector)"),
		*GetNameSafe(this), MaxZone);

	ensureMsgf(MaxZone > DeadZone,
		TEXT("Max Zone must be greater than Dead Zone an input vector! (Actor %s, Max Zone %f, Dead Zone %f, CalculateCrossInputVector)"),
		*GetNameSafe(this), MaxZone, DeadZone);

	// If the X axis is greater than the dead zone
	float AdjustedAxisX = FMath::Abs(AxisX);
	if (AdjustedAxisX > DeadZone)
	{
		// Normalize the X axis to the space between the dead zone and max zone
		if (AdjustedAxisX < MaxZone)
		{
			AdjustedAxisX = ((AdjustedAxisX - DeadZone) / (MaxZone - DeadZone)) * FMath::Sign(AxisX);
		}
		else
		{
			AdjustedAxisX = 1.0f;
		}
	}

	// Otherwise, X axis is 0
	else
	{
		AdjustedAxisX = 0.0f;
	}

	// If the Y axis is greater than the dead zone
	float AdjustedAxisY = FMath::Abs(AxisY);
	if (AdjustedAxisY > DeadZone)
	{
		// Normalize the Y axis to the space between the dead zone and max zone
		if (AdjustedAxisY < MaxZone)
		{
			AdjustedAxisY = ((AdjustedAxisY - DeadZone) / (MaxZone - DeadZone)) * FMath::Sign(AxisY);
		}
		else
		{
			AdjustedAxisY = 1.0f;
		}
	}

	// Otherwise, Y axis is 0
	else
	{
		AdjustedAxisY = 0.0f;
	}

	FVector RetVal = FVector(AdjustedAxisX, AdjustedAxisY, 0.0f);
	return RetVal;
}

void AAsgardVRCharacter::StartWalking()
{
	if (!bIsWalking)
	{
		bIsWalking = true;
		OnWalkingStarted();
	}
	return;
}

void AAsgardVRCharacter::StopWalking()
{
	if (bIsWalking)
	{
		bIsWalking = false;
		OnWalkingStopped();
	}

	return;
}

