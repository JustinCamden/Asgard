// Copyright © 2020 Justin Camden All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "VRCharacter.h"
#include "AsgardPlayerCharacter.generated.h"

UENUM(BlueprintType)
enum class EAsgardWalkingOrientationMode : uint8
{
	LeftController,
	RightController,
	VRCamera,
	CharacterCapsule
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

class UAsgardPlayerMovementComponent;


/**
 * Base class for the player avatar
 */
UCLASS()
class ASGARD_API AAsgardPlayerCharacter : public AVRCharacter
{
	GENERATED_BODY()

public:
	// Overrides
	AAsgardPlayerCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// Input updates
	void OnMoveForwardAxis(float Value);
	void OnMoveRightAxis(float Value);

protected:
	// Overrides
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/**
	* References to the movement component
	*/
	UPROPERTY(Category = AsgardPlayerCharacter, VisibleAnywhere, Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UAsgardPlayerMovementComponent* AsgardMovementComponent;

	/**
	* Whether the character can currently walk
	* By default, returns bIsWalkingEnabled
	*/
	UFUNCTION(BlueprintNativeEvent, Category = AsgardPlayerCharacter)
	bool CanWalk();

	/**
	* Whether the character can currently fly
	* By default, returns bIsFlyingEnabled
	*/
	UFUNCTION(BlueprintNativeEvent, Category = AsgardPlayerCharacter)
	bool CanFly();

	/**
	* Called when the character starts walking on the ground
	*/
	UFUNCTION(BlueprintImplementableEvent, Category = AsgardPlayerCharacter)
	void OnWalkingStarted();

	/**
	* Called when the character stops walking on the ground
	*/
	UFUNCTION(BlueprintImplementableEvent, Category = AsgardPlayerCharacter)
	void OnWalkingStopped();

private:
	/**
	* Stores the forward input axis
	*/
	float MoveForwardAxis;

	/**
	* Stores the right input axis
	*/
	float MoveRightAxis;

	/**
	* Whether walking is enabled for the character
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardPlayerCharacter, meta = (AllowPrivateAccess = "true"))
	uint32 bIsWalkingEnabled : 1;

	/**
	* Whether flying is enabled for the character
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardPlayerCharacter, meta = (AllowPrivateAccess = "true"))
	uint32 bIsFlyingEnabled : 1;

	/**
	* Whether the character is currently walking
	*/
	UPROPERTY(BlueprintReadOnly, Category = AsgardPlayerCharacter, meta = (AllowPrivateAccess = "true"))
	uint32 bIsWalking : 1;

	/**
	 * Updates movement depending on current settings and player input
	 */
	void UpdateMovement();

	/**
	* The axis weight mode for the character when walking
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardPlayerCharacter, meta = (AllowPrivateAccess = "true"))
	EAsgardInputAxisWeightMode WalkingInputAxisWeightMode;

	/**
	* The orientation mode for the character when walking
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AsgardPlayerCharacter, meta = (AllowPrivateAccess = "true"))
	EAsgardWalkingOrientationMode WalkingOrientationMode;

	/**
	* Setter for WalkingOrientationMode
	* Sets the WalkingOrientationComponent to match orientation mode
	*/
	UFUNCTION(BlueprintCallable, Category = AsgardPlayerCharacter)
	void SetWalkingOrientationMode(const EAsgardWalkingOrientationMode NewWalkingOrientationMode);

	/**
	* The dead zone of the input vector when walking 
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardPlayerCharacter, meta = (AllowPrivateAccess = "true"))
	float WalkingInputDeadZone;

	/**
	* The max zone of the input vector when walking
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardPlayerCharacter, meta = (AllowPrivateAccess = "true"))
	float WalkingInputMaxZone;

	/**
	* Multiplier that will be applied to the forward or back input vector when walking forward
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardPlayerCharacter, meta = (AllowPrivateAccess = "true"))
	float WalkingInputForwardMultiplier;

	/**
	* Multiplier that will be applied to the forward or back input vector when walking backward
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardPlayerCharacter, meta = (AllowPrivateAccess = "true"))
	float WalkingInputBackwardMultiplier;

	/**
	* Multiplier that will be applied to the right or left input vector when strafing
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardPlayerCharacter, meta = (AllowPrivateAccess = "true"))
	float WalkingInputStrafeMultiplier;

	/**
	* Reference used to orient walking input
	*/
	USceneComponent* WalkingOrientationComponent;

	/**
	* The absolute world pitch angle of forward vector of the walking orientation component must be less than or equal to this many radians
	* Otherwise, no walking input will be applied
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardPlayerCharacter, meta = (AllowPrivateAccess = "true", ClamMin = "0.0"))
	float WalkingOrientationMaxAbsPitchRadians;

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

	/**
	* Called when the character starts walking on the ground
	*/
	void StartWalking();

	/**
	* Called when the character stops walking on the ground
	*/
	void StopWalking();
};