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
enum class EAsgardFlyingControlMode : uint8
{
	AnalogStick,
	ControllerThrusters
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

	// Input updates
	void OnMoveForwardAxis(float Value);
	void OnMoveRightAxis(float Value);

	/**
	* Setter for WalkingOrientationMode
	* Sets the WalkingOrientationComponent to match orientation mode
	*/
	UFUNCTION(BlueprintCallable, Category = AsgardVRCharacter)
	void SetWalkingOrientationMode(const EAsgardWalkingOrientationMode NewWalkingOrientationMode);

	/**
	* Setter for FlyingOrientationMode
	* Sets the FlyingOrientationComponent to match orientation mode
	* This is only used with analog stick style flying
	*/
	UFUNCTION(BlueprintCallable, Category = AsgardVRCharacter)
	void SetFlyingOrientationMode(const EAsgardFlyingAnalogOrientationMode NewFlyingOrientationMode);

protected:
	// Overrides
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/**
	* References to the movement component
	*/
	UPROPERTY(Category = AsgardVRCharacter, VisibleAnywhere, Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UAsgardVRMovementComponent* AsgardMovementComponent;

	/**
	* Whether the character can currently walk
	* By default, returns bIsWalkingEnabled
	*/
	UFUNCTION(BlueprintNativeEvent, Category = AsgardVRCharacter)
	bool CanWalk();

	/**
	* Whether the character can currently fly
	* By default, returns bIsFlyingEnabled
	*/
	UFUNCTION(BlueprintNativeEvent, Category = AsgardVRCharacter)
	bool CanFly();

	/**
	* Called when the character starts walking on the ground
	*/
	UFUNCTION(BlueprintImplementableEvent, Category = AsgardVRCharacter)
	void OnWalkingStarted();

	/**
	* Called when the character stops walking on the ground
	*/
	UFUNCTION(BlueprintImplementableEvent, Category = AsgardVRCharacter)
	void OnWalkingStopped();

	/**
	* Returns whether the player is above the ground up to the max height
	* If they are moving on the ground, returns true
	* Otherwise, performs a trace down from the center of the capsule component,
	* with a length of the capsule half height + MaxHeight,
	* and attempts to project the impact point onto the NavMesh
	*/
	bool IsAboveNavMesh(float MaxHeight, FVector& OutProjectedPoint);

private:

	// ---------------------------------------------------------
	//	Generic input

	/**
	* Stores the forward input axis
	*/
	float MoveForwardAxis;

	/**
	* Stores the right input axis
	*/
	float MoveRightAxis;

	/**
	* Calculates an evenly weighted, circular input vector from two directions and a deadzone
	* Deadzone must always be less than 1 with a minimum of 0
	* Maxzone must always be less or equal to 1 and greater or equal to 0
	*/
	const FVector CalculateCircularInputVector(float AxisX, float AxisY, float DeadZone, float MaxZone) const;

	/**
	* Calculates an input vector weighted to more heavily favor cardinal directions
	* (up, down, left, right) from two directions, a deadzone, and a maxzone
	* Deadzone must always be less than 1 with a minimum of 0
	* Maxzone must always be less or equal to 1 and greater or equal to 0
	*/
	const FVector CalculateCrossInputVector(float AxisX, float AxisY, float DeadZone, float MaxZone) const;


	// ---------------------------------------------------------
	//	Navigation

	/** The the navmesh data when searching for a place on the navmesh this character can stand on */
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AsgardVRCharacter, meta = (AllowPrivateAccess = "true"))
	ANavigationData* NavData;

	/**
	* The filter class when searching for a place on the navmesh this character can stand on
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AsgardVRCharacter, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UNavigationQueryFilter> NavFilterClass;

	/**
	* The extent of the query when searching for a place on the navmesh this character can stand on
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AsgardVRCharacter, meta = (AllowPrivateAccess = "true"))
	FVector NavQueryExtent;

	/**
	* Finds and caches the VR navigation data
	*/
	void CacheNavData();

	/**
	* Projects a point to the navmesh according to the query extent and filter classes set on this actor.
	*/
	bool ProjectPointToVRNavigation(const FVector& Point, FVector& OutPoint, bool bCheckIfIsOnGround);

	/**
	* Updates movement depending on current settings and player input
	*/
	void UpdateMovement();


	// ---------------------------------------------------------
	//	Movement modes enabled

	/**
	* Whether walking is enabled for the character
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardVRCharacter, meta = (AllowPrivateAccess = "true"))
	uint32 bIsWalkingEnabled : 1;

	/**
	* Whether flying is enabled for the character
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardVRCharacter, meta = (AllowPrivateAccess = "true"))
	uint32 bIsFlyingEnabled : 1;


	// ---------------------------------------------------------
	//	Walking state

	/**
	* Whether the character is currently walking
	*/
	UPROPERTY(BlueprintReadOnly, Category = AsgardVRCharacter, meta = (AllowPrivateAccess = "true"))
	uint32 bIsWalking : 1;


	// ---------------------------------------------------------
	//	Flying state

	/**
	* Whether the left hand thruster is currently engaged for flying
	*/
	UPROPERTY(BlueprintReadOnly, Category = AsgardVRCharacter, meta = (AllowPrivateAccess = "true"))
	uint32 bIsLeftThrusterActive : 1;

	/**
	* Whether the left hand thruster is currently engaged for flying
	*/
	UPROPERTY(BlueprintReadOnly, Category = AsgardVRCharacter, meta = (AllowPrivateAccess = "true"))
	uint32 bIsRightThrusterActive : 1;


	// ---------------------------------------------------------
	//	Walking settings

	/**
	* The axis weight mode for the character when walking
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardVRCharacter, meta = (AllowPrivateAccess = "true"))
	EAsgardInputAxisWeightMode WalkingInputAxisWeightMode;

	/**
	* The orientation mode for the character when walking
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AsgardVRCharacter, meta = (AllowPrivateAccess = "true"))
	EAsgardWalkingOrientationMode WalkingOrientationMode;

	/**
	* Reference to component used to orient walking input
	*/
	USceneComponent* WalkingOrientationComponent;

	/**
	* The dead zone of the input vector when walking
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardVRCharacter, meta = (AllowPrivateAccess = "true"))
	float WalkingInputDeadZone;

	/**
	* The max zone of the input vector when walking
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardVRCharacter, meta = (AllowPrivateAccess = "true"))
	float WalkingInputMaxZone;

	/**
	* Multiplier that will be applied to the forward or back input vector when walking forward
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardVRCharacter, meta = (AllowPrivateAccess = "true"))
	float WalkingInputForwardMultiplier;

	/**
	* Multiplier that will be applied to the forward or back input vector when walking backward
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardVRCharacter, meta = (AllowPrivateAccess = "true"))
	float WalkingInputBackwardMultiplier;

	/**
	* Multiplier that will be applied to the right or left input vector when strafing
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardVRCharacter, meta = (AllowPrivateAccess = "true"))
	float WalkingInputStrafeMultiplier;

	/**
	* The absolute world pitch angle of forward vector of the walking orientation component must be less than or equal to this many radians
	* Otherwise, no walking input will be applied
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardVRCharacter, meta = (AllowPrivateAccess = "true", ClamMin = "0.0"))
	float WalkingOrientationMaxAbsPitchRadians;

	/**
	* Called when the character starts walking on the ground
	*/
	void StartWalking();

	/**
	* Called when the character stops walking on the ground
	*/
	void StopWalking();


	// ---------------------------------------------------------
	//	Flying settings

	/**
	* The control mode for flying
	* AnalogStick: Movement will be controlled by an analog stick and oriented to a scene component
	* ControllerThrusters: Movement will be controlled by left and right thrusters oriented and engaged by each controller
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AsgardVRCharacter, meta = (AllowPrivateAccess = "true"))
	EAsgardFlyingControlMode FlyingControlMode;

	/**
	* The orientation mode for the character when flying
	* This is only used with analog stick style flying
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AsgardVRCharacter, meta = (AllowPrivateAccess = "true"))
	EAsgardFlyingAnalogOrientationMode FlyingAnalogOrientationMode;

	/**
	* Reference to component used to orient flying input
	* This is only used with analog stick style flying
	*/
	USceneComponent* FlyingAnalogOrientationComponent;

	/**
	* Whether to allow the player to automatically take off and start flying by engaging the thrusters
	* Only used with ControllerThrusters FlyingControlMode
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardVRCharacter, meta = (AllowPrivateAccess = "true"))
	uint32 bIsThrusterAutoStartFlyingEnabled: 1;

	/**
	* Whether to allow the player to automatically land and stop flying by disengaging the thrusters when over the ground.
	* Only used with ControllerThrusters FlyingControlMode
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardVRCharacter, meta = (AllowPrivateAccess = "true"))
	uint32 bIsThrusterAutoStopFlyingEnabled : 1;

};