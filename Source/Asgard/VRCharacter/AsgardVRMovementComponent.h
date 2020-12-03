// Copyright © 2020 Justin Camden All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "VRCharacterMovementComponent.h"
#include "AsgardVRMovementComponent.generated.h"

// We use 4 bits for this so a maximum of 16 elements
UENUM(BlueprintType)
enum class EAsgardMovementMode : uint8
{
	C_MOVE_None = 0x00	UMETA(DisplayName = "None"),
	C_MOVE_Walking = 0x01	UMETA(DisplayName = "Walking"),
	C_MOVE_NavWalking = 0x02	UMETA(DisplayName = "Navmesh Walking"),
	C_MOVE_Falling = 0x03	UMETA(DisplayName = "Falling"),
	C_MOVE_Swimming = 0x04	UMETA(DisplayName = "Swimming"),
	C_MOVE_Flying = 0x05		UMETA(DisplayName = "Flying"),
	//C_MOVE_Custom = 0x06	UMETA(DisplayName = "Custom"), // Skip this, could technically get a Custom7 out of using this slot but who needs 7?
	C_MOVE_MAX = 0x07		UMETA(Hidden),
	C_VRMOVE_Climbing = 0x08 UMETA(DisplayName = "Climbing"),
	C_VRMOVE_LowGrav = 0x09 UMETA(DisplayName = "LowGrav"),
	//C_VRMOVE_Spider = 0x0A UMETA(DisplayName = "Spider"),
	C_VRMOVE_Seated = 0x0A UMETA(DisplayName = "Seated"),
	C_VRMOVE_Custom1 = 0x0B UMETA(DisplayName = "Custom1"),
	C_VRMOVE_Custom2 = 0x0C UMETA(DisplayName = "Custom2"),
	C_VRMOVE_Custom3 = 0x0D UMETA(DisplayName = "Custom3"),
	C_VRMOVE_Custom4 = 0x0E UMETA(DisplayName = "Custom4"),
	C_VRMOVE_Custom5 = 0x0F UMETA(DisplayName = "Custom5")
};

/**
 * 
 */
UCLASS()
class ASGARD_API UAsgardVRMovementComponent : public UVRCharacterMovementComponent
{
	GENERATED_BODY()
public:

	UAsgardVRMovementComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;

	/**
	* Sets the current movement mode
	*/
	UFUNCTION(BlueprintCallable, Category = "Asgard|VRMovementComponent")
	void SetAsgardMovementMode(EAsgardMovementMode NewMovementMode);

	/**
	* Gets the current movement mode
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Asgard|VRMovementComponent")
	const EAsgardMovementMode GetAsgardMovementMode() const;

protected:
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

	/**
	* The current movement mode of this component
	*/
	UPROPERTY(BlueprintReadOnly, Category = "Asgard|VRMovementComponent")
	EAsgardMovementMode AsgardMovementMode;

private:

};

FORCEINLINE void UAsgardVRMovementComponent::SetAsgardMovementMode(EAsgardMovementMode NewMovementMode)
{
	AsgardMovementMode = NewMovementMode;
	SetReplicatedMovementMode((EVRConjoinedMovementModes)NewMovementMode);
}

FORCEINLINE const EAsgardMovementMode UAsgardVRMovementComponent::GetAsgardMovementMode() const
{
	return AsgardMovementMode;
}
