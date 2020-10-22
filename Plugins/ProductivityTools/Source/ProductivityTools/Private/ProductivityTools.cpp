// Copyright (c) 2019 Isara Technologies. All Rights Reserved.

#include "ProductivityTools.h"
#include "ProductivityToolsStyle.h"
#include "ProductivityToolsCommands.h"
#include "Misc/MessageDialog.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#include "Consolidate.h"
#include "Clean.h"
#include "CreatePackage.h"
#include "OrganizePackage.h"
#include "FixNaming.h"
#include "OrganizeBlueprint.h"
#include "OptimizeTextureSize.h"
#include "Tools.h"

#include "ProductivityToolsSettings.h"

#include "LevelEditor.h"
#include "ContentBrowserModule.h"

#include "ISettingsModule.h"
#include "ISettingsSection.h"

#include "EdGraphUtilities.h"

//#include "MergeActors/Public/IMergeActorsModule.h"

#include "ConsolidateWindow.h"
#include "UnrealEd.h"

//#include "ConfigCacheIni.h"


static const FName ProductivityToolsTabName("ProductivityTools");

#define LOCTEXT_NAMESPACE "FProductivityToolsModule"


void FProductivityToolsModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FProductivityToolsStyle::Initialize();
	FProductivityToolsStyle::ReloadTextures();

	FProductivityToolsCommands::Register();

	SelectedAssets = TArray<FAssetData>();
	
	ISettingsModule* const SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
	if (SettingsModule != nullptr)
	{
		ISettingsSectionPtr SettingsSection = SettingsModule->RegisterSettings("Project", "Plugins", "ProductivityTools",
			LOCTEXT("ProductivityToolsSettingsName", "Productivity Tools"),
			LOCTEXT("ProductivityToolsSettingsDescription", "Configure the Productivity Tools plug-in."),
			GetMutableDefault<UProductivityToolsSettings>()
		);
		
		auto ResetLambda = []() { GetMutableDefault<UProductivityToolsSettings>()->Reset(); return true; };
		SettingsSection->OnResetDefaults().BindLambda(ResetLambda);
	}

	// Retrieve the specified settings in the project settings
	UProductivityToolsSettings::Settings = GetDefault<UProductivityToolsSettings>();

	// Create custom factories
	TSharedPtr<FCustomPinFactory> CustomGraphPinFactory = MakeShareable(new FCustomPinFactory());
	TSharedPtr<FCustomNodeFactory> CustomGraphNodeFactory = FCustomNodeFactory::GetInstance();

	// Register these factories
	FEdGraphUtilities::RegisterVisualPinFactory(CustomGraphPinFactory);
	FEdGraphUtilities::RegisterVisualNodeFactory(CustomGraphNodeFactory);

	PluginCommands = MakeShareable(new FUICommandList);

	// Extend the Blueprint Editor
	FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
	BlueprintEditorModule.OnGatherBlueprintMenuExtensions().AddRaw(this,&FProductivityToolsModule::OnGatherExtensions);
	BlueprintEditorModule.OnRegisterTabsForEditor().AddRaw(this, &FProductivityToolsModule::OnGatherTabs);

	// Retrieve the ContentBrowserModule
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	
	// Extend the Asset Selection Context Menu
	TArray<FContentBrowserMenuExtender_SelectedAssets>& CBAssetMenuExtenderDelegates = ContentBrowserModule.GetAllAssetViewContextMenuExtenders();
	CBAssetMenuExtenderDelegates.Add(FContentBrowserMenuExtender_SelectedAssets::CreateRaw(this, &FProductivityToolsModule::OnExtendContentBrowserAssetSelectionMenu));
	ContentBrowserAssetExtenderDelegateHandle = CBAssetMenuExtenderDelegates.Last().GetHandle();

	// Extend the Path Selection Context Menu
	TArray<FContentBrowserMenuExtender_SelectedPaths>& CBPathExtenderDelegates = ContentBrowserModule.GetAllPathViewContextMenuExtenders();
	CBPathExtenderDelegates.Add(FContentBrowserMenuExtender_SelectedPaths::CreateRaw(this, &FProductivityToolsModule::OnExtendContentBrowserPathSelectionMenu));
	ContentBrowserPathExtenderDelegateHandle = CBPathExtenderDelegates.Last().GetHandle();

	// Extend the Content Browser Commands
	TArray<FContentBrowserCommandExtender>& CBCommandExtenderDelegates = ContentBrowserModule.GetAllContentBrowserCommandExtenders();
	CBCommandExtenderDelegates.Add(FContentBrowserCommandExtender::CreateRaw(this, &FProductivityToolsModule::OnExtendContentBrowserCommands));
	ContentBrowserCommandExtenderDelegateHandle = CBCommandExtenderDelegates.Last().GetHandle();
}

TSharedRef<FExtender> FProductivityToolsModule::OnExtendContentBrowserAssetSelectionMenu(const TArray<FAssetData>& selectedAssets)
{
	// Retrieve the selected assets in the content browser
	this->SelectedAssets = selectedAssets;
	this->SelectedPaths.Empty();

	// Create an extension in the Asset Selection Context Menu after the AssetContextReferences section
	TSharedRef<FExtender> Extender(new FExtender());
	Extender->AddMenuExtension(
		"AssetContextReferences",
		EExtensionHook::After,
		nullptr,
		FMenuExtensionDelegate::CreateRaw(this, &FProductivityToolsModule::CreateAssetSelectionContextMenu));

	return Extender;
}

TSharedRef<FExtender> FProductivityToolsModule::OnExtendContentBrowserPathSelectionMenu(const TArray<FString>& selectedPaths)
{
	// Retrieve the selected assets in the content browser
	this->SelectedPaths = selectedPaths;
	this->SelectedAssets.Empty();

	// Create an extension in the Path Selection Context Menu after the PathContextBulkOperations section
	TSharedRef<FExtender> Extender(new FExtender());
	Extender->AddMenuExtension(
		"PathContextBulkOperations",
		EExtensionHook::After,
		nullptr,
		FMenuExtensionDelegate::CreateRaw(this, &FProductivityToolsModule::CreatePathSelectionContextMenu));

	return Extender;
}

void FProductivityToolsModule::OnExtendContentBrowserCommands(TSharedRef<FUICommandList> CommandList, FOnContentBrowserGetSelection GetSelectionDelegate)
{
	// Map a function to the Consolidate button
	CommandList->MapAction(FProductivityToolsCommands::Get().ConsolidateAsset,
		FExecuteAction::CreateLambda([this]
		{
			FConsolidate::ConsolidateAsset(SelectedAssets);
		})
	);

	// Map a function to the ConsolidateAll button
	CommandList->MapAction(FProductivityToolsCommands::Get().ConsolidateAllAssets,
		FExecuteAction::CreateLambda([this]
		{
			FConsolidate::ConsolidateAllAssets();
		})
	);

	// Map a function to the Clean button
	CommandList->MapAction(FProductivityToolsCommands::Get().Clean,
		FExecuteAction::CreateLambda([this]
		{
			FClean::Clean(SelectedPaths);
		})
	);

	// Map a function to the CleanProject button
	CommandList->MapAction(FProductivityToolsCommands::Get().CleanProject,
		FExecuteAction::CreateLambda([this]
		{
			FClean::CleanProject();
		})
	);

	// Map a function to the CreatePackage button
	CommandList->MapAction(FProductivityToolsCommands::Get().CreatePackage,
		FExecuteAction::CreateLambda([this]
		{
			FCreatePackage::CreatePackage(SelectedPaths);
		})
	);

	// Map a function to the OrganizePackage button
	CommandList->MapAction(FProductivityToolsCommands::Get().OrganizePackage,
		FExecuteAction::CreateLambda([this]
		{
			FOrganizePackage::OrganizePackage(SelectedAssets, SelectedPaths);
		})
	);

	// Map a function to the FixNaming button
	CommandList->MapAction(FProductivityToolsCommands::Get().FixNaming,
		FExecuteAction::CreateLambda([this]
		{
			FFixNaming::FixNaming(SelectedAssets, SelectedPaths);
		})
	);
	
	// Map a function to the OptimizeTextureSize button
	CommandList->MapAction(FProductivityToolsCommands::Get().OptimizeTextureSize,
		FExecuteAction::CreateLambda([this]
		{
			FOptimizeTextureSize::OptimizeTextureSize(SelectedAssets, SelectedPaths);
		})
	);
}

void FProductivityToolsModule::OnGatherExtensions(TSharedPtr<FExtender> Extender, UBlueprint * Blueprint)
{
	// We create a new command list for each time we extend the blueprint editor toolbar
	// This means we will be able to organize the very blueprint we opened the blueprint editor for
	TSharedPtr<FUICommandList> OrganizeBlueprintCommands = MakeShared<FUICommandList>(); 

	// Map a function to the OrganizeBlueprint button
	OrganizeBlueprintCommands->MapAction(
		FProductivityToolsCommands::Get().OrganizeBlueprint,
		FExecuteAction::CreateRaw(this, &FProductivityToolsModule::OnClickedOrganizeBlueprint, Blueprint)
	);

	// Create an extension in the Blueprint Editor Toolbar after the Settings section
	Extender->AddToolBarExtension(
		"Settings",
		EExtensionHook::After,
		OrganizeBlueprintCommands,
		FToolBarExtensionDelegate::CreateRaw(this, &FProductivityToolsModule::AddToolbarExtension));
}

void FProductivityToolsModule::OnClickedOrganizeBlueprint(UBlueprint* Blueprint)
{
	FOrganizeBlueprint::OrganizeBlueprint(Blueprint);
}

void FProductivityToolsModule::OnGatherTabs(FWorkflowAllowedTabSet& WorkflowAllowedTabSet, FName ModeName, TSharedPtr<FBlueprintEditor> BlueprintEditor)
{
	// Retrieve and map the BlueprintEditor instance with the Blueprint it is editing
	// (this way, we will be able to retrieve the BlueprintEditor when organizing inside a Blueprint )
	if(BlueprintEditor.Get() != nullptr && BlueprintEditor.Get()->GetBlueprintObj() != nullptr)
	{
		FOrganizeBlueprint::BlueprintEditors.Add(BlueprintEditor.Get()->GetBlueprintObj(), BlueprintEditor.Get());
	}
}

void FProductivityToolsModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FProductivityToolsStyle::Shutdown();

	FProductivityToolsCommands::Unregister();
}

void FProductivityToolsModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
	// Add the OrganizeBlueprint button
	Builder.AddToolBarButton(FProductivityToolsCommands::Get().OrganizeBlueprint);
}

void FProductivityToolsModule::CreateAssetSelectionContextMenu(FMenuBuilder& Builder)
{
	// Add a section for Productivity Tools in Asset Selection Context Menu
	Builder.BeginSection("AssetContextProductivityTools", LOCTEXT("ProductivityToolsMenuHeading", "ProductivityTools"));
	{
		// If only one asset is selected in the content browser
		if (SelectedAssets.Num() == 1) {
			// Add the Consolidate button
			Builder.AddMenuEntry(FProductivityToolsCommands::Get().ConsolidateAsset);
		}
		// Add the ConsolidateAll button
		Builder.AddMenuEntry(FProductivityToolsCommands::Get().ConsolidateAllAssets);

		// Add the CleanProject button
		Builder.AddMenuEntry(FProductivityToolsCommands::Get().CleanProject);

		// Add the OrganizePackage button
		Builder.AddMenuEntry(FProductivityToolsCommands::Get().OrganizePackage);

		// Add the Fix Naming button
		Builder.AddMenuEntry(FProductivityToolsCommands::Get().FixNaming);
		
		// If selected assets are textures
		if (FTools::CheckAssetsClass(SelectedAssets, UTexture2D::StaticClass()))
		{
			// Add the Optimize Texture Size button
			Builder.AddMenuEntry(FProductivityToolsCommands::Get().OptimizeTextureSize);
		}
	}
	Builder.EndSection();
}

void FProductivityToolsModule::CreatePathSelectionContextMenu(FMenuBuilder& Builder)
{
	// Add a section for Productivity Tools in Path Selection Context Menu
	Builder.BeginSection("PathContextProductivityTools", LOCTEXT("ProductivityToolsMenuHeading", "ProductivityTools"));
	{
		// Add the ConsolidateAll button
		Builder.AddMenuEntry(FProductivityToolsCommands::Get().ConsolidateAllAssets);

		// Add the CleanProject button
		Builder.AddMenuEntry(FProductivityToolsCommands::Get().Clean);

		// Add the CreatePackage button
		Builder.AddMenuEntry(FProductivityToolsCommands::Get().CreatePackage);	

		// Add the OrganizePackage button
		Builder.AddMenuEntry(FProductivityToolsCommands::Get().OrganizePackage);

		// Add the Fix Naming button
		Builder.AddMenuEntry(FProductivityToolsCommands::Get().FixNaming);
		
		// Add the Optimize Texture Size button
		Builder.AddMenuEntry(FProductivityToolsCommands::Get().OptimizeTextureSize);
	}
	Builder.EndSection();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FProductivityToolsModule, ProductivityTools)