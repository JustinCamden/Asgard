// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "SMProjectEditorSettings.generated.h"


UCLASS(config = Editor)
class USMProjectEditorSettings : public UObject
{
	GENERATED_BODY()

public:
	USMProjectEditorSettings();

	UPROPERTY(config, VisibleAnywhere, Category = "Version Updates")
	FString InstalledVersion;
	
	/**
	 * Automatically update assets saved by older versions to the most current version. It is strongly recommended to leave this on.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Version Updates")
	bool bUpdateAssetsOnStartup;

	/**
	 * Display a progress bar when updating assets to a new version.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Version Updates", meta = (EditCondition = "bUpdateAssetsOnStartup"))
	bool bDisplayAssetUpdateProgress;

	/**
	 * Display a popup with a link to the patch notes when a new version is detected.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Version Updates")
	bool bDisplayUpdateNotification;
	
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

	// UObject
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	// ~UObject
};
