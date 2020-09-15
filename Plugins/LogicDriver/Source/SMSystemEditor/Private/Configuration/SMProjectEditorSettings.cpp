// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#include "SMProjectEditorSettings.h"

USMProjectEditorSettings::USMProjectEditorSettings()
{
	bUpdateAssetsOnStartup = true;
	bDisplayAssetUpdateProgress = true;
	bValidateInstanceOnCompile = true;
	bWarnIfChildrenAreOutOfDate = true;
	bConfigureNewConduitsAsTransitions = true;
	bDisplayUpdateNotification = true;
	InstalledVersion = "";
}

void USMProjectEditorSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	SaveConfig();
}
