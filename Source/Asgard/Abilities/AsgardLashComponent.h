// Copyright © 2020 Justin Camden All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "AsgardLashComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ASGARD_API UAsgardLashComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAsgardLashComponent();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	/**
	* The points in the lash at the end of the last frame.
	*/
	TArray<FVector> LashPointLastLocations;

	/**
	* The points after calculations in the current frame.
	*/
	TArray<FVector> LashPointCurrentLocations;

	/**
	* The current number of lash segments.
	*/
	UPROPERTY(BlueprintReadOnly, Category = AsgardLashComponent, meta = (AllowPrivateAccess = "true"))
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
	* The minimum length of the last lash segment before the lash can 'grow' by adding another segment at the end.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AsgardLashComponent, meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float LashSegmentMinGrowthLength;

	/**
	* The maximum length of the last lash segment before the lash can 'shrink' by removing it.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AsgardLashComponent, meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float LashSegmentMaxShrinkLength;

	/**
	* Adds a point at the end of the lash.
	*/
	void AddLashSegmentAtEnd();

	/**
	* Removes a point at the end of the lash.
	*/
	void RemoveLashSegmentFromEnd();

	/**
	* Updates the length of the lash depending on whether it is active.
	*/
	void UpdateLashLength();

	/**
	* Calculates the new current positions for each point, based on velocity from the previous frame.
	*/
	void CalculateLashPointInertialLocations();

	/**
	* Applies constraints to individual points in the lash.
	*/
	void ApplyConstraintsToLashPoints();
		
};
