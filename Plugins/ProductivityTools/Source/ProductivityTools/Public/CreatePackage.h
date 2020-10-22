// Copyright (c) 2019 Isara Technologies. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FCreatePackage
{
public:
	/**
	* Function called by the Create Package button
	* Create a new folder in content browser with specified sub folders in project settings
	*/
	static void CreatePackage(TArray<FString> SelectedPaths);
};