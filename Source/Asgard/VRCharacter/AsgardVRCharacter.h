// Copyright © 2020 Justin Camden All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "VRCharacter.h"
#include "AsgardVRCharacter.generated.h"

// Delegates
DECLARE_DELEGATE(FOnTeleportFadeOutFinished)
DECLARE_DELEGATE(FOnTeleportFadeInFinished)

// Stats
DECLARE_STATS_GROUP(TEXT("AsgardVRCharacter"), STATGROUP_ASGARD_VRCharacter, STATCAT_Advanced);

UENUM(BlueprintType)
enum class EAsgardTeleportMode: uint8
{
	Fade,
	Instant,
	ContinuousRate,
	ContinuousTime
};

UENUM(BlueprintType)
enum class EAsgardWalkOrientationMode : uint8
{
	VRCamera,
	LeftController,
	RightController,
	CharacterCapsule
};

UENUM(BlueprintType)
enum class EAsgardBinaryHand : uint8
{
	LeftHand,
	RightHand
};

UENUM(BlueprintType)
enum class EAsgardFlightAnalogOrientationMode : uint8
{
	VRCamera,
	LeftController,
	RightController
};

UENUM(BlueprintType)
enum class EAsgarFlightOrientationMode : uint8
{
	Controllers,
	VRCamera
};

UENUM(BlueprintType)
enum class EAsgardInputAxisWeightMode: uint8
{
	Circular,
	Cross
};

// Forward declarations
class UAsgardVRMovementComponent;


/**
 * Base class for the player avatar
 */
UCLASS()
class ASGARD_API AAsgardVRCharacter : public AVRCharacter
{
	GENERATED_BODY()

public:
	// Overrides
	AAsgardVRCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// Axis events
	void OnMoveForwardAxis(float Value);
	void OnMoveRightAxis(float Value);

	// Action events
	void OnFlightThrusterLeftActionPressed();
	void OnFlightThrusterLeftActionReleased();
	void OnFlightThrusterRightActionPressed();
	void OnFlightThrusterRightActionReleased();
	void OnToggleFlightActionPressed();
	void OnTraceTeleportActionPressed();
	void OnTraceTeleportActionReleased();

	/**
	* Setter for WalkOrientationMode.
	* Sets the WalkOrientationComponent to match orientation mode.
	*/
	UFUNCTION(BlueprintCallable, Category = AsgardVRCharacter)
	void SetWalkOrientationMode(const EAsgardWalkOrientationMode NewWalkOrientationMode);

	/**
	* Setter for FlightOrientationMode.
	* Sets the FlightOrientationComponent to match orientation mode.
	* This is only used with analog stick style flying.
	*/
	UFUNCTION(BlueprintCallable, Category = AsgardVRCharacter)
	void SetFlightAnalogOrientationMode(const EAsgardFlightAnalogOrientationMode NewFlightOrientationMode);

	/**
	* Setter for MoveActionTraceTeleportOrientationMode.
	* Sets the MoveActionTraceTeleportOrientationComponent to match orientation mode.
	*/
	UFUNCTION(BlueprintCallable, Category = AsgardVRCharacter)
	void SetMoveActionTraceTeleportOrientationMode(const EAsgardBinaryHand NewMoveActionTraceTeleportOrientationMode);


	// ---------------------------------------------------------
	//	Navigation

	/**
	* The filter class when searching for a place on the navmesh this character can stand on.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement")
	TSubclassOf<UNavigationQueryFilter> NavFilterClass;

	/**
	* The extent of the query when searching for a place on the navmesh this character can stand on.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement")
	FVector NavQueryExtent;

	// ---------------------------------------------------------
	//	Teleport settings
	/**
	* Whether teleporting is enabled for the character.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Teleport")
	uint32 bTeleportEnabled : 1;

	/** 
	* How long  it takes to fade out when teleporting using Fade teleport mode.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Teleport")
	float TeleportFadeOutTime;

	/**
	* How long  it takes to fade in when teleporting using Fade teleport mode.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Teleport")
	float TeleportFadeInTime;

	/**
	* The method of teleportation used for teleporting to a location.
	* Instant: Character is instantly placed at their goal location.
	* Blink: Brief delay, before the character is placed at their goal location.
	* ContinuousRate: Character is smoothly interpolated to their goal location at a given rate.
	* ContinuousTime: Character is smoothly interpolated to their goal location over a given time.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Teleport")
	EAsgardTeleportMode TeleportToLocationMode;

	/**
	* The method of teleportation used for teleporting to a specific Rotation.
	* Instant: Character is instantly placed at their goal Rotation.
	* Blink: Brief delay, before the character is placed at their goal Rotation.
	* ContinuousRate: Character is smoothly interpolated to their goal Rotation at a given rate.
	* ContinuousTime: Character is smoothly interpolated to their goal Rotation over a given time.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Teleport")
	EAsgardTeleportMode TeleportToRotationMode;

	/**
	* The speed of the character when teleporting to a location in a Continuous teleport mode.
	* ContinuousRate: Treated as centimeters per second.
	* ContinuousTime: Treated treated as how long each teleport will take.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Teleport", meta = (ClampMin = "0.01"))
	float ContinuousTeleportToLocationSpeed;

	/**
	* The speed of the character when teleporting to a rotation in a Continuous teleport mode.
	* ContinuousRate: Treated as degrees per second.
	* ContinuousTime: Treated treated as how long each teleport will take.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Teleport", meta = (ClampMin = "0.01"))
	float ContinuousTeleportToRotationSpeed;

	/**
	* The interpolation speed for the direction of a move action trace teleport, updated from frame to frame.
	* Instant if <= 0.
	* Measured in pi * radians per second (1 = 90 degrees).
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Teleport")
	float MoveActionTraceTeleportDirectionInterpSpeedPiRadians;

	/**
	* The maximum angle difference for the direction of a move action trace teleport, from frame to frame.
	* Measured in pi * radians per second (1 = 90 degrees).
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Teleport", meta = (ClampMax = "1.99"))
	float MoveActionTraceTeleportDirectionMaxAnglePiRadians;

	/**
	* The magnitude of velocity of the move action trace teleport.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Teleport", meta = (ClampMax = "1.99"))
	float MoveActionTraceTeleportVelocityMagnitude;


	// ---------------------------------------------------------
	//	Walk settings

	/**
	* Whether walking is enabled for the character.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Walk")
	uint32 bWalkEnabled : 1;

	/**
	* The axis weight mode for the character when walking.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Walk")
	EAsgardInputAxisWeightMode WalkInputAxisWeightMode;

	/**
	* The dead zone of the input vector when walking.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Walk")
	float WalkInputDeadZone;

	/**
	* The max zone of the input vector when walking.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Walk")
	float WalkInputMaxZone;

	/**
	* Multiplier that will be applied to the forward or back input vector when walking forward.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Walk")
	float WalkInputForwardMultiplier;

	/**
	* Multiplier that will be applied to the forward or back input vector when walking backward.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Walk")
	float WalkInputBackwardMultiplier;

	/**
	* Multiplier that will be applied to the right or left input vector when strafing.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Walk")
	float WalkInputStrafeMultiplier;

	/**
	* The absolute world pitch angle of forward vector of the walking orientation component must be less than or equal to this many radians.
	* Otherwise, no walking input will be applied.
	* Unit is PiRadians (1 = 90 degrees).
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Walk", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WalkOrientationMaxAbsPitchPiRadians;

	// ---------------------------------------------------------
	//	Flight settings

	/**
	* Whether flying is enabled for the character.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Flight")
	uint32 bFlightEnabled : 1;

	/**
	* Whether the player should automatically land and stop flying when attempting to move into the ground.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Flight")
	uint32 bFlightAutoLandingEnabled : 1;

	/**
	* Whether auto landing requires velocity as well as desired input to be facing the ground.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Flight")
	uint32 bFlightAutoLandingRequiresDownwardVelocity : 1;

	/**
	* The maximum height above navigable ground the player can be to automatically land by flying into the ground.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Flight")
	float FlightAutoLandingMaxHeight;

	/**
	* The maximum angle away from the world down vector that the player can be moving to automatically land by flying into the ground.
	* Unit is pi radians (1 = 90 degrees).
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Flight")
	float FlightAutoLandingMaxAnglePiRadians;

	/**
	* Whether analog stick movement is enabled for Flight.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Flight|Analog")
	uint32 bIsFlightAnalogMovementEnabled : 1;

	/**
	* The axis weight mode for the character when flying and moving using the analog stick.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Flight|Analog")
	EAsgardInputAxisWeightMode FlightAnalogAxisWeightMode;

	/**
	* The dead zone of the input vector when flying and moving using the analog stick
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Flight|Analog")
	float FlightAnalogInputDeadZone;

	/**
	* The max zone of the input vector when flying and moving using the analog stick.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Flight|Analog")
	float FlightAnalogInputMaxZone;

	/**
	* Multiplier that will be applied to the forward / backward input vector when flying and moving forward using the analog stick.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Flight|Analog")
	float FlightAnalogInputForwardMultiplier;

	/**
	* Multiplier that will be applied to the forward / backward input vector when flying and moving backward using the analog stick.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Flight|Analog")
	float FlightAnalogInputBackwardMultiplier;

	/**
	* Multiplier that will be applied to the right / left input vector when flying and moving using the analog stick.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Flight|Analog")
	float FlightAnalogInputStrafeMultiplier;

	/**
	* Whether controller thruster movement is enabled for Flight.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Flight")
	uint32 bFlightControllerThrustersEnabled : 1;

	/**
	* Whether to the player should automatically take off and start flying by engaging the thrusters when walking on the ground.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Flight")
	uint32 bShouldFlightControllerThrustersAutoStartFlight : 1;

	/**
	* Multiplier applied to controller thruster movement input when flying.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Flight")
	float FlightControllerThrusterInputMultiplier;

	// ---------------------------------------------------------
	//	Teleport functions

	/**
	* Attempts to teleport to the target location, using the given teleport mode.
	* Returns whether the initial request succeeeded.
	*/
	UFUNCTION(BlueprintCallable, Category = "AsgardVRCharacter|Movement|Teleportation")
	bool MoveActionTeleportToLocation(const FVector& GoalLocation, EAsgardTeleportMode Mode);

	/**
	* Teleports to the target rotation, using the given teleport mode.
	* Does not return a value because this function should be impossible to fail.
	*/
	UFUNCTION(BlueprintCallable, Category = "AsgardVRCharacter|Movement|Teleportation")
	void MoveActionTeleportToRotation(const FRotator& GoalRotation, EAsgardTeleportMode Mode);

	/**
	* Attempts to teleport to the target location, then rotation, using the given teleport mode.
	* Returns whether the initial teleport to location request succeeeded.
	*/
	UFUNCTION(BlueprintCallable, Category = "AsgardVRCharacter|Movement|Teleportation")
	bool MoveActionTeleportToLocationAndRotation(const FVector& GoalLocation, const FRotator& GoalRotation, EAsgardTeleportMode Mode);

	/**
	* Overiddable function for performing the fade out portion of a teleport.
	* If overriden, be sure to call TeleportFadeOutFinished at the end.
	*/
	UFUNCTION(BlueprintNativeEvent, Category = "AsgardVRCharacter|Movement|Teleportation")
	void TeleportFadeOut();

	/**
	* Called when the fade out portion of a teleport is finished.
	*/
	UFUNCTION(BlueprintCallable, Category = "AsgardVRCharacter|Movement|Teleportation")
	void TeleportFadeOutFinished();

	/**
	* Called when the fade out portion of a teleport is finished.
	*/
	FOnTeleportFadeOutFinished OnTeleportFadeOutFinished;

	/**
	* Overiddable function for performing the fade in portion of a teleport.
	* If overriden, be sure to call TeleportFadeInFinished at the end.
	*/
	UFUNCTION(BlueprintNativeEvent, Category = "AsgardVRCharacter|Movement|Teleportation")
	void TeleportFadeIn();

	/**
	* Called when the fade in portion of a teleport is finished.
	*/
	UFUNCTION(BlueprintCallable, Category = "AsgardVRCharacter|Movement|Teleportation")
	void TeleportFadeInFinished();

	/**
	* Called when the fade in portion of a teleport is finished.
	*/
	FOnTeleportFadeOutFinished OnTeleportFadeInFinished;

	/**
	* Deferred teleport functions for binding to fade delegates.
	*/
	UFUNCTION()
	void DeferredTeleportToLocation();
	UFUNCTION()
	void DeferredTeleportToLocationFinished();
	UFUNCTION()
	void DeferredTeleportToRotation();
	UFUNCTION()
	void DeferredTeleportToRotationFinished();
	UFUNCTION()
	void DeferredTeleportToLocationAndRotation();
	UFUNCTION()
	void DeferredTeleportToLocationAndRotationFinished();

	/**
	* Updates for continuous teleports.
	*/
	void UpdateContinuousTeleportToLocation(float DeltaSeconds);
	void UpdateContinuousTeleportToRotation(float DeltaSeconds);

	/**
	* Traces an arc and attempts to obtain a valid teleport location.
	* Returns true if there was a valid location found otherwise returns false.
	*/
	bool TraceTeleportLocation(
		const FVector& TraceOrigin, 
		const FVector& TraceVelocity, 
		TEnumAsByte<ECollisionChannel> TraceChannel, 
		FVector& OutTeleportLocation,
		FVector& OutImpactPoint,
		TArray<FVector>& OutTracePathPoints);

	/**
	* Radius of the trace when searching for a teleport location.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Teleportation")
	float TeleportTraceRadius;

	/**
	* Maximum simulation time for tracing a teleport location.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Teleportation")
	float TeleportTraceMaxSimTime;
	
	/**
	* Trace channel use for teleport traces.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Teleportation")
	TEnumAsByte<ECollisionChannel> MoveActionTeleportTraceChannel;

protected:
	// Overrides
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/**
	* References to the movement component.
	*/
	UPROPERTY(Category = AsgardVRCharacter, VisibleAnywhere, Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UAsgardVRMovementComponent* AsgardVRMovementRef;

	/**
	* Whether the character can currently teleport.
	*/
	UFUNCTION(BlueprintNativeEvent, Category = AsgardVRCharacter)
	bool CanTeleport();

	/**
	* Whether the character can currently walk.
	*/
	UFUNCTION(BlueprintNativeEvent, Category = AsgardVRCharacter)
	bool CanWalk();

	/**
	* Whether the character can currently fly.
	*/
	UFUNCTION(BlueprintNativeEvent, Category = AsgardVRCharacter)
	bool CanFly();

	/**
	* Called when the character starts walking on the ground.
	*/
	UFUNCTION(BlueprintImplementableEvent, Category = AsgardVRCharacter)
	void OnWalkStarted();

	/**
	* Called when the character stops walking on the ground.
	*/
	UFUNCTION(BlueprintImplementableEvent, Category = AsgardVRCharacter)
	void OnWalkStopped();

	/**
	* Returns whether the player is above the ground up to the max height.
	* If they are moving on the ground, returns true.
	* Otherwise, performs a trace down from the center of the capsule component,
	* with a length of the capsule half height + MaxHeight,
	* and attempts to project the impact point onto the NavMesh.
	*/
	bool ProjectCharacterToVRNavigation(float MaxHeight, FVector& OutProjectedPoint);

	/**
	* Called when the character starts walking on the ground.
	*/
	void StartWalk();

	/**
	* Called when the character stops walking on the ground.
	*/
	void StopWalk();

private:
	// ---------------------------------------------------------
	//	Generic input

	// ---------------------------------------------------------
	//	Axis

	/**
	* The value of MoveForwardAxis.
	*/
	UPROPERTY(BlueprintReadOnly, Category = "AsgardVRCharacter|Input", meta = (AllowPrivateAccess = "true"))
	float MoveForwardAxis;

	/**
	* The value of MoveRightAxis.
	*/
	UPROPERTY(BlueprintReadOnly, Category = "AsgardVRCharacter|Input", meta = (AllowPrivateAccess = "true"))
	float MoveRightAxis;


	// ---------------------------------------------------------
	//	Actions

	/**
	* Whether FlightThrusterLeftAction is pressed.
	*/
	UPROPERTY(BlueprintReadOnly, Category = "AsgardVRCharacter|Input", meta = (AllowPrivateAccess = "true"))
	uint32 bIsFlightThrusterLeftActionPressed : 1;

	/**
	* Whether FlightThrusterRightAction is pressed.
	*/
	UPROPERTY(BlueprintReadOnly, Category = "AsgardVRCharacter|Input", meta = (AllowPrivateAccess = "true"))
	uint32 bIsFlightThrusterRightActionPressed : 1;

	/**
	* Whether TraceTeleportAction is pressed.
	*/
	UPROPERTY(BlueprintReadOnly, Category = "AsgardVRCharacter|Input", meta = (AllowPrivateAccess = "true"))
	uint32 bIsTraceTeleportActionPressed : 1;

	/**
	* Calculates an evenly weighted, circular input vector from two directions and a deadzone.
	* Deadzone must always be less than 1 with a minimum of 0.
	* Maxzone must always be less or equal to 1 and greater or equal to 0.
	*/
	const FVector CalculateCircularInputVector(float AxisX, float AxisY, float DeadZone, float MaxZone) const;

	/**
	* Calculates an input vector weighted to more heavily favor cardinal directions.
	* (up, down, left, right) from two directions, a deadzone, and a maxzone.
	* Deadzone must always be less than 1 with a minimum of 0.
	* Maxzone must always be less or equal to 1 and greater or equal to 0.
	*/
	const FVector CalculateCrossInputVector(float AxisX, float AxisY, float DeadZone, float MaxZone) const;

	/**
	* The the navmesh data when searching for a place on the navmesh this character can stand on.
	*/
	ANavigationData* NavData;

	/**
	* Finds and caches the VR navigation data.
	*/
	void CacheNavData();

	/**
	* Projects a point to the navmesh according to the query extent and filter classes set on this actor.
	*/
	bool ProjectPointToVRNavigation(const FVector& Point, FVector& OutPoint, bool bCheckIfIsOnGround);

	/**
	* Updates movement depending on current settings and player input.
	*/
	void UpdateMovement();

	// ---------------------------------------------------------
	//	Teleporting state
	/**
	* Whether the character is tracing for a teleport location.
	*/
	UPROPERTY(BlueprintReadOnly, Category = "AsgardVRCharacter|Movement|Teleporting", meta = (AllowPrivateAccess = "true"))
	uint32 bIsMoveActionTraceTeleportActive : 1;

	/**
	* Whether the character is currently teleporting to a location.
	*/
	UPROPERTY(BlueprintReadOnly, Category = "AsgardVRCharacter|Movement|Teleporting", meta = (AllowPrivateAccess = "true"))
	uint32 bIsTeleportingToLocation : 1;

	/**
	* Whether the character is currently teleporting to a pivot.
	*/
	UPROPERTY(BlueprintReadOnly, Category = "AsgardVRCharacter|Movement|Teleporting", meta = (AllowPrivateAccess = "true"))
	uint32 bIsTeleportingToRotation : 1;

	/** Whether the most recent traced teleport location was valid. */
	UPROPERTY(BlueprintReadOnly, Category = "AsgardVRCharacter|Movement|Teleporting", meta = (AllowPrivateAccess = "true"))
	uint32 bIsMoveActionTraceTeleportLocationValid: 1;

	/** The orientation mode for the character when performing a move action teleport trace. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AsgardVRCharacter|Movement|Teleporting", meta = (AllowPrivateAccess = "true"))
	EAsgardBinaryHand MoveActionTraceTeleportOrientationMode;

	/**
	* The start location of the character when teleporting to a location.
	*/
	UPROPERTY(BlueprintReadOnly, Category = "AsgardVRCharacter|Movement|Teleporting", meta = (AllowPrivateAccess = "true"))
	FVector TeleportStartLocation;

	/**
	* The goal location of the character when teleporting to a location.
	*/
	UPROPERTY(BlueprintReadOnly, Category = "AsgardVRCharacter|Movement|Teleporting", meta = (AllowPrivateAccess = "true"))
	FVector TeleportGoalLocation;

	/**
	* The Rotation location of the character when teleporting to a location.
	*/
	UPROPERTY(BlueprintReadOnly, Category = "AsgardVRCharacter|Movement|Teleporting", meta = (AllowPrivateAccess = "true"))
	FRotator TeleportStartRotation;

	/**
	* The goal Rotation of the character when teleporting to a Rotation.
	*/
	UPROPERTY(BlueprintReadOnly, Category = "AsgardVRCharacter|Movement|Teleporting", meta = (AllowPrivateAccess = "true"))
	FRotator TeleportGoalRotation;

	/**
	* The mode of the current or last teleport to location performed.
	*/
	UPROPERTY(BlueprintReadOnly, Category = "AsgardVRCharacter|Movement|Teleporting", meta = (AllowPrivateAccess = "true"))
	EAsgardTeleportMode CurrentTeleportToLocationMode;

	/**
	* The mode of the current or last teleport to Rotation performed.
	*/
	UPROPERTY(BlueprintReadOnly, Category = "AsgardVRCharacter|Movement|Teleporting", meta = (AllowPrivateAccess = "true"))
	EAsgardTeleportMode CurrentTeleportToRotationMode;

	/**
	* Used to scale the speed of the teleport to location being performed.
	* Only used with Continuous teleport mode.
	*/
	UPROPERTY(BlueprintReadOnly, Category = "AsgardVRCharacter|Movement|Teleporting", meta = (AllowPrivateAccess = "true"))
	float TeleportToLocationContinuousMultiplier;

	/**
	* The progress of the teleport to location being performed.
	* Only used with Continuous teleport mode.
	*/
	UPROPERTY(BlueprintReadOnly, Category = "AsgardVRCharacter|Movement|Teleporting", meta = (AllowPrivateAccess = "true"))
	float TeleportToLocationContinuousProgress;

	/**
	* Used to scale the speed of the teleport to rotation being performed.
	* Only used with Continuous teleport mode.
	*/
	UPROPERTY(BlueprintReadOnly, Category = "AsgardVRCharacter|Movement|Teleporting", meta = (AllowPrivateAccess = "true"))
	float TeleportToRotationContinuousMultiplier;

	/**
	* The progress of the teleport to rotation being performed.
	* Only used with Continuous teleport mode.
	*/
	UPROPERTY(BlueprintReadOnly, Category = "AsgardVRCharacter|Movement|Teleporting", meta = (AllowPrivateAccess = "true"))
	float TeleportToRotationContinuousProgress;

	/**
	* Whether the most recent teleport was successful.
	*/
	UPROPERTY(BlueprintReadOnly, Category = "AsgardVRCharacter|Movement|Teleporting", meta = (AllowPrivateAccess = "true"))
	uint32 bTeleportToLocationSucceeded : 1;

	/** The most recent location returned by a move action teleport trace. */
	UPROPERTY(BlueprintReadOnly, Category = "AsgardVRCharacter|Movement|Teleporting", meta = (AllowPrivateAccess = "true"))
	FVector MoveActionTraceTeleportLocation;

	/** The most recent impact point returned by a move action teleport trace. */
	UPROPERTY(BlueprintReadOnly, Category = "AsgardVRCharacter|Movement|Teleporting", meta = (AllowPrivateAccess = "true"))
	FVector MoveActionTraceTeleportImpactPoint;

	/** The most recent trace path returned by a move action teleport trace. */
	UPROPERTY(BlueprintReadOnly, Category = "AsgardVRCharacter|Movement|Teleporting", meta = (AllowPrivateAccess = "true"))
	TArray<FVector> MoveActionTraceTeleportTracePath;
	
	/** Direction of teleport trace . */
	UPROPERTY(BlueprintReadOnly, Category = "AsgardVRCharacter|Movement|Teleporting", meta = (AllowPrivateAccess = "true"))
	FVector MoveActionTraceTeleportDirection;

	/**
	* Reference to component used to orient a move action teleport trace.
	*/
	UPROPERTY(BlueprintReadOnly, Category = "AsgardVRCharacter|Movement|Walk", meta = (AllowPrivateAccess = "true"))
	USceneComponent* MoveActionTraceTeleportOrientationComponent;


	/**
	* Setup for move action teleport trace.
	*/
	void StartMoveActionTraceTeleport();

	/**
	* Resets a move action teleport trace and performs a move action if appropriate.
	*/
	void StopMoveActionTraceTeleport();

	/**
	* Updates a move action teleport trace for a teleport location.
	*/
	void UpdateMoveActionTraceTeleport(float DeltaSeconds);


	// ---------------------------------------------------------
	//	Walk state

	/**
	* Whether the character is currently walking.
	*/
	UPROPERTY(BlueprintReadOnly, Category = "AsgardVRCharacter|Movement|Walk", meta = (AllowPrivateAccess = "true"))
	uint32 bIsWalk : 1;

	/**
	* The orientation mode for the character when walking.
	*/
	UPROPERTY(BlueprintReadOnly, Category = "AsgardVRCharacter|Movement|Walk", meta = (AllowPrivateAccess = "true"))
	EAsgardWalkOrientationMode WalkOrientationMode;

	/**
	* Reference to component used to orient walking input.
	*/
	UPROPERTY(BlueprintReadOnly, Category = "AsgardVRCharacter|Movement|Walk", meta = (AllowPrivateAccess = "true"))
	USceneComponent* WalkOrientationComponent;


	// ---------------------------------------------------------
	//	Flight state

	/**
	* The orientation mode for the character when flying and using the analog stick.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AsgardVRCharacter|Movement|Flight|Analog", meta = (AllowPrivateAccess = "true"))
	EAsgardFlightAnalogOrientationMode FlightAnalogOrientationMode;

	/**
	* Whether the left hand Flight thruster is currently active.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AsgardVRCharacter|Movement|Flight", meta = (AllowPrivateAccess = "true"))
	uint32 bIsFlightThrusterLeftActive : 1;

	/**
	* Whether the right hand Flight thruster is currently active.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AsgardVRCharacter|Movement|Flight", meta = (AllowPrivateAccess = "true"))
	uint32 bIsFlightThrusterRightActive : 1;

	/**
	* Reference to component used to orient flying input when moving using the analog stick.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AsgardVRCharacter|Movement|Flight", meta = (AllowPrivateAccess = "true"))
	USceneComponent* FlightAnalogOrientationComponent;
};