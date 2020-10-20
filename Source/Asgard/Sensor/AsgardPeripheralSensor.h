// Copyright © 2020 Justin Camden All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Asgard/Sensor/AsgardSphereSensor.h"
#include "AsgardPeripheralSensor.generated.h"

/**
 *	Class used to sense primitives around the periphery of the player's Sensor without triggering overlap or hit events.
 */
UCLASS( ClassGroup = (Custom), meta = (BlueprintSpawnableComponent) )
class ASGARD_API UAsgardPeripheralSensor : public UAsgardSphereSensor
{
	GENERATED_BODY()
public:
	// Sets default values for this component's properties
	UAsgardPeripheralSensor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**
	* The maximum number of primitives to track. 
	* Ignored if <= 0.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardPeripheralSensor)
	int32 MaxTrackedPoints;

	/**
	* The minimum angle from the front or back of the Sensor for an object to be considered in the Sensor's periphery.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardPeripheralSensor)
	float Min3DAngleFromSensor;

	/**
	* The minimum distance from the Sensor for an object to be considered in the Sensor's periphery.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardPeripheralSensor)
	float MinDistanceFromSensor;

	/**
	* The minimum 2D angle between the nearest points on tracked primitives, measured across the Sensor plane.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardPeripheralSensor)
	float Min2DAngleBetweenTrackedPoints;

private:
	/**
	* Current list of the nearest points on tracked primitives.
	*/
	UPROPERTY(BlueprintReadOnly, Category = AsgardPeripheralSensor, meta = (AllowPrivateAccess = "true"))
	TArray<float> RollAnglesToNearestTrackedPoints;
};
