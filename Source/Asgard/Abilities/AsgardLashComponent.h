// Copyright © 2020 Justin Camden All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "AsgardLashComponent.generated.h"

// Stats group
DECLARE_STATS_GROUP(TEXT("AsgardLash"), STATGROUP_ASGARD_Lash, STATCAT_Advanced);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnComponentEnterLashSignature, USceneComponent*, DetectedComponent, bool, bBlockingHit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnComponentExitLashSignature, UPrimitiveComponent*, LostComponent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnActorEnterLashSignature, AActor*, DetectedActor, UPrimitiveComponent*, DetectedComponent, bool, bBlockingHit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorExitLashSignature, AActor*, LostActor);


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ASGARD_API UAsgardLashComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAsgardLashComponent();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**
	* Whether the lash is currently extended.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardLashComponent)
	bool bLashExtended;

	/**
	* Called when the sensor detects a new component.
	*/
	UPROPERTY(BlueprintAssignable, Category = "AsgardLashComponent|Collision")
	FOnComponentEnterLashSignature OnComponentEnterLash;

	/**
	* Called when the sensor can no longer detect a previously detected component.
	*/
	UPROPERTY(BlueprintAssignable, Category = "AsgardLashComponent|Collision")
	FOnComponentExitLashSignature OnComponentExitLash;

	/**
	* Called when the sensor detects a new actor.
	*/
	UPROPERTY(BlueprintAssignable, Category = "AsgardLashComponent|Collision")
	FOnActorEnterLashSignature OnActorEnterLash;

	/**
	* Called when the sensor can no longer detect a previously detected actor.
	*/
	UPROPERTY(BlueprintAssignable, Category = "AsgardLashComponent|Collision")
	FOnActorExitLashSignature OnActorExitLash;

	/**
	* Reduces velocity by this percentage per second.
	* Does not affect gravity.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardLashComponent)
	FVector Damping;

	/**
	* The amount of gravity to apply to each lash segment velocity per second.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardLashComponent)
	FVector Gravity;

	/**
	* The number of physics simulations per second.
	* Higher numbers will result in more accurate simulation but will be more costly.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardLashComponent, meta = (ClampMin = "30.0"))
	int32 PhysicsStepsPerSecond;

	/**
	* How much of a correction is apply to the 'child' point further down down the lash when solving constraints, on a scale of 0-1.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardLashComponent, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ChildPointCorrectionWeight;

	/**
	* List of actors to ignore collision with.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardLashComponent|Collision")
	TArray<AActor*> IgnoredActors;

	/**
	* When EnableCollision, the lash will trace for objects in this channel.
	* Overlapping objects and Blocking objects will trigger OnComponentDetected and OnActorDetected events.
	* When BlockedByObjects is true, blocking objects will force the lash to reposition its points to compensate.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardLashComponent|Collision")
	TEnumAsByte<ECollisionChannel> CollisionChannel;

	/**
	* Radius of the lash segments when checking for collision.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardLashComponent|Collision")
	float CollisionRadius;

	/**
	* Used to offset lash points from blocking collisions with when BlockedByObjects is true.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardLashComponent|Collision")
	float CollisionSkinWidth;

	/**
	* Whether collision detection on the lash is enabled.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardLashComponent|Collision", meta = (AllowPrivateAccess = "true"))
	bool bEnableCollision;

	/**
	* Whether collision on the lash should automatically ignore the owning actor.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardLashComponent|Collision")
	bool bAutoIgnoreOwner;

	/**
	* Whether blocking hits should affect the position of the lash points.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AsgardLashComponent|Collision")
	bool bBlockedByObjects;

#if WITH_EDITORONLY_DATA
	/**
	* Whether to debug draw the lash.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardLashComponent)
	bool bDebugDrawLash;
#endif

	/**
	* Returns the list of components within the lash's segments.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AsgardLashComponent|Collision")
	const TSet<UPrimitiveComponent*>& GetComponentsInLash() const { return ComponentsInLash; }

	/**
	* Returns the list of detected within the lash's segments.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AsgardLashComponent|Collision")
	const TSet<AActor*>& GetActorsInLash() const { return ActorsInLash; }

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	/**
	* Remainder from the last time physics were simulated.
	*/
	float PhysicsStepRemainder;

	/**
	* The points in the lash at the end of the last simulation step.
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = AsgardLashComponent, meta = (AllowPrivateAccess = "true"))
	TArray<FVector> LashPointLastLocations;

	/**
	* The points in the lash at the beginning of the frame.
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = AsgardLashComponent, meta = (AllowPrivateAccess = "true"))
	TArray<FVector> LashPointFrameStartLocations;

	/**
	* The points in the lash as they are currently located.
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = AsgardLashComponent, meta = (AllowPrivateAccess = "true"))
	TArray<FVector> LashPointCurrentLocations;

	/**
	* Component blocking each lash segment (if any).
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = AsgardLashComponent, meta = (AllowPrivateAccess = "true"))
	TArray<UPrimitiveComponent*> LashPointBlockingComponents;

	/**
	* The current number of lash segments.
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = AsgardLashComponent, meta = (AllowPrivateAccess = "true"))
	int32 NumLashSegments;

	/**
	* The maximum number of lash points.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AsgardLashComponent, meta = (AllowPrivateAccess = "true"))
	int32 MaxLashSegments;

	/**
	* The maximum length of each lash segment.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AsgardLashComponent, meta = (AllowPrivateAccess = "true"))
	float LashSegmentMaxLength;

	/**
	* How much the lash should shorten every frame while shrinking.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AsgardLashComponent, meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float LashShrinkSpeed;

	/**
	* When shrinking the lash, the length of the first segment is stored here and used to cap the length of the last segment during the constraints step.
	* This prevents velocity from re-lengthening the lash while it is shrinking.
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = AsgardLashComponent, meta = (AllowPrivateAccess = "true"))
	float LashShrinFirstSegmentLastLength;

	/**
	* The minimum length of the last lash segment before the lash can 'grow' by adding another segment at the end.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AsgardLashComponent, meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float LashGrowthMinSegmentLength;

	/**
	* The maximum length of the first lash segment before the lash can 'shrink' by removing it.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AsgardLashComponent, meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float LashShrinkMaxSegmentLength;

	/**
	* List of detected components.
	*/
	UPROPERTY()
	TSet<UPrimitiveComponent*> ComponentsInLash;

	/**
	* List of detected actors.
	*/
	UPROPERTY()
	TSet<AActor*> ActorsInLash;

	/**
	* Adds a point at the end of the lash.
	*/
	void AddLashSegmentAtEnd();

	/**
	* Removes a point at the end of the lash.
	*/
	void RemoveLashSegmentFromFront();

	/**
	* Updates the length of the lash depending on whether it is active.
	* Returns whether the lash has fully shrunk.
	*/
	bool ShrinkLash(float DeltaTime);

	/**
	* Calculates the new current positions for each point, based on velocity from the previous frame.
	*/
	void ApplyVelocityToLashPoints(float DeltaTime);

	/**
	* Applies constraints to individual points in the lash, starting from the front.
	*/
	void ApplyConstraintsToLashPointsFromFront();

	/**
	* Applies constraints to individual points in the lash, starting from the back.
	*/
	void ApplyConstraintsToLashPointsFromBack();

	/**
	* Performs a trace to check for collision.
	* Does not adjust the position of the lash points.
	*/
	void DetectActorsInLashSegments();

	/**
	* Performs a trace to check for collision.
	* Adjusts the position of the lash in the case of blocking hits.
	*/
	void CollideWithActorsInLashSegments(float DeltaTime, float PhysicsStepTime);
		
};
