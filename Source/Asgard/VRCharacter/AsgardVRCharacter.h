// Copyright © 2020 Justin Camden All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "VRCharacter.h"
#include "AsgardVRCharacter.generated.h"

UENUM(BlueprintType)
enum class EAsgardWalkingOrientationMode : uint8
{
	VRCamera,
	LeftController,
	RightController,
	CharacterCapsule
};

UENUM(BlueprintType)
enum class EAsgardFlyingAnalogOrientationMode : uint8
{
	VRCamera,
	LeftController,
	RightController
};

UENUM(BlueprintType)
enum class EAsgarFlyingOrientationMode : uint8
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
	void OnFlyingThrusterLeftActionPressed();
	void OnFlyingThrusterLeftActionReleased();
	void OnFlyingThrusterRightActionPressed();
	void OnFlyingThrusterRightActionReleased();
	void OnToggleFlyingActionPressed();

	/**
	* Setter for WalkingOrientationMode.
	* Sets the WalkingOrientationComponent to match orientation mode.
	*/
	UFUNCTION(BlueprintCallable, Category = AsgardVRCharacter)
	void SetWalkingOrientationMode(const EAsgardWalkingOrientationMode NewWalkingOrientationMode);

	/**
	* Setter for FlyingOrientationMode.
	* Sets the FlyingOrientationComponent to match orientation mode.
	* This is only used with analog stick style flying.
	*/
	UFUNCTION(BlueprintCallable, Category = AsgardVRCharacter)
	void SetFlyingAnalogOrientationMode(const EAsgardFlyingAnalogOrientationMode NewFlyingOrientationMode);


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
	//	Walking settings

	/**
	* Whether walking is enabled for the character.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Walking")
	uint32 bWalkingEnabled : 1;

	/**
	* The axis weight mode for the character when walking.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Walking")
	EAsgardInputAxisWeightMode WalkingInputAxisWeightMode;

	/**
	* The dead zone of the input vector when walking.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Walking")
	float WalkingInputDeadZone;

	/**
	* The max zone of the input vector when walking.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Walking")
	float WalkingInputMaxZone;

	/**
	* Multiplier that will be applied to the forward or back input vector when walking forward.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Walking")
	float WalkingInputForwardMultiplier;

	/**
	* Multiplier that will be applied to the forward or back input vector when walking backward.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Walking")
	float WalkingInputBackwardMultiplier;

	/**
	* Multiplier that will be applied to the right or left input vector when strafing.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Walking")
	float WalkingInputStrafeMultiplier;

	/**
	* The absolute world pitch angle of forward vector of the walking orientation component must be less than or equal to this many radians.
	* Otherwise, no walking input will be applied.
	* Unit is PiRadians (1 = 180 degrees).
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Walking", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WalkingOrientationMaxAbsPitchPiRadians;

	// ---------------------------------------------------------
	//	Flying settings

	/**
	* Whether flying is enabled for the character.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Flying")
	uint32 bFlyingEnabled : 1;

	/**
	* Whether the player should automatically land and stop flying when attempting to move into the ground.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Flying")
	uint32 bFlyingAutoLandingEnabled : 1;

	/**
	* Whether auto landing requires velocity as well as desired input to be facing the ground.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Flying")
	uint32 bFlyingAutoLandingRequiresDownwardVelocity : 1;

	/**
	* The maximum height above navigable ground the player can be to automatically land by flying into the ground.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Flying")
	float FlyingAutoLandingMaxHeight;

	/**
	* The maximum angle away from the world down vector that the player can be moving to automatically land by flying into the ground.
	* Unit is pi radians (1 = 180 degrees).
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Flying")
	float FlyingAutoLandingMaxAnglePiRadians;

	/**
	* Whether analog stick movement is enabled for Flying.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Flying|Analog")
	uint32 bIsFlyingAnalogMovementEnabled : 1;

	/**
	* The orientation mode for the character when flying and using the analog stick.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Flying|Analog")
	EAsgardFlyingAnalogOrientationMode FlyingAnalogOrientationMode;

	/**
	* The axis weight mode for the character when flying and moving using the analog stick.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Flying|Analog")
	EAsgardInputAxisWeightMode FlyingAnalogAxisWeightMode;

	/**
	* The dead zone of the input vector when flying and moving using the analog stick
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Flying|Analog")
	float FlyingAnalogInputDeadZone;

	/**
	* The max zone of the input vector when flying and moving using the analog stick.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Flying|Analog")
	float FlyingAnalogInputMaxZone;

	/**
	* Multiplier that will be applied to the forward / backward input vector when flying and moving forward using the analog stick.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Flying|Analog")
	float FlyingAnalogInputForwardMultiplier;

	/**
	* Multiplier that will be applied to the forward / backward input vector when flying and moving backward using the analog stick.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Flying|Analog")
	float FlyingAnalogInputBackwardMultiplier;

	/**
	* Multiplier that will be applied to the right / left input vector when flying and moving using the analog stick.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Flying|Analog")
	float FlyingAnalogInputStrafeMultiplier;

	/**
	* Whether controller thruster movement is enabled for Flying.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Flying")
	uint32 bFlyingControllerThrustersEnabled : 1;

	/**
	* Whether to the player should automatically take off and start flying by engaging the thrusters when walking on the ground.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Flying")
	uint32 bShouldFlyingControllerThrustersAutoStartFlying : 1;

	/**
	* Multiplier applied to controller thruster movement input when flying.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardVRCharacter|Movement|Flying")
	float FlyingControllerThrusterInputMultiplier;



protected:
	// Overrides
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/**
	* References to the movement component.
	*/
	UPROPERTY(Category = AsgardVRCharacter, VisibleAnywhere, Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UAsgardVRMovementComponent* AsgardVRMovementRef;

	/**
	* Whether the character can currently walk.
	* By default, returns bIsWalkingEnabled.
	*/
	UFUNCTION(BlueprintNativeEvent, Category = AsgardVRCharacter)
	bool CanWalk();

	/**
	* Whether the character can currently fly.
	* By default, returns bIsFlyingEnabled.
	*/
	UFUNCTION(BlueprintNativeEvent, Category = AsgardVRCharacter)
	bool CanFly();

	/**
	* Called when the character starts walking on the ground.
	*/
	UFUNCTION(BlueprintImplementableEvent, Category = AsgardVRCharacter)
	void OnWalkingStarted();

	/**
	* Called when the character stops walking on the ground.
	*/
	UFUNCTION(BlueprintImplementableEvent, Category = AsgardVRCharacter)
	void OnWalkingStopped();

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
	void StartWalking();

	/**
	* Called when the character stops walking on the ground.
	*/
	void StopWalking();

private:
	// ---------------------------------------------------------
	//	Generic input

	// ---------------------------------------------------------
	//	Axis

	/**
	* The value of MoveForwardAxis.
	*/
	UPROPERTY(BlueprintReadOnly, Category = AsgardVRCharacter, meta = (AllowPrivateAccess = "true"))
	float MoveForwardAxis;

	/**
	* The value of MoveRightAxis.
	*/
	UPROPERTY(BlueprintReadOnly, Category = AsgardVRCharacter, meta = (AllowPrivateAccess = "true"))
	float MoveRightAxis;


	// ---------------------------------------------------------
	//	Actions

	/**
	* Whether FlyingThrusterLeftAction is pressed.
	*/
	UPROPERTY(BlueprintReadOnly, Category = AsgardVRCharacter, meta = (AllowPrivateAccess = "true"))
	uint32 bIsFlyingThrusterLeftActionPressed : 1;

	/**
	* Whether FlyingThrusterRightAction is pressed.
	*/
	UPROPERTY(BlueprintReadOnly, Category = AsgardVRCharacter, meta = (AllowPrivateAccess = "true"))
	uint32 bIsFlyingThrusterRightActionPressed : 1;

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
	//	Walking state

	/**
	* Whether the character is currently walking.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AsgardVRCharacter|Movement|Walking", meta = (AllowPrivateAccess = "true"))
	uint32 bIsWalking : 1;

	/**
	* The orientation mode for the character when walking.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AsgardVRCharacter|Movement|Walking", meta = (AllowPrivateAccess = "true"))
	EAsgardWalkingOrientationMode WalkingOrientationMode;

	/**
	* Reference to component used to orient walking input.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AsgardVRCharacter|Movement|Walking", meta = (AllowPrivateAccess = "true"))
	USceneComponent* WalkingOrientationComponent;


	// ---------------------------------------------------------
	//	Flying state

	/**
	* Whether the left hand Flying thruster is currently active.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AsgardVRCharacter|Movement|Flying", meta = (AllowPrivateAccess = "true"))
	uint32 bIsFlyingThrusterLeftActive : 1;

	/**
	* Whether the right hand Flying thruster is currently active.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AsgardVRCharacter|Movement|Flying", meta = (AllowPrivateAccess = "true"))
	uint32 bIsFlyingThrusterRightActive : 1;

	/**
	* Reference to component used to orient flying input when moving using the analog stick.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AsgardVRCharacter|Movement|Flying", meta = (AllowPrivateAccess = "true"))
	USceneComponent* FlyingAnalogOrientationComponent;
};