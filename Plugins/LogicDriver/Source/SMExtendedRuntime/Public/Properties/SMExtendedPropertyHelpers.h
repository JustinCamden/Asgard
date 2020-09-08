// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/TextProperty.h" // Required for plugin packaging to work.
#include "SMTextGraphProperty.h"
#include "SMExtendedPropertyHelpers.generated.h"


UCLASS(MinimalAPI)
class USMExtendedGraphPropertyHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintPure, Category = "Graph Property", meta = (NativeBreakFunc))
	static SMEXTENDEDRUNTIME_API void BreakTextGraphProperty(const FSMTextGraphProperty& GraphProperty, FText& Result);
};