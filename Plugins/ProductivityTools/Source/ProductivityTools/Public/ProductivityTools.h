// Copyright (c) 2019 Isara Technologies. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Framework/MultiBox/MultiBoxExtender.h"
#include "ContentBrowserDelegates.h"
#include "BlueprintEditorModule.h"

class FMenuBuilder;

class FProductivityToolsModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
private:

	/** The current selected assets in the content browser */
	TArray<FAssetData> SelectedAssets;

	/** The current selected paths in the content browser */
	TArray<FString> SelectedPaths;

	void AddToolbarExtension(FToolBarBuilder& Builder);

	void CreateAssetSelectionContextMenu(FMenuBuilder& Builder);
	void CreatePathSelectionContextMenu(FMenuBuilder& Builder);

	TSharedRef<FExtender> OnExtendContentBrowserAssetSelectionMenu(const TArray<FAssetData>& SelectedAssets);
	TSharedRef<FExtender> OnExtendContentBrowserPathSelectionMenu(const TArray<FString>& SelectedPaths);

	void OnExtendContentBrowserCommands(TSharedRef<FUICommandList> CommandList, FOnContentBrowserGetSelection GetSelectionDelegate);

	FDelegateHandle ContentBrowserAssetExtenderDelegateHandle;
	FDelegateHandle ContentBrowserPathExtenderDelegateHandle;
	FDelegateHandle ContentBrowserCommandExtenderDelegateHandle;

	void OnGatherExtensions(TSharedPtr<FExtender> Extender, UBlueprint* Blueprint);
	void OnGatherTabs(FWorkflowAllowedTabSet& WorkflowAllowedTabSet, FName ModeName, TSharedPtr<FBlueprintEditor> BlueprintEditor);

	void OnClickedOrganizeBlueprint(UBlueprint* Blueprint);

private:
	TSharedPtr<class FUICommandList> PluginCommands;

	
};
