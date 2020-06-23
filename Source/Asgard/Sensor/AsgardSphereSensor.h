// Copyright © 2020 Justin Camden All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "AsgardSphereSensor.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnComponentDetectedSignature, USceneComponent*, DetectedComponent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnComponentLostSignature, UPrimitiveComponent*, LostComponent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActorDetectedSignature, AActor*, DetectedActor, UPrimitiveComponent*, DetectedComponent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorLostSignature, AActor*, LostActor);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ASGARD_API UAsgardSphereSensor : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAsgardSphereSensor();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**
	* Called when the sensor detects a new component.
	*/
	UPROPERTY(BlueprintAssignable, Category = AsgardSphereSensor)
	FOnComponentDetectedSignature OnComponentDetected;

	/**
	* Called when the sensor can no longer detect a previously detected component.
	*/
	UPROPERTY(BlueprintAssignable, Category = AsgardSphereSensor)
	FOnComponentLostSignature OnComponentLost;

	/**
	* Called when the sensor detects a new actor.
	*/
	UPROPERTY(BlueprintAssignable, Category = AsgardSphereSensor)
	FOnActorDetectedSignature OnActorDetected;

	/**
	* Called when the sensor can no longer detect a previously detected actor.
	*/
	UPROPERTY(BlueprintAssignable, Category = AsgardSphereSensor)
	FOnActorLostSignature OnActorLost;
	
	/**
	* Radius of the sphere sensor.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardSphereSensor)
	float Radius;

	/**
	* Channel to filter for objects on.
	* The sensor will detect blocking objects in this channel.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardSphereSensor)
	TEnumAsByte<ECollisionChannel> DetectionChannel;

	/**
	* Actors to ignore when checking for overlaps.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardSphereSensor)
	TArray<AActor*> ActorsToIgnore;

	/**
	* Whether to filter based on blocking hits.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardSphereSensor)
	uint32 bDetectBlockingHitsOnly : 1;

#if WITH_EDITORONLY_DATA
	/**
	* Whether to draw the sphere sensor.
	*/
	UPROPERTY(EditAnywhere, Category = AsgardSphereSensor)
	uint32 bDebugDrawSensor : 1;
#endif

	/**
	* Returns list of detected components.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = AsgardSphereSensor)
	const TSet<UPrimitiveComponent*> GetDetectedComponents() const;

	/**
	* Returns list of detected actors.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = AsgardSphereSensor)
	const TSet<AActor*> GetDetectedActors() const;

private:
	/**
	* List of detected components.
	*/
	UPROPERTY()
	TSet<UPrimitiveComponent*> DetectedComponents;

	/**
	* List of detected actors.
	*/
	UPROPERTY()
	TSet<AActor*> DetectedActors;
};

// Inline functions
FORCEINLINE const TSet<UPrimitiveComponent*> UAsgardSphereSensor::GetDetectedComponents() const
{
	return DetectedComponents;
}

FORCEINLINE const TSet<AActor*> UAsgardSphereSensor::GetDetectedActors() const
{
	return DetectedActors;
}