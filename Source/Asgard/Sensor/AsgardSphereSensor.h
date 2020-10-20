// Copyright © 2020 Justin Camden All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "AsgardSphereSensor.generated.h"

// Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnComponentDetectedSignature, USceneComponent*, DetectedComponent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnComponentLostSignature, UPrimitiveComponent*, LostComponent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActorDetectedSignature, AActor*, DetectedActor, UPrimitiveComponent*, DetectedComponent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorLostSignature, AActor*, LostActor);

// Stats group
DECLARE_STATS_GROUP(TEXT("AsgardSphereSensor"), STATGROUP_ASGARD_SphereSensor, STATCAT_Advanced);

/**
 *	Component used for one-sided detection, so other objects can be detected without triggering hit or overlap events.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ASGARD_API UAsgardSphereSensor : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAsgardSphereSensor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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
	TArray<AActor*> IgnoredActors;

	/**
	* Whether to filter based on blocking hits.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardSphereSensor)
	bool bDetectBlockingHitsOnly;

	/**
	* Whether to automatically add the owner actor to ignored actors for every trace.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardSphereSensor)
	bool bAutoIgnoreOwner;

	/**
	* Whether to use async overlap tests.
	* If disabled, overlap tests will be more accurate, but slower.
	* If enabled, overlap tests will be one frame out of date, but faster.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardSphereSensor)
	bool bUseAsyncOverlapTests;

#if WITH_EDITORONLY_DATA
	/**
	* Whether to debug draw the sphere sensor.
	*/
	UPROPERTY(EditAnywhere, Category = AsgardSphereSensor)
	bool bDebugDrawSensor;
#endif

	/**
	* Returns list of detected components.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = AsgardSphereSensor)
	const TSet<UPrimitiveComponent*>& GetDetectedComponents() const { return DetectedComponents; }

	/**
	* Returns list of detected actors.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = AsgardSphereSensor)
	const TSet<AActor*>& GetDetectedActors() const { return DetectedActors; }

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

	/**
	* Handle for async Overlap, if enabled.
	*/
	FTraceHandle AsyncOverlapTestHandle;

	/**
	* Delegate for async Overlap, if enabled.
	*/
	FOverlapDelegate AsyncOverlapTestDelegate;

	/**
	* Called when the async Overlap completes.
	*/
	void OnAsyncOverlapTestCompleted(const FTraceHandle& Handle, FOverlapDatum& Data);

	/**
	* Processes overlaps and updates state accordingly.
	*/
	void ProcessOverlaps(TArray<FOverlapResult>& Overlaps, bool bBlockingHits);
};
