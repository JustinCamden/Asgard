// Copyright (c) 2019 Isara Technologies. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "ProductivityToolsStyle.h"

class FProductivityToolsCommands : public TCommands<FProductivityToolsCommands>
{
public:

	FProductivityToolsCommands()
		: TCommands<FProductivityToolsCommands>(TEXT("ProductivityTools"), NSLOCTEXT("Contexts", "ProductivityTools", "ProductivityTools Plugin"), NAME_None, FProductivityToolsStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > PluginAction;

	TSharedPtr< FUICommandInfo > ConsolidateAsset;
	TSharedPtr< FUICommandInfo > ConsolidateAllAssets;

	TSharedPtr< FUICommandInfo > Clean;
	TSharedPtr< FUICommandInfo > CleanProject;

	TSharedPtr< FUICommandInfo > CreatePackage;
	TSharedPtr< FUICommandInfo > OrganizePackage;

	TSharedPtr< FUICommandInfo > FixNaming;

	TSharedPtr< FUICommandInfo > OrganizeBlueprint;
	
	TSharedPtr< FUICommandInfo > OptimizeTextureSize;
};
