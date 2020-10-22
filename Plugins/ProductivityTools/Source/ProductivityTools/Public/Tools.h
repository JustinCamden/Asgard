// Copyright (c) 2019 Isara Technologies. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetData.h"
#include "Misc/Paths.h"
#include "Engine/ObjectLibrary.h"

class FTools
{
public:
	/**
	* Retrieve Assets of the specified class in the content browser from the specified Paths
	*
	* @param	SelectedPaths		The Paths in the content browser from which retrieving the Assets
	* @param	AssetClass			The Class of the assets to retrieve (by default retrieves all UObjects)
	*/
	static TArray<FAssetData> GetAssetsFromPaths(TArray<FString> SelectedPaths, UClass* AssetClass = UObject::StaticClass())
	{
		TArray<FAssetData> AssetDatas;
		for (FString Path : SelectedPaths)
		{
			// Retrieve all assets in the path in content browser
			AssetDatas.Append(GetAssetsFromPath(Path, AssetClass));
		}

		return AssetDatas;
	}

	/**
	* Retrieve Assets of the specified class in the content browser from the specified Path
	*
	* @param	SelectedPath		The Path in the content browser from which retrieving the Assets
	* @param	AssetClass			The Class of the assets to retrieve (by default retrieves all UObjects)
	*/
	static TArray<FAssetData> GetAssetsFromPath(FString SelectedPath, UClass* AssetClass = UObject::StaticClass())
	{
		// Retrieve all assets in the path in content browser
		UObjectLibrary *ObjectLibrary = UObjectLibrary::CreateLibrary(AssetClass, false, true);
		ObjectLibrary->LoadAssetDataFromPath(SelectedPath);
		TArray<FAssetData> AssetDatasInPath;
		ObjectLibrary->GetAssetDataList(AssetDatasInPath);

		return AssetDatasInPath;
	}

	/**
	* Check if the class of all the specified assets match the specified class
	*
	* @param	AssetsData		The Assets to check the class from
	* @param	AssetClass		The Class used to compare the assets class with
	*
	* @return	True if all the specified assets are of the specified class
	*/
	static bool CheckAssetsClass(TArray<FAssetData> AssetsData, UClass* AssetClass)
	{
		for (FAssetData AssetData : AssetsData)
		{
			if (AssetData.GetAsset()->GetClass() != AssetClass)
			{
				return false;
			}
		}
		return true;
	}

	/**
	* Get the full platform path from a path given by the content browser
	* Exemple : "/Game" becomes ".../ProjectName/Content"
	*
	* @param	ContentBrowserPath		The Path in the content browser
	*/
	static FString GetFullPathFromContentBrowserPath(FString ContentBrowserPath)
	{
		FString SelectedFolder;
		FString Unused;

		// Retrieve all path after "/Game/"
		ContentBrowserPath.Split("/Game/", &Unused, &SelectedFolder);

		// Construct the full path
		FString FullPath = FPaths::ProjectContentDir() / SelectedFolder;

		return FullPath;
	}

	/**
	* Get the full platform path from a path given by the content browser
	* Exemple : "/Game" becomes ".../ProjectName/Content"
	*
	* @param	ContentBrowserPath		The Path in the content browser
	*/
	//static FString GetContentBrowserPathFromFullPath(FString FullPath)
	//{
	//	FString SelectedFolder;
	//	FString Unused;

	//	// Retrieve all path after "/Game/"
	//	ContentBrowserPath.Split("/Game/", &Unused, &SelectedFolder);

	//	// Construct the full path
	//	FString FullPath = FPaths::ProjectContentDir() / SelectedFolder;

	//	return ContentBrowserPath;
	//}
};