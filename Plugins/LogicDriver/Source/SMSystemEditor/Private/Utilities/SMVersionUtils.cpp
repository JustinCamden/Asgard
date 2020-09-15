// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#include "SMVersionUtils.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "AssetRegistryModule.h"
#include "Utilities/SMBlueprintEditorUtils.h"
#include "Misc/ScopedSlowTask.h"


#define LOCTEXT_NAMESPACE "SMVersionUtils"

void FSMVersionUtils::UpdateBlueprintsToNewVersion()
{
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry")).Get();
	
	TArray<FAssetData> OutAssets;
	AssetRegistry.GetAssetsByClass(USMBlueprint::StaticClass()->GetFName(), OutAssets, true);

	TArray<FAssetData> AssetsToUpdate;
	for (const FAssetData& Asset : OutAssets)
	{
		int32 Version;
		
		const bool bVersionFound = Asset.GetTagValue(GET_MEMBER_NAME_CHECKED(USMBlueprint, AssetVersion), Version);
		if (!bVersionFound || !IsStateMachineUpToDate(Version))
		{
			AssetsToUpdate.Add(Asset);
		}
	}

	if (AssetsToUpdate.Num() > 0)
	{
		FScopedSlowTask Feedback(AssetsToUpdate.Num(), NSLOCTEXT("LogicDriver", "LogicDriverAssetUpdate", "Updating Logic Driver assets to the current version..."));

		if (FSMBlueprintEditorUtils::GetProjectEditorSettings()->bDisplayAssetUpdateProgress)
		{
			Feedback.MakeDialog(true);
		}
		
		for (FAssetData& Asset : AssetsToUpdate)
		{
			USMBlueprint* SMBlueprint = CastChecked<USMBlueprint>(Asset.GetAsset());
			
			TArray<USMGraphNode_Base*> GraphNodes;
			FSMBlueprintEditorUtils::GetAllNodesOfClassNested<USMGraphNode_Base>(SMBlueprint, GraphNodes);

			for (USMGraphNode_Base* Node : GraphNodes)
			{
				Node->ConvertToCurrentVersion(false);
			}

			SetToLatestVersion(SMBlueprint);
			SMBlueprint->MarkPackageDirty();
			
			Feedback.CompletedWork += 1;
		}
	}
}

bool FSMVersionUtils::IsStateMachineUpToDate(int32 CompareVersion)
{
	return CompareVersion >= GetCurrentBlueprintVersion();
}

bool FSMVersionUtils::IsStateMachineUpToDate(USMBlueprint* Blueprint)
{
	const int32 Version = Blueprint->AssetVersion;
	return IsStateMachineUpToDate(Version);
}

void FSMVersionUtils::SetToLatestVersion(UBlueprint* Blueprint)
{
	if (USMBlueprint* SMBlueprint = Cast<USMBlueprint>(Blueprint))
	{
		SMBlueprint->AssetVersion = GetCurrentBlueprintVersion();
	}
}

#undef LOCTEXT_NAMESPACE
