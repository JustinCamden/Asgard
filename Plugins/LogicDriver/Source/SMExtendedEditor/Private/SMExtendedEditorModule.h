// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "ISMExtendedEditorModule.h"


#define LOCTEXT_NAMESPACE "SMSystemEditorModule"

class FExtensibilityManager;

class FSMExtendedEditorModule : public ISMExtendedEditorModule
{
public:
	/** IModuleInterface implementation */
	void StartupModule() override;
	void ShutdownModule() override;

protected:
	void RegisterSettings();
	void UnregisterSettings();

private:
	FDelegateHandle RenameVariableReferencesDelegateHandle;
	FDelegateHandle RenameGraphsDelegateHandle;
	FDelegateHandle OnEditorCommandsCreatedHandle;
};

IMPLEMENT_MODULE(FSMExtendedEditorModule, SMExtendedEditor)


#undef LOCTEXT_NAMESPACE

