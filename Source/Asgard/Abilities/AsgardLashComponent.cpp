// Copyright © 2020 Justin Camden All Rights Reserved


#include "AsgardLashComponent.h"

// Sets default values for this component's properties
UAsgardLashComponent::UAsgardLashComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	MaxLashSegments = 20;
}


// Called when the game starts
void UAsgardLashComponent::BeginPlay()
{
	Super::BeginPlay();

	// Reserve space for the lash points
	LashPointLastLocations.Reserve(MaxLashSegments);
	LashPointCurrentLocations.Reserve(MaxLashSegments);

	// Set the first point to be equal to the location of the component
	FVector ComponentLocation = GetComponentLocation();
	LashPointLastLocations.Add(ComponentLocation);
	LashPointCurrentLocations.Add(ComponentLocation);
}

void UAsgardLashComponent::AddLashSegmentAtEnd()
{
	FVector NewPoint = LashPointCurrentLocations.Last();
	LashPointLastLocations.Add(NewPoint);
	LashPointCurrentLocations.Add(NewPoint);
	NumLashSegments++;

	return;
}

void UAsgardLashComponent::RemoveLashSegmentFromEnd()
{
	checkf(NumLashSegments > 0, TEXT("ERROR: NumLashPoints was <= 0. (AsgardLashComponent, RemoveLashPointFromEnd, %s"), * GetNameSafe(this));
	LashPointLastLocations.RemoveAt(NumLashSegments, 1, false);
	LashPointCurrentLocations.RemoveAt(NumLashSegments, 1, false);
	NumLashSegments--;

	return;
}

void UAsgardLashComponent::UpdateLashLength()
{

}

void UAsgardLashComponent::CalculateLashPointInertialLocations()
{
}

void UAsgardLashComponent::ApplyConstraintsToLashPoints()
{
}


// Called every frame
void UAsgardLashComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (NumLashSegments > 0)
	{
		UpdateLashLength();
		CalculateLashPointInertialLocations();
		ApplyConstraintsToLashPoints();
	}
	else if (IsActive())
	{
		FVector ComponentLocation = GetComponentLocation();
		LashPointCurrentLocations[0] = ComponentLocation;
		LashPointLastLocations[0] = ComponentLocation;
		AddLashSegmentAtEnd();
	}

	return;
}
