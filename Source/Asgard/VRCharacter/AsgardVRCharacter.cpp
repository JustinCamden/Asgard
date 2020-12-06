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
#include "Runtime/Engine/Classes/Kismet/KismetMathLibrary.h"

// Log category
DEFINE_LOG_CATEGORY_STATIC(LogAsgardVRCharacter, Log, All);

// Stat cycles
DECLARE_CYCLE_STAT(TEXT("AsgardVRCharacter PrecisionTeleportLocation"), STAT_ASGARD_VRCharacterPrecisionTeleportLocation, STATGROUP_ASGARD_VRCharacter);
DECLARE_CYCLE_STAT(TEXT("AsgardVRCharacter SmoothTeleportToLocation"), STAT_ASGARD_VRCharacterSmoothTeleportToLocation, STATGROUP_ASGARD_VRCharacter);
DECLARE_CYCLE_STAT(TEXT("AsgardVRCharacter SmoothTeleportToRotation"), STAT_ASGARD_VRCharacterSmoothTeleportToRotation, STATGROUP_ASGARD_VRCharacter);

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
#include "DrawDebugHelpers.h"
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
	PathfindingNavAgentProperties = GetCharacterMovement()->NavAgentProps;

	// Teleport settings
	bTeleportEnabled = true;
	TeleportTraceRadius = 4.0f;
	TeleportTraceMaxSimTime = 3.0f;
	PrecisionTeleportTraceMagnitude = 1000.0f;
	PrecisionTeleportDedicatedOrientationMode = EAsgardBinaryHand::RightHand;
	PrecisionTeleportInferred1OrientationMode = EAsgardBinaryHand::RightHand;
	PrecisionTeleportInferred2OrientationMode = EAsgardBinaryHand::LeftHand;
	PrecisionTeleportTraceChannel = UAsgardTraceChannels::VRTeleportTrace;
	PrecisionTeleportTraceDirectionMaxPitch = 45.0f;
	TeleportTurnAngleInterval = 45.0f;
	AxisPressedMinAbsValue = 0.5f;
	AxisPressedMaxAngle = 20.0f;
	TeleportTurnHeldInterval = 0.25f;
	TeleportFadeOutTime = 0.15f;
	TeleportFadeInTime = 0.15f;
	TeleportWalkTraceChannel = UAsgardTraceChannels::VRTeleportTrace;
	PrecisionTeleportTraceDirectionMaxErrorAngle = 10.0f;
	PrecisionTeleportTraceDirectionAngleLerpSpeed = 20.0f;
	TeleportWalkMagnitude = 750.0f;
	SmoothTeleportToLocationSpeed = 0.1f;
	SmoothTeleportToRotationSpeed = 0.1f;
	TeleportWalkMaxFallbackTraces = 2;
	TeleportWalkFallbackTraceMagnitudeInterval = 250.0f;
	TeleportWalkHeldInterval = 0.5f;

	// Walk Settings
	bWalkEnabled = true;
	SmoothWalkInputDeadZone = 0.15f;
	SmoothWalkInputMaxZone = 0.9f;
	SmoothWalkInputForwardMultiplier = 1.0f;
	SmoothWalkInputStrafeMultiplier = 1.0f;
	SmoothWalkInputBackwardMultiplier = 1.0f;
	WalkForwardMaxAbsPitch = 85.0f;

	// Flight settings
	bFlightEnabled = true;
	bFlightAutoLandingEnabled = true;
	FlightAutoLandingMaxHeight = 50.0f;
	FlightAutoLandingMinPitch = 0.75f;
	bIsSmoothFlightMovementEnabled = true;
	SmoothFlightInputDeadZone = 0.15f;
	SmoothFlightInputMaxZone = 0.9f;
	SmoothFlightInputForwardMultiplier = 1.0f;
	SmoothFlightInputStrafeMultiplier = 1.0f;
	SmoothFlightInputBackwardMultiplier = 1.0f;
	bShouldFlightControllerThrustersAutoStartFlight = true;
	FlightControllerThrusterInputMultiplier = 1.0f;

	// Movement state
	bIsSmoothWalking = false;

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
	SetSmoothFlightOrientationMode(SmoothFlightOrientationMode);

	// Cache the navdata
	CacheNavData();

	Super::BeginPlay();
}

void AAsgardVRCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bIsTeleportingToLocation)
	{
		if (CurrentTeleportToLocationMode >= EAsgardTeleportMode::SmoothRate)
		{
			UpdateSmoothTeleportToLocation(DeltaSeconds);
		}
	}
	else if (bIsTeleportingToRotation)
	{
		if (CurrentTeleportToRotationMode >= EAsgardTeleportMode::SmoothRate)
		{
			UpdateSmoothTeleportToRotation(DeltaSeconds);
		}
	}
	else
	{
		if (bIsPrecisionTeleportActive)
		{
			UpdatePrecisionTeleport(DeltaSeconds);
		}
		switch (AsgardVRMovementRef->GetAsgardMovementMode())
		{
			case EAsgardMovementMode::C_MOVE_Walking:
			{
				UpdateSmoothWalk();
				break;
			}
			case EAsgardMovementMode::C_MOVE_NavWalking:
			{
				UpdateSmoothWalk();
				break;
			}
			case EAsgardMovementMode::C_MOVE_Flying:
			{
				UpdateSmoothFlight();
				break;
			}
			default:
			{
				break;
			}
		}
	}

	return;
}

void AAsgardVRCharacter::OnMoveForwardAxis(float Value)
{
	MoveForwardAxis = Value;
	SetMoveInputVector(FVector(MoveForwardAxis, MoveRightAxis, 0.0f));

	return;
}

void AAsgardVRCharacter::OnMoveRightAxis(float Value)
{
	MoveRightAxis = Value;
	SetMoveInputVector(FVector(MoveForwardAxis, MoveRightAxis, 0.0f));

	return;
}

void AAsgardVRCharacter::OnTurnForwardAxis(float Value)
{
	TurnForwardAxis = Value;
	SetTurnInputVector(FVector(TurnForwardAxis, TurnRightAxis, 0.0f));

	return;
}

void AAsgardVRCharacter::OnTurnRightAxis(float Value)
{
	TurnRightAxis = Value;
	SetTurnInputVector(FVector(TurnForwardAxis, TurnRightAxis, 0.0f));

	return;
}

void AAsgardVRCharacter::OnMoveActionPressed()
{
	bIsMoveActionPressed = true;

	if (DefaultGroundMovementMode == EAsgardGroundMovementMode::PrecisionTeleportToLocation 
		&& CanTeleport() )
	{
		// Set input source and orientation component
		PrecisionTeleportInputSource = EAsgardInputSource::Inferred2;
		SetPrecisionTeleportOrientationComponent(PrecisionTeleportInferred2OrientationMode);
		StartPrecisionTeleport();
	}

	return;
}

void AAsgardVRCharacter::OnMoveActionReleased()
{
	bIsMoveActionPressed = false;
	if (bIsPrecisionTeleportActive && PrecisionTeleportInputSource == EAsgardInputSource::Inferred2)
	{
		StopPrecisionTeleport();
	}

	return;
}

void AAsgardVRCharacter::OnTurnActionPressed()
{
	bIsTurnActionPressed = true;
	return;
}

void AAsgardVRCharacter::OnTurnActionReleased()
{
	bIsTurnActionPressed = false;

	// Cancel precision teleport if appropriate
	if (bIsPrecisionTeleportActive && PrecisionTeleportInputSource == EAsgardInputSource::Inferred1)
	{
		StopPrecisionTeleport();
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

void AAsgardVRCharacter::OnPrecisionTeleportActionPressed()
{
	bIsPrecisionTeleportActionPressed = true;

	if (CanTeleport())
	{
		// Set input source and orientation component
		PrecisionTeleportInputSource = EAsgardInputSource::Dedicated;
		SetPrecisionTeleportOrientationComponent(PrecisionTeleportDedicatedOrientationMode);
		StartPrecisionTeleport();
	}

	return;
}

void AAsgardVRCharacter::OnPrecisionTeleportActionReleased()
{
	bIsPrecisionTeleportActionPressed = false;
	if (bIsPrecisionTeleportActive && PrecisionTeleportInputSource == EAsgardInputSource::Dedicated)
	{
		StopPrecisionTeleport();
	}

	return;
}


void AAsgardVRCharacter::SetPrecisionTeleportOrientationComponent(EAsgardBinaryHand NewPrecisionTeleportOrientationMode)
{
	PrecisionTeleportOrientationMode = NewPrecisionTeleportOrientationMode;
	if (NewPrecisionTeleportOrientationMode == EAsgardBinaryHand::LeftHand)
	{
		PrecisionTeleportOrientationComponent = LeftMotionController;
	}
	else
	{
		PrecisionTeleportOrientationComponent = RightMotionController;
	}

	return;
}

void AAsgardVRCharacter::SetWalkOrientationMode(EAsgardOrientationMode NewWalkOrientationMode)
{
	WalkOrientationMode = NewWalkOrientationMode;
	switch (NewWalkOrientationMode)
	{
		case EAsgardOrientationMode::Character:
		{
			WalkOrientationComponent = nullptr;
			break;
		}
		case EAsgardOrientationMode::LeftController:
		{
			WalkOrientationComponent = LeftMotionController;
			break;
		}
		case EAsgardOrientationMode::RightController:
		{
			WalkOrientationComponent = RightMotionController;
			break;
		}
		default:
		{
			break;
		}
	}

	return;
}


void AAsgardVRCharacter::SetSmoothFlightOrientationMode(EAsgardFlightOrientationMode NewFlightOrientationMode)
{
	SmoothFlightOrientationMode = NewFlightOrientationMode;
	switch (SmoothFlightOrientationMode)
	{
		case EAsgardFlightOrientationMode::VRCamera:
		{
			SmoothFlightOrientationComponent = VRReplicatedCamera;
			break;
		}
		case EAsgardFlightOrientationMode::LeftController:
		{
			SmoothFlightOrientationComponent = LeftMotionController;
			break;
		}
		case EAsgardFlightOrientationMode::RightController:
		{
			SmoothFlightOrientationComponent = RightMotionController;
			break;
		}
		default:
		{	
			break;
		}
	}

	return;
}

bool AAsgardVRCharacter::TeleportToLocation(const FVector& GoalLocation, EAsgardTeleportMode TeleportMode)
{
	// Cache teleport variables
	TeleportStartLocation = GetVRLocation();
	TeleportStartLocation.Z -= GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	TeleportGoalLocation = GoalLocation;
	CurrentTeleportToLocationMode = TeleportMode;

	// Depending on the teleport mode
	switch (CurrentTeleportToLocationMode)
	{
		case EAsgardTeleportMode::Fade:
		{
			bIsTeleportingToLocation = true;
			OnTeleportFadeOutFinished.BindUObject(this, &AAsgardVRCharacter::DeferredTeleportToLocation);
			OnTeleportFadeInFinished.BindUObject(this, &AAsgardVRCharacter::DeferredTeleportToLocationFinished);
			bTeleportToLocationSucceeded = true;
			TeleportFadeOut();
			break;
		}
		case EAsgardTeleportMode::Instant:
		{
			bTeleportToLocationSucceeded = TeleportTo(GetTeleportLocation(TeleportGoalLocation), GetActorRotation());
			if (!bTeleportToLocationSucceeded)
			{
				UE_LOG(LogAsgardVRCharacter, Error, TEXT("TeleportToLocation failed! TeleportTo returned false."));
			}
			OnTeleportToLocationFinished.Broadcast();
			break;
		}
		// Smooth cases
		default:
		{
			checkf(SmoothTeleportToLocationSpeed > 0.0f,
				TEXT("Invalid SmoothTeleportToLocationSpeed! Must be > 0.0! (AsgardVRCharacter, Value %f, TeleportToLocation)"),
				SmoothTeleportToLocationSpeed);
			if (CurrentTeleportToLocationMode == EAsgardTeleportMode::SmoothRate)
			{
				TeleportToLocationSmoothMultiplier = 1.0f / ((TeleportGoalLocation - TeleportStartLocation).Size() / SmoothTeleportToLocationSpeed);
			}
			else
			{
				TeleportToLocationSmoothMultiplier = 1.0f / SmoothTeleportToLocationSpeed;
			}
			TeleportToLocationSmoothProgress = 0.0f;
			SetActorEnableCollision(false);
			bIsTeleportingToLocation = true;
			bTeleportToLocationSucceeded = true;
			break;
		}
	}

	return bTeleportToLocationSucceeded;
}

bool AAsgardVRCharacter::TeleportToRotation(const FRotator& GoalRotation, EAsgardTeleportMode TeleportMode)
{
	// Cache teleport variables
	CurrentTeleportToRotationMode = TeleportMode;
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
		// Smooth cases
		default:
		{
			checkf(SmoothTeleportToRotationSpeed > 0.0f,
				TEXT("Invalid SmoothTeleportToRotationSpeed! Must be > 0.0! (AsgardVRCharacter, Value %f, TeleportToRotation)"),
				SmoothTeleportToRotationSpeed);
			if (CurrentTeleportToRotationMode == EAsgardTeleportMode::SmoothRate)
			{
				TeleportToRotationSmoothMultiplier = 1.0f / ((TeleportGoalRotation - TeleportStartRotation).GetNormalized().Yaw / SmoothTeleportToRotationSpeed);
			}
			else
			{
				TeleportToRotationSmoothMultiplier = 1.0f / SmoothTeleportToRotationSpeed;
			}
			TeleportToRotationSmoothProgress = 0.0f;
			bIsTeleportingToRotation = true;
			break;
		}
	}
	return true;
}

bool AAsgardVRCharacter::TeleportToLocationAndRotation(const FVector& GoalLocation, const FRotator& GoalRotation, EAsgardTeleportMode TeleportMode)
{
	// Cache teleport variables
	TeleportStartLocation = GetVRLocation();
	TeleportStartLocation.Z -= GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	TeleportGoalLocation = GoalLocation;
	TeleportGoalRotation = GoalRotation;

	// Use the most comfortable mode
	CurrentTeleportToLocationMode = TeleportMode;
	CurrentTeleportToRotationMode = TeleportMode;

	// Depending on the teleport mode
	switch (TeleportMode)
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
				UE_LOG(LogAsgardVRCharacter, Error, TEXT("TeleportToLocationAndRotation failed! TeleportTo returned false."));
			}
			OnTeleportToLocationFinished.Broadcast();
			OnTeleportToRotationFinished.Broadcast();
			SetActorRotationVR(TeleportGoalRotation);
			break;
		}
		// Smooth cases
		default:
		{
			checkf(SmoothTeleportToRotationSpeed > 0.0f,
				TEXT("Invalid SmoothTeleportToLocationSpeed! Must be > 0.0! (AsgardVRCharacter, Value %f, TeleportToLocationAndRotation)"),
				SmoothTeleportToLocationSpeed);
			checkf(SmoothTeleportToRotationSpeed > 0.0f,
				TEXT("Invalid SmoothTeleportToRotationSpeed! Must be > 0.0! (AsgardVRCharacter, Value %f, TeleportToLocationAndRotation)"),
				SmoothTeleportToRotationSpeed);
			if (CurrentTeleportToLocationMode == EAsgardTeleportMode::SmoothRate)
			{
				TeleportToLocationSmoothMultiplier = 1.0f / ((TeleportGoalLocation - TeleportStartLocation).Size() / SmoothTeleportToLocationSpeed);
				TeleportToRotationSmoothMultiplier = 1.0f / ((TeleportGoalRotation - TeleportStartRotation).GetNormalized().Yaw / SmoothTeleportToRotationSpeed);
			}
			else
			{
				TeleportToLocationSmoothMultiplier = 1.0f / SmoothTeleportToLocationSpeed;
				TeleportToRotationSmoothMultiplier = 1.0f / SmoothTeleportToRotationSpeed;
			}
			TeleportToLocationSmoothProgress = 0.0f;
			TeleportToRotationSmoothProgress = 0.0f;
			SetActorEnableCollision(false);
			bIsTeleportingToLocation = true;
			bIsTeleportingToRotation = true;
			bTeleportToLocationSucceeded = true;
			break;
		}
	}

	return bTeleportToLocationSucceeded;
}

void AAsgardVRCharacter::TeleportFadeOut_Implementation()
{
	if (TeleportFadeOutTime > 0.0f)
	{
		
		Cast<APlayerController>(GetController())->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, TeleportFadeOutTime, FLinearColor::Black, true);
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
		Cast<APlayerController>(GetController())->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, TeleportFadeInTime, FLinearColor::Black, false);
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
	OnTeleportToLocationFinished.Broadcast();
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
	OnTeleportToLocationFinished.Broadcast();
	OnTeleportToRotationFinished.Broadcast();
	return;
}

void AAsgardVRCharacter::DeferredTeleportTurnFinished()
{
	OnTeleportToRotationFinished.Remove(TeleportTurnFinishedDelegateHandle);
	TeleportTurnFinishedDelegateHandle.Reset();
	if (TeleportTurnHeldInterval > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(TeleportTurnHeldTimer, this, &AAsgardVRCharacter::TeleportTurn, TeleportTurnHeldInterval);
	}
	return;
}

void AAsgardVRCharacter::DeferredTeleportWalkFinished()
{
	OnTeleportToLocationFinished.Remove(TeleportWalkFinishedDelegateHandle);
	TeleportWalkFinishedDelegateHandle.Reset();
	if (TeleportWalkHeldInterval > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(TeleportWalkHeldTimer, this, &AAsgardVRCharacter::TeleportWalk, TeleportWalkHeldInterval);
	}
	return;
}

void AAsgardVRCharacter::UpdateSmoothTeleportToLocation(float DeltaSeconds)
{
	// Profile this function since it is a potentially expensive operation
	SCOPE_CYCLE_COUNTER(STAT_ASGARD_VRCharacterSmoothTeleportToLocation);

	// Progress interpolation
	TeleportToLocationSmoothProgress = FMath::Min
	(TeleportToLocationSmoothProgress + (TeleportToLocationSmoothMultiplier * DeltaSeconds), 
		1.0f);

	// Continue interpolation
	if (TeleportToLocationSmoothProgress < 1.0f)
	{

		SetActorLocation(FMath::Lerp(
			GetTeleportLocation(TeleportStartLocation),
			GetTeleportLocation(TeleportGoalLocation),
			TeleportToLocationSmoothProgress),
			false,
			nullptr, 
			ETeleportType::TeleportPhysics);
	}

	// Finish interpolation
	else
	{
		SetActorLocation(GetTeleportLocation(TeleportGoalLocation),
			false,
			nullptr,
			ETeleportType::ResetPhysics);
		SetActorEnableCollision(true);

		// Update variables for Smooth teleport rotation if appropriate
		if (bIsTeleportingToRotation && TeleportToRotationDefaultMode >= EAsgardTeleportMode::SmoothRate)
		{
			TeleportStartRotation = GetVRRotation();
		}

		bIsTeleportingToLocation = false;
		OnTeleportToLocationFinished.Broadcast();
	}

	return;
}

void AAsgardVRCharacter::UpdateSmoothTeleportToRotation(float DeltaSeconds)
{
	// Profile this function since it is a potentially expensive operation
	SCOPE_CYCLE_COUNTER(STAT_ASGARD_VRCharacterSmoothTeleportToRotation);

	// Progress interpolation
	TeleportToRotationSmoothProgress = FMath::Min
	(TeleportToRotationSmoothProgress + (TeleportToRotationSmoothMultiplier * DeltaSeconds),
		1.0f);

	// Continue interpolation
	if (TeleportToLocationSmoothProgress < 1.0f)
	{
		SetActorRotationVR(FMath::Lerp(
			TeleportStartRotation, 
			TeleportGoalRotation, 
			TeleportToRotationSmoothProgress));
	}

	// Finish interpolation
	else
	{
		SetActorRotationVR(TeleportGoalRotation);
		bIsTeleportingToRotation = false;
		OnTeleportToRotationFinished.Broadcast();
	}
}

bool AAsgardVRCharacter::TraceForTeleportLocation(
	const FVector& TraceOrigin,
	const FVector& TraceVelocity,
	bool bRequiresNavmeshPath,
	TEnumAsByte<ECollisionChannel> TraceChannel,
	FVector& OutTeleportLocation,
	FVector* OptionalOutImpactPoint,
	TArray<FVector>* OptionalOutTracePathPoints)
{
	// Profile this function since it is a potentially expensive operation
	SCOPE_CYCLE_COUNTER(STAT_ASGARD_VRCharacterPrecisionTeleportLocation);

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
	if (OptionalOutTracePathPoints != nullptr)
	{
		OptionalOutTracePathPoints->Empty();
		for (const FPredictProjectilePathPointData& PathPoint : TraceResult.PathData)
		{
			OptionalOutTracePathPoints->Add(PathPoint.Location);
		}
	}


	// Return result
	if (bTraceHit)
	{
		if (OptionalOutImpactPoint != nullptr)
		{
			*OptionalOutImpactPoint = TraceResult.HitResult.ImpactPoint;
		}

		TELEPORT_LOC(TraceResult.HitResult.ImpactPoint, 20.0f, FColor::Yellow);

		if (ProjectPointToVRNavigation(TraceResult.HitResult.ImpactPoint, OutTeleportLocation, true))
		{
			if (!bRequiresNavmeshPath || DoesPathToPointExistVR(OutTeleportLocation))
			{
				TELEPORT_LOC(OutTeleportLocation, 25.0f, FColor::Cyan);
				TELEPORT_LINE(TraceOrigin, OutTeleportLocation, FColor::Green);
				return true;
			}
			else
			{
				TELEPORT_LOC(OutTeleportLocation, 25.0f, FColor::Magenta);
				TELEPORT_LINE(TraceOrigin, TraceResult.HitResult.ImpactPoint, FColor::Red);
			}
		}
	}

	return false;
}

void AAsgardVRCharacter::TeleportTurn()
{
	// If can teleport turn
	if (CanTeleport())
	{
		// Set up the next call of this function if appropriate
		if (TeleportTurnHeldInterval > 0.0f)
		{
			// In instant mode, use a simple timer
			if (TeleportToRotationDefaultMode == EAsgardTeleportMode::Instant)
			{
				GetWorld()->GetTimerManager().SetTimer(TeleportTurnHeldTimer, this, &AAsgardVRCharacter::TeleportTurn, TeleportTurnHeldInterval);
			}

			// Otherwise bind to teleport completion
			else
			{
				TeleportTurnFinishedDelegateHandle = OnTeleportToRotationFinished.AddUObject(this, &AAsgardVRCharacter::DeferredTeleportTurnFinished);
			}
		}
		checkf(TurnInput2DActionState != EAsgard2DAxis::Neutral, TEXT("Invalid TurnInputAxisState for TeleportTurn! Was Neutral. (AsgardVRCharacter)"));
		checkf(TurnInput2DActionState != EAsgard2DAxis::Forward, TEXT("Invalid TurnInputAxisState for TeleportTurn! Was Forward. (AsgardVRCharacter)"));
		switch (TurnInput2DActionState)
		{
			case EAsgard2DAxis::Right:
			{
				TeleportToRotation(GetVRRotation() + FRotator(0.0f, TeleportTurnAngleInterval, 0.0f), TeleportToRotationDefaultMode);
				break;
			}
			case EAsgard2DAxis::Left:
			{
				TeleportToRotation(GetVRRotation() + FRotator(0.0f, -TeleportTurnAngleInterval, 0.0f), TeleportToRotationDefaultMode);
				break;
			}
			case EAsgard2DAxis::Backward:
			{
				TeleportToRotation(GetVRRotation() + FRotator(0.0f, 180.0f, 0.0f), TeleportToRotationDefaultMode);
				break;
			}
			default:
			{
				break;
			}
		}
	}

	// Bind a retry if we could not teleport
	else
	{
		GetWorld()->GetTimerManager().SetTimer(TeleportTurnHeldTimer, this, &AAsgardVRCharacter::TeleportTurn, TeleportTurnHeldInterval);
	}
}

void AAsgardVRCharacter::TeleportWalk()
{
	// If can teleport walk
	EAsgardMovementMode MovementMode = AsgardVRMovementRef->GetAsgardMovementMode();
	if ((MovementMode == EAsgardMovementMode::C_MOVE_Walking
		|| MovementMode == EAsgardMovementMode::C_MOVE_NavWalking)
		&& bWalkEnabled 
		&& DefaultGroundMovementMode == EAsgardGroundMovementMode::TeleportWalk
		&& CanTeleport())
	{
		// Calculate the direction
		FVector ForwardDirection;
		FVector RightDirection;
		if (CalculateOrientedFlattenedDirectionalVectors(ForwardDirection, RightDirection, WalkOrientationComponent, WalkForwardMaxAbsPitch))
		{
			FVector TeleportDirection;
			checkf(MoveInput2DActionState != EAsgard2DAxis::Neutral, TEXT("Invalid MoveInputAxisState for TeleportWalk! Was Neutral. (AsgardVRCharacter)"));
			switch (MoveInput2DActionState)
			{
				case EAsgard2DAxis::Forward:
				{
					TeleportDirection = ForwardDirection;
					break;
				}
				case EAsgard2DAxis::Backward:
				{
					TeleportDirection = ForwardDirection * -1.0f;
					break;
				}
				case EAsgard2DAxis::Left:
				{
					TeleportDirection = RightDirection * -1.0f;
					break;
				}
				case EAsgard2DAxis::Right:
				{
					TeleportDirection = RightDirection;
					break;
				}
				default:
				{
					break;
				}
			}

			// Bind delegate if appropriate
			bool bBindDelegate = (TeleportWalkHeldInterval > 0.0f && TeleportToLocationDefaultMode != EAsgardTeleportMode::Instant);
			if (bBindDelegate)
			{
				TeleportWalkFinishedDelegateHandle = OnTeleportToLocationFinished.AddUObject(this, &AAsgardVRCharacter::DeferredTeleportWalkFinished);
			}

			// If teleport successful
			if (TeleportInDirection(
				TeleportDirection,
				TeleportWalkMagnitude,
				MovementMode == EAsgardMovementMode::C_MOVE_NavWalking,
				TeleportWalkTraceChannel,
				TeleportToLocationDefaultMode,
				TeleportWalkMaxFallbackTraces,
				TeleportWalkFallbackTraceMagnitudeInterval))
			{
				// If we bound the delegate return now
				if (bBindDelegate)
				{
					return;
				}
			}

			// If teleport failed and the delegate is still bound, unbind it
			else if (TeleportWalkFinishedDelegateHandle.IsValid())
			{
				OnTeleportToLocationFinished.Remove(TeleportWalkFinishedDelegateHandle);
				TeleportWalkFinishedDelegateHandle.Reset();
			}
		}
	}

	// If we didn't teleport or if completion was not deferred, set a timer for the held event
	if (TeleportWalkHeldInterval > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(TeleportWalkHeldTimer, this, &AAsgardVRCharacter::TeleportWalk, TeleportWalkHeldInterval);
	}

	return;
}

bool AAsgardVRCharacter::TeleportInDirection(
	const FVector& Direction, 
	float InitialMagnitude, 
	bool bRequiresNavMeshPath, 
	TEnumAsByte<ECollisionChannel> TraceChannel, 
	EAsgardTeleportMode TeleportMode, 
	int32 MaxRetries, 
	float FallbackMagnitudeInterval)
{
	// Trace in the indicated direction, retrying if appropriate
	bool bTraceSuccessful;
	FVector TeleportGoal;
	do
	{
		bTraceSuccessful = TraceForTeleportLocation(GetVRLocation(), Direction * InitialMagnitude, bRequiresNavMeshPath, TraceChannel, TeleportGoal);
		MaxRetries -= 1;
		InitialMagnitude -= FallbackMagnitudeInterval;
	} while (!bTraceSuccessful && MaxRetries > -1);

	// If trace was successful
	if (bTraceSuccessful)
	{
		return TeleportToLocation(TeleportGoal, TeleportMode);
	}

	return false;
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
	PlayerInputComponent->BindAction(UAsgardInputBindings::PrecisionTeleportAction(), IE_Pressed, this, &AAsgardVRCharacter::OnPrecisionTeleportActionPressed);
	PlayerInputComponent->BindAction(UAsgardInputBindings::PrecisionTeleportAction(), IE_Released, this, &AAsgardVRCharacter::OnPrecisionTeleportActionReleased);

	// Axis bindings
	PlayerInputComponent->BindAxis(UAsgardInputBindings::MoveForwardAxis(), this, &AAsgardVRCharacter::OnMoveForwardAxis);
	PlayerInputComponent->BindAxis(UAsgardInputBindings::MoveRightAxis(), this, &AAsgardVRCharacter::OnMoveRightAxis);
	PlayerInputComponent->BindAxis(UAsgardInputBindings::TurnForwardAxis(), this, &AAsgardVRCharacter::OnTurnForwardAxis);
	PlayerInputComponent->BindAxis(UAsgardInputBindings::TurnRightAxis(), this, &AAsgardVRCharacter::OnTurnRightAxis);

	return;
}

bool AAsgardVRCharacter::CanTeleport_Implementation()
{
	return bTeleportEnabled && !(bIsTeleportingToLocation || bIsTeleportingToRotation);
}

bool AAsgardVRCharacter::CanSmoothWalk_Implementation()
{
	return bWalkEnabled && !(bIsTeleportingToLocation || bIsTeleportingToRotation);
}

bool AAsgardVRCharacter::CanFly_Implementation()
{
	return bFlightEnabled && (bIsSmoothFlightMovementEnabled || bFlightControllerThrustersEnabled) && !(bIsTeleportingToLocation || bIsTeleportingToRotation);
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
			if (GetNameSafe(CurrNavData) == "RecastNavMesh-VRCharacterProjection")
			{
				ProjectionNavData = CurrNavData;
			}
			else if (GetNameSafe(CurrNavData) == "RecastNavMesh-VRCharacterPathfinding")
			{
				PathfindingNavData = CurrNavData;
			}
			if (ProjectionNavData && PathfindingNavData)
			{
				return;
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
		if (NavSys->ProjectPointToNavigation(Point, ProjectedNavLoc, (NavQueryExtent.IsNearlyZero() ? INVALID_NAVEXTENT : NavQueryExtent), ProjectionNavData, UNavigationQueryFilter::GetQueryFilter(*ProjectionNavData, this, NavQueryFilter)))
		{	
			// Update the out projected point
			OutProjectedPoint = ProjectedNavLoc.Location;
			
			// If we want to check the point is on the ground
			if (bCheckIfIsOnGround)
			{
				// Trace downwards and see if we hit something
				FHitResult GroundTraceHitResult;
				const FVector GroundTraceOrigin = ProjectedNavLoc.Location;
				const FVector GroundTraceEnd = GroundTraceOrigin + FVector(0.0f, 0.0f, -100.0f);
				FCollisionQueryParams GroundTraceParams(FName(TEXT("VRCharacterGroundTrace")), false, this);
				bool bGroundTraceSuccess = GetWorld()->LineTraceSingleByProfile(GroundTraceHitResult, GroundTraceOrigin, GroundTraceEnd, UAsgardCollisionProfiles::VRRoot(), GroundTraceParams);
				if (bGroundTraceSuccess)
				{
					OutProjectedPoint = GroundTraceHitResult.ImpactPoint;
				}
				return bGroundTraceSuccess;
			}
			return true;
		}
	}
	return false;
}

bool AAsgardVRCharacter::DoesPathToPointExistVR(const FVector& GoalLocation)
{
	if (PathfindingNavData)
	{
		FPathFindingQuery QuerySettings;
		QuerySettings.StartLocation = GetVRLocation();
		QuerySettings.StartLocation.Z -= GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
		QuerySettings.EndLocation = GoalLocation;
		QuerySettings.bAllowPartialPaths = false;
		QuerySettings.NavData = PathfindingNavData;
		QuerySettings.NavAgentProperties = PathfindingNavAgentProperties;
		QuerySettings.QueryFilter = UNavigationQueryFilter::GetQueryFilter(*PathfindingNavData, NavQueryFilter);
		int32 NumNodesVisited;
		return PathfindingNavData->TestPath(PathfindingNavAgentProperties, QuerySettings, &NumNodesVisited);
	}
	return false;
}

void AAsgardVRCharacter::UpdateSmoothWalk()
{
	// If we can walk
	if (DefaultGroundMovementMode == EAsgardGroundMovementMode::SmoothWalk && CanSmoothWalk())
	{
		// Get the input vector depending on the walking weight mode
		FVector WalkInput = FVector::ZeroVector;
		if (SmoothWalkInputAxisWeightMode == EAsgardInputAxisWeightMode::Circular)
		{
			WalkInput = CalculateCircularInputVector(
				MoveInputVector,
				SmoothWalkInputDeadZone,
				SmoothWalkInputMaxZone);
		}
		else
		{
			WalkInput = CalculateCrossInputVector(
				MoveForwardAxis,
				MoveRightAxis,
				SmoothWalkInputDeadZone,
				SmoothWalkInputMaxZone);
		}

		// Guard against no input
		if (FMath::IsNearlyZero(WalkInput.SizeSquared2D()))
		{
			StopSmoothWalk();
			return;
		}

		// Get the forward and right vectors
		FVector WalkForward;
		FVector WalkRight;

		// Guard against the forward being nearly up or down as appropriate
		if (!CalculateOrientedFlattenedDirectionalVectors(WalkForward, WalkRight, WalkOrientationComponent, WalkForwardMaxAbsPitch))
		{
			StopSmoothWalk();
			return;
		}

		// Scale the input vector by the forward, backward, and strafing multipliers
		WalkInput *= FVector(WalkInput.X > 0.0f ? SmoothWalkInputForwardMultiplier : SmoothWalkInputBackwardMultiplier, SmoothWalkInputStrafeMultiplier, 0.0f);
		WalkInput = (WalkInput.X * WalkForward) + (WalkInput.Y * WalkRight);

		// Guard against the input vector being nullified
		if (FMath::IsNearlyZero(WalkInput.SizeSquared2D()))
		{
			StopSmoothWalk();
			return;
		}

		// Apply the movement input
		StartSmoothWalk();
		AsgardVRMovementRef->AddInputVector(WalkInput);
	}

	// Else, check we are not in a walking state
	else
	{
		StopSmoothWalk();
	}

	return;
}

void AAsgardVRCharacter::UpdateSmoothFlight()
{
	// If we can fly
	if (CanFly())
	{
		// Cache initial variables
		bool bShouldApplyInput = false;
		FVector FlightInput = FVector::ZeroVector;

		// Analog stick input
		if (bIsSmoothFlightMovementEnabled)
		{
			// Calculate the Flight input vector
			if (SmoothFlightAxisWeightMode == EAsgardInputAxisWeightMode::Circular)
			{
				FlightInput = CalculateCircularInputVector(
					MoveInputVector,
					SmoothFlightInputDeadZone,
					SmoothFlightInputMaxZone);
			}
			else
			{
				FlightInput = CalculateCrossInputVector(
					MoveForwardAxis,
					MoveRightAxis,
					SmoothFlightInputDeadZone,
					SmoothFlightInputMaxZone);
			}

			// Guard against no input
			if (!FlightInput.IsNearlyZero())
			{
				bShouldApplyInput = true;

				// Get the forward and right Flight vectors
				FVector FlightForward = SmoothFlightOrientationComponent->GetForwardVector();
				FVector FlightRight = SmoothFlightOrientationComponent->GetRightVector();

				// Scale the input vector by the forward, backward, and strafing multipliers
				FlightInput *= FVector(FlightInput.X > 0.0f ? SmoothFlightInputForwardMultiplier : SmoothFlightInputBackwardMultiplier, SmoothFlightInputStrafeMultiplier, 0.0f);
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
			if (FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(FVector::DownVector, FlightInput.GetSafeNormal()))) <= FlightAutoLandingMinPitch
				&& (!bFlightAutoLandingRequiresDownwardVelocity
					|| FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(FVector::DownVector, VRMovementReference->Velocity.GetSafeNormal())) <= FlightAutoLandingMinPitch)))
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

	return;
}

void AAsgardVRCharacter::SetMoveInputVector(const FVector& NewMoveInputVector)
{
	MoveInputVector = NewMoveInputVector;

	// Calculate move action pressed events
	if (bIsMoveActionPressed)
	{
		if (MoveInputVector.SizeSquared2D() < AxisPressedMinAbsValue * AxisPressedMinAbsValue)
		{
			OnMoveActionReleased();
		}
	}
	else if (MoveInputVector.SizeSquared2D() >= AxisPressedMinAbsValue * AxisPressedMinAbsValue)
	{
		OnMoveActionPressed();
	}

	// Calculate 2D move action pressed events
	if (bIsMoveActionPressed)
	{
		SetMoveInput2DActionState(Calculate2DActionState(MoveInputVector));
	}
	else
	{
		SetMoveInput2DActionState(EAsgard2DAxis::Neutral);
	}

	return;
}

void AAsgardVRCharacter::SetTurnInputVector(const FVector& NewTurnInputVector)
{
	TurnInputVector = NewTurnInputVector;

	// Calculate Turn action pressed events
	if (bIsTurnActionPressed)
	{
		if (TurnInputVector.SizeSquared2D() < AxisPressedMinAbsValue * AxisPressedMinAbsValue)
		{
			OnTurnActionReleased();
		}
	}
	else if (TurnInputVector.SizeSquared2D() >= AxisPressedMinAbsValue * AxisPressedMinAbsValue)
	{
		OnTurnActionPressed();
	}

	// Calculate 2D Turn action pressed events
	if (bIsTurnActionPressed)
	{
		SetTurnInput2DActionState(Calculate2DActionState(TurnInputVector));
	}
	else
	{
		SetTurnInput2DActionState(EAsgard2DAxis::Neutral);
	}

	return;
}

EAsgard2DAxis AAsgardVRCharacter::Calculate2DActionState(const FVector& InputVector)
{
	if (FMath::Abs(InputVector.X) > FMath::Abs(InputVector.Y))
	{
		if (InputVector.X > 0.0f)
		{
			if (InputVector.X > AxisPressedMinAbsValue
				&& (AxisPressedMaxAngle < 0.0f || FMath::Abs(FMath::RadiansToDegrees(FMath::Atan2(InputVector.Y, InputVector.X))) < AxisPressedMaxAngle))
			{
				return EAsgard2DAxis::Forward;
			}
		}
		else
		{
			if (InputVector.X < -AxisPressedMinAbsValue
				&& (AxisPressedMaxAngle < 0.0f || FMath::Abs(FMath::RadiansToDegrees(FMath::Atan2(InputVector.Y, InputVector.X))) >= 180.0f - AxisPressedMaxAngle))
			{
				return EAsgard2DAxis::Backward;
			}
		}
	}
	else
	{
		if (InputVector.Y > 0.0f)
		{
			if (InputVector.Y > AxisPressedMinAbsValue
				&& (AxisPressedMaxAngle < 0.0f || FMath::Abs(FMath::RadiansToDegrees(FMath::Atan2(InputVector.X, InputVector.Y))) < AxisPressedMaxAngle))
			{
				return EAsgard2DAxis::Right;
			}
		}
		else
		{
			if (InputVector.Y < -AxisPressedMinAbsValue
				&& (AxisPressedMaxAngle < 0.0f || FMath::Abs(FMath::RadiansToDegrees(FMath::Atan2(InputVector.X, InputVector.Y))) >= 180.0f - AxisPressedMaxAngle))
			{
				return EAsgard2DAxis::Left;
			}
		}
	}

	return EAsgard2DAxis::Neutral;
}

void AAsgardVRCharacter::SetMoveInput2DActionState(EAsgard2DAxis NewMoveInput2DActionState)
{
	if (MoveInput2DActionState != NewMoveInput2DActionState)
	{
		// Update state
		MoveInput2DActionState = NewMoveInput2DActionState;

		// If new state is not released
		if (MoveInput2DActionState != EAsgard2DAxis::Neutral)
		{
			TeleportWalk();

		}

		// Cancel the old held timer and delegate binding if appropriate
		else
		{
			if (TeleportWalkHeldTimer.IsValid())
			{
				GetWorld()->GetTimerManager().ClearTimer(TeleportWalkHeldTimer);
			}
			if (TeleportWalkFinishedDelegateHandle.IsValid())
			{
				OnTeleportToLocationFinished.Remove(TeleportWalkFinishedDelegateHandle);
				TeleportWalkFinishedDelegateHandle.Reset();
			}
		}
	}
	return;
}

void AAsgardVRCharacter::SetTurnInput2DActionState(EAsgard2DAxis NewTurnInput2DActionState)
{
	if (TurnInput2DActionState != NewTurnInput2DActionState)
	{
		// Update state
		TurnInput2DActionState = NewTurnInput2DActionState;

		switch (TurnInput2DActionState)
		{
			case EAsgard2DAxis::Forward:
			{
				// Cancel the old held timer and delegate binding if appropriate
				if (TeleportTurnHeldTimer.IsValid())
				{
					GetWorld()->GetTimerManager().ClearTimer(TeleportTurnHeldTimer);
				}
				if (TeleportTurnFinishedDelegateHandle.IsValid())
				{
					OnTeleportToRotationFinished.Remove(TeleportTurnFinishedDelegateHandle);
					TeleportTurnFinishedDelegateHandle.Reset();
				}

				// Start precision teleport if the dedicated action on this hand is not already pressed
				if (CanTeleport() && (!bIsPrecisionTeleportActive || PrecisionTeleportInputSource == EAsgardInputSource::Inferred2))
				{
					// Set input source and orientation component
					PrecisionTeleportInputSource = EAsgardInputSource::Inferred1;
					SetPrecisionTeleportOrientationComponent(PrecisionTeleportInferred1OrientationMode);
					StartPrecisionTeleport();
				}
				break;
			}
			case EAsgard2DAxis::Neutral:
			{
				// Cancel the old held timer and delegate binding if appropriate
				if (TeleportTurnHeldTimer.IsValid())
				{
					GetWorld()->GetTimerManager().ClearTimer(TeleportTurnHeldTimer);
				}
				if (TeleportTurnFinishedDelegateHandle.IsValid())
				{
					OnTeleportToRotationFinished.Remove(TeleportTurnFinishedDelegateHandle);
					TeleportTurnFinishedDelegateHandle.Reset();
				}

				break;
			}

			default:
			{
				// Fort turning cases, attempt a teleport turn so long as we are not in precision teleport mode from this input
				if (!bIsPrecisionTeleportActive || !(PrecisionTeleportInputSource == EAsgardInputSource::Inferred1))
				{
					TeleportTurn();
				}
				break;
			}
		}
	}
	return;
}

void AAsgardVRCharacter::UpdatePrecisionTeleport(float DeltaSeconds)
{
	if (CanTeleport())
	{
		// Updating trace direction
		// If interpolated
		if (PrecisionTeleportTraceDirectionMaxErrorAngle > 0.0f && PrecisionTeleportTraceDirectionAngleLerpSpeed > 0.0f)
		{
			FVector NewPrecisionTeleportDirection = PrecisionTeleportOrientationComponent->GetForwardVector();

			// If the pitch angle is high...
			if (PrecisionTeleportTraceDirectionMaxPitch < 90.0f
				&& FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(NewPrecisionTeleportDirection, FVector::UpVector))) < 90.0f - PrecisionTeleportTraceDirectionMaxPitch)
			{
				// Make a new vector from the yaw and a fixed pitch
				NewPrecisionTeleportDirection = UKismetMathLibrary::CreateVectorFromYawPitch(FMath::RadiansToDegrees(FMath::Atan2(NewPrecisionTeleportDirection.Y, NewPrecisionTeleportDirection.X)), PrecisionTeleportTraceDirectionMaxPitch);
			}

			// Calculate the angular difference from the last trace direction

			float DifferenceAngle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(NewPrecisionTeleportDirection, PrecisionTeleportTraceDirection)));

			// If the difference is significant
			if (DifferenceAngle > SMALL_NUMBER)
			{
				// Interpolate new difference angle
				DifferenceAngle = FMath::FInterpTo(DifferenceAngle, 0.0f, DeltaSeconds, PrecisionTeleportTraceDirectionAngleLerpSpeed);
				
				// Cap the new difference angle
				if (DifferenceAngle > PrecisionTeleportTraceDirectionMaxErrorAngle)
				{
					DifferenceAngle = PrecisionTeleportTraceDirectionMaxErrorAngle;
				}

				// Rotate direction by the new difference angle along the difference axis
				NewPrecisionTeleportDirection = NewPrecisionTeleportDirection.RotateAngleAxis(DifferenceAngle, FVector::CrossProduct(NewPrecisionTeleportDirection, PrecisionTeleportTraceDirection).GetSafeNormal());
			}

			// Update the direction
			PrecisionTeleportTraceDirection = NewPrecisionTeleportDirection;
		}

		// Else if instant
		else
		{
			PrecisionTeleportTraceDirection = PrecisionTeleportOrientationComponent->GetForwardVector();

			// If the pitch angle is high...
			if (PrecisionTeleportTraceDirectionMaxPitch < 90.0f
				&& FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(PrecisionTeleportTraceDirection, FVector::UpVector))) < 90.0f - PrecisionTeleportTraceDirectionMaxPitch)
			{
					// Make a new vector from the yaw and a fixed pitch
					PrecisionTeleportTraceDirection = UKismetMathLibrary::CreateVectorFromYawPitch(FMath::RadiansToDegrees(FMath::Atan2(PrecisionTeleportTraceDirection.Y, PrecisionTeleportTraceDirection.X)), PrecisionTeleportTraceDirectionMaxPitch);
			}
		}

		// Perform the teleport trace
		bIsPrecisionTeleportLocationValid = TraceForTeleportLocation(
			PrecisionTeleportOrientationComponent->GetComponentLocation(),
			PrecisionTeleportTraceDirection * PrecisionTeleportTraceMagnitude,
			bPrecisionTeleportLocationRequiresNavmeshPath,
			PrecisionTeleportTraceChannel,
			PrecisionTeleportLocation,
			&PrecisionTeleportImpactPoint,
			&PrecisionTeleportTracePath);

		return;
	}
	else
	{
		StopPrecisionTeleport();
	}
}

bool AAsgardVRCharacter::CalculateOrientedFlattenedDirectionalVectors(FVector& OutForward, FVector& OutRight, USceneComponent* OrientationComponent, float MaxAbsPitch) const
{
	// If no orientation component, get the VR right and forward vectors
	if (!OrientationComponent)
	{
		OutForward = GetVRForwardVector();
		OutRight = GetVRRightVector();
		return true;
	}

	// Otherwise, get the vectors from the orientation component
	FVector Forward = WalkOrientationComponent->GetForwardVector();

	// Guard against the forward vector being nearly up or down
	if (MaxAbsPitch < 90.0f
		&& FMath::Abs(FVector::DotProduct(Forward, FVector::UpVector)) > FMath::Sin(FMath::DegreesToRadians(WalkForwardMaxAbsPitch)))
	{
		return false;
	}

	// Flatten the forward vector
	Forward.Z = 0.0f;
	Forward = Forward.GetSafeNormal();
	OutForward = Forward;

	// Calculcate right vector
	OutRight = FVector::CrossProduct(FVector::UpVector, Forward);

	return true;
}

void AAsgardVRCharacter::StartPrecisionTeleport()
{
	bIsPrecisionTeleportLocationValid = false;
	PrecisionTeleportTraceDirection = PrecisionTeleportOrientationComponent->GetForwardVector();
	bIsPrecisionTeleportActive = true;
	
	return;
}

void AAsgardVRCharacter::StopPrecisionTeleport()
{
	bIsPrecisionTeleportActive = false;
	if (bIsPrecisionTeleportLocationValid && CanTeleport())
	{
		TeleportToLocation(PrecisionTeleportLocation, TeleportToLocationDefaultMode);
	}

	return;
}

const FVector AAsgardVRCharacter::CalculateCircularInputVector(FVector RawInputVector, float DeadZone, float MaxZone) const
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
	float InputVectorLength = RawInputVector.SizeSquared2D();

	// If the size of the input vector is greater than the deadzone
	if (InputVectorLength > DeadZone * DeadZone)
	{
		// Normalize the size of the vector to the remaining space between the deadzone and the maxzone
		RawInputVector = RawInputVector.GetSafeNormal();
		if (InputVectorLength < MaxZone * MaxZone)
		{
			InputVectorLength = FMath::Sqrt(InputVectorLength);
			RawInputVector *= (InputVectorLength - DeadZone) / (MaxZone - DeadZone);
		}
		return RawInputVector;
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

void AAsgardVRCharacter::StartSmoothWalk()
{
	if (!bIsSmoothWalking)
	{
		bIsSmoothWalking = true;
		OnSmoothWalkStarted();
	}
	return;
}

void AAsgardVRCharacter::StopSmoothWalk()
{
	if (bIsSmoothWalking)
	{
		bIsSmoothWalking = false;
		OnSmoothWalkStopped();
	}

	return;
}
