// Copyright (c) 2019 Isara Technologies. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FClean
{
public:
	/**
	* Function called by the Clean button
	* Removed all unused assets and empty folders in specified paths
	*/
	static void Clean(TArray<FString> SelectedPaths);

	/**
	* Function called by the Clean Project button
	* Removed all unused assets and empty directories in project
	*/
	static void CleanProject();
};