// Copyright © 2020 Justin Camden All Rights Reserved


#include "AsgardOptionsSubsystem.h"

void UAsgardOptionsSubsystem::SetDefaultGroundMovementMode(const EAsgardGroundMovementMode NewDefaultGroundMovementMode)
{
	if (NewDefaultGroundMovementMode != DefaultGroundMovementMode)
	{
		DefaultGroundMovementMode = NewDefaultGroundMovementMode;
		OnDefaultGroundMovementModeChanged.Broadcast(DefaultGroundMovementMode);
	}

	return;
}

void UAsgardOptionsSubsystem::SetWalkOrientationMode(const EAsgardOrientationMode NewWalkOrientationMode)
{
	if (NewWalkOrientationMode != WalkOrientationMode)
	{
		WalkOrientationMode = NewWalkOrientationMode;
		OnWalkOrientationModeChanged.Broadcast(WalkOrientationMode);
	}

	return;
}

void UAsgardOptionsSubsystem::SetTeleportToLocationDefaultMode(const EAsgardTeleportMode NewTeleportToLocationDefaultMode)
{
	if (NewTeleportToLocationDefaultMode != TeleportToLocationDefaultMode)
	{
		TeleportToLocationDefaultMode = NewTeleportToLocationDefaultMode;
		OnTeleportToLocationDefaultModeChanged.Broadcast(TeleportToLocationDefaultMode);
	}

	return;
}

void UAsgardOptionsSubsystem::SetTeleportToRotationDefaultMode(const EAsgardTeleportMode NewTeleportToRotationDefaultMode)
{
	if (TeleportToRotationDefaultMode != NewTeleportToRotationDefaultMode)
	{
		TeleportToRotationDefaultMode = NewTeleportToRotationDefaultMode;
		OnTeleportToRotationDefaultModeChanged.Broadcast(TeleportToRotationDefaultMode);
	}

	return;
}

void UAsgardOptionsSubsystem::SetTeleportTurnAngleInterval(const float NewTeleportTurnAngleInterval)
{
	float ProposedTurnAngle = FMath::Clamp(NewTeleportTurnAngleInterval, 0.0f, 180.0f);
	if (ProposedTurnAngle != TeleportTurnAngleInterval)
	{
		TeleportTurnAngleInterval = ProposedTurnAngle;
		OnTeleportTurnAngleIntervalChanged.Broadcast(ProposedTurnAngle);
	}

	return;
}
