// Copyright (c) 2019 Isara Technologies. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ProductivityToolsSettings.generated.h"

UENUM()
namespace EAssetTypeToClean
{
	enum Type {
		NOT_REFERENCED			UMETA(DisplayName = "Not Referenced"),
		ORPHANS					UMETA(DisplayName = "Orphans"),
	};
}

UENUM()
namespace EExistingPrefixMode
{
	enum Type {
		REPLACE					UMETA(DisplayName = "Replace"),
		ADD_BEFORE				UMETA(DisplayName = "Add before"),
		LEAVE_UNCHANGED			UMETA(DisplayName = "Leave unchanged"),
	};
}

USTRUCT()
struct FClassNameToPrefix 
{
	GENERATED_BODY()

public:
	/** The name of the class to add a prefix */
	UPROPERTY(config, EditAnywhere, Category = FixNaming)
	FString ClassName;

	/** The prefix to use */
	UPROPERTY(config, EditAnywhere, Category = FixNaming)
	FString Prefix;
};

USTRUCT()
struct FClassGroupToPrefix
{
	GENERATED_BODY()

public:
	/** The name the classes must contain to add a prefix */
	UPROPERTY(config, EditAnywhere, Category = FixNaming)
	FString ClassGroupName;

	/** The prefix to use */
	UPROPERTY(config, EditAnywhere, Category = FixNaming)
	FString Prefix;
};

#define SETTINGS UProductivityToolsSettings::Settings

UCLASS(config = Engine, defaultconfig)
class UProductivityToolsSettings  : public UObject
{
	GENERATED_BODY()

public:
	// CONSOLIDATE

	/** The minimum likeness value for two assets to be considered duplicates (for Consolidate) */
	UPROPERTY(config, EditAnywhere, Category = Consolidate, meta = (ClampMin = "0", ClampMax = "1"))
	float DuplicateThreshold_Consolidate;

	/** The minimum likeness value for two assets to be considered duplicates (for ConsolidateAll) */
	UPROPERTY(config, EditAnywhere, Category = Consolidate, meta = (ClampMin = "0", ClampMax = "1"))
	float DuplicateThreshold_ConsolidateAll;

	/** Enable comparing asset file names to find duplicates */
	UPROPERTY(config, EditAnywhere, Category = Consolidate)
	bool bEnableFileNameCriteria;

	/** The weight of the file names criteria */
	UPROPERTY(config, EditAnywhere, Category = Consolidate, meta = (EditCondition = "bEnableFileNameCriteria"))
	float FileNameCriteriaWeight;

	/** Enable comparing asset file sizes to find duplicates */
	UPROPERTY(config, EditAnywhere, Category = Consolidate)
	bool bEnableFileSizeCriteria;

	/** The weight of the file sizes criteria */
	UPROPERTY(config, EditAnywhere, Category = Consolidate, meta = (EditCondition = "bEnableFileSizeCriteria"))
	float FileSizeCriteriaWeight;

	/** Enable comparing asset text file to find duplicates */
	UPROPERTY(config, EditAnywhere, Category = Consolidate)
	bool bEnableTextFileCriteria;

	/** The weight of the text file criteria */
	UPROPERTY(config, EditAnywhere, Category = Consolidate, meta = (EditCondition = "bEnableTextFileCriteria"))
	float TextFileCriteriaWeight;

	// CLEAN PROJECT

	/** The type of assets to clean */
	UPROPERTY(config, EditAnywhere, Category = CleanProject)
	TEnumAsByte<EAssetTypeToClean::Type> AssetTypeToClean;

	// CREATE PACKAGE

	/** The name of the package to create */
	UPROPERTY(config, EditAnywhere, Category = CreatePackage)
	FString PackageName;

	/** The sub folders to create */
	UPROPERTY(config, EditAnywhere, Category = CreatePackage)
	TArray<FString> SubFoldersToCreate;

	// FIX NAMING

	/** The behavior of the fix naming when an already existing prefix is found */
	UPROPERTY(config, EditAnywhere, Category = FixNaming)
	TEnumAsByte<EExistingPrefixMode::Type> ExistingPrefixMode;

	/** The prefixes to add for each class when fix naming */
	UPROPERTY(config, EditAnywhere, Category = FixNaming)
	TArray<FClassNameToPrefix> Prefixes;

	/** The prefixes to add to class groups when fix naming */
	UPROPERTY(config, EditAnywhere, Category = FixNaming)
	TArray<FClassGroupToPrefix> PrefixesGroup;

	/** Enable the addition of suffixes when fix naming (Experimental) */
	UPROPERTY(config, EditAnywhere, Category = FixNaming)
	bool bEnableSuffixes;

	// ORGANIZE BLUEPRINT

	/** The minimum distance between two blueprints nodes after they have been moved during the reorganization */
	UPROPERTY(config, EditAnywhere, Category = OrganizeBlueprint)
	float NodesMinDistance;
	
	// ORGANIZE PACKAGE

	/** Mapping an asset type to its corresponding folder (the key is the asset type) */
	UPROPERTY(config, EditAnywhere, Category = Organize)
	TMap<FString, FString> OrganizeGroups;

	// OPTIMIZE TEXTURE SIZE

	/** The Offset used after converting the ratio between static mesh sizes and texture resolution into LOD Bias (Higher means less increase in texture's LOD Bias) */
	UPROPERTY(config, EditAnywhere, Category = OptimizeTextureSize)
		int32 LODBiasOffset;

	/** The minimum in-game resolution for all optimized textures, the changed LOD bias won't make the texture resolution lower than this parameter */
	UPROPERTY(config, EditAnywhere, Category = OptimizeTextureSize)
		FIntPoint MinimumInGameResolution;

	/** Only reduce texture size for textures used by small static meshes */
	UPROPERTY(config, EditAnywhere, Category = OptimizeTextureSize)
		bool bReduceSizeForSmallObjectsOnly;

	/** The maximum size for a static mesh to be considered as a small object */
	UPROPERTY(config, EditAnywhere, Category = OptimizeTextureSize, meta = (EditCondition = "bReduceSizeForSmallObjectsOnly"))
		FVector MaximumSizeForSmallObjects;

	/** Allow the optimization to decrease the LOD Bias for some textures */
	UPROPERTY(config, EditAnywhere, Category = OptimizeTextureSize)
		bool bAllowDecreaseLODBias;
	
	void Reset();


	/** The plug-in Settings retrieved from the project settings */
	static UProductivityToolsSettings const* Settings;

	UProductivityToolsSettings(const FObjectInitializer& ObjectInitializer);
};