// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#include "SMExtendedEditorModule.h"
#include "Configuration/SMExtendedEditorStyle.h"
#include "ISettingsModule.h"
#include "Configuration/SMExtendedEditorSettings.h"
#include "SMExtendedEditorUtils.h"
#include "Commands/SMExtendedEditorCommands.h"
#include "SMBlueprintEditor.h"

#define LOCTEXT_NAMESPACE "SMExtendedEditorModule"

void FSMExtendedEditorModule::StartupModule()
{
	// 4.24 Fix for getting around developer module deprecation and BP nativization not being supported.
	const FString LongName = FPackageName::ConvertToLongScriptPackageName(TEXT("SMExtendedEditor"));
	if (UPackage* Package = Cast<UPackage>(StaticFindObjectFast(UPackage::StaticClass(), nullptr, *LongName, false, false)))
	{
		Package->SetPackageFlags(PKG_EditorOnly);
	}
	
	FSMExtendedEditorStyle::Initialize();
	FSMExtendedEditorCommands::Register();
	RegisterSettings();

	// Variable renames have special handling.
	RenameVariableReferencesDelegateHandle = FSMExtendedEditorUtils::OnRenameVariableReferencesEvent.AddStatic(&FSMExtendedEditorUtils::HandleRenameVariableReferencesEvent);
	RenameGraphsDelegateHandle = USMBlueprint::OnRenameGraphEvent.AddStatic(&FSMExtendedEditorUtils::HandleRenameGraphEvent);
	OnEditorCommandsCreatedHandle = FSMBlueprintEditor::OnCreateGraphEditorCommandsEvent.AddStatic(&FSMExtendedEditorCommands::OnEditorCommandsCreated);
}

void FSMExtendedEditorModule::ShutdownModule()
{
	FSMExtendedEditorUtils::OnRenameVariableReferencesEvent.Remove(RenameVariableReferencesDelegateHandle);
	USMBlueprint::OnRenameGraphEvent.Remove(RenameGraphsDelegateHandle);
	FSMBlueprintEditor::OnCreateGraphEditorCommandsEvent.Remove(OnEditorCommandsCreatedHandle);
	
	FSMExtendedEditorStyle::Shutdown();
	FSMExtendedEditorCommands::Unregister();
	UnregisterSettings();
}

void FSMExtendedEditorModule::RegisterSettings()
{
	/* TODO: Enable if there's ever specific extended editor settings.
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings("Editor", "ContentEditors", "StateMachineDialogueEditor",
			LOCTEXT("SMStateMachineSettingsName", "State Machine Dialogue Editor"),
			LOCTEXT("SMStateMachineSettingsDescription", "Configure state machines for dialogues."),
			GetMutableDefault<USMExtendedEditorSettings>());
	}
	*/
}

void FSMExtendedEditorModule::UnregisterSettings()
{
	/*
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Editor", "ContentEditors", "StateMachineDialogueEditor");
	}
	*/
}

#undef LOCTEXT_NAMESPACE

