// Copyright (c) 2019 Isara Technologies. All Rights Reserved.

#include "FixNaming.h"
#include "ProductivityToolsSettings.h"
#include "Tools.h"

#include "Engine/ObjectLibrary.h"

#include "AssetToolsModule.h"

#include "Engine/Blueprint.h"
#include "Engine/TextureRenderTarget.h"

DEFINE_LOG_CATEGORY(LogProductivityToolsFixNaming);

#define LOCTEXT_NAMESPACE "FProductivityToolsModule"

void FFixNaming::FixNaming(TArray<FAssetData> SelectedAssets, TArray<FString> SelectedPaths)
{
	SelectedAssets.Append(FTools::GetAssetsFromPaths(SelectedPaths));
	FixNaming(SelectedAssets);
}

void FFixNaming::FixNaming(TArray<FString> SelectedPaths)
{
	FixNaming(FTools::GetAssetsFromPaths(SelectedPaths));
}

void FFixNaming::FixNaming(TArray<FAssetData> SelectedAssets)
{
	TArray<FAssetRenameData> RenameDatas = TArray<FAssetRenameData>();

	for (FAssetData AssetData : SelectedAssets)
	{
		// Retrieve the package path of each selected asset
		const FString PackagePath = FPackageName::GetLongPackagePath(AssetData.ToSoftObjectPath().GetAssetPathName().ToString());
		
		FString NewName;
		bool bRename = false;
		bool bExistingPrefix = false;

		// Find the corresponding prefix in the project settings to add to the asset name 
		FString Prefix = GetPrefix(AssetData.GetAsset());
		
		// If a prefix was found in the project settings for this asset class
		if (Prefix != "")
		{
			FString NameRemainder;
			// If the asset name possibly contains a prefix ("_" character)
			if (AssetData.AssetName.ToString().Contains("_"))
			{
				FString ExistingPrefix;
				// Retrieve the characters before and after the "_" character
				AssetData.AssetName.ToString().Split("_", &ExistingPrefix, &NameRemainder);
				
				// Make sure it's a prefix and not just a name with a "_" character
				// A prefix must not take more than 3 characters, otherwise if the prefix is not specified in the project settings,
				// then it's most likely not a prefix
				bExistingPrefix = ExistingPrefix.Len() <= 3 || IsPrefixSpecified(ExistingPrefix + "_");	
			}

			// If the asset name contains a prefix
			if (bExistingPrefix)
			{
				if (SETTINGS->ExistingPrefixMode == EExistingPrefixMode::ADD_BEFORE)
				{
					// if it's not the same prefix
					if (!AssetData.AssetName.ToString().StartsWith(Prefix))
					{
						// Add the prefix before the asset name
						NewName = Prefix + AssetData.AssetName.ToString();
						bRename = true;
					}
				}
				else if (SETTINGS->ExistingPrefixMode == EExistingPrefixMode::REPLACE)
				{
					// if it's not the same prefix
					if (!AssetData.AssetName.ToString().StartsWith(Prefix))
					{
						// Replace the existing prefix by the new prefix in the asset name
						NewName = Prefix + NameRemainder;
						bRename = true;
					}
				}
				else if (SETTINGS->ExistingPrefixMode == EExistingPrefixMode::LEAVE_UNCHANGED)
				{
					bRename = false;
				}
			}
			// If the asset name does not contain a prefix already
			else
			{
				// Add the prefix before the asset name
				NewName = Prefix + AssetData.AssetName.ToString();
				bRename = true;
			}
		}

		// If we did not add a prefix
		if (!bRename)
		{
			// Set the new name as the current asset name
			// In case we will add a suffix
			NewName = AssetData.AssetName.ToString();
		}

		// Find a suitable suffix if any
		FString Suffix = GetSuffix(AssetData.GetAsset());

		// If we did find a suffix
		if (Suffix != "")
		{
			// If the Asset does not have the same suffix already
			if (!AssetData.AssetName.ToString().EndsWith(Suffix))
			{
				// Add the suffix at the end of the name
				NewName = NewName + Suffix;
				bRename = true;
			}
		}

		// If a new name has been defined for the asset
		if (bRename)
		{
			// Add this asset to the array of assets to rename
			FAssetRenameData RenameData = FAssetRenameData(AssetData.GetAsset(), PackagePath, NewName);
			RenameDatas.Add(RenameData);
		}
	}

	// Rename all the assets we changed the name
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	AssetToolsModule.Get().RenameAssets(RenameDatas);
}

FString FFixNaming::GetPrefix(UObject* Asset)
{
	FString ClassName = Asset->GetClass()->GetName();
	FString Prefix;
	// For each specified prefix in the project settings
	for (FClassNameToPrefix ClassNameToPrefix : SETTINGS->Prefixes)
	{
		// If we found the correponding class name
		if (ClassName == ClassNameToPrefix.ClassName)
		{
			// set the prefix to the correponding one
			Prefix = ClassNameToPrefix.Prefix;
			break;
		}
	}

	// Check if this asset actually need a special prefix (Blueprint Interface, ...)
	FString PrimarySpecialPrefix = CheckSpecialPrefixes(Asset);
	
	// Use the Special Prefix in priority
	if (PrimarySpecialPrefix != "")
	{
		return PrimarySpecialPrefix;
	}
	else 
	{
		// Else use the prefix found before
		if (Prefix != "")
		{
			return Prefix;
		}
		else 
		{
			// Else use the other Special Prefix if any
			FString SecondarySpecialPrefix = CheckClassGroupPrefixes(Asset);
			if (SecondarySpecialPrefix != "")
			{
				return SecondarySpecialPrefix;
			}
			// If no prefixes were found
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Class name %s not defined for fix naming"), *ClassName);
				return "";
			}
		}
	}	
}

FString FFixNaming::GetPrefixByClassName(FString ClassName)
{
	for (FClassNameToPrefix ClassNameToPrefix : SETTINGS->Prefixes)
	{
		if (ClassNameToPrefix.ClassName == ClassName)
		{
			return ClassNameToPrefix.Prefix;
		}
	}
	return "";
}

FString FFixNaming::CheckSpecialPrefixes(UObject* Asset)
{
	FString ClassName = Asset->GetClass()->GetName();
	FString SpecialPrefix = "";
	// Check special cases for Blueprints
	if (ClassName == "Blueprint")
	{
		// if the asset is a Blueprint Interface
		if (((UBlueprint*)Asset)->BlueprintType == EBlueprintType::BPTYPE_Interface)
		{
			SpecialPrefix = GetPrefixByClassName("BlueprintInterface");
		}	
	}
	return SpecialPrefix;
}

FString FFixNaming::CheckClassGroupPrefixes(UObject* Asset)
{
	FString ClassName = Asset->GetClass()->GetName();

	// For each specified class group prefix in project settings
	for (FClassGroupToPrefix ClassGroupToPrefix : SETTINGS->PrefixesGroup)
	{
		// If the class name of the asset contains the class group name
		if (ClassName.Contains(ClassGroupToPrefix.ClassGroupName))
		{
			// Use the prefix for this class group
			return ClassGroupToPrefix.Prefix;
		}
	}
	return "";
}

FString FFixNaming::GetSuffix(UObject* Asset)
{
	FString ClassName = Asset->GetClass()->GetName();
	FString Suffix = "";

	// Check Suffixes for Textures
	// Try to cast the Asset to a UTexture
	UTexture* Texture = Cast<UTexture>(Asset);

	// If the Asset is a Texture
	if (Texture != NULL)
	{
		// If the Asset use Normalmap compression setting
		if (Texture->CompressionSettings == TextureCompressionSettings::TC_Normalmap)
		{
			// It is most likely a Normal texture
			Suffix = "_N";
		}
	}
	return Suffix;
}

bool FFixNaming::IsPrefixSpecified(FString Prefix)
{
	for (FClassNameToPrefix ClassNameToPrefix : SETTINGS->Prefixes)
	{
		if (Prefix == ClassNameToPrefix.Prefix)
		{
			return true;
		}
	}
	return false;
}

#undef LOCTEXT_NAMESPACE
