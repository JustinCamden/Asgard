// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "SMProjectEditorSettings.generated.h"


UCLASS(config = Editor, defaultconfig)
class USMProjectEditorSettings : public UObject
{
	GENERATED_BODY()

public:
	USMProjectEditorSettings();

	/**
	 * Perform additional validation when compiling the state machine default object by attempting a low level instantiation.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Validation")
	bool bValidateInstanceOnCompile;
	
	/**
	 * Children which reference a parent state machine graph risk being out of date if a package the parent references is modified.
	 * Compiling the package will signal that affected state machine children need to be compiled, however if you start a PIE session
	 * instead of pressing the compile button, the children may not be updated. In this case the state machine compiler will attempt to warn you.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Validation")
	bool bWarnIfChildrenAreOutOfDate;

	/**
	 * Newly placed conduits will automatically be configured as transitions.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Conduits")
	bool bConfigureNewConduitsAsTransitions;
};
