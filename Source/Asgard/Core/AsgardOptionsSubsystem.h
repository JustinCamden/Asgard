// Copyright © 2020 Justin Camden All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "AsgardOptionsTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AsgardOptionsSubsystem.generated.h"

// Delegate declarations
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGroundMovementModeSignature, EAsgardGroundMovementMode, GroundMovementMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOrientationModeSignature, EAsgardOrientationMode, OrientationMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTeleportModeSignature, EAsgardTeleportMode, TeleportMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTeleportTurnAngleIntervalSignature, float, AngleInterval);

/**
 * System for central management of game settings.
 */
UCLASS()
class ASGARD_API UAsgardOptionsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ---------------------------------------------------------
	//	Movement settings
	UPROPERTY(BlueprintAssignable)
	FOnGroundMovementModeSignature OnDefaultGroundMovementModeChanged;

	UPROPERTY(BlueprintAssignable)
	FOnOrientationModeSignature OnWalkOrientationModeChanged;

	UPROPERTY(BlueprintAssignable)
	FOnTeleportModeSignature OnTeleportToLocationDefaultModeChanged;

	UPROPERTY(BlueprintAssignable)
	FOnTeleportModeSignature OnTeleportToRotationDefaultModeChanged;

	UPROPERTY(BlueprintAssignable)
	FOnTeleportTurnAngleIntervalSignature OnTeleportTurnAngleIntervalChanged;

	// ---------------------------------------------------------
	//	Getters

	/** Whether the move axis should start a precision teleport, perform short, repeated teleports, or smoothly walk using the analog stick. */
	UFUNCTION(BlueprintCallable, Category = "Asgard|OptionsSubSystem|Movement")
	EAsgardGroundMovementMode GetDefaultGroundMovementMode() const { return DefaultGroundMovementMode; }

	/**
	* The orientation mode for the character when walking.
	* Used with both smooth and teleport walk.
	*/
	UFUNCTION(BlueprintCallable, Category = "Asgard|OptionsSubSystem|Movement")
	EAsgardOrientationMode GetWalkOrientationMode() const { return WalkOrientationMode; }

	/**
	* The method of teleportation used for teleporting to a location.
	* Fade: The camera will fade out, the player will be placed at the goal location, and then the camera will fade back in.
	* Instant: Character is instantly placed at their goal location.
	* SmoothRate: Character is smoothly interpolated to their goal location at a given rate.
	* SmoothTime: Character is smoothly interpolated to their goal location over a given time.
	*/
	UFUNCTION(BlueprintCallable, Category = "Asgard|OptionsSubSystem|Teleport")
	EAsgardTeleportMode GetTeleportToLocationDefaultMode() const { return TeleportToLocationDefaultMode; }

	/**
	* The method of teleportation used for teleporting to a specific Rotation.
	* Fade: The camera will fade out, the player will be placed at the goal rotation, and then the camera will fade back in.
	* Instant: Character is instantly placed at their goal Rotation.
	* SmoothRate: Character is smoothly interpolated to their goal Rotation at a given rate.
	* SmoothTime: Character is smoothly interpolated to their goal Rotation over a given time.
	*/
	UFUNCTION(BlueprintCallable, Category = "Asgard|OptionsSubSystem|Teleport")
	EAsgardTeleportMode GetTeleportToRotationDefaultMode() const { return TeleportToRotationDefaultMode; }

	/** How many degrees to turn when performing a teleport turn. */
	UFUNCTION(BlueprintCallable, Category = "Asgard|OptionsSubSystem|Teleport")
	float GetTeleportTurnAngleInterval() const { return TeleportTurnAngleInterval; }


	// ---------------------------------------------------------
	//	Setters

	/** Whether the move axis should start a precision teleport, perform short, repeated teleports, or smoothly walk using the analog stick. */
	UFUNCTION(BlueprintCallable, Category = "Asgard|OptionsSubSystem|Movement")
	void SetDefaultGroundMovementMode(const EAsgardGroundMovementMode NewDefaultGroundMovementMode);

	/**
	* The orientation mode for the character when walking.
	* Used with both smooth and teleport walk.
	*/
	UFUNCTION(BlueprintCallable, Category = "Asgard|OptionsSubSystem|Movement")
	void SetWalkOrientationMode(const EAsgardOrientationMode NewWalkOrientationMode);

	/**
	* The method of teleportation used for teleporting to a location.
	* Fade: The camera will fade out, the player will be placed at the goal location, and then the camera will fade back in.
	* Instant: Character is instantly placed at their goal location.
	* SmoothRate: Character is smoothly interpolated to their goal location at a given rate.
	* SmoothTime: Character is smoothly interpolated to their goal location over a given time.
	*/
	UFUNCTION(BlueprintCallable, Category = "Asgard|OptionsSubSystem|Teleport")
	void SetTeleportToLocationDefaultMode(const EAsgardTeleportMode NewTeleportToLocationDefaultMode);

	/**
	* The method of teleportation used for teleporting to a specific Rotation.
	* Fade: The camera will fade out, the player will be placed at the goal rotation, and then the camera will fade back in.
	* Instant: Character is instantly placed at their goal Rotation.
	* SmoothRate: Character is smoothly interpolated to their goal Rotation at a given rate.
	* SmoothTime: Character is smoothly interpolated to their goal Rotation over a given time.
	*/
	UFUNCTION(BlueprintCallable, Category = "Asgard|OptionsSubSystem|Teleport")
	void SetTeleportToRotationDefaultMode(const EAsgardTeleportMode NewTeleportToRotationDefaultMode);
	
	/** How many degrees to turn when performing a teleport turn. */
	UFUNCTION(BlueprintCallable, Category = "Asgard|OptionsSubSystem|Teleport")
	void SetTeleportTurnAngleInterval(const float NewTeleportTurnAngleInterval);


private:
	// ---------------------------------------------------------
	//	Movement settings

	/** Whether the move axis should start a precision teleport, perform short, repeated teleports, or smoothly walk using the analog stick. */
	UPROPERTY(EditDefaultsOnly, Category = "Asgard|OptionsSubSystem|Player|Movement")
	EAsgardGroundMovementMode DefaultGroundMovementMode;

	/**
	* The orientation mode for the character when walking.
	* Used with both smooth and teleport walk.
	*/
	UPROPERTY(EditDefaultsOnly, Category = "Asgard|OptionsSubSystem|Player|Movement")
	EAsgardOrientationMode WalkOrientationMode;


	// ---------------------------------------------------------
	//	Teleport settings

	/**
	* The method of teleportation used for teleporting to a location.
	* Fade: The camera will fade out, the player will be placed at the goal location, and then the camera will fade back in.
	* Instant: Character is instantly placed at their goal location.
	* SmoothRate: Character is smoothly interpolated to their goal location at a given rate.
	* SmoothTime: Character is smoothly interpolated to their goal location over a given time.
	*/
	UPROPERTY(EditDefaultsOnly, Category = "Asgard|OptionsSubSystem|Player|Teleport")
	EAsgardTeleportMode TeleportToLocationDefaultMode;

	/**
	* The method of teleportation used for teleporting to a specific Rotation.
	* Fade: The camera will fade out, the player will be placed at the goal rotation, and then the camera will fade back in.
	* Instant: Character is instantly placed at their goal Rotation.
	* SmoothRate: Character is smoothly interpolated to their goal Rotation at a given rate.
	* SmoothTime: Character is smoothly interpolated to their goal Rotation over a given time.
	*/
	UPROPERTY(EditDefaultsOnly, Category = "Asgard|OptionsSubSystem|Player|Teleport")
	EAsgardTeleportMode TeleportToRotationDefaultMode;

	/** How many degrees to turn when performing a teleport turn. */
	UPROPERTY(EditDefaultsOnly, Category = "Asgard|OptionsSubSystem|Player|Teleport", meta = (ClampMin = "0.01", ClampMax = "180.0"))
	float TeleportTurnAngleInterval;
};
