// Copyright © 2020 Justin Camden All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
//#include "Runtime/PhysicsCore/Public/CollisionShape.h"
#include "AsgardCameraProximityComponent.generated.h"

// Delegate signatures
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBeginDetectOtherActorSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEndDetectOtherActorSignature);

// Stats group
DECLARE_STATS_GROUP(TEXT("AsgardCameraProximityComponent"), STATGROUP_ASGARD_CameraProximityComponent, STATCAT_Advanced);

// Forward declarations
class AVRBaseCharacter;

/**
* Component for detecting when objects are nearby the camera without triggering volume overlap events
*/
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ASGARD_API UAsgardCameraProximityComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAsgardCameraProximityComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void PostLoad() override;

	/**
	* Returns whether any proximity detection is enabled.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = AsgardCameraProximityComponent)
	bool AreAnyProximityChecksEnabled() const;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**
	* Whether to enable clipping extent checks.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardCameraProximityComponent)
	uint32 bEnableClippingChecks : 1;

	/**
	* Whether to enable clipping buffer extent checks.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardCameraProximityComponent)
	uint32 bEnableClippingBufferChecks : 1;

	/**
	* Whether to enable peripheral (Left, Right, Above, Below) extent checks.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardCameraProximityComponent)
	uint32 bEnablePeripheralChecks : 1;

	/**
	* Whether to enable in front of camera extent checks.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardCameraProximityComponent)
	uint32 bEnableFrontOfCameraChecks : 1;

	/**
	* Object types to check for proximity to.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AsgardCameraProximityComponent)
	TArray<TEnumAsByte<EObjectTypeQuery>> DetectionObjectTypes;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:

	/** 
	* The distance of the Near Clipping Plane.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AsgardCameraProximityComponent, meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float NearClippingPlane;

	/**
	* Multiplier applied to the calculated width of the view frustum.
	* Used to account for HMDs having variable FOV.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AsgardCameraProximityComponent, meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float ViewFrustumWidthMultiplier;

	/**
	* The total height of detection, as a multiplier of the calculated view frustum at the distance of the Near Clipping Plane.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AsgardCameraProximityComponent, meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float ViewFrustumHeightMultiplier;

	/**
	* Detection extent for objects clipping the camera.
	*/
	UPROPERTY()
	FVector ClippingExtent;

	/**
	* Location of the Clipping detection, relative to the camera.
	*/
	UPROPERTY()
	FVector ClippingRelativeLocation;

	/**
	* Location of the Clipping detection, in worldspace.
	*/
	UPROPERTY()
	FVector ClippingLocation;

	/**
	* Called when another actor is detected in the area of the Clipping extent.
	*/
	UPROPERTY(BlueprintAssignable, Category = AsgardCameraProximityComponent)
	FOnBeginDetectOtherActorSignature OnDetectOtherActorInClippingBegin;

	/**
	* Called when no actors are detected in the area of the Clipping extent any longer.
	*/
	UPROPERTY(BlueprintAssignable, Category = AsgardCameraProximityComponent)
	FOnEndDetectOtherActorSignature OnDetectOtherActorInClippingEnd;

	/**
	* The distance of the clipping buffer plane beyond the Near Clipping Plane, in world units.
	* Will be ignored if <= 0.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AsgardCameraProximityComponent, meta = (AllowPrivateAccess = "true"))
	float ClippingBufferDetection;

	/*
	* Detection extent for objects almost clipping the camera.
	*/
	UPROPERTY()
	FVector ClippingBufferExtent;

	/**
	* Location of the Clipping Buffer detection, relative to the camera.
	*/
	UPROPERTY()
	FVector ClippingBufferRelativeLocation;

	/**
	* Location of the Clipping Buffer detection, in worldspace.
	*/
	UPROPERTY()
	FVector ClippingBufferLocation;

	/**
	* Called when another actor is detected in the area of the ClippiingBuffer extent.
	*/
	UPROPERTY(BlueprintAssignable, Category = AsgardCameraClippiingBufferComponent)
	FOnBeginDetectOtherActorSignature OnDetectOtherActorInClippingBufferBegin;

	/**
	* Called when no actors are detected in the area of the ClippiingBuffer extent any longer.
	*/
	UPROPERTY(BlueprintAssignable, Category = AsgardCameraClippiingBufferComponent)
	FOnEndDetectOtherActorSignature OnDetectOtherActorInClippingBufferEnd;

	/**
	* The combined left and right detection distance.
	* Taken as a proportion of TotalDetectionHeightMultiplier.
	* (If 0.25, the clipping extent will be the middle 0.75 of detection, while the left and right extents will each be 0.125)
	* Will be ignored if <= 0.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AsgardCameraProximityComponent, meta = (AllowPrivateAccess = "true", ClampMax = "0.99"))
	float RightLeftDetection;

	/*
	* Detection extent for objects to the left and right of the camera.
	*/
	UPROPERTY()
	FVector RightLeftExtent;

	/**
	* Location of the Right of Camera detection, relative to the camera.
	*/
	UPROPERTY()
	FVector RightOfCameraRelativeLocation;

	/**
	* Location of the Right of Camera detection, in worldspace.
	*/
	UPROPERTY()
	FVector RightOfCameraLocation;

	/**
	* Called when another actor is detected in the area of the RightOfCamera extent.
	*/
	UPROPERTY(BlueprintAssignable, Category = AsgardCameraRightOfCameraComponent)
	FOnBeginDetectOtherActorSignature OnDetectOtherActorInRightOfCameraBegin;

	/**
	* Called when no actors are detected in the area of the RightOfCamera extent any longer.
	*/
	UPROPERTY(BlueprintAssignable, Category = AsgardCameraRightOfCameraComponent)
	FOnEndDetectOtherActorSignature OnDetectOtherActorInRightOfCameraEnd;

	/**
	* Location of the Left of Camera detection, relative to the camera.
	*/
	UPROPERTY()
	FVector LeftOfCameraRelativeLocation;

	/**
	* Location of the Left of Camera detection, in worldspace.
	*/
	UPROPERTY()
	FVector LeftOfCameraLocation;

	/**
	* Called when another actor is detected in the area of the LeftOfCamera extent.
	*/
	UPROPERTY(BlueprintAssignable, Category = AsgardCameraLeftOfCameraComponent)
	FOnBeginDetectOtherActorSignature OnDetectOtherActorInLeftOfCameraBegin;

	/**
	* Called when no actors are detected in the area of the LeftOfCamera extent any longer.
	*/
	UPROPERTY(BlueprintAssignable, Category = AsgardCameraLeftOfCameraComponent)
	FOnEndDetectOtherActorSignature OnDetectOtherActorInLeftOfCameraEnd;

	/**
	* The above and below detection distance.
	* Taken as a proportion of the total detection height.
	* (If 0.25, the clipping extent will be the middle 0.75 of detection, while the left and right extents will each be 0.125)
	* Will be ignored if <= 0.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AsgardCameraProximityComponent, meta = (AllowPrivateAccess = "true", ClampMax = "0.99"))
	float AboveBelowDetection;

	/*
	* Detection extent for objects above and below the camera.
	*/
	UPROPERTY()
	FVector AboveBelowExtent;

	/**
	* Location of the Above Camera detection, relative to the camera.
	*/
	UPROPERTY()
	FVector AboveCameraRelativeLocation;

	/**
	* Location of the Above Camera detection, in worldspace.
	*/
	UPROPERTY()
	FVector AboveCameraLocation;

	/**
	* Called when another actor is detected in the area of the AboveCamera extent.
	*/
	UPROPERTY(BlueprintAssignable, Category = AsgardCameraAboveCameraComponent)
	FOnBeginDetectOtherActorSignature OnDetectOtherActorInAboveCameraBegin;

	/**
	* Called when no actors are detected in the area of the AboveCamera extent any longer.
	*/
	UPROPERTY(BlueprintAssignable, Category = AsgardCameraAboveCameraComponent)
	FOnEndDetectOtherActorSignature OnDetectOtherActorInAboveCameraEnd;

	/**
	* Location of the Below Camera detection, relative to the camera.
	*/
	UPROPERTY()
	FVector BelowCameraRelativeLocation;

	/**
	* Location of the Below Camera detection, in worldspace.
	*/
	UPROPERTY()
	FVector BelowCameraLocation;

	/**
	* Called when another actor is detected in the area of the BelowCamera extent.
	*/
	UPROPERTY(BlueprintAssignable, Category = AsgardCameraBelowCameraComponent)
	FOnBeginDetectOtherActorSignature OnDetectOtherActorInBelowCameraBegin;

	/**
	* Called when no actors are detected in the area of the BelowCamera extent any longer.
	*/
	UPROPERTY(BlueprintAssignable, Category = AsgardCameraBelowCameraComponent)
	FOnEndDetectOtherActorSignature OnDetectOtherActorInBelowCameraEnd;

	/**
	* The distance of the clipping buffer plane beyond the combined Near Clipping Plane and Clipping Buffer Detection, if appropriate.
	* Will be ignored if <= 0.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AsgardCameraProximityComponent, meta = (AllowPrivateAccess = "true"))
	float FrontOfCameraDetection;

	/**
	* Detection extent for objects in front of the camera.
	*/
	UPROPERTY()
	FVector FrontOfCameraExtent;

	/**
	* Location of the In Front Of Camera detection, relative to the camera.
	*/
	UPROPERTY()
	FVector FrontOfCameraRelativeLocation;

	/**
	* Location of the In Front Of Camera detection detection, in worldspace.
	*/
	UPROPERTY()
	FVector FrontOfCameraLocation;

	/**
	* Called when another actor is detected in the area of the FrontOfCamera extent.
	*/
	UPROPERTY(BlueprintAssignable, Category = AsgardCameraFrontOfCameraComponent)
	FOnBeginDetectOtherActorSignature OnDetectOtherActorInFrontOfCameraBegin;

	/**
	* Called when no actors are detected in the area of the FrontOfCamera extent any longer.
	*/
	UPROPERTY(BlueprintAssignable, Category = AsgardCameraFrontOfCameraComponent)
	FOnEndDetectOtherActorSignature OnDetectOtherActorInFrontOfCameraEnd;

	/**
	*  Detection extent for total combined detection.
	*/
	UPROPERTY()
	FVector ProximityExtent;

	/**
	*  Center of combined total detection, relative to the camera.
	*/
	UPROPERTY()
	FVector ProximityRelativeLocation;

	/**
	* Center of combined total detection, in worldspace.
	*/
	UPROPERTY()
	FVector ProximityLocation;

	/**
	* Called when another actor is detected in the area of the TotalDetectionCamera extent.
	*/
	UPROPERTY(BlueprintAssignable, Category = AsgardCameraTotalDetectionCameraComponent)
	FOnBeginDetectOtherActorSignature OnDetectOtherActorInProximityBegin;

	/**
	* Called when no actors are detected in the area of the TotalDetectionCamera extent any longer.
	*/
	UPROPERTY(BlueprintAssignable, Category = AsgardCameraTotalDetectionCameraComponent)
	FOnEndDetectOtherActorSignature OnDetectOtherActorInProximityEnd;

	/**
	* Reference to the VRCharacterBase that owns this component.
	*/
	AVRBaseCharacter* VRBaseCharacterOwner;

	/**
	* Whether a non-self actor is within the total detection extent.
	*/
	UPROPERTY(BlueprintReadOnly, Category = AsgardCameraProximityComponent, meta = (AllowPrivateAccess = "true"))
	uint32 bIsActorInProximity : 1;

	/**
	* Whether a non-self actor is within the clipping extent.
	*/
	UPROPERTY(BlueprintReadOnly, Category = AsgardCameraProximityComponent, meta = (AllowPrivateAccess = "true"))
	uint32 bIsActorInClipping : 1;

	/**
	* Whether a non-self actor is within the clipping buffer extent.
	*/
	UPROPERTY(BlueprintReadOnly, Category = AsgardCameraProximityComponent, meta = (AllowPrivateAccess = "true"))
	uint32 bIsActorInClippingBuffer : 1;

	/**
	* Whether a non-self actor is within the right of camera extent.
	*/
	UPROPERTY(BlueprintReadOnly, Category = AsgardCameraProximityComponent, meta = (AllowPrivateAccess = "true"))
	uint32 bIsActorInRightOfCamera : 1;

	/**
	* Whether a non-self actor is within the left of camera extent.
	*/
	UPROPERTY(BlueprintReadOnly, Category = AsgardCameraProximityComponent, meta = (AllowPrivateAccess = "true"))
	uint32 bIsActorInLeftOfCamera : 1;

	/**
	* Whether a non-self actor is within the above camera extent.
	*/
	UPROPERTY(BlueprintReadOnly, Category = AsgardCameraProximityComponent, meta = (AllowPrivateAccess = "true"))
	uint32 bIsActorInAboveCamera : 1;

	/**
	* Whether a non-self actor is within the below camera extent.
	*/
	UPROPERTY(BlueprintReadOnly, Category = AsgardCameraProximityComponent, meta = (AllowPrivateAccess = "true"))
	uint32 bIsActorInBelowCamera : 1;

	/**
	* Whether a non-self actor is within the in front of camera extent.
	*/
	UPROPERTY(BlueprintReadOnly, Category = AsgardCameraProximityComponent, meta = (AllowPrivateAccess = "true"))
	uint32 bIsActorInFrontOfCamera : 1;

	/**
	* Sizes and places all detection extents according to the current component settings
	*/
	void InitializeDetectionExtents();
};
