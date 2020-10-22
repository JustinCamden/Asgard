// Copyright (c) 2019 Isara Technologies. All Rights Reserved.

#include "ProductivityToolsSettings.h"

UProductivityToolsSettings const* UProductivityToolsSettings::Settings = nullptr;

UProductivityToolsSettings::UProductivityToolsSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Reset();
}

void UProductivityToolsSettings::Reset()
{
	// CONSOLIDATE
	DuplicateThreshold_Consolidate = 0.85f;
	DuplicateThreshold_ConsolidateAll = 0.85f;
	bEnableFileNameCriteria = true;
	FileNameCriteriaWeight = 1;
	bEnableFileSizeCriteria = true;
	FileSizeCriteriaWeight = 3;
	bEnableTextFileCriteria = true;
	TextFileCriteriaWeight = 3;
	
	// CLEAN PROJECT
	AssetTypeToClean = EAssetTypeToClean::NOT_REFERENCED;
	
	// ORGANIZE BLUEPRINT
	NodesMinDistance = 100.f;
	
	// CREATE PACKAGE
	PackageName = "NewPackage";

	// FIX NAMING
	ExistingPrefixMode = EExistingPrefixMode::LEAVE_UNCHANGED;
	bEnableSuffixes = false;

	// DEFAULT SUB FOLDERS TO CREATE FOR "CREATE PACKAGE"
	SubFoldersToCreate = TArray<FString>();
	SubFoldersToCreate.Add("Audio");
	SubFoldersToCreate.Add("Blueprints");
	SubFoldersToCreate.Add("Meshes");
	SubFoldersToCreate.Add("Materials");
	SubFoldersToCreate.Add("Textures");
	SubFoldersToCreate.Add("Animations");
	SubFoldersToCreate.Add("Particles");
	SubFoldersToCreate.Add("LensFlares");
	SubFoldersToCreate.Add("Morphs");
	SubFoldersToCreate.Add("FaceFX");

	// DEFAULT PREFIXES FOR "FIX NAMING"
	Prefixes = TArray<FClassNameToPrefix>();
	Prefixes.Add({ "Blueprint",						"BP_" });
	Prefixes.Add({ "BlueprintInterface",			"BPI_" });
	Prefixes.Add({ "WidgetBlueprint",				"Widget_" });

	Prefixes.Add({ "SkeletalMesh",					"SK_" });
	Prefixes.Add({ "StaticMesh",					"SM_" });

	Prefixes.Add({ "SoundClass",					"S_" });
	Prefixes.Add({ "SoundWave",						"S_" });
	Prefixes.Add({ "SoundCue",						"SC_" });

	Prefixes.Add({ "ParticleSystem",				"P_" });

	Prefixes.Add({ "Material",						"M_" });
	Prefixes.Add({ "MaterialInstanceConstant",		"MIC_" });
	Prefixes.Add({ "MaterialFunction",				"MF_" });
	Prefixes.Add({ "MaterialParameterCollection",	"MPC_" });
	Prefixes.Add({ "PhysicalMaterial",				"PM_" });

	Prefixes.Add({ "MediaPlayer",					"MP_" });
	Prefixes.Add({ "MediaTexture",					"MDT_" });
	Prefixes.Add({ "ImgMediaSource",				"MS_" });
	Prefixes.Add({ "LevelSequence",					"S_" });

	Prefixes.Add({ "UserDefinedEnum",				"E_" });
	Prefixes.Add({ "DataTable",						"DT_" });
	Prefixes.Add({ "UserDefinedStruct",				"Struct_" });


	Prefixes.Add({ "CurveFloat",					"Curve_" });

	Prefixes.Add({ "PaperSprite",					"SP_" });

	Prefixes.Add({ "VectorFieldStatic",				"VF_" });

	Prefixes.Add({ "Font",							"Font_" });
	Prefixes.Add({ "BlackboardData",				"BB_" });
	Prefixes.Add({ "TextureCube",					"TC_" });

	// DEFAULT GROUP PREFIXES FOR "FIX NAMING"
	PrefixesGroup = TArray<FClassGroupToPrefix>();
	PrefixesGroup.Add({ "RenderTarget",		"RT_" });
	PrefixesGroup.Add({ "Texture",			"T_" });

	// DEFAULT ORGANIZE GROUPS
	OrganizeGroups.Add("SoundCue", "Audio");
	OrganizeGroups.Add("SoundWave", "Audio");
	OrganizeGroups.Add("Blueprint", "Blueprints");
	OrganizeGroups.Add("BlueprintInterface", "Blueprints");
	OrganizeGroups.Add("CurveFloat", "Curves");
	OrganizeGroups.Add("CurveLinearColor", "Curves");
	OrganizeGroups.Add("StaticMesh", "Meshes");
	OrganizeGroups.Add("Material", "Materials");
	OrganizeGroups.Add("MaterialInstanceConstant", "Materials");
	OrganizeGroups.Add("Texture", "Textures");
	OrganizeGroups.Add("Texture2D", "Textures");

	// OPTIMIZE TEXTURE SIZE
	LODBiasOffset = 3;
	MinimumInGameResolution = FIntPoint(256, 256);
	bReduceSizeForSmallObjectsOnly = true;
	MaximumSizeForSmallObjects = FVector(50.0f, 50.0f, 50.0f);
	bAllowDecreaseLODBias = true;
}