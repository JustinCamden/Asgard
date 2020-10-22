// Copyright (c) 2019 Isara Technologies. All Rights Reserved.

#include "ProductivityToolsCommands.h"

#define LOCTEXT_NAMESPACE "FProductivityToolsModule"

void FProductivityToolsCommands::RegisterCommands()
{
	UI_COMMAND(PluginAction, "ProductivityTools", 
		"Execute ProductivityTools action", 
		EUserInterfaceActionType::Button, 
		FInputChord());

	UI_COMMAND(ConsolidateAsset, "Consolidate", 
		"Remove similar assets and replace with an instance of the selected one", 
		EUserInterfaceActionType::Button, 
		FInputChord(/*EModifierKey::Shift | EModifierKey::Alt, EKeys::R*/));

	UI_COMMAND(ConsolidateAllAssets, "Consolidate All",
		"Consolidate all assets",
		EUserInterfaceActionType::Button,
		FInputChord());

	UI_COMMAND(Clean, "Clean",
		"Remove unused assets and empty folders in selected folder",
		EUserInterfaceActionType::Button,
		FInputChord());

	UI_COMMAND(CleanProject, "Clean Project",
		"Remove unused assets and empty folders in project",
		EUserInterfaceActionType::Button,
		FInputChord());

	UI_COMMAND(CreatePackage, "Create Package",
		"Create a folder with a configured preset of sub-folders",
		EUserInterfaceActionType::Button,
		FInputChord());

	UI_COMMAND(OrganizePackage, "Organize Package",
		"Split selected assets into folders according to their types",
		EUserInterfaceActionType::Button,
		FInputChord());

	UI_COMMAND(FixNaming, "Fix Naming",
		"Apply Assets naming convention to selected Assets",
		EUserInterfaceActionType::Button,
		FInputChord());

	UI_COMMAND(OrganizeBlueprint, "Organize",
		"Organize nodes in graph",
		EUserInterfaceActionType::Button,
		FInputChord());
		
	UI_COMMAND(OptimizeTextureSize, "Optimize Texture Size",
		"Reduce textures size for all textures used by static meshes in project if needed",
		EUserInterfaceActionType::Button,
		FInputChord());
}

#undef LOCTEXT_NAMESPACE
