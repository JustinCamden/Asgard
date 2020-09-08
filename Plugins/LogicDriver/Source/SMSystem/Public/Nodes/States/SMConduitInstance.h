// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "SMStateInstance.h"
#include "SMConduitInstance.generated.h"


/**
 * The base class for conduit nodes.
 */
UCLASS(Blueprintable, BlueprintType, classGroup = "State Machine", hideCategories = (SMConduitInstance), meta = (DisplayName = "Conduit Class"))
class SMSYSTEM_API USMConduitInstance : public USMStateInstance_Base
{
	GENERATED_BODY()

public:
	USMConduitInstance();

	/** Is this conduit allowed to switch states. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Node Instance")
	bool CanEnterTransition() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Node Instance")
	void OnConduitEntered();
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Node Instance")
	void OnConduitInitialized();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Node Instance")
	void OnConduitShutdown();
	
protected:
	/* Override in native classes to implement. Never call these directly. */

	virtual bool CanEnterTransition_Implementation() const { return false; }
	virtual void OnConduitEntered_Implementation() {}
	virtual void OnConduitInitialized_Implementation() {}
	virtual void OnConduitShutdown_Implementation() {}
	
public:
	/**
	 * This conduit will be evaluated with inbound and outbound transitions.
	 * If any transition fails the entire transition fails. In that case the
	 * state leading to this conduit will not take this transition.
	 *
	 * This makes the behavior similar to AnimGraph conduits.
	 */
	UPROPERTY(EditDefaultsOnly, Category = Conduit)
	bool bEvalWithTransitions;

};

