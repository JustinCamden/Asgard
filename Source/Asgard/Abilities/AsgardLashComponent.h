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

	// ---------------------------------------------------------
	//	Physics settings

	/**
	* Reduces velocity by this percentage per second.
	* Does not affect gravity.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asgard|LashComponent")
	FVector Damping;

	/** The amount of gravity to apply to each lash segment velocity per second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asgard|LashComponent")
	FVector Gravity;

	// ---------------------------------------------------------
	//	Collision settings

	/** List of actors to ignore collision with. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asgard|LashComponent|Collision")
	TArray<AActor*> IgnoredActors;

	/**
	* When EnableCollision, the lash will trace for objects in this channel.
	* Overlapping objects and Blocking objects will trigger OnComponentDetected and OnActorDetected events.
	* When BlockedByObjects is true, blocking objects will force the lash to reposition its points to compensate.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asgard|LashComponent|Collision")
	TEnumAsByte<ECollisionChannel> CollisionChannel;

	/** How much of a 'sliding' velocity correction is to a point on a lash when it collides with an object. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asgard|LashComponent", meta = (ClampMin = "0.5", ClampMax = "1.0"))
	float SlidingVelocityCorrectionWeight;

	// ---------------------------------------------------------
	//	Collision delegates

	/** Called when the sensor detects a new component. */
	UPROPERTY(BlueprintAssignable, Category = "Asgard|LashComponent|Collision")
	FOnComponentEnterLashSignature OnComponentEnterLash;

	/** Called when the sensor can no longer detect a previously detected component. */
	UPROPERTY(BlueprintAssignable, Category = "AsgardLashComponent|Collision")
	FOnComponentExitLashSignature OnComponentExitLash;

	/** Called when the sensor detects a new actor. */
	UPROPERTY(BlueprintAssignable, Category = "Asgard|LashComponent|Collision")
	FOnActorEnterLashSignature OnActorEnterLash;

	/** Called when the sensor can no longer detect a previously detected actor. */
	UPROPERTY(BlueprintAssignable, Category = "Asgard|LashComponent|Collision")
	FOnActorExitLashSignature OnActorExitLash;

	// ---------------------------------------------------------
	//	Attach settings

	/**
	* Offset for the end of the lash from the component it is attached to (if any).
	* Coordinates are in the relative space of the attached to component.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asgard|LashComponent")
	FVector AttachOffset;

	/**
	* Maximum distance beyond lash segment length that the last point can stretch while attached to a component.
	* Ignored if <= 0.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asgard|LashComponent", meta = (AllowPrivateAccess = "true"))
	float AttachMaxStretchDistance;

	// ---------------------------------------------------------
	//	Editor debug

#if WITH_EDITORONLY_DATA
	/** Whether to debug draw the lash. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asgard|LashComponent")
	bool bDebugDrawLash;
#endif

	// ---------------------------------------------------------
	//	Public Functions

	/** Returns the list of components within the lash's segments. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Asgard|LashComponent|Collision")
	const TSet<UPrimitiveComponent*>& GetComponentsInLash() const { return ComponentsInLash; }

	/** Returns the list of detected within the lash's segments. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Asgard|LashComponent|Collision")
	const TSet<AActor*>& GetActorsInLash() const { return ActorsInLash; }

	/** Attaches the end of the lash to a scene component. */
	UFUNCTION(BlueprintCallable, Category = "Asgard|LashComponent")
	void AttachLashEndToComponent(USceneComponent* AttachToComponent);

	/** Detaches the end of the lash from another component. */
	UFUNCTION(BlueprintCallable, Category = "Asgard|LashComponent")
	void DetachLashEndFromComponent();

	UFUNCTION(BlueprintCallable, Category = "Asgard|LashComponent")
	void SetLashExtended(bool bNewExtended);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	// ---------------------------------------------------------
	//	Lash settings

		/**
	* Whether the lash should be extended or not.
	* If set to not extended, it will shrink until no lash segments remain.
	*/
	UPROPERTY(BlueprintReadOnly, Category = "Asgard|LashComponent", meta = (AllowPrivateAccess = "true"))
	bool bLashExtended;

	/**
	* The number of physics simulations per second.
	* Higher numbers will result in more accurate simulation but will be more costly.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asgard|LashComponent", meta = (AllowPrivateAccess = "true", ClampMin = "30.0"))
	int32 PhysicsStepsPerSecond;

	/** How much of a correction is applied to the 'child' point further down the lash when solving constraints, on a scale of 0-1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asgard|LashComponent", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0"))
	float ChildPointCorrectionWeight;

	/** The maximum number of lash segments.*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asgard|LashComponent", meta = (AllowPrivateAccess = "true"))
	int32 MaxLashSegments;

	/** The maximum length of each lash segment. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asgard|LashComponent", meta = (AllowPrivateAccess = "true"))
	float LashSegmentMaxLength;

	/**
	* How much the lash should shorten every frame while not extended.
	* Takes priority over float LashShrinkSpeedBlocked.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asgard|LashComponent", meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float LashShrinkSpeedNotExtended;

	/** The minimum length of the last lash segment before the lash can 'grow' by adding another segment at the end while extended. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asgard|LashComponent", meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float LashGrowthMinSegmentLength;

	/** The maximum length of a lash segment before the lash can `shrink` by removing it while not extended. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asgard|LashComponent", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float LashShrinkMaxSegmentLength;

	// ---------------------------------------------------------
	//	Collision Settings

		/** Whether collision detection on the lash is enabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asgard|LashComponent|Collision", meta = (AllowPrivateAccess = "true"))
	bool bEnableCollision;

	/** Whether collision on the lash should automatically ignore the owning actor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asgard|LashComponent|Collision", meta = (AllowPrivateAccess = "true"))
	bool bAutoIgnoreOwner;

	/** Whether blocking hits should affect the position of the lash points. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asgard|LashComponent|Collision", meta = (AllowPrivateAccess = "true"))
	bool bBlockedByObjects;

	/** Radius of the lash segments when checking for collision. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asgard|LashComponent|Collision", meta = (AllowPrivateAccess = "true"))
	float CollisionRadius;

	/** Used to offset lash points from blocking collisions with when BlockedByObjects is true. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asgard|LashComponent|Collision", meta = (AllowPrivateAccess = "true"))
	float CollisionSkinWidth;

	/** 
	* How much of a correction is applied to a blocked point further down the lash when solving constraints, on a scale of 0-1. 
	* Overrides ChildPointCorrectionWeight when appropriate
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asgard|LashComponent", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0"))
	float BlockdPointCorrectionWeight;


	// ---------------------------------------------------------
	//	Lash state

	/** Whether the lash is currently blocked. */
	UPROPERTY(BlueprintReadOnly, Category = "Asgard|LashComponent|Collision", meta = (AllowPrivateAccess = "true"))
	bool bLashBlocked;

	/** The position of each lash point at the start of the frame. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Asgard|LashComponent", meta = (AllowPrivateAccess = "true"))
	TArray<FVector> LashPointFrameStartLocations;

	/** The position of each lash point at the start of the last physics simulation. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Asgard|LashComponent", meta = (AllowPrivateAccess = "true"))
	TArray<FVector> LashPointSimulationStartLocations;

	/** The current position of each lash point. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Asgard|LashComponent", meta = (AllowPrivateAccess = "true"))
	TArray<FVector> LashPointLocations;

	/**
	* If a lash segment is blocked, the component is stored here.
	* Otherwise, it is null.
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Asgard|LashComponent", meta = (AllowPrivateAccess = "true"))
	TArray<UPrimitiveComponent*> LashPointBlockingComponents;

	/** The current number of lash segments. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Asgard|LashComponent", meta = (AllowPrivateAccess = "true"))
	int32 NumLashSegments;

	/**
	* How much the lash should shorten every frame while blocked.
	* Subordinate to LashShrinkSpeedNotExtended;
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asgard|LashComponent", meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float LashShrinkSpeedBlocked;

	/**
	* When shrinking the lash, the length of the first segment is stored here and used to cap the length of the last segment during the constraints step.
	* This prevents velocity from re-lengthening the lash while it is shrinking.
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Asgard|LashComponent", meta = (AllowPrivateAccess = "true"))
	float LashFirstSegmentMaxLength;

	/** The minimum length of the a lash that is blocked by collision before it is removed for optimization. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asgard|LashComponent|Collision", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float LashCollisionMinSegmentLength;

	/**  Remainder from the last time physics were simulated. */
	float PhysicsStepRemainder;

	/** List of detected components. */
	UPROPERTY()
	TSet<UPrimitiveComponent*> ComponentsInLash;

	/** List of detected actors. */
	UPROPERTY()
	TSet<AActor*> ActorsInLash;

	/** Actor that the end of the lash is attached to (if any). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Asgard|LashComponent", meta = (AllowPrivateAccess = "true"))
	AActor* AttachedToActor;

	/** Scene component that the end of the lash is attached to (if any). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Asgard|LashComponent", meta = (AllowPrivateAccess = "true"))
	USceneComponent* AttachedToComponent;


	// ---------------------------------------------------------
	//	Private Functions

	/** Adds a point at the end of the lash. */
	void AddLashSegmentAtEnd();

	/** Removes a point at the end of the lash. */
	void RemoveLashSegmentFromFront();

	/** Removes a point at an index in the lash. */
	void RemoveLashSegmentAtIndex(int32 Idx);

	/**
	* Updates the length of the lash depending on whether it is active.
	* Returns whether the lash has fully shrunk.
	*/
	bool ShrinkLash(float DeltaTime, float ShrinkSpeed);

	/** Calculates the new current positions for each point, based on velocity from the previous frame. */
	void ApplyVelocityToLashPoints(float DeltaTime);

	/** Applies constraints to individual points in the lash, starting from the front. */
	void ApplyConstraintsToLashPointsFromFront();

	/** Applies constraints to individual points in the lash, starting from the back. */
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
