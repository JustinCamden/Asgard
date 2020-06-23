// Copyright © 2020 Justin Camden All Rights Reserved


#include "AsgardCameraProximityComponent.h"
#include "VRBaseCharacter.h"

// Stat cycles
DECLARE_CYCLE_STAT(TEXT("AsgardCameraProximityComponent CheckProximityToCamera"), STAT_ASGARD_CameraProximityComponentCheckProximityToCamera, STATGROUP_ASGARD_CameraProximityComponent);

// Console variable setup so we can enable and disable debugging from the console
// Draw detection debug
static TAutoConsoleVariable<int32> CVarAsgardTeleportDrawDebug(
	TEXT("Asgard.CameraProximityDrawDebug"),
	0,
	TEXT("Whether to enable CameraProximity detection debug.\n")
	TEXT("0: Disabled, 1: Enabled"),
	ECVF_Scalability | ECVF_RenderThreadSafe);
static const auto CameraProximityDrawDebug = IConsoleManager::Get().FindConsoleVariable(TEXT("Asgard.CameraProximityDrawDebug"));

// Macros for debug builds
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
#include "DrawDebugHelpers.h"
#define DETECTION_BOX(_Loc, _Extent, _Rotation, _Color)	if (CameraProximityDrawDebug->GetInt()) { DrawDebugBox(GetWorld(), _Loc, _Extent, _Rotation, _Color, false, 1.0f / 60.0f, 0, 0.5f); }
#else
#define DETECTION_BOX(_Loc, _Extent, _Rotation, _Color)	/* nothing */
#endif

// Sets default values for this component's properties
UAsgardCameraProximityComponent::UAsgardCameraProximityComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = ETickingGroup::TG_PostPhysics;
	bAutoActivate = true;
	NearClippingPlane = 10.0f;
	ViewFrustumHeightMultiplier = 1.5f;
	ViewFrustumWidthMultiplier = 1.5f;
	ClippingBufferDetection = 2.0f;
	RightLeftDetection = 0.4f;
	AboveBelowDetection = 1.0f/3.0f;
	FrontOfCameraDetection = 5.0f;
	bEnableClippingChecks = true;
	bEnableClippingBufferChecks = true;
	bEnablePeripheralChecks = true;
	bEnableFrontOfCameraChecks = true;
	DetectionObjectTypes.Add((TEnumAsByte<EObjectTypeQuery>)ECC_WorldDynamic);
	DetectionObjectTypes.Add((TEnumAsByte<EObjectTypeQuery>)ECC_Pawn);
}


void UAsgardCameraProximityComponent::PostLoad()
{
	Super::PostLoad();
	VRBaseCharacterOwner = Cast<AVRBaseCharacter>(GetOwner());
}

// Called when the game starts
void UAsgardCameraProximityComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeDetectionExtents();
}


bool UAsgardCameraProximityComponent::AreAnyProximityChecksEnabled() const
{
	return bEnableClippingChecks || bEnableClippingBufferChecks || bEnablePeripheralChecks || bEnableFrontOfCameraChecks;
}

// Called every frame
void UAsgardCameraProximityComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// If any checks are active
	if (AreAnyProximityChecksEnabled())
	{
		// Check if actor is in proximity
		const FTransform& CameraTransform = VRBaseCharacterOwner->VRReplicatedCamera->GetComponentTransform();
		ProximityLocation = CameraTransform.TransformPosition(ProximityRelativeLocation);
		FQuat CameraRotation = CameraTransform.GetRotation();
		UWorld* World = GetWorld();
		FCollisionQueryParams CollisionQueryParams = FCollisionQueryParams(FName(TEXT("CameraProximityComponentBoxOverlap")), false, GetOwner());
		FCollisionObjectQueryParams CollisionObjectQueryParams = FCollisionObjectQueryParams(DetectionObjectTypes);
		bool bNewIsActorInProximity = World->OverlapAnyTestByObjectType(
			ProximityLocation, 
			CameraRotation, 
			CollisionObjectQueryParams, 
			FCollisionShape::MakeBox(ProximityExtent), 
			CollisionQueryParams);

		// If actor is in proximity
		if (bNewIsActorInProximity)
		{
			DETECTION_BOX(ProximityLocation, ProximityExtent, CameraRotation, FColor::Red);

			// Broadcast and update state as necessary
			if (!bIsActorInProximity)
			{
				bIsActorInProximity = true;
				OnDetectOtherActorInProximityBegin.Broadcast();
			}

			// Clipping
			if (bEnableClippingChecks)
			{
				ClippingLocation = CameraTransform.TransformPosition(ClippingRelativeLocation);
				bool bNewIsActorClippingCamera = World->OverlapAnyTestByObjectType(
					ClippingLocation,
					CameraRotation,
					CollisionObjectQueryParams,
					FCollisionShape::MakeBox(ClippingExtent),
					CollisionQueryParams);
				if (bNewIsActorClippingCamera)
				{
					DETECTION_BOX(ClippingLocation, ClippingExtent, CameraRotation, FColor::Red);
					if (!bIsActorInClipping)
					{
						bIsActorInClipping = true;
						OnDetectOtherActorInClippingBegin.Broadcast();
					}
				}
				else
				{
					DETECTION_BOX(ClippingLocation, ClippingExtent, CameraRotation, FColor::Green);
					if (bIsActorInClipping)
					{
						bIsActorInClipping = false;
						OnDetectOtherActorInClippingEnd.Broadcast();
					}
				}
			}

			// Clipping buffer
			if (bEnableClippingBufferChecks && ClippingBufferDetection > 0.0f)
			{
				ClippingBufferLocation = CameraTransform.TransformPosition(ClippingBufferRelativeLocation);
				bool bNewIsActorClippingBufferCamera = World->OverlapAnyTestByObjectType(
					ClippingBufferLocation,
					CameraRotation,
					CollisionObjectQueryParams,
					FCollisionShape::MakeBox(ClippingBufferExtent),
					CollisionQueryParams);
				if (bNewIsActorClippingBufferCamera)
				{
					DETECTION_BOX(ClippingBufferLocation, ClippingBufferExtent, CameraRotation, FColor::Red);
					if (!bIsActorInClippingBuffer)
					{
						bIsActorInClippingBuffer = true;
						OnDetectOtherActorInClippingBufferBegin.Broadcast();
					}
				}
				else
				{
					DETECTION_BOX(ClippingBufferLocation, ClippingBufferExtent, CameraRotation, FColor::Green);
					if (bIsActorInClippingBuffer)
					{
						bIsActorInClippingBuffer = false;
						OnDetectOtherActorInClippingBufferEnd.Broadcast();
					}
				}
			}

			// Peripheral checks
			if (bEnablePeripheralChecks)
			{
				// Right / left of camera
				if (RightLeftDetection > 0.0f)
				{
					// Right of Camera
					RightOfCameraLocation = CameraTransform.TransformPosition(RightOfCameraRelativeLocation);
					bool bNewIsActorRightOfCameraCamera = World->OverlapAnyTestByObjectType(
						RightOfCameraLocation,
						CameraRotation,
						CollisionObjectQueryParams,
						FCollisionShape::MakeBox(RightLeftExtent),
						CollisionQueryParams);
					if (bNewIsActorRightOfCameraCamera)
					{
						DETECTION_BOX(RightOfCameraLocation, RightLeftExtent, CameraRotation, FColor::Red);
						if (!bIsActorInRightOfCamera)
						{
							bIsActorInRightOfCamera = true;
							OnDetectOtherActorInRightOfCameraBegin.Broadcast();
						}
					}
					else
					{
						DETECTION_BOX(RightOfCameraLocation, RightLeftExtent, CameraRotation, FColor::Green);
						if (bIsActorInRightOfCamera)
						{
							bIsActorInRightOfCamera = false;
							OnDetectOtherActorInRightOfCameraEnd.Broadcast();
						}
					}

					// Left of Camera
					LeftOfCameraLocation = CameraTransform.TransformPosition(LeftOfCameraRelativeLocation);
					bool bNewIsActorLeftOfCameraCamera = World->OverlapAnyTestByObjectType(
						LeftOfCameraLocation,
						CameraRotation,
						CollisionObjectQueryParams,
						FCollisionShape::MakeBox(RightLeftExtent),
						CollisionQueryParams);
					if (bNewIsActorLeftOfCameraCamera)
					{
						DETECTION_BOX(LeftOfCameraLocation, RightLeftExtent, CameraRotation, FColor::Red);
						if (!bIsActorInLeftOfCamera)
						{
							bIsActorInLeftOfCamera = true;
							OnDetectOtherActorInLeftOfCameraBegin.Broadcast();
						}
					}
					else
					{
						DETECTION_BOX(LeftOfCameraLocation, RightLeftExtent, CameraRotation, FColor::Green);
						if (bIsActorInLeftOfCamera)
						{
							bIsActorInLeftOfCamera = false;
							OnDetectOtherActorInLeftOfCameraEnd.Broadcast();
						}
					}
				}

				// Above / below of camera
				if (AboveBelowDetection > 0.0f)
				{
					// Above Camera
					AboveCameraLocation = CameraTransform.TransformPosition(AboveCameraRelativeLocation);
					bool bNewIsActorAboveCameraCamera = World->OverlapAnyTestByObjectType(
						AboveCameraLocation,
						CameraRotation,
						CollisionObjectQueryParams,
						FCollisionShape::MakeBox(AboveBelowExtent),
						CollisionQueryParams);
					if (bNewIsActorAboveCameraCamera)
					{
						DETECTION_BOX(AboveCameraLocation, AboveBelowExtent, CameraRotation, FColor::Red);
						if (!bIsActorInAboveCamera)
						{
							bIsActorInAboveCamera = true;
							OnDetectOtherActorInAboveCameraBegin.Broadcast();
						}
					}
					else
					{
						DETECTION_BOX(AboveCameraLocation, AboveBelowExtent, CameraRotation, FColor::Green);
						if (bIsActorInAboveCamera)
						{
							bIsActorInAboveCamera = false;
							OnDetectOtherActorInAboveCameraEnd.Broadcast();
						}
					}

					// Below Camera
					BelowCameraLocation = CameraTransform.TransformPosition(BelowCameraRelativeLocation);
					bool bNewIsActorBelowCameraCamera = World->OverlapAnyTestByObjectType(
						BelowCameraLocation,
						CameraRotation,
						CollisionObjectQueryParams,
						FCollisionShape::MakeBox(AboveBelowExtent),
						CollisionQueryParams);
					if (bNewIsActorBelowCameraCamera)
					{
						DETECTION_BOX(BelowCameraLocation, AboveBelowExtent, CameraRotation, FColor::Red);
						if (!bIsActorInBelowCamera)
						{
							bIsActorInBelowCamera = true;
							OnDetectOtherActorInBelowCameraBegin.Broadcast();
						}
					}
					else
					{
						DETECTION_BOX(BelowCameraLocation, AboveBelowExtent, CameraRotation, FColor::Green);
						if (bIsActorInBelowCamera)
						{
							bIsActorInBelowCamera = false;
							OnDetectOtherActorInBelowCameraEnd.Broadcast();
						}
					}
				}
			}

			// In front of camera checks
			if (bEnableFrontOfCameraChecks && FrontOfCameraDetection > 0.0f)
			{
				FrontOfCameraLocation = CameraTransform.TransformPosition(FrontOfCameraRelativeLocation);
				bool bNewIsActorFrontOfCameraCamera = World->OverlapAnyTestByObjectType(
					FrontOfCameraLocation,
					CameraRotation,
					CollisionObjectQueryParams,
					FCollisionShape::MakeBox(FrontOfCameraExtent),
					CollisionQueryParams);
				if (bNewIsActorFrontOfCameraCamera)
				{
					DETECTION_BOX(FrontOfCameraLocation, FrontOfCameraExtent, CameraRotation, FColor::Red);
					if (!bIsActorInFrontOfCamera)
					{
						bIsActorInFrontOfCamera = true;
						OnDetectOtherActorInFrontOfCameraBegin.Broadcast();
					}
				}
				else
				{
					DETECTION_BOX(FrontOfCameraLocation, FrontOfCameraExtent, CameraRotation, FColor::Green);
					if (bIsActorInFrontOfCamera)
					{
						bIsActorInFrontOfCamera = false;
						OnDetectOtherActorInFrontOfCameraEnd.Broadcast();
					}
				}
			}

		}
		else
		{
			DETECTION_BOX(ProximityLocation, ProximityExtent, CameraRotation, FColor::Green);

			// Else if no actor is in proximity
			if (bIsActorInProximity)
			{
				// Signal all detections to end
				bIsActorInProximity = false;
				OnDetectOtherActorInProximityEnd.Broadcast();

				if (bIsActorInClipping)
				{
					bIsActorInClipping = false;
					OnDetectOtherActorInClippingEnd.Broadcast();
				}

				if (bIsActorInClippingBuffer)
				{
					bIsActorInClippingBuffer = false;
					OnDetectOtherActorInClippingBufferEnd.Broadcast();
				}

				if (bIsActorInRightOfCamera)
				{
					bIsActorInRightOfCamera = false;
					OnDetectOtherActorInRightOfCameraEnd.Broadcast();
				}

				if (bIsActorInLeftOfCamera)
				{
					bIsActorInLeftOfCamera = false;
					OnDetectOtherActorInLeftOfCameraEnd.Broadcast();
				}

				if (bIsActorInAboveCamera)
				{
					bIsActorInAboveCamera = false;
					OnDetectOtherActorInAboveCameraEnd.Broadcast();
				}

				if (bIsActorInBelowCamera)
				{
					bIsActorInBelowCamera = false;
					OnDetectOtherActorInBelowCameraEnd.Broadcast();
				}

				if (bIsActorInFrontOfCamera)
				{
					bIsActorInFrontOfCamera = false;
					OnDetectOtherActorInFrontOfCameraEnd.Broadcast();
				}
			}
		}
	}
	// Else if no actor is in proximity
	else if (bIsActorInProximity)
	{
		// Signal all detections to end
		bIsActorInProximity = false;
		OnDetectOtherActorInProximityEnd.Broadcast();

		if (bIsActorInClipping)
		{
			bIsActorInClipping = false;
			OnDetectOtherActorInClippingEnd.Broadcast();
		}

		if (bIsActorInClippingBuffer)
		{
			bIsActorInClippingBuffer = false;
			OnDetectOtherActorInClippingBufferEnd.Broadcast();
		}

		if (bIsActorInRightOfCamera)
		{
			bIsActorInRightOfCamera = false;
			OnDetectOtherActorInRightOfCameraEnd.Broadcast();
		}

		if (bIsActorInLeftOfCamera)
		{
			bIsActorInLeftOfCamera = false;
			OnDetectOtherActorInLeftOfCameraEnd.Broadcast();
		}

		if (bIsActorInAboveCamera)
		{
			bIsActorInAboveCamera = false;
			OnDetectOtherActorInAboveCameraEnd.Broadcast();
		}

		if (bIsActorInBelowCamera)
		{
			bIsActorInBelowCamera = false;
			OnDetectOtherActorInBelowCameraEnd.Broadcast();
		}

		if (bIsActorInFrontOfCamera)
		{
			bIsActorInFrontOfCamera = false;
			OnDetectOtherActorInFrontOfCameraEnd.Broadcast();
		}
	}
}

void UAsgardCameraProximityComponent::InitializeDetectionExtents()
{
	checkf(VRBaseCharacterOwner, TEXT("VRBaseCharacterOwner was invalid! (AsgardCameraProximityComponent, Actor %f, InitializeDetectionExtents"), *GetNameSafe(GetOwner()));

	// Aspect ratio
	FVector2D ViewportSize = FVector2D();
	VRBaseCharacterOwner->VRReplicatedCamera->GetWorld()->GetGameViewport()->GetViewportSize(ViewportSize);
	float CameraAspectRatio = ViewportSize.X / ViewportSize.Y;

	// Frustum Width and height
	FMinimalViewInfo ViewInfo;
	VRBaseCharacterOwner->VRReplicatedCamera->GetCameraView(0, ViewInfo);
	float FrustumWidth = FMath::Tan(FMath::DegreesToRadians(ViewInfo.FOV * 0.5f)) * 2.0f;
	float FrustumHeight = FrustumWidth / CameraAspectRatio;
	float TotalDetectionDepth = NearClippingPlane;
	float TotalDetectionWidth = FrustumWidth * TotalDetectionDepth * ViewFrustumWidthMultiplier;
	float TotalDetectionHeight = FrustumHeight * TotalDetectionDepth * ViewFrustumHeightMultiplier;

	// Clipping Detection
	ClippingExtent = FVector(TotalDetectionDepth,
		TotalDetectionWidth * (1.0f - RightLeftDetection),
		TotalDetectionHeight * (1.0f - AboveBelowDetection)) * 0.5f;
	ClippingRelativeLocation = FVector(ClippingExtent.X, 0.0f, 0.0f);

	// Clipping buffer detection
	if (ClippingBufferDetection > 0.0f)
	{
		TotalDetectionDepth += ClippingBufferDetection;
		ClippingBufferExtent = FVector(ClippingBufferDetection * 0.5f, ClippingExtent.Y, ClippingExtent.Z);
		ClippingBufferRelativeLocation = FVector(NearClippingPlane + ClippingBufferExtent.X, 0.0f, 0.0f);
	}

	// Left / right detection
	if (RightLeftDetection > 0.0f)
	{
		RightLeftExtent = FVector(TotalDetectionDepth * 0.5f, TotalDetectionWidth * 0.5f, ClippingExtent.Z);
		RightLeftExtent.Y = (RightLeftExtent.Y - ClippingBufferExtent.Y) * 0.5f;
		RightOfCameraRelativeLocation = FVector(RightLeftExtent.X, ClippingExtent.Y + RightLeftExtent.Y, 0.0f);
		LeftOfCameraRelativeLocation = FVector(RightLeftExtent.X, -ClippingExtent.Y - RightLeftExtent.Y, 0.0f);
	}

	// Above / below detection
	if (AboveBelowDetection > 0.0f)
	{
		AboveBelowExtent = FVector(TotalDetectionDepth * 0.5f, ClippingExtent.Y, TotalDetectionHeight * 0.5f);
		AboveBelowExtent.Z = (AboveBelowExtent.Z - ClippingBufferExtent.Z) * 0.5f;
		AboveCameraRelativeLocation = FVector(AboveBelowExtent.X, 0.0f, ClippingExtent.Z + AboveBelowExtent.Z);
		BelowCameraRelativeLocation = FVector(AboveBelowExtent.X, 0.0f, -ClippingExtent.Z - AboveBelowExtent.Z);
	}

	// In front of camera detection
	if (FrontOfCameraDetection > 0.0f)
	{
		TotalDetectionDepth += FrontOfCameraDetection;
		TotalDetectionWidth = FrustumWidth * TotalDetectionDepth * ViewFrustumWidthMultiplier;
		TotalDetectionHeight = FrustumHeight * TotalDetectionDepth * ViewFrustumHeightMultiplier;
		FrontOfCameraExtent = FVector(TotalDetectionDepth - NearClippingPlane, TotalDetectionWidth, TotalDetectionHeight) * 0.5f;
		FrontOfCameraRelativeLocation = FVector(NearClippingPlane + FrontOfCameraExtent.X, 0.0f, 0.0f);
	}

	// Total detection
	ProximityExtent = FVector(TotalDetectionDepth, TotalDetectionWidth, TotalDetectionHeight);
	ProximityRelativeLocation = FVector(ProximityExtent.X, 0.0f, 0.0f);

	return;
}

