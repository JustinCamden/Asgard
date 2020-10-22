// Copyright (c) 2019 Isara Technologies. All Rights Reserved.

#include "Clean.h"
#include "ProductivityToolsSettings.h"
#include "Tools.h"

#include "ObjectTools.h"
#include "Modules/ModuleManager.h"
#include "AssetRegistryModule.h"
#include "Engine/ObjectLibrary.h"
#include "Engine/World.h"
#include "Misc/Paths.h"
#include "Misc/ScopedSlowTask.h"
//#include "Editor/PackagesDialog/Public/PackagesDialog.h"
//#include "ISourceControlModule.h"
//#include "PlatformFilemanager.h"
//#include "FileManager.h"


#define LOCTEXT_NAMESPACE "FProductivityToolsModule"

void FClean::CleanProject()
{
	TArray<FString> ContentPath = TArray<FString>();
	ContentPath.Add("/Game");
	Clean(ContentPath);
}

void FClean::Clean(TArray<FString> SelectedPaths)
{
	// Retrieve all assets in content browser
	TArray<FAssetData> AssetDatas = FTools::GetAssetsFromPaths(SelectedPaths);

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	// Array to keep the unused assets found
	TArray<FAssetData> UnusedAssets = TArray<FAssetData>();
	//TArray<UObject*> ObjectsToDelete = TArray<UObject*>();

	// for each asset in content browser
	for (FAssetData AssetData : AssetDatas)
	{
		TArray<FName> Referencers;

		// find referencers of this asset
		AssetRegistryModule.Get().GetReferencers(AssetData.PackageName, Referencers);

		bool IsUsed = false;
		for (FName Referencer : Referencers)
		{
			// if the referencer does not have the same name
			if (Referencer != AssetData.PackageName)
			{
				// it means there is a valid referencer to this asset (the asset is not unused)
				IsUsed = true;
			}
		}

		// if the user chose to clean the orphans asset (without referencers nor dependencies)
		if (SETTINGS->AssetTypeToClean == EAssetTypeToClean::ORPHANS)
		{
			TArray<FName> Dependencies;

			// find referencers of this asset
			AssetRegistryModule.Get().GetDependencies(AssetData.PackageName, Dependencies);

			for (FName Dependency : Dependencies)
			{
				// if the dependency does not have the same name
				if (Dependency != AssetData.PackageName)
				{
					// it means there is a valid dependency of this asset (the asset is not unused)
					IsUsed = true;
				}
			}
		}

		// if the asset has no valid referencers or dependencies
		if (!IsUsed)
		{
			// do not delete the level assets
			if (AssetData.GetClass() != UWorld::StaticClass())
			{
				// add this asset to the unused assets list
				UnusedAssets.Add(AssetData);
				//ObjectsToDelete.Add(AssetData.GetAsset());
			}
		}
	}

	// For the progress bar
	FScopedSlowTask ExplorePathsTask(SelectedPaths.Num(), LOCTEXT("LookingForUnusedAssetsText", "Looking assets to remove"));
	ExplorePathsTask.MakeDialog();
	ExplorePathsTask.EnterProgressFrame();
	
	// for each selected path
	for (FString SelectedPath : SelectedPaths)
	{
		ExplorePathsTask.EnterProgressFrame();

		// Retrieve Folders in this path recursively
		TArray<FString> SubFolders;
		AssetRegistryModule.Get().GetSubPaths(SelectedPath, SubFolders, true);

		FScopedSlowTask ExploreFoldersTask(SubFolders.Num(), LOCTEXT("LookingForUnusedAssetsText", "Looking assets to remove"));
		ExploreFoldersTask.MakeDialog();
		ExploreFoldersTask.EnterProgressFrame();

		// for each folder found in path
		for (FString SubFolder : SubFolders)
		{
			ExploreFoldersTask.EnterProgressFrame();

			// Retrieve assets inside this folder
			TArray<FAssetData> AssetDatasFromPath = FTools::GetAssetsFromPath(SubFolder);
			// If there isn't any assets
			if (AssetDatasFromPath.Num() == 0)
			{
				// Remove the folder
				AssetRegistryModule.Get().RemovePath(SubFolder);
			}
		}
	}
	
	// Create a window to confirm the deletion of all the unused assets found
	ObjectTools::DeleteAssets(UnusedAssets);
	//ShowDeleteConfirmationDialog(ObjectsToDelete);
}

//bool ShowDeleteConfirmationDialog(const TArray<UObject*>& ObjectsToDelete)
//{
//	TArray<UPackage*> PackagesToDelete;
//
//	// Gather a list of packages which may need to be deleted once the objects are deleted.
//	for (int32 ObjIdx = 0; ObjIdx < ObjectsToDelete.Num(); ++ObjIdx)
//	{
//		PackagesToDelete.AddUnique(ObjectsToDelete[ObjIdx]->GetOutermost());
//	}
//
//	// Cull out packages which cannot be found on disk or are not UAssets
//	for (int32 PackageIdx = PackagesToDelete.Num() - 1; PackageIdx >= 0; --PackageIdx)
//	{
//		UPackage* Package = PackagesToDelete[PackageIdx];
//
//		FString PackageFilename;
//		if (!FPackageName::DoesPackageExist(Package->GetName(), NULL, &PackageFilename))
//		{
//			// Could not determine filename for package so we can not delete
//			PackagesToDelete.RemoveAt(PackageIdx);
//		}
//	}
//
//	// If we found any packages that we may delete
//	if (PackagesToDelete.Num())
//	{
//		// Set up the delete package dialog
//		FPackagesDialogModule& PackagesDialogModule = FModuleManager::LoadModuleChecked<FPackagesDialogModule>(TEXT("PackagesDialog"));
//		PackagesDialogModule.CreatePackagesDialog(NSLOCTEXT("PackagesDialogModule", "DeleteAssetsDialogTitle", "Delete Assets"), NSLOCTEXT("PackagesDialogModule", "DeleteAssetsDialogMessage", "The following assets will be deleted."), /*InReadOnly=*/true);
//		PackagesDialogModule.AddButton(DRT_Save, NSLOCTEXT("PackagesDialogModule", "DeleteSelectedButton", "Delete"), NSLOCTEXT("PackagesDialogModule", "DeleteSelectedButtonTip", "Delete the listed assets"));
//		if (!ISourceControlModule::Get().IsEnabled())
//		{
//			PackagesDialogModule.AddButton(DRT_MakeWritable, NSLOCTEXT("PackagesDialogModule", "MakeWritableAndDeleteSelectedButton", "Make Writable and Delete"), NSLOCTEXT("PackagesDialogModule", "MakeWritableAndDeleteSelectedButtonTip", "Makes the listed assets writable and deletes them"));
//		}
//		PackagesDialogModule.AddButton(DRT_Cancel, NSLOCTEXT("PackagesDialogModule", "CancelButton", "Cancel"), NSLOCTEXT("PackagesDialogModule", "CancelDeleteButtonTip", "Do not delete any assets and cancel the current operation"));
//
//		for (int32 PackageIdx = 0; PackageIdx < PackagesToDelete.Num(); ++PackageIdx)
//		{
//			UPackage* Package = PackagesToDelete[PackageIdx];
//			PackagesDialogModule.AddPackageItem(Package, Package->GetName(), ECheckBoxState::Checked);
//		}
//
//		// Display the delete dialog
//		const EDialogReturnType UserResponse = PackagesDialogModule.ShowPackagesDialog();
//
//		if (UserResponse == DRT_MakeWritable)
//		{
//			// make each file writable before attempting to delete
//			for (int32 PackageIdx = 0; PackageIdx < PackagesToDelete.Num(); ++PackageIdx)
//			{
//				const UPackage* Package = PackagesToDelete[PackageIdx];
//				FString PackageFilename;
//				if (FPackageName::DoesPackageExist(Package->GetName(), NULL, &PackageFilename))
//				{
//					FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*PackageFilename, false);
//				}
//			}
//		}
//
//		// If the user selected a "Delete" option return true
//		return UserResponse == DRT_Save || UserResponse == DRT_MakeWritable;
//	}
//	else
//	{
//		// There are no packages that are considered for deletion. Return true because this is a safe delete.
//		return true;
//	}
//}

#undef LOCTEXT_NAMESPACE
