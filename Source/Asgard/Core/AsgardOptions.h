// Copyright © 2020 Justin Camden All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Asgard/Core/AsgardOptionsTypes.h"
#include "GameFramework/SaveGame.h"
#include "AsgardOptions.generated.h"

/**
 * Container for saving player options.
 */
UCLASS()
class ASGARD_API UAsgardOptions : public USaveGame
{
	GENERATED_BODY()
public:

	// ---------------------------------------------------------
	//	Controls settings

	/** Whether the controls should be right or left handed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asgard|Options|Controls")
	EAsgardBinaryHand Handedness;

	/** Whether the move axis should start a precision teleport, perform short, repeated teleports, or smoothly walk using the analog stick. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asgard|Options|Controls")
	EAsgardGroundMovementMode DefaultGroundMovementMode;

	/**
	* The orientation mode for the character when walking.
	* Used with both smooth and teleport walk.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asgard|Options|Controls")
	EAsgardOrientationMode WalkOrientationMode;

	/**
	* The method of teleportation used for teleporting to a location.
	* Fade: The camera will fade out, the player will be placed at the goal location, and then the camera will fade back in.
	* Instant: Character is instantly placed at their goal location.
	* SmoothRate: Character is smoothly interpolated to their goal location at a given rate.
	* SmoothTime: Character is smoothly interpolated to their goal location over a given time.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asgard|Options|Controls")
	EAsgardTeleportMode TeleportToLocationDefaultMode;

	/**
	* The method of teleportation used for teleporting to a specific Rotation.
	* Fade: The camera will fade out, the player will be placed at the goal rotation, and then the camera will fade back in.
	* Instant: Character is instantly placed at their goal Rotation.
	* SmoothRate: Character is smoothly interpolated to their goal Rotation at a given rate.
	* SmoothTime: Character is smoothly interpolated to their goal Rotation over a given time.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asgard|Options|Controls")
	EAsgardTeleportMode TeleportToRotationDefaultMode;

	/** How many degrees to turn when performing a teleport turn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asgard|Opotions|Controls", meta = (ClampMin = "0.01", ClampMax = "180.0"))
	float TeleportTurnAngleInterval;
};
