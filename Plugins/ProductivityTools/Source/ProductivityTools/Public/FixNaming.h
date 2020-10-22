// Copyright (c) 2019 Isara Technologies. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetData.h"

DECLARE_LOG_CATEGORY_EXTERN(LogProductivityToolsFixNaming, Log, All);

class FFixNaming
{
public:
	/**
	* Function called by the Fix Naming button
	* Rename the specified assets with the Unreal naming convention
	*
	* @param	SelectedAssets		The Selected Assets in the content browser
	* @param	SelectedPaths		The Selected Paths in the content browser
	*/
	static void FixNaming(TArray<FAssetData> SelectedAssets, TArray<FString> SelectedPaths);

	/**
	* Rename the assets in the specified paths with the Unreal naming convention
	*
	* @param	SelectedPaths		The Selected paths in the content browser
	*/
	static void FixNaming(TArray<FString> SelectedPaths);

	/**
	* Rename the specified assets with the Unreal naming convention
	*
	* @param	SelectedAssets		The Selected Assets in the content browser
	*/
	static void FixNaming(TArray<FAssetData> SelectedAssets);



	/**
	* Retrieve the correponding prefix in the project settings
	*
	* @param	Asset		The Asset it will retrieve the prefix for
	*/
	static FString GetPrefix(UObject* Asset);

	/**
	* Retrieve the correponding prefix in the project settings by class name
	*
	* @param	ClassName		The class name from which retrieving the prefix
	*/
	static FString GetPrefixByClassName(FString ClassName);

	/** 
	* Check for special prefixes to use in priority to other prefixes
	* Exemple : use prefix for "BlueprintInterface" instead of "Blueprint" even if Blueprint Interfaces are considered "Blueprint" classes
	*
	* @param	Asset		The Asset from which checking special prefixes
	*/
	static FString CheckSpecialPrefixes(UObject* Asset);

	/**
	* Check for class group prefixes to use if no other prefixes were found
	* Exemple : use prefix for "RenderTarget" instead of creating prefixes for "TextureRenderTarget2D", "TextureRenderTargetCube", ...
	* This way, user can still override these group prefixes by defining more precise prefixes
	* If an asset belongs to more than one class group, it will take the prefix of the first defined group
	*
	* @param	Asset		The Asset from which checking special prefixes
	*/
	static FString CheckClassGroupPrefixes(UObject* Asset);

	/** 
	* Retrieve the suitable suffix for the asset
	* 
	* @param	Asset		The Asset it will retrieve the suffix for
	*/
	static FString GetSuffix(UObject* Asset);

	static bool IsPrefixSpecified(FString Prefix);

};