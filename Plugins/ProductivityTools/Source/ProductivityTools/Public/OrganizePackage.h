// Copyright (c) 2019 Isara Technologies. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetData.h"

class FOrganizePackage
{
public:
	/**
	* Function called by the Organize Package button
	* Split specified assets into folders according to their types
	*
	* @param	SelectedAssets		The Selected Assets in the content browser
	* @param	SelectedPaths		The Selected Paths in the content browser
	*/
	static void OrganizePackage(TArray<FAssetData> SelectedAssets, TArray<FString> SelectedPaths);

	/**
	* Split assets in specified paths into folders according to their types
	*
	* @param	SelectedPaths		The Selected paths in the content browser
	*/
	static void OrganizePackage(TArray<FString> SelectedPaths);

	/**
	* Split specified assets into folders according to their types
	*
	* @param	SelectedAssets		The Selected Assets in the content browser
	*/
	static void OrganizePackage(TArray<FAssetData> SelectedAssets);
	
};