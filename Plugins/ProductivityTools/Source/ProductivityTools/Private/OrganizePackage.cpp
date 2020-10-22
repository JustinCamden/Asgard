// Copyright (c) 2019 Isara Technologies. All Rights Reserved.

#include "OrganizePackage.h"
#include "ProductivityToolsSettings.h"
#include "Tools.h"

#include "HAL/FileManager.h"
#include "AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Engine/MapBuildDataRegistry.h"

#define LOCTEXT_NAMESPACE "FProductivityToolsModule"

void FOrganizePackage::OrganizePackage(TArray<FAssetData> SelectedAssets, TArray<FString> SelectedPaths)
{
	SelectedAssets.Append(FTools::GetAssetsFromPaths(SelectedPaths));
	OrganizePackage(SelectedAssets);
}

void FOrganizePackage::OrganizePackage(TArray<FString> SelectedPaths)
{
	OrganizePackage(FTools::GetAssetsFromPaths(SelectedPaths));
}

void FOrganizePackage::OrganizePackage(TArray<FAssetData> SelectedAssets)
{
	TArray<FAssetRenameData> RenameDatas = TArray<FAssetRenameData>();
	//TArray<FString> PathsToLeaveUnchanged = TArray<FString>();
	//TArray<FString> PathsToOrganize = TArray<FString>();
	TMap<FString, bool> PathsToOrganize = TMap<FString, bool>();

	// For each selected asset
	for (FAssetData AssetData : SelectedAssets)
	{
		// Ignore the MapBuildDataRegistry Assets
		// They will be moved along their corresponding World Assets within the RenameAssets function (at FixReferencesAndRename in AssetRenameManager)
		// Ignore Object Redirectors as well
		if (AssetData.GetClass() != UMapBuildDataRegistry::StaticClass() && AssetData.GetClass() != UObjectRedirector::StaticClass())
		{
			// Retrieve the path to the folder containing the asset
			const FString PackagePath = FPackageName::GetLongPackagePath(AssetData.ToSoftObjectPath().GetAssetPathName().ToString());
		
			// Verify if we already saw an asset in the same folder
			// If it's the first time going through this folder
			if (!PathsToOrganize.Contains(PackagePath))
			{
				// Check if all the assets in this folder have the same class
				for (FAssetData AssetDataInPath : FTools::GetAssetsFromPath(PackagePath))
				{
					// If some assets in this folder have a different class (MapBuildDataRegistry and ObjectRedirector Assets do not count)
					if (AssetData.GetClass() != AssetDataInPath.GetClass() 
						&& AssetDataInPath.GetClass() != UMapBuildDataRegistry::StaticClass()
						&& AssetDataInPath.GetClass() != UObjectRedirector::StaticClass())
					{
						// This means we will organize this folder into sub folders
						PathsToOrganize.Add(PackagePath, true);
						break;
					}
				}
				// If all the assets in this folder have the same class
				if (!PathsToOrganize.Contains(PackagePath))
				{
					// Do not check assets classes in this folder again
					PathsToOrganize.Add(PackagePath, false);
				}
			}

			// If the assets in the parent folder should be organized into sub folders
			if (PathsToOrganize[PackagePath])
			{
				FString ClassName = AssetData.GetClass()->GetName();
				
				// If the asset class name is associated to a group, the name of the group will replace the name of the class for the
				// package
				if(UProductivityToolsSettings::Settings->OrganizeGroups.Contains(ClassName))
				{
					ClassName = UProductivityToolsSettings::Settings->OrganizeGroups[ClassName];
				}

				FString Unused;
				FString FolderName;
		
				// Verify if the asset is not already in a package with its class name
				PackagePath.Split("/", &Unused, &FolderName, ESearchCase::IgnoreCase, ESearchDir::FromEnd);

				if (!FolderName.Contains(ClassName) && !ClassName.Contains(FolderName))
				{
					FString AssetName = AssetData.AssetName.ToString();

					// Make the path to the new folder to create
					const FString NewPackagePath = PackagePath / ClassName;

					// Add the Asset to an array of assets to move
					FAssetRenameData RenameData = FAssetRenameData(AssetData.GetAsset(), NewPackagePath, AssetName);
					RenameDatas.Add(RenameData);
				}
			}
		}
	}

	// Finally move the assets to their new folder
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	AssetToolsModule.Get().RenameAssets(RenameDatas);
}

#undef LOCTEXT_NAMESPACE
