// Copyright © 2020 Justin Camden All Rights Reserved


#include "AsgardPlayerCharacter.h"
#include "AsgardInputBindings.h"
#include "AsgardPlayerMovementComponent.h"

AAsgardPlayerCharacter::AAsgardPlayerCharacter(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer.SetDefaultSubobjectClass<UAsgardPlayerMovementComponent>(ACharacter::CharacterMovementComponentName))
{
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
		AsgardMovementComponent = Cast<UAsgardPlayerMovementComponent>(MoveComp);
	}
}

void AAsgardPlayerCharacter::BeginPlay()
{
	// Initialize input settings
	SetWalkingOrientationMode(WalkingOrientationMode);

	Super::BeginPlay();
}

void AAsgardPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateMovement();

	return;
}

void AAsgardPlayerCharacter::OnMoveForwardAxis(float Value)
{
	MoveForwardAxis = Value;

	return;
}

void AAsgardPlayerCharacter::OnMoveRightAxis(float Value)
{
	MoveRightAxis = Value;

	return;
}

void AAsgardPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up movement bindings
	PlayerInputComponent->BindAxis(UAsgardInputBindings::MoveForwardAxis(), this, &AAsgardPlayerCharacter::OnMoveForwardAxis);
	PlayerInputComponent->BindAxis(UAsgardInputBindings::MoveRightAxis(), this, &AAsgardPlayerCharacter::OnMoveRightAxis);

	return;
}

/**
* Whether the character can currently walk
* By default, returns bIsWalkingEnabled
*/
bool AAsgardPlayerCharacter::CanWalk_Implementation()
{
	return bIsWalkingEnabled;
}

/**
* Whether the character can currently fly
* By default, returns bIsFlyingingEnabled
*/
bool AAsgardPlayerCharacter::CanFly_Implementation()
{
	return bIsFlyingEnabled;
}

/**
* Setter for WalkingOrientationMode
* Sets the WalkingOrientationComponent to match orientation mode
*/
void AAsgardPlayerCharacter::SetWalkingOrientationMode(const EAsgardWalkingOrientationMode NewWalkingOrientationMode)
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

/**
 * Updates movement depending on current settings and player input
 */
void AAsgardPlayerCharacter::UpdateMovement()
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

/**
* Calculates an evenly weighted, circular input vector from two directions and a deadzone
* Deadzone must always be less than 1 with a minimum of 0
* Maxzone must always be less or equal to 1 and greater or equal to 0
* Maxzone must always be greater than the deadzone
*/
const FVector AAsgardPlayerCharacter::CalculateCircularInputVector(float AxisX, float AxisY, float DeadZone, float MaxZone) const
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

/**
* Calculates an input vector weighted to more heavily favor axis aligned directions
* (up, down, left, right) from two directions, a deadzone, and a maxzone
* Deadzone must always be less than 1 with a minimum of 0
*/
const FVector AAsgardPlayerCharacter::CalculateCrossInputVector(float AxisX, float AxisY, float DeadZone, float MaxZone) const
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

/**
* Called when the character starts walking on the ground
*/
void AAsgardPlayerCharacter::StartWalking()
{
	if (!bIsWalking)
	{
		bIsWalking = true;
		OnWalkingStarted();
	}
	return;
}

/**
* Called when the character stops walking on the ground
*/
void AAsgardPlayerCharacter::StopWalking()
{
	if (bIsWalking)
	{
		bIsWalking = false;
		OnWalkingStopped();
	}

	return;
}

