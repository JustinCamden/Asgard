// Copyright © 2020 Justin Camden All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "AsgardVelocityTracker.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ASGARD_API UAsgardVelocityTracker : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAsgardVelocityTracker();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void Activate(bool bReset = false) override;

	virtual void Deactivate() override;

	/** 
	* DeltaLocation will be calculated by the average over this time. 
	* If <= 0, only the latest frame will be considered.
	*/
	float VelocityAverageInterval = 0.05f;

	/** If this scene component is valid, location changes will be offset by the location of said component. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asgard|VelocityTracker")
	USceneComponent* OffsetComponent;

private:
	/** The location of the component on the last frame that tracking was active.*/
	UPROPERTY(BlueprintReadOnly, Category = "Asgard|VelocityTracker", meta = (AllowPrivateAccess = "true"))
	FVector LastLocation;

	/** The calculated velocity of this component, in world space. */
	UPROPERTY(BlueprintReadOnly, Category = "Asgard|VelocityTracker", meta = (AllowPrivateAccess = "true"))
	FVector CalculatedVelocity;

	TArray<FVector> FrameLocationHistory;
	TArray<float> FrameDurationHistory;
};
