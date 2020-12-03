// Copyright © 2020 Justin Camden All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AsgardAbilityComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAbilityActivatedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAbilityDeactivatedSignature);


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ASGARD_API UAsgardAbilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAsgardAbilityComponent();

	/** Activates the ability, including any necessary initialization.*/
	UFUNCTION(BlueprintCallable, Category = "Asgard|AbilityComponent")
	void ActivateAbility();

	/** Called when the ability is activated. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Asgard|AbilityComponent")
	void OnAbilityActivated();

	/** Called when the ability is activated. */
	UPROPERTY(BlueprintAssignable, Category = "Asgard|AbilityComponent", meta = (DisplayName = "On Ability Activated"))
	FOnAbilityActivatedSignature NotifyAbilityActivated;

	/** Deactivates the ability, including any necessary de-initialization.*/
	UFUNCTION(BlueprintCallable, Category = "Asgard|AbilityComponent")
	void DeactivateAbility();

	/** Called when the ability is deactivated. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Asgard|AbilityComponent")
	void OnAbilityDeactivated();

	/** Called when the ability is deactivated. */
	UPROPERTY(BlueprintAssignable, Category = "Asgard|AbilityComponent", meta = (DisplayName = "On Ability Deactivated"))
	FOnAbilityDeactivatedSignature NotifyAbilityDeactivated;

private:
	/** Whether this ability is currently active. */
	UPROPERTY(BlueprintReadOnly, Category = "Asgard|AbilityComponent", meta = (AllowPrivateAccess = "true"))
	bool bIsAbilityActive;
};
