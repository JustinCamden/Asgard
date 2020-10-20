// Copyright © 2020 Justin Camden All Rights Reserved

#include "AsgardVelocityTracker.h"

// Sets default values for this component's properties
UAsgardVelocityTracker::UAsgardVelocityTracker()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	bAutoActivate = false;
	SetComponentTickEnabled(false);

	// ...
}


// Called when the game starts
void UAsgardVelocityTracker::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UAsgardVelocityTracker::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Cache the location of the component
	FVector CurrentLocation = GetComponentLocation();

	// Offset if necessary
	if (OffsetComponent)
	{
		CurrentLocation -= OffsetComponent->GetComponentLocation();
	}

	// If the velocity interval is greater than 0, average over the past few frames up to the interval duration
	if (VelocityAverageInterval > 0.0f)
	{
		FVector IntervalStartLocation = LastLocation;
		float TotalDuration = DeltaTime;

		if (FrameLocationHistory.Num() > 0)
		{
			int32 Idx = -1;
			for (float CurrentFrameDuration : FrameDurationHistory)
			{
				Idx++;
				TotalDuration += CurrentFrameDuration;
				if (TotalDuration >= VelocityAverageInterval)
				{
					IntervalStartLocation = FrameLocationHistory[Idx];
					break;
				}
			}

			FrameLocationHistory.SetNum(Idx, false);
			FrameDurationHistory.SetNum(Idx, false);

		}

		FrameLocationHistory.EmplaceAt(0, CurrentLocation);
		FrameDurationHistory.Insert(0, DeltaTime);

		CalculatedVelocity = (CurrentLocation - IntervalStartLocation) / TotalDuration;
	}

	else
	{
		CalculatedVelocity = (CurrentLocation - LastLocation) / DeltaTime;
	}

	LastLocation = CurrentLocation;
}

void UAsgardVelocityTracker::Activate(bool bReset)
{
	LastLocation = GetComponentLocation();
	SetComponentTickEnabled(true);
	Super::Activate();
}

void UAsgardVelocityTracker::Deactivate()
{
	SetComponentTickEnabled(false);
	CalculatedVelocity = FVector::ZeroVector;
	FrameLocationHistory.Reset();
	FrameDurationHistory.Reset();
	Super::Deactivate();
}

