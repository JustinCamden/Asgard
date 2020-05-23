// Copyright © 2020 Justin Camden All Rights Reserved


#include "AsgardVRCharacter.h"
#include "AsgardVRMovementComponent.h"
#include "Asgard/Core/AsgardInputBindings.h"
#include "Asgard/Core/AsgardCollisionProfiles.h"
#include "Asgard/Core/AsgardTraceChannels.h"
#include "NavigationSystem/Public/NavigationSystem.h"
#include "Runtime/Engine/Public/EngineUtils.h"
#include "Runtime/Engine/Public/TimerManager.h"
#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"

// Log category
DEFINE_LOG_CATEGORY_STATIC(LogAsgardVRCharacter, Log, All);

// Stat cycles
DECLARE_CYCLE_STAT(TEXT("AsgardVRCharacter TraceTeleportLocation"), STAT_ASGARD_VRCharacterTraceTeleportLocation, STATGROUP_ASGARD_VRCharacter);
DECLARE_CYCLE_STAT(TEXT("AsgardVRCharacter ContinuousTeleportToLocation"), STAT_ASGARD_VRCharacterContinuousTeleportToLocation, STATGROUP_ASGARD_VRCharacter);
DECLARE_CYCLE_STAT(TEXT("AsgardVRCharacter ContinuousTeleportToRotation"), STAT_ASGARD_VRCharacterContinuousTeleportToRotation, STATGROUP_ASGARD_VRCharacter);

// Console variable setup so we can enable and disable debugging from the console
// Draw teleport debug
static TAutoConsoleVariable<int32> CVarAsgardTeleportDrawDebug(
	TEXT("Asgard.TeleportDrawDebug"),
	0,
	TEXT("Whether to enable teleportation debug.\n")
	TEXT("0: Disabled, 1: Enabled"),
	ECVF_Scalability | ECVF_RenderThreadSafe);
static const auto TeleportDrawDebug = IConsoleManager::Get().FindConsoleVariable(TEXT("Asgard.TeleportDrawDebug"));

// Draw teleport debug lifetime
static TAutoConsoleVariable<float> CVarAsgardTeleportDebugLifetime(
	TEXT("Asgard.TeleportDebugLifetime"),
	0.04f,
	TEXT("Duration of debug drawing for teleporting, in seconds."),
	ECVF_Scalability | ECVF_RenderThreadSafe);
static const auto TeleportDebugLifetime = IConsoleManager::Get().FindConsoleVariable(TEXT("Asgard.TeleportDebugLifetime"));

// Macros for debug builds
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
#define TELEPORT_LOC(_Loc, _Radius, _Color)				if (TeleportDrawDebug->GetInt()) { DrawDebugSphere(GetWorld(), _Loc, _Radius, 16, _Color, false, -1.0f, 0, 3.0f); }
#define TELEPORT_LINE(_Loc, _Dest, _Color)				if (TeleportDrawDebug->GetInt()) { DrawDebugLine(GetWorld(), _Loc, _Dest, _Color, false,  -1.0f, 0, 3.0f); }
#define TELEPORT_LOC_DURATION(_Loc, _Radius, _Color)	if (TeleportDrawDebug->GetInt()) { DrawDebugSphere(GetWorld(), _Loc, _Radius, 16, _Color, false, TeleportDebugLifetime->GetFloat(), 0, 3.0f); }
#define TELEPORT_LINE_DURATION(_Loc, _Dest, _Color)		if (TeleportDrawDebug->GetInt()) { DrawDebugLine(GetWorld(), _Loc, _Dest, _Color, false, TeleportDebugLifetime->GetFloat(), 0, 3.0f); }
#define TELEPORT_TRACE_DEBUG_TYPE(_Params)				if (TeleportDrawDebug->GetInt()) { _Params.DrawDebugType = EDrawDebugTrace::ForOneFrame; }
#else
#define TELEPORT_LOC(_Loc, _Radius, _Color)				/* nothing */
#define TELEPORT_LINE(_Loc, _Dest, _Color)				/* nothing */
#define TELEPORT_LOC_DURATION(_Loc, _Radius, _Color)	/* nothing */
#define TELEPORT_LINE_DURATION(_Loc, _Dest, _Color)		/* nothing */
#define TELEPORT_TRACE_DEBUG_TYPE()						/* nothing */
#endif

AAsgardVRCharacter::AAsgardVRCharacter(const FObjectInitializer& ObjectInitializer)
:Super(ObjectInitializer.SetDefaultSubobjectClass<UAsgardVRMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;

	// Navigation settings
	NavQueryExtent = FVector(150.f, 150.f, 150.f);

	// Teleport settings
	bTeleportEnabled = true;
	TeleportTraceRadius = 4.0f;
	TeleportTraceMaxSimTime = 3.0f;
	MoveActionTraceTeleportVelocityMagnitude = 1000.0f;
	MoveActionTraceTeleportOrientationMode = EAsgardBinaryHand::RightHand;
	MoveActionTraceTeleportChannel = UAsgardTraceChannels::VRTeleportTrace;
	TeleportTurnAngleIntervalDegrees = 45.0f;
	TurnActionPressedThreshold = 0.5f;
	TeleportTurnHeldInterval = 0.75f;

	// Walk Settings
	bWalkEnabled = true;
	WalkInputDeadZone = 0.15f;
	WalkInputMaxZone = 0.9f;
	WalkInputForwardMultiplier = 1.0f;
	WalkInputStrafeMultiplier = 1.0f;
	WalkInputBackwardMultiplier = 1.0f;
	WalkOrientationMaxAbsPitchPiRadians = 0.9f;

	// Flight settings
	bFlightEnabled = true;
	bFlightAutoLandingEnabled = true;
	FlightAutoLandingMaxHeight = 50.0f;
	FlightAutoLandingMaxAnglePiRadians = 0.15f;
	bIsFlightAnalogMovementEnabled = true;
	FlightAnalogInputDeadZone = 0.15f;
	FlightAnalogInputMaxZone = 0.9f;
	FlightAnalogInputForwardMultiplier = 1.0f;
	FlightAnalogInputStrafeMultiplier = 1.0f;
	FlightAnalogInputBackwardMultiplier = 1.0f;
	bShouldFlightControllerThrustersAutoStartFlight = true;
	FlightControllerThrusterInputMultiplier = 1.0f;

	// Movement state
	bIsWalk = false;

	// Component references
	if (UPawnMovementComponent* MoveComp = GetMovementComponent())
	{
		AsgardVRMovementRef = Cast<UAsgardVRMovementComponent>(MoveComp);
	}

	// Collision profiles
	GetCapsuleComponent()->SetCollisionProfileName(UAsgardCollisionProfiles::VRRoot());
}

void AAsgardVRCharacter::BeginPlay()
{
	// Initialize input settings
	SetWalkOrientationMode(WalkOrientationMode);
	SetFlightAnalogOrientationMode(FlightAnalogOrientationMode);
	SetMoveActionTraceTeleportOrientationMode(MoveActionTraceTeleportOrientationMode);

	// Cache the navdata
	CacheNavData();

	Super::BeginPlay();
}

void AAsgardVRCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bIsTeleportingToLocation)
	{
		if (CurrentTeleportToLocationMode >= EAsgardTeleportMode::ContinuousRate)
		{
			UpdateContinuousTeleportToLocation(DeltaSeconds);
		}
	}
	else if (bIsTeleportingToRotation)
	{
		if (CurrentTeleportToRotationMode >= EAsgardTeleportMode::ContinuousRate)
		{
			UpdateContinuousTeleportToRotation(DeltaSeconds);
		}
	}
	else
	{
		if (bIsMoveActionTraceTeleportActive)
		{
			UpdateMoveActionTraceTeleport(DeltaSeconds);
		}
		UpdateMovement();
	}

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

void AAsgardVRCharacter::OnTurnRightAxis(float Value)
{
	TurnRightAxis = Value;
	if (bIsTurnRightActionPressed)
	{
		if (TurnRightAxis < TurnActionPressedThreshold)
		{
			OnTurnRightActionReleased();
			if (TurnRightAxis < -TurnActionPressedThreshold)
			{
				OnTurnLeftActionPressed();
			}
		}
	}
	else if (bIsTurnLeftActionPressed)
	{
		if (TurnRightAxis > -TurnActionPressedThreshold)
		{
			OnTurnLeftActionReleased();
			if (TurnRightAxis > TurnActionPressedThreshold)
			{
				OnTurnRightActionPressed();
			}
		}
	}
	else
	{
		if (TurnRightAxis > TurnActionPressedThreshold)
		{
			OnTurnRightActionPressed();
		}
		else if (TurnRightAxis < -TurnActionPressedThreshold)
		{
			OnTurnLeftActionPressed();
		}
	}
}

void AAsgardVRCharacter::OnTurnRightActionPressed()
{
	bIsTurnRightActionPressed = true;
	TeleportTurnRight();
	return;
}

void AAsgardVRCharacter::OnTurnRightActionReleased()
{
	bIsTurnRightActionPressed = false;
	if (TurnActionHeldTimer.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(TurnActionHeldTimer);
	}
	return;
}

void AAsgardVRCharacter::OnTurnLeftActionPressed()
{
	bIsTurnLeftActionPressed = true;
	TeleportTurnLeft();
	return;
}

void AAsgardVRCharacter::OnTurnLeftActionReleased()
{
	bIsTurnLeftActionPressed = false;
	if (TurnActionHeldTimer.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(TurnActionHeldTimer);
	}
	return;
}

void AAsgardVRCharacter::OnFlightThrusterLeftActionPressed()
{
	bIsFlightThrusterLeftActionPressed = true;
	return;
}

void AAsgardVRCharacter::OnFlightThrusterLeftActionReleased()
{
	bIsFlightThrusterLeftActionPressed = false;
	return;
}

void AAsgardVRCharacter::OnFlightThrusterRightActionPressed()
{
	bIsFlightThrusterRightActionPressed = true;
	return;
}

void AAsgardVRCharacter::OnFlightThrusterRightActionReleased()
{
	bIsFlightThrusterRightActionPressed = true;
}

void AAsgardVRCharacter::OnToggleFlightActionPressed()
{
	switch (AsgardVRMovementRef->GetAsgardMovementMode())
	{
		case EAsgardMovementMode::C_MOVE_Walking:
		{
			if (CanFly())
			{
				AsgardVRMovementRef->SetAsgardMovementMode(EAsgardMovementMode::C_MOVE_Flying);
			}
			break;
		}
		case EAsgardMovementMode::C_MOVE_Falling:
		{
			if (CanFly())
			{
				AsgardVRMovementRef->SetAsgardMovementMode(EAsgardMovementMode::C_MOVE_Flying);
			}
			break;
		}
		case EAsgardMovementMode::C_MOVE_Flying:
		{
			AsgardVRMovementRef->SetAsgardMovementMode(EAsgardMovementMode::C_MOVE_Falling);
			break;
		}
		default:
		{
			break;
		}
	}

	return;
}

void AAsgardVRCharacter::OnTraceTeleportActionPressed()
{
	bIsTraceTeleportActionPressed = true;
	if (!bIsMoveActionTraceTeleportActive && CanTeleport())
	{
		StartMoveActionTraceTeleport();
	}
}

void AAsgardVRCharacter::OnTraceTeleportActionReleased()
{
	bIsTraceTeleportActionPressed = false;
	if (bIsMoveActionTraceTeleportActive)
	{
		StopMoveActionTraceTeleport();
	}
}

void AAsgardVRCharacter::SetWalkOrientationMode(const EAsgardWalkOrientationMode NewWalkOrientationMode)
{
	WalkOrientationMode = NewWalkOrientationMode;
	switch (WalkOrientationMode)
	{
		case EAsgardWalkOrientationMode::LeftController:
		{
			WalkOrientationComponent = LeftMotionController;
			break;
		}
		case EAsgardWalkOrientationMode::RightController:
		{
			WalkOrientationComponent = RightMotionController;
			break;
		}
		case EAsgardWalkOrientationMode::VRCamera:
		{
			WalkOrientationComponent = VRReplicatedCamera;
			break;
		}
		case EAsgardWalkOrientationMode::CharacterCapsule:
		{
			WalkOrientationComponent = VRRootReference;
			break;
		}
		default:
		{
			break;
		}
	}

	return;
}

void AAsgardVRCharacter::SetFlightAnalogOrientationMode(const EAsgardFlightAnalogOrientationMode NewFlightOrientationMode)
{
	FlightAnalogOrientationMode = NewFlightOrientationMode;
	switch (FlightAnalogOrientationMode)
	{
		case EAsgardFlightAnalogOrientationMode::LeftController:
		{
			FlightAnalogOrientationComponent = LeftMotionController;
			break;
		}
		case EAsgardFlightAnalogOrientationMode::RightController:
		{
			FlightAnalogOrientationComponent = RightMotionController;
			break;
		}
		case EAsgardFlightAnalogOrientationMode::VRCamera:
		{
			FlightAnalogOrientationComponent = VRReplicatedCamera;
			break;
		}
		default:
		{
			break;
		}
	}

	return;
}

void AAsgardVRCharacter::SetMoveActionTraceTeleportOrientationMode(const EAsgardBinaryHand NewTeleportTraceOrientationMode)
{
	MoveActionTraceTeleportOrientationMode = NewTeleportTraceOrientationMode;
	switch (MoveActionTraceTeleportOrientationMode)
	{
		case EAsgardBinaryHand::LeftHand:
		{
			MoveActionTraceTeleportOrientationComponent = LeftMotionController;
			break;
		}
		case EAsgardBinaryHand::RightHand:
		{
			MoveActionTraceTeleportOrientationComponent = RightMotionController;
			break;
		}
		default:
		{
			break;
		}
	}

	return;
}

bool AAsgardVRCharacter::MoveActionTeleportToLocation(const FVector& GoalLocation)
{
	if (CanTeleport())
	{
		// Cache teleport variables
		TeleportStartLocation = GetVRLocation();
		TeleportStartLocation.Z -= GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
		TeleportGoalLocation = GoalLocation;
		CurrentTeleportToLocationMode = TeleportToLocationMode;

		// Depending on the teleport mode
		switch (CurrentTeleportToLocationMode)
		{
			case EAsgardTeleportMode::Fade:
			{
				bIsTeleportingToLocation = true;
				OnTeleportFadeOutFinished.BindUObject(this, &AAsgardVRCharacter::DeferredTeleportToLocation);
				OnTeleportFadeOutFinished.BindUObject(this, &AAsgardVRCharacter::DeferredTeleportToLocationFinished);
				bTeleportToLocationSucceeded = true;
				TeleportFadeOut();
				break;
			}
			case EAsgardTeleportMode::Instant:
			{
				bTeleportToLocationSucceeded = TeleportTo(GetTeleportLocation(TeleportGoalLocation), GetActorRotation());
				if (!bTeleportToLocationSucceeded)
				{
					UE_LOG(LogAsgardVRCharacter, Error, TEXT("MoveActionTeleportToLocation failed! TeleportTo returned false."));
				}
				break;
			}
			// Continuous cases
			default:
			{
				checkf(ContinuousTeleportToRotationSpeed > 0.0f,
					TEXT("Invalid ContinuousTeleportToLocationSpeed! Must be > 0.0! (AsgardVRCharacter, Value %f, MoveActionTeleportToLocation)"),
					ContinuousTeleportToLocationSpeed);
				if (CurrentTeleportToLocationMode == EAsgardTeleportMode::ContinuousRate)
				{
					TeleportToLocationContinuousMultiplier = 1.0f / ((TeleportGoalLocation - TeleportStartLocation).Size() / ContinuousTeleportToLocationSpeed);
				}
				else
				{
					TeleportToLocationContinuousMultiplier = 1.0f / ContinuousTeleportToLocationSpeed;
				}
				TeleportToLocationContinuousProgress = 0.0f;
				SetActorEnableCollision(false);
				bIsTeleportingToLocation = true;
				bTeleportToLocationSucceeded = true;
				break;
			}
		}

		return bTeleportToLocationSucceeded;
	}

	return false;
}

void AAsgardVRCharacter::MoveActionTeleportToRotation(const FRotator& GoalRotation)
{
	// Cache teleport variables
	CurrentTeleportToRotationMode = TeleportToRotationMode;
	TeleportStartRotation = GetVRRotation();
	TeleportGoalRotation = GoalRotation;

	// Depending on teleport mode
	switch (CurrentTeleportToRotationMode)
	{
		case EAsgardTeleportMode::Fade:
		{
			bIsTeleportingToRotation = true;
			OnTeleportFadeOutFinished.BindUObject(this, &AAsgardVRCharacter::DeferredTeleportToRotation);
			OnTeleportFadeInFinished.BindUObject(this, &AAsgardVRCharacter::DeferredTeleportToRotationFinished);
			TeleportFadeOut();
			break;
		}
		case EAsgardTeleportMode::Instant:
		{
			SetActorRotationVR(TeleportGoalRotation);
			OnTeleportToRotationFinished.Broadcast();
			break;
		}
		// Continuous cases
		default:
		{
			checkf(ContinuousTeleportToRotationSpeed > 0.0f,
				TEXT("Invalid ContinuousTeleportToRotationSpeed! Must be > 0.0! (AsgardVRCharacter, Value %f, MoveActionTeleportToRotation)"),
				ContinuousTeleportToRotationSpeed);
			if (CurrentTeleportToRotationMode == EAsgardTeleportMode::ContinuousRate)
			{
				TeleportToRotationContinuousMultiplier = 1.0f / ((TeleportGoalRotation - TeleportStartRotation).GetNormalized().Yaw / ContinuousTeleportToRotationSpeed);
			}
			else
			{
				TeleportToRotationContinuousMultiplier = 1.0f / ContinuousTeleportToRotationSpeed;
			}
			TeleportToRotationContinuousProgress = 0.0f;
			bIsTeleportingToRotation = true;
			break;
		}
	}

	return;
}

bool AAsgardVRCharacter::MoveActionTeleportToLocationAndRotation(const FVector& GoalLocation, const FRotator& GoalRotation)
{
	if (CanTeleport())
	{
		// Cache teleport variables
		TeleportStartLocation = GetVRLocation();
		TeleportStartLocation.Z -= GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
		TeleportGoalLocation = GoalLocation;
		TeleportGoalRotation = GoalRotation;

		// Use the most comfortable mode
		CurrentTeleportToLocationMode = (TeleportToLocationMode < TeleportToRotationMode? TeleportToLocationMode : TeleportToRotationMode);
		CurrentTeleportToRotationMode = CurrentTeleportToLocationMode;

		// Depending on the teleport mode
		switch (CurrentTeleportToLocationMode)
		{
			case EAsgardTeleportMode::Fade:
			{
				TeleportStartRotation = GetVRRotation();
				bIsTeleportingToLocation = true;
				bIsTeleportingToRotation = true;
				OnTeleportFadeOutFinished.BindUObject(this, &AAsgardVRCharacter::DeferredTeleportToLocationAndRotation);
				OnTeleportFadeInFinished.BindUObject(this, &AAsgardVRCharacter::DeferredTeleportToLocationAndRotationFinished);
				bTeleportToLocationSucceeded = true;
				TeleportFadeOut();
				break;
			}
			case EAsgardTeleportMode::Instant:
			{
				TeleportStartRotation = GetVRRotation();
				bTeleportToLocationSucceeded = TeleportTo(GetTeleportLocation(TeleportGoalLocation), GetActorRotation());
				if (!bTeleportToLocationSucceeded)
				{
					UE_LOG(LogAsgardVRCharacter, Error, TEXT("MoveActionTeleportToLocationAndRotation failed! TeleportTo returned false."));
				}
				SetActorRotationVR(TeleportGoalRotation);
				break;
			}
			// Continuous cases
			default:
			{
				checkf(ContinuousTeleportToRotationSpeed > 0.0f,
					TEXT("Invalid ContinuousTeleportToLocationSpeed! Must be > 0.0! (AsgardVRCharacter, Value %f, MoveActionTeleportToLocationAndRotation)"),
					ContinuousTeleportToLocationSpeed);
				checkf(ContinuousTeleportToRotationSpeed > 0.0f,
					TEXT("Invalid ContinuousTeleportToRotationSpeed! Must be > 0.0! (AsgardVRCharacter, Value %f, MoveActionTeleportToLocationAndRotation)"),
					ContinuousTeleportToRotationSpeed);
				if (CurrentTeleportToLocationMode == EAsgardTeleportMode::ContinuousRate)
				{
					TeleportToLocationContinuousMultiplier = 1.0f / ((TeleportGoalLocation - TeleportStartLocation).Size() / ContinuousTeleportToLocationSpeed);
					TeleportToRotationContinuousMultiplier = 1.0f / ((TeleportGoalRotation - TeleportStartRotation).GetNormalized().Yaw / ContinuousTeleportToRotationSpeed);
				}
				else
				{
					TeleportToLocationContinuousMultiplier = 1.0f / ContinuousTeleportToLocationSpeed;
					TeleportToRotationContinuousMultiplier = 1.0f / ContinuousTeleportToRotationSpeed;
				}
				TeleportToLocationContinuousProgress = 0.0f;
				TeleportToRotationContinuousProgress = 0.0f;
				SetActorEnableCollision(false);
				bIsTeleportingToLocation = true;
				bIsTeleportingToRotation = true;
				bTeleportToLocationSucceeded = true;
				break;
			}
		}

		return bTeleportToLocationSucceeded;
	}
	return false;
}

void AAsgardVRCharacter::TeleportFadeOut_Implementation()
{
	if (TeleportFadeOutTime > 0.0f)
	{
		FTimerHandle TempTimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TempTimerHandle, this, &AAsgardVRCharacter::TeleportFadeOutFinished, TeleportFadeOutTime);
	}
	else
	{
		TeleportFadeOutFinished();
	}
	return;
}

void AAsgardVRCharacter::TeleportFadeOutFinished()
{
	if (OnTeleportFadeOutFinished.IsBound())
	{
		OnTeleportFadeOutFinished.Execute();
		OnTeleportFadeOutFinished.Unbind();
	}
	TeleportFadeIn();
	return;
}

void AAsgardVRCharacter::TeleportFadeIn_Implementation()
{
	if (TeleportFadeInTime > 0.0f)
	{
		FTimerHandle TempTimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TempTimerHandle, this, &AAsgardVRCharacter::TeleportFadeInFinished, TeleportFadeOutTime);
	}
	else
	{
		TeleportFadeInFinished();
	}
	return;
}


void AAsgardVRCharacter::TeleportFadeInFinished()
{
	if (OnTeleportFadeInFinished.IsBound())
	{
		OnTeleportFadeInFinished.Execute();
		OnTeleportFadeInFinished.Unbind();
	}
	return;
}

void AAsgardVRCharacter::DeferredTeleportToLocation()
{
	if (!TeleportTo(GetTeleportLocation(TeleportGoalLocation), GetActorRotation()))
	{
		UE_LOG(LogAsgardVRCharacter, Error, TEXT("DeferredTeleportToLocation failed! TeleportTo returned false."));
		bTeleportToLocationSucceeded = false;
	}
	return;
}

void AAsgardVRCharacter::DeferredTeleportToLocationFinished()
{
	bIsTeleportingToLocation = false;
	return;
}

void AAsgardVRCharacter::DeferredTeleportToRotation()
{
	SetActorRotationVR(TeleportGoalRotation);
	return;
}

void AAsgardVRCharacter::DeferredTeleportToRotationFinished()
{
	bIsTeleportingToRotation = false;
	OnTeleportToRotationFinished.Broadcast();
	return;
}

void AAsgardVRCharacter::DeferredTeleportToLocationAndRotation()
{
	if (!TeleportTo(GetTeleportLocation(TeleportGoalLocation), GetActorRotation()))
	{
		UE_LOG(LogAsgardVRCharacter, Error, TEXT("DeferredTeleportToLocationAndRotation failed! TeleportTo returned false."));
		bTeleportToLocationSucceeded = false;
	}
	SetActorRotationVR(TeleportGoalRotation);
	return;
}

void AAsgardVRCharacter::DeferredTeleportToLocationAndRotationFinished()
{
	bIsTeleportingToLocation = false;
	bIsTeleportingToRotation = false;
	OnTeleportToRotationFinished.Broadcast();
	return;
}

void AAsgardVRCharacter::DeferredTeleportTurnRightFinished()
{
	OnTeleportToRotationFinished.Remove(TeleportTurnFinishedDelegateHandle);
	if (bIsTurnRightActionPressed)
	{
		GetWorld()->GetTimerManager().SetTimer(TurnActionHeldTimer, this, &AAsgardVRCharacter::TeleportTurnRight, TeleportTurnHeldInterval);
	}
	return;
}

void AAsgardVRCharacter::DeferredTeleportTurnLeftFinished()
{
}

void AAsgardVRCharacter::UpdateContinuousTeleportToLocation(float DeltaSeconds)
{
	// Profile this function since it is a potentially expensive operation
	SCOPE_CYCLE_COUNTER(STAT_ASGARD_VRCharacterContinuousTeleportToLocation);

	// Progress interpolation
	TeleportToLocationContinuousProgress = FMath::Min
	(TeleportToLocationContinuousProgress + (TeleportToLocationContinuousMultiplier * DeltaSeconds), 
		1.0f);

	// Continue interpolation
	if (TeleportToLocationContinuousProgress < 1.0f)
	{

		SetActorLocation(FMath::Lerp(
			GetTeleportLocation(TeleportStartLocation),
			GetTeleportLocation(TeleportGoalLocation),
			TeleportToLocationContinuousProgress));
	}

	// Finish interpolation
	else
	{
		SetActorLocation(GetTeleportLocation(TeleportGoalLocation));
		SetActorEnableCollision(true);

		// Update variables for continuous teleport rotation if appropriate
		if (bIsTeleportingToRotation && TeleportToRotationMode >= EAsgardTeleportMode::ContinuousRate)
		{
			TeleportStartRotation = GetVRRotation();
		}

		bIsTeleportingToLocation = false;
	}

	return;
}

void AAsgardVRCharacter::UpdateContinuousTeleportToRotation(float DeltaSeconds)
{
	// Profile this function since it is a potentially expensive operation
	SCOPE_CYCLE_COUNTER(STAT_ASGARD_VRCharacterContinuousTeleportToRotation);

	// Progress interpolation
	TeleportToRotationContinuousProgress = FMath::Min
	(TeleportToRotationContinuousProgress + (TeleportToRotationContinuousMultiplier * DeltaSeconds),
		1.0f);

	// Continue interpolation
	if (TeleportToLocationContinuousProgress < 1.0f)
	{
		SetActorRotationVR(FMath::Lerp(
			TeleportStartRotation, 
			TeleportGoalRotation, 
			TeleportToRotationContinuousProgress));
	}

	// Finish interpolation
	else
	{
		SetActorRotationVR(TeleportGoalRotation);
		bIsTeleportingToRotation = false;
		OnTeleportToRotationFinished.Broadcast();
	}
}

bool AAsgardVRCharacter::TraceTeleportLocation(
	const FVector& TraceOrigin, 
	const FVector& TraceVelocity,
	TEnumAsByte<ECollisionChannel> TraceChannel,
	FVector& OutTeleportLocation,
	FVector& OutImpactPoint,
	TArray<FVector>& OutTracePathPoints)
{
	// Profile this function since it is a potentially expensive operation
	SCOPE_CYCLE_COUNTER(STAT_ASGARD_VRCharacterTraceTeleportLocation);

	// Initialize trace parameters
	FPredictProjectilePathParams TeleportTraceParams = FPredictProjectilePathParams(
		TeleportTraceRadius, 
		TraceOrigin, 
		TraceVelocity, 
		TeleportTraceMaxSimTime, 
		TraceChannel,
		this);
	TELEPORT_TRACE_DEBUG_TYPE(TeleportTraceParams);

	// Perform the trace
	FPredictProjectilePathResult TraceResult;
	bool bTraceHit = UGameplayStatics::PredictProjectilePath(this, TeleportTraceParams, TraceResult);
	
	// Update path points
	OutTracePathPoints.Empty();
	for (const FPredictProjectilePathPointData& PathPoint : TraceResult.PathData)
	{
		OutTracePathPoints.Add(PathPoint.Location);
	}

	// Return result
	if (bTraceHit)
	{
		OutImpactPoint = TraceResult.HitResult.ImpactPoint;
		return ProjectPointToVRNavigation(OutImpactPoint, OutTeleportLocation, true);
	}

	return false;
}

void AAsgardVRCharacter::TeleportTurnRight()
{
	// Attempt to turn right
	if (CanTeleport())
	{
		// Set up the next call of this function if appropriate
		if (TeleportTurnHeldInterval > 0.0f)
		{
			// In instant mode, use a simple timer
			if (TeleportToRotationMode == EAsgardTeleportMode::Instant)
			{
				GetWorld()->GetTimerManager().SetTimer(TurnActionHeldTimer, this, &AAsgardVRCharacter::TeleportTurnRight, TeleportTurnHeldInterval);
			}

			// Otherwise bind to teleport completion
			else
			{
				TeleportTurnFinishedDelegateHandle = OnTeleportToRotationFinished.AddUObject(this, &AAsgardVRCharacter::DeferredTeleportTurnRightFinished);
			}
		}
		MoveActionTeleportToRotation(GetVRRotation() + FRotator(0.0f, TeleportTurnAngleIntervalDegrees, 0.0f));
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(TurnActionHeldTimer, this, &AAsgardVRCharacter::TeleportTurnRight, TeleportTurnHeldInterval);
	}

	return;
}

void AAsgardVRCharacter::TeleportTurnLeft()
{
	// Attempt to turn Left
	if (CanTeleport())
	{
		// Set up the next call of this function if appropriate
		if (TeleportTurnHeldInterval > 0.0f)
		{
			// In instant mode, use a simple timer
			if (TeleportToRotationMode == EAsgardTeleportMode::Instant)
			{
				GetWorld()->GetTimerManager().SetTimer(TurnActionHeldTimer, this, &AAsgardVRCharacter::TeleportTurnLeft, TeleportTurnHeldInterval);
			}

			// Otherwise bind to teleport completion
			else
			{
				TeleportTurnFinishedDelegateHandle = OnTeleportToRotationFinished.AddUObject(this, &AAsgardVRCharacter::DeferredTeleportTurnLeftFinished);
			}
		}
		MoveActionTeleportToRotation(GetVRRotation() + FRotator(0.0f, -TeleportTurnAngleIntervalDegrees, 0.0f));
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(TurnActionHeldTimer, this, &AAsgardVRCharacter::TeleportTurnLeft, TeleportTurnHeldInterval);
	}

	return;
}

void AAsgardVRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Action bindings
	PlayerInputComponent->BindAction(UAsgardInputBindings::FlightThrusterLeftAction(), IE_Pressed, this,&AAsgardVRCharacter::OnFlightThrusterLeftActionPressed);
	PlayerInputComponent->BindAction(UAsgardInputBindings::FlightThrusterLeftAction(), IE_Released, this, &AAsgardVRCharacter::OnFlightThrusterLeftActionReleased);
	PlayerInputComponent->BindAction(UAsgardInputBindings::FlightThrusterRightAction(), IE_Pressed, this, &AAsgardVRCharacter::OnFlightThrusterRightActionPressed);
	PlayerInputComponent->BindAction(UAsgardInputBindings::FlightThrusterRightAction(), IE_Released, this, &AAsgardVRCharacter::OnFlightThrusterRightActionReleased);
	PlayerInputComponent->BindAction(UAsgardInputBindings::ToggleFlightAction(), IE_Pressed, this, &AAsgardVRCharacter::OnToggleFlightActionPressed);
	PlayerInputComponent->BindAction(UAsgardInputBindings::TraceTeleportAction(), IE_Pressed, this, &AAsgardVRCharacter::OnTraceTeleportActionPressed);
	PlayerInputComponent->BindAction(UAsgardInputBindings::TraceTeleportAction(), IE_Released, this, &AAsgardVRCharacter::OnTraceTeleportActionReleased);

	// Axis bindings
	PlayerInputComponent->BindAxis(UAsgardInputBindings::MoveForwardAxis(), this, &AAsgardVRCharacter::OnMoveForwardAxis);
	PlayerInputComponent->BindAxis(UAsgardInputBindings::MoveRightAxis(), this, &AAsgardVRCharacter::OnMoveRightAxis);
	PlayerInputComponent->BindAxis(UAsgardInputBindings::TurnRightAxis(), this, &AAsgardVRCharacter::OnTurnRightAxis);

	return;
}

bool AAsgardVRCharacter::CanTeleport_Implementation()
{
	return bTeleportEnabled && !bIsTeleportingToLocation;
}

bool AAsgardVRCharacter::CanWalk_Implementation()
{
	return bWalkEnabled;
}

bool AAsgardVRCharacter::CanFly_Implementation()
{
	return bFlightEnabled && (bIsFlightAnalogMovementEnabled || bFlightControllerThrustersEnabled);
}

bool AAsgardVRCharacter::ProjectCharacterToVRNavigation(float MaxHeight, FVector& OutProjectedPoint)
{
	// Perform a trace from the bottom of the capsule collider towards the ground
	FVector TraceOrigin = GetCapsuleComponent()->GetComponentLocation();
	FVector TraceEnd = TraceOrigin;
	TraceEnd.Z -= (MaxHeight + GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight());
	FHitResult TraceHitResult;
	FCollisionQueryParams TraceParams(FName(TEXT("AsgardVRCharacterAboveNavMeshTrace")), false, this);
	if (GetWorld()->LineTraceSingleByProfile(TraceHitResult, TraceOrigin, TraceEnd, UAsgardCollisionProfiles::VRRoot(), TraceParams))
	{
		// If we hit an object, check to see if the point can be projected onto a navmesh;
		return ProjectPointToVRNavigation(TraceHitResult.ImpactPoint, OutProjectedPoint, false);
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

		// If point projected successfully and we want to check the point is on the ground
		if (bRetVal && bCheckIfIsOnGround)
		{
			// Trace downwards and see if we hit something
			FHitResult GroundTraceHitResult;
			const FVector GroundTraceOrigin = OutProjectedPoint;
			const FVector GroundTraceEnd = GroundTraceOrigin + FVector(0.0f, 0.0f, -200.0f);
			FCollisionQueryParams GroundTraceParams(FName(TEXT("VRCharacterGroundTrace")), false, this);
			if (GetWorld()->LineTraceSingleByProfile(GroundTraceHitResult, GroundTraceOrigin, GroundTraceEnd, UAsgardCollisionProfiles::VRRoot(), GroundTraceParams))
			{
				// If so, return the point of impact
				OutProjectedPoint = GroundTraceHitResult.ImpactPoint;
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
	switch (AsgardVRMovementRef->GetAsgardMovementMode())
	{
		case EAsgardMovementMode::C_MOVE_Walking:
		{
			// If we can walk
			if (CanWalk())
			{
				// Get the input vector depending on the walking weight mode
				FVector WalkInput = FVector::ZeroVector;
				if (WalkInputAxisWeightMode == EAsgardInputAxisWeightMode::Circular)
				{
					WalkInput = CalculateCircularInputVector(
						MoveForwardAxis,
						MoveRightAxis,
						WalkInputDeadZone,
						WalkInputMaxZone);
				}
				else
				{
					WalkInput = CalculateCrossInputVector(
						MoveForwardAxis,
						MoveRightAxis,
						WalkInputDeadZone,
						WalkInputMaxZone);
				}

				// Guard against no input
				if (WalkInput.IsNearlyZero())
				{
					StopWalk();
					break;
				}

				// Transform the input vector from the forward of the orientation component
				FVector WalkForward = WalkOrientationComponent->GetForwardVector();

				// Guard against the forward vector being nearly up or down
				if (FMath::Abs(FVector::UpVector | WalkForward) > (WalkOrientationMaxAbsPitchPiRadians))
				{ 
					StopWalk();
					break;
				}

				// Flatten the input vector
				WalkForward.Z = 0.0f;
				WalkForward = WalkForward.GetSafeNormal();
				FVector WalkRight = FVector::CrossProduct(FVector::UpVector, WalkForward);

				// Scale the input vector by the forward, backward, and strafing multipliers
				WalkInput *= FVector(WalkInput.X > 0.0f ? WalkInputForwardMultiplier : WalkInputBackwardMultiplier, WalkInputStrafeMultiplier, 0.0f);
				WalkInput = (WalkInput.X * WalkForward) + (WalkInput.Y * WalkRight);

				// Guard against the input vector being nullified
				if (FMath::IsNearlyZero(WalkInput.SizeSquared()))
				{
					StopWalk();
					break;
				}

				// Apply the movement input
				StartWalk();
				AsgardVRMovementRef->AddInputVector(WalkInput);
			}

			// Else, check we are not in a walking state
			else
			{
				StopWalk();
			}

			break;
		}
		case EAsgardMovementMode::C_MOVE_Flying:
		{
			// If we can fly
			if (CanFly())
			{
				// Cache initial variables
				bool bShouldApplyInput = false;
				FVector FlightInput = FVector::ZeroVector;

				// Analog stick input
				if (bIsFlightAnalogMovementEnabled)
				{
					// Calculate the Flight input vector
					if (FlightAnalogAxisWeightMode == EAsgardInputAxisWeightMode::Circular)
					{
						FlightInput = CalculateCircularInputVector(
						MoveForwardAxis,
						MoveRightAxis,
						FlightAnalogInputDeadZone,
						FlightAnalogInputMaxZone);
					}
					else
					{
						FlightInput = CalculateCrossInputVector(
						MoveForwardAxis,
						MoveRightAxis,
						FlightAnalogInputDeadZone,
						FlightAnalogInputMaxZone);
					}

					// Guard against no input
					if (!FlightInput.IsNearlyZero())
					{
						bShouldApplyInput = true;

						// Get the forward and right Flight vectors
						FVector FlightForward = FlightAnalogOrientationComponent->GetForwardVector();
						FVector FlightRight = FlightAnalogOrientationComponent->GetRightVector();

						// Scale the input vector by the forward, backward, and strafing multipliers
						FlightInput *= FVector(FlightInput.X > 0.0f ? FlightAnalogInputForwardMultiplier : FlightAnalogInputBackwardMultiplier, FlightAnalogInputStrafeMultiplier, 0.0f);
						FlightInput = (FlightInput.X * FlightForward) + (FlightInput.Y * FlightRight);
					}
				}

				// Thruster input
				if (bFlightControllerThrustersEnabled)
				{
					FVector ProjectedPoint;

					// Add thrust from the left and right hand thrusters as appropriate
					if (bIsFlightThrusterLeftActive)
					{
						bShouldApplyInput = true;
						FlightInput += LeftMotionController->GetForwardVector() * FlightControllerThrusterInputMultiplier;
					}
					if (bIsFlightThrusterRightActive)
					{
						bShouldApplyInput = true;
						FlightInput += RightMotionController->GetForwardVector() * FlightControllerThrusterInputMultiplier;
					}
				}

				// Apply input if appropriate
				if (bShouldApplyInput)
				{
					VRMovementReference->AddInputVector(FlightInput);

					// If we are flying towards the ground
					if ((FVector::DownVector | FlightInput.GetSafeNormal()) <= FlightAutoLandingMaxAnglePiRadians
						&& (!bFlightAutoLandingRequiresDownwardVelocity 
							|| (FVector::DownVector | FlightInput.GetSafeNormal()) >= 1.0f - FlightAutoLandingMaxAnglePiRadians))
					{
						// If we are close to the ground
						FVector ProjectedPoint;
						if (ProjectCharacterToVRNavigation(FlightAutoLandingMaxHeight, ProjectedPoint))
						{
							// Stop flying
							AsgardVRMovementRef->SetAsgardMovementMode(EAsgardMovementMode::C_MOVE_Falling);
						}
					}
				}
			}

			// Otherwise, change movement mode to walking
			else
			{
				AsgardVRMovementRef->SetAsgardMovementMode(EAsgardMovementMode::C_MOVE_Falling);
			}

			break;
		}

		default:
		{
			break;
		}
	}

	return;
}

void AAsgardVRCharacter::UpdateMoveActionTraceTeleport(float DeltaSeconds)
{
	if (CanTeleport())
	{
		// Updating trace direction
		// If interpolated
		if (MoveActionTraceTeleportDirectionMaxAnglePiRadians > 0.0f)
		{
			// Get the difference angle
			FVector NewTraceTeleportDirection = MoveActionTraceTeleportOrientationComponent->GetForwardVector();
			float DifferenceAngle = 1.0f - FVector::DotProduct(NewTraceTeleportDirection, MoveActionTraceTeleportDirection);
			
			// If the difference is significant
			if (DifferenceAngle > SMALL_NUMBER)
			{
				// Interpolate new difference angle
				DifferenceAngle = FMath::FInterpTo(DifferenceAngle, 0.0f, DeltaSeconds, MoveActionTraceTeleportDirectionInterpSpeedPiRadians);
				
				// Cap the new difference angle
				if (DifferenceAngle > MoveActionTraceTeleportDirectionMaxAnglePiRadians)
				{
					DifferenceAngle = MoveActionTraceTeleportDirectionMaxAnglePiRadians;
				}

				// Rotate direction by the new difference angle along the difference axis
				NewTraceTeleportDirection.RotateAngleAxis(DifferenceAngle,FVector::CrossProduct(NewTraceTeleportDirection, MoveActionTraceTeleportDirection).GetSafeNormal());
			}

			// Update the direction
			MoveActionTraceTeleportDirection = NewTraceTeleportDirection;
		}

		// Else if instant
		else
		{
			MoveActionTraceTeleportDirection = MoveActionTraceTeleportOrientationComponent->GetForwardVector();
		}

		// Perform the teleport trace
		bIsMoveActionTraceTeleportLocationValid = TraceTeleportLocation(
			MoveActionTraceTeleportOrientationComponent->GetComponentLocation(),
			MoveActionTraceTeleportDirection * MoveActionTraceTeleportVelocityMagnitude,
			MoveActionTraceTeleportChannel,
			MoveActionTraceTeleportLocation,
			MoveActionTraceTeleportImpactPoint,
			MoveActionTraceTeleportTracePath);

		return;
	}
	else
	{
		StopMoveActionTraceTeleport();
	}
}

void AAsgardVRCharacter::StartMoveActionTraceTeleport()
{
	bIsMoveActionTraceTeleportLocationValid = false;
	MoveActionTraceTeleportDirection = MoveActionTraceTeleportOrientationComponent->GetForwardVector();
	bIsMoveActionTraceTeleportActive = true;
	
	return;
}

void AAsgardVRCharacter::StopMoveActionTraceTeleport()
{
	bIsMoveActionTraceTeleportActive = false;
	if (bIsMoveActionTraceTeleportLocationValid)
	{
		MoveActionTeleportToLocation(MoveActionTraceTeleportLocation);
	}

	return;
}

const FVector AAsgardVRCharacter::CalculateCircularInputVector(float AxisX, float AxisY, float DeadZone, float MaxZone) const
{
	checkf(DeadZone < 1.0f && DeadZone >= 0.0f, 
		TEXT("Invalid Dead Zone for calculating an input vector! (AsgardVRCharacter, Value %f, CalculateCircularInputVector)"), 
		DeadZone);

	checkf(MaxZone <= 1.0f && MaxZone >= 0.0f,
		TEXT("Invalid Max Zone for calculating an input vector! (AsgardVRCharacter, Value %f, CalculateCircularInputVector)"),
		MaxZone);

	checkf(MaxZone > DeadZone,
		TEXT("Max Zone must be greater than Dead Zone for calculating an input vector! (AsgardVRCharacter, Max Zone %f, Dead Zone %f, CalculateCircularInputVector)"),
		MaxZone, DeadZone);

	// Cache the size of the input vector
	FVector AdjustedInputVector = FVector(AxisX, AxisY, 0.0f);
	float InputVectorLength = AdjustedInputVector.SizeSquared();

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
	checkf(DeadZone < 1.0f && DeadZone >= 0.0f,
		TEXT("Invalid Dead Zone for calculating an input vector! (AsgardVRCharacter, Value %f, CalculateCrossInputVector)"),
		DeadZone);

	checkf(MaxZone <= 1.0f && MaxZone >= 0.0f,
		TEXT("Invalid Max Zone for calculating an input vector! (AsgardVRCharacter, Value %f, CalculateCrossInputVector)"),
		MaxZone);

	checkf(MaxZone > DeadZone,
		TEXT("Max Zone must be greater than Dead Zone an input vector! (AsgardVRCharacter, Max Zone %f, Dead Zone %f, CalculateCrossInputVector)"),
		MaxZone, DeadZone);

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

void AAsgardVRCharacter::StartWalk()
{
	if (!bIsWalk)
	{
		bIsWalk = true;
		OnWalkStarted();
	}
	return;
}

void AAsgardVRCharacter::StopWalk()
{
	if (bIsWalk)
	{
		bIsWalk = false;
		OnWalkStopped();
	}

	return;
}

