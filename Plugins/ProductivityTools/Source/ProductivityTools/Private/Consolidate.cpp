// Copyright (c) 2019 Isara Technologies. All Rights Reserved.

#include "Consolidate.h"
#include "ProductivityToolsSettings.h"

#include "ObjectTools.h"
#include "Engine/ObjectLibrary.h"
#include "Widgets/SWindow.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Framework/Application/SlateApplication.h"
#include "EditorStyle.h"
#include "AssetToolsModule.h"
#include "Misc/FileHelper.h"
//#include "MainFrame/Public/MainFrame.h"
#include "Editor/MainFrame/Public/Interfaces/IMainFrameModule.h"

DEFINE_LOG_CATEGORY(LogProductivityToolsConsolidate);

#define LOCTEXT_NAMESPACE "FProductivityToolsModule"

FAssetInfos FConsolidate::RetrieveAssetInfos(TMap< FAssetData, FAssetInfos>* AssetInfosMap, FAssetData AssetData)
{
	FAssetInfos AssetInfos = FAssetInfos();
	// Verify if we already calculated the infos of the asset
	if (AssetInfosMap->Contains(AssetData))
	{
		// if we already calculated the infos of the asset
		// retrieve the infos from the map
		AssetInfos = (*AssetInfosMap)[AssetData];
	}
	else
	{
		// else, calculate new infos for the asset
		AssetInfos = FAssetInfos(AssetData);
		AssetInfosMap->Add(AssetData, AssetInfos);
	}
	return AssetInfos;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void FConsolidate::ConsolidateAsset(TArray<FAssetData> SelectedAssets)
{
	// Make sure there is only one selected asset
	if (SelectedAssets.Num() == 1)
	{
		FDateTime StartTime = FDateTime::Now();

		FAssetData SelectedAssetData = SelectedAssets[0];
		UE_LOG(LogProductivityToolsConsolidate, Warning, TEXT("%s"), *SelectedAssetData.GetFullName());

		// Retrieve all assets with same class as selected asset
		UObjectLibrary *ObjectLibrary = UObjectLibrary::CreateLibrary(SelectedAssetData.GetClass(), false, true);
		ObjectLibrary->LoadAssetDataFromPath(TEXT("/Game"));
		TArray<FAssetData> AssetDatas;
		ObjectLibrary->GetAssetDataList(AssetDatas);

		// A map associating an asset's full path to the percentage resemblance of this asset with the selected asset
		TMap<FString, float> LikenessMap = TMap<FString, float>();

		// Arrays containing the available assets to consolidate
		TArray<UObject*> SelectedObjectsToConsolidate = TArray<UObject*>();
		TArray<UObject*> OtherObjectsToConsolidate = TArray<UObject*>();

		// Retrieve Selected Asset Infos for likeness calculation 
		// this way, it won't recreate infos for the selected asset each iteration
		FAssetInfos SourceAssetInfos = FAssetInfos(SelectedAssetData);

		// for each asset of the same class as the selected asset
		for (FAssetData AssetData : AssetDatas)
		{
			// Don't use the current selected asset
			if (SelectedAssetData != AssetData)
			{
				// Retrieve Infos from the Asset to compare
				FAssetInfos AssetInfos = FAssetInfos(AssetData);

				// Calculate the likeness of the Asset to compare to be a duplicate to consolidate
				// (Likeness is between 0 and 1)
				float Likeness = CalculateLikeness(SourceAssetInfos, AssetInfos);

				// Map the likeness to the asset to compare in order to sort these assets later
				LikenessMap.Add(AssetData.GetFullName(), Likeness);
				
				// If the Asset is most likely a duplicate of the SelectedAsset
				if (Likeness >= SETTINGS->DuplicateThreshold_Consolidate)
				{
					SelectedObjectsToConsolidate.Add(AssetData.GetAsset());
				}
				// Still keep the Assets of the same class for the user to choose
				else
				{
					OtherObjectsToConsolidate.Add(AssetData.GetAsset());
				}
			}
		}
		
		// Create a window for the user to confirm the consolidation
		CreateConsolidateWindow(SelectedAssetData.GetAsset(),
			SelectedObjectsToConsolidate, OtherObjectsToConsolidate, LikenessMap);

		// Print to log the Execution Time
		FTimespan ExecTime = FDateTime::Now() - StartTime;
		UE_LOG(LogProductivityToolsConsolidate, Warning, TEXT("Compared %d assets in %fs"), AssetDatas.Num() - 1, ExecTime.GetTotalSeconds());
	}
}

void FConsolidate::ConsolidateAllAssets()
{
	FDateTime StartTime = FDateTime::Now();

	// Retrieve all assets in the content browser
	UObjectLibrary *ObjectLibrary = UObjectLibrary::CreateLibrary(UObject::StaticClass(), false, true);
	ObjectLibrary->LoadAssetDataFromPath(TEXT("/Game"));
	TArray<FAssetData> AssetDatas;
	ObjectLibrary->GetAssetDataList(AssetDatas);

	// Make a loading bar
	FScopedSlowTask Progress(AssetDatas.Num() - 1, LOCTEXT("ConsolidateAllLoading", "Finding assets to consolidate..."));
	Progress.MakeDialog();

	// Map to save the likeness between assets
	TMap< FLikenessMapKey, float > AssetLikenessMap = TMap< FLikenessMapKey, float >();

	// Map to save the infos for each asset (to not calculate it multiple times)
	TMap< FAssetData, FAssetInfos> AssetInfosMap = TMap< FAssetData, FAssetInfos>();

	// Array to save the assets for which we created a consolidate window (to avoid multiple windows for same duplicates)
	TArray< FAssetData> WindowCreatedAssets = TArray< FAssetData>();

	// for each asset in the content browser
	for (FAssetData SourceAssetData : AssetDatas)
	{
		// Retrieve infos of the asset
		FAssetInfos SourceAssetInfos = RetrieveAssetInfos(&AssetInfosMap, SourceAssetData);

		// Retrieve all assets of the same class as the source asset
		ObjectLibrary = UObjectLibrary::CreateLibrary(SourceAssetData.GetClass(), false, true);
		ObjectLibrary->LoadAssetDataFromPath(TEXT("/Game"));
		TArray<FAssetData> CompareAssetDatas;
		ObjectLibrary->GetAssetDataList(CompareAssetDatas);

		// A map associating an asset's full path to the percentage resemblance of this asset with the source asset
		TMap<FString, float> LikenessMap = TMap<FString, float>();

		// Arrays containing the available assets to consolidate with the source asset
		TArray<UObject*> SelectedObjectsToConsolidate = TArray<UObject*>();
		TArray<UObject*> OtherObjectsToConsolidate = TArray<UObject*>();

		bool IsConsolidateNecessary = false;
		bool IsWindowAlreadyCreated = false;

		// for each asset with the same class as current source asset
		for (FAssetData CompareAssetData : CompareAssetDatas) 
		{
			// Verify that it does not compare the same asset but that the assets have the same class
			if (SourceAssetData != CompareAssetData 
				&& SourceAssetData.GetAsset()->GetClass() == CompareAssetData.GetAsset()->GetClass())
			{
				// Verify if the likeness between the assets have not already been calculated
				if (!AssetLikenessMap.Contains({ CompareAssetData,SourceAssetData }))
				{
					// Retrieve infos of the asset
					FAssetInfos CompareAssetInfos = RetrieveAssetInfos(&AssetInfosMap, CompareAssetData);

					// Calculate likeness between the assets
					float Likeness = CalculateLikeness(SourceAssetInfos, CompareAssetInfos);

					// Save the Likeness into a map used for the displayed window
					LikenessMap.Add(CompareAssetData.GetFullName(), Likeness);

					// If the Asset is most likely a duplicate of the SelectedAsset
					if (Likeness >= SETTINGS->DuplicateThreshold_ConsolidateAll)
					{
						IsConsolidateNecessary = true;
						SelectedObjectsToConsolidate.Add(CompareAssetData.GetAsset());
					}
					// Still keep the Assets of the same class for the user to choose
					else
					{
						OtherObjectsToConsolidate.Add(CompareAssetData.GetAsset());
					}

					// Save it into a map to not have to calculate it again
					AssetLikenessMap.Add({ SourceAssetData,CompareAssetData }, Likeness);

					UE_LOG(LogProductivityToolsConsolidate, Warning, TEXT("{ %s, %s } : %f%%"), *SourceAssetData.GetFullName(), 
						*CompareAssetData.GetFullName(), Likeness * 100);
				}
				// if the Likeness has already been calculated
				else
				{
					// Retrieve the Likeness between the assets
					float Likeness = AssetLikenessMap[{CompareAssetData, SourceAssetData}];

					// Save the Likeness into a map used for the displayed window
					LikenessMap.Add(CompareAssetData.GetFullName(), Likeness);

					// Verify if they are likely to be duplicates
					// in this case, a window should have already been created
					if (Likeness >= SETTINGS->DuplicateThreshold_ConsolidateAll)
					{
						//SelectedObjectsToConsolidate.Add(CompareAssetData.GetAsset());
						IsWindowAlreadyCreated = true;
						// No need to search trough other assets 
						// because they were already compared with the one which created the previous window
						break;
					}
					else {
						OtherObjectsToConsolidate.Add(CompareAssetData.GetAsset());
					}

				}
			}
		}

		// if one of the compared asset is likely to be a duplicate and no windows have been created for this case
		if (IsConsolidateNecessary && !IsWindowAlreadyCreated)
		{
			// Create a new window for the user to confirm the consolidation for these assets
			CreateConsolidateWindow(SourceAssetData.GetAsset(),
				SelectedObjectsToConsolidate, OtherObjectsToConsolidate, LikenessMap);
			WindowCreatedAssets.Add(SourceAssetData);
		}

		// Add progress to the loading bar
		Progress.EnterProgressFrame(1.f);
	}

	// Print to log the Execution Time
	FTimespan ExecTime = FDateTime::Now() - StartTime;
	UE_LOG(LogProductivityToolsConsolidate, Warning, TEXT("Compared %d assets in %fs"), AssetDatas.Num() - 1, ExecTime.GetTotalSeconds());
}

TArray<TWeakPtr<SListWidgetConsolidate>> FConsolidate::WidgetInstances;

void FConsolidate::CreateConsolidateWindow(UObject* AssetToConsolidateTo,
	TArray<UObject*> SelectedObjectsToConsolidate, TArray<UObject*> OtherObjectsToConsolidate, TMap<FString, float> LikenessMap)
{
	// Sort the ObjectsToConsolidate by likeness
	FSortByLikeness::LikenessMap = LikenessMap;
	Algo::Sort(SelectedObjectsToConsolidate, FSortByLikeness::SortObjectArray);
	Algo::Sort(OtherObjectsToConsolidate, FSortByLikeness::SortObjectArray);

	// Find an widget instance of a window to consolidate to the specified asset (if any)
	TWeakPtr<SListWidgetConsolidate> WidgetInstance = RetrieveWidgetInstance(AssetToConsolidateTo);
	
	// if a valid widget instance has been found
	if (WidgetInstance != nullptr && WidgetInstance.IsValid())
	{
		//Use the existing widget by setting the new Assets to Consolidate
		WidgetInstance.Pin()->SetObjectToConsolidateTo(AssetToConsolidateTo);
		WidgetInstance.Pin()->SetObjectsToConsolidate(SelectedObjectsToConsolidate, OtherObjectsToConsolidate);
	}
	// if no widget instance were found
	else
	{
		// Create a new window
		TSharedRef<SWindow> NewWindow = SNew(SWindow)
			.Title(FText::FromString("Consolidate Assets"))
			.ClientSize(FVector2D(768, 300))
			.SupportsMinimize(true)
			.SupportsMaximize(false);

		// Create the widget
		TSharedRef<SListWidgetConsolidate> NewWidget =
			SNew(SListWidgetConsolidate)
			.AssetNameToConsolidateTo(AssetToConsolidateTo->GetName())
			.ParentWindow(NewWindow);

		// Add the objects into the new widget
		NewWidget->SetObjectToConsolidateTo(AssetToConsolidateTo);
		NewWidget->SetObjectsToConsolidate(SelectedObjectsToConsolidate, OtherObjectsToConsolidate);

		NewWindow->SetContent(NewWidget);

		// Add the widget instance to an array (in order to find it later)
		WidgetInstances.Add(NewWidget);

		//FSlateApplication::Get().AddWindow(NewWindow);
		IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));

		if (MainFrameModule.GetParentWindow().IsValid())
		{
			FSlateApplication::Get().AddWindowAsNativeChild(NewWindow, MainFrameModule.GetParentWindow().ToSharedRef());
		}
		else
		{
			FSlateApplication::Get().AddWindow(NewWindow);
		}
	}
}

TWeakPtr<SListWidgetConsolidate> FConsolidate::RetrieveWidgetInstance(UObject* AssetToConsolidateTo)
{
	for (TWeakPtr<SListWidgetConsolidate> WidgetInstance : WidgetInstances)
	{
		if (WidgetInstance.IsValid())
		{
			if (WidgetInstance.Pin()->ObjectToConsolidateTo == AssetToConsolidateTo)
			{
				return WidgetInstance;
			}
		}

	}
	return nullptr;
}

float FConsolidate::CalculateLikeness(FAssetInfos SourceAssetInfos, FAssetInfos CompareAssetInfos)
{
	UE_LOG(LogProductivityToolsConsolidate, Warning, TEXT("====================="));
	//UE_LOG(LogProductivityToolsConsolidate, Warning, TEXT("asset temp filename : %s"), *SourceAssetInfos.TempFile);
	//UE_LOG(LogProductivityToolsConsolidate, Warning, TEXT("asset temp filename : %s"), *CompareAssetInfos.TempFile);

	UE_LOG(LogProductivityToolsConsolidate, Warning, TEXT("asset name : %s"), *SourceAssetInfos.Name);
	UE_LOG(LogProductivityToolsConsolidate, Warning, TEXT("asset name : %s"), *CompareAssetInfos.Name);

	//UE_LOG(LogProductivityToolsConsolidate, Warning, TEXT("file data line count : %d"), SourceAssetInfos.FileData.Num());
	//UE_LOG(LogProductivityToolsConsolidate, Warning, TEXT("file data line count : %d"), CompareAssetInfos.FileData.Num());

	TMap<float, float> WeightedLikenessMap = TMap<float, float>();

	// Compare the file names
	if (SETTINGS->bEnableFileNameCriteria)
	{
		int32 CharCountMin = FMath::Min(SourceAssetInfos.Name.Len(), CompareAssetInfos.Name.Len());
		int32 CharCountMax = FMath::Max(SourceAssetInfos.Name.Len(), CompareAssetInfos.Name.Len());
		int32 EqualCharCount = 0;
		if (SourceAssetInfos.Name.Contains(CompareAssetInfos.Name))
		{
			EqualCharCount = CompareAssetInfos.Name.Len();
		}
		else if (CompareAssetInfos.Name.Contains(SourceAssetInfos.Name))
		{
			EqualCharCount = SourceAssetInfos.Name.Len();
		}
		else
		{
			int32 i = 0;
			while (i < CharCountMin - 1)
			{
				if (SourceAssetInfos.Name[i] == CompareAssetInfos.Name[i])
				{
					EqualCharCount++;
				}
				i++;
			}
		}
		float LikenessEqualChar = (float)EqualCharCount / (float)CharCountMin;
		UE_LOG(LogProductivityToolsConsolidate, Warning, TEXT("filename equal char count : %d out of %d"), EqualCharCount, CharCountMax);
		WeightedLikenessMap.Add(LikenessEqualChar, SETTINGS->FileNameCriteriaWeight);
	}

	// Compare the txt files of the Assets
	if (SETTINGS->bEnableTextFileCriteria)
	{
		int32 LineCountMin = FMath::Min(SourceAssetInfos.FileData.Num(), CompareAssetInfos.FileData.Num());
		int32 LineCountMax = FMath::Max(SourceAssetInfos.FileData.Num(), CompareAssetInfos.FileData.Num());
		int32 i = 0;
		int32 DifferentLinesCount = 0;
		int32 ComparableLines = LineCountMin;
		while (i < LineCountMin - 1)
		{
			if (SourceAssetInfos.FileData[i].Contains("End Object") || CompareAssetInfos.FileData[i].Contains("End Object"))
			{
				ComparableLines--;
			}
			else if (!CompareAssetInfos.FileData.Contains(SourceAssetInfos.FileData[i]) 
				&& !SourceAssetInfos.FileData.Contains(CompareAssetInfos.FileData[i]))
			{
				DifferentLinesCount++;
			}
			i++;
		}
		float LikenessDifferentLines = 1 - ((float)DifferentLinesCount / (float)ComparableLines);
		UE_LOG(LogProductivityToolsConsolidate, Warning, TEXT("diff lines : %d out of %d"), DifferentLinesCount, ComparableLines);
		WeightedLikenessMap.Add(LikenessDifferentLines, SETTINGS->TextFileCriteriaWeight);
	}

	// Compare file sizes
	if (SETTINGS->bEnableFileSizeCriteria)
	{
		int64 FileSizeMin = FMath::Min(SourceAssetInfos.FileSize, CompareAssetInfos.FileSize);
		int64 FileSizeMax = FMath::Max(SourceAssetInfos.FileSize, CompareAssetInfos.FileSize);
		float LikenessFileSize = (float)FileSizeMin / (float)FileSizeMax;
		UE_LOG(LogProductivityToolsConsolidate, Warning, TEXT("size diff : %d"), FMath::Abs(SourceAssetInfos.FileSize - CompareAssetInfos.FileSize));
		WeightedLikenessMap.Add(LikenessFileSize, SETTINGS->FileSizeCriteriaWeight);
	}

	// Calculate total likeness
	float Likeness = 0;
	float Weight = 0;
	for (const TPair<float, float>& Pair : WeightedLikenessMap)
	{
		Likeness += Pair.Key * Pair.Value;
		Weight += Pair.Value;
	}
	Likeness = Likeness / Weight;
	UE_LOG(LogProductivityToolsConsolidate, Warning, TEXT("LIKENESS : %f%%"), Likeness * 100);

	return Likeness;
}

void SListWidgetConsolidate::Construct(const FArguments& Args)
{
	ParentWindowPtr = Args._ParentWindow.Get();
	FString AssetNameToConsolidateTo = Args._AssetNameToConsolidateTo.Get();
	ChildSlot
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(5)
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Text(FText::Format(LOCTEXT("Consolidate_Select", "Select the assets to consolidate. This will replace all uses of the selected assets below with {0}."), FText::FromString(AssetNameToConsolidateTo)))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			.Padding(5)
			[
				SNew(SBorder)
				.Padding(5)
				[
					SAssignNew(ListViewWidget, SListView<TSharedPtr<FListItem>>)
					.ItemHeight(24)
					.ListItemsSource(&Items)
					.OnGenerateRow(this, &SListWidgetConsolidate::OnGenerateRowForList)
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Fill)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.Padding(5)
				.HAlign(HAlign_Right)
				.FillWidth(1)
				[
					SNew(SButton)
					.Text(LOCTEXT("ConsolidateAssetsButton", "Consolidate Assets"))
					.IsEnabled(this, &SListWidgetConsolidate::IsConsolidateButtonEnabled)
					.OnClicked(this, &SListWidgetConsolidate::OnConsolidateButtonClicked)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(5)
				.HAlign(HAlign_Right)
				[
					SNew(SButton)
					.Text(LOCTEXT("CancelConsolidateButton", "Cancel"))
					.OnClicked(this, &SListWidgetConsolidate::OnCancelButtonClicked)
				]
			]
		];
}

void SListWidgetConsolidate::SetObjectToConsolidateTo(UObject* objectToConsolidateTo)
{
	this->ObjectToConsolidateTo = objectToConsolidateTo;
}

void SListWidgetConsolidate::SetObjectsToConsolidate(TArray<UObject*> SelectedObjectsToConsolidate, TArray<UObject*> OtherObjectsToConsolidate)
{
	Items.Empty();
	SelectedItemCount = SelectedObjectsToConsolidate.Num();
	for (UObject* Asset : SelectedObjectsToConsolidate)
	{
		// Add the Asset to the item list as a checked item
		Items.Add(MakeShareable(new FListItem(this, Asset, true)));
	}
	for (UObject* Asset : OtherObjectsToConsolidate)
	{
		// Add the Asset to the item list as an unchecked item
		Items.Add(MakeShareable(new FListItem(this, Asset, false)));
	}

	ListViewWidget->RequestListRefresh();
}

FReply SListWidgetConsolidate::OnConsolidateButtonClicked()
{
	TArray<UObject*> FinalObjectsToConsolidate = TArray<UObject*>();
	// Retrieve all the assets that have been selected by the user
	for (TSharedPtr<FListItem>& Item : Items)
	{
		if (Item->IsAssetSelected())
		{
			// Verify if the Object has not been deleted since the creation of the window
			if (Item->GetObject()->IsValidLowLevel())
			{
				FinalObjectsToConsolidate.Add(Item->GetObject());
			}
		}
	}
	ObjectTools::ConsolidateObjects(ObjectToConsolidateTo, FinalObjectsToConsolidate);

	ParentWindowPtr.Pin()->RequestDestroyWindow();
	Items.Empty();

	return FReply::Handled();
}

FReply SListWidgetConsolidate::OnCancelButtonClicked()
{
	// Close the window
	ParentWindowPtr.Pin()->RequestDestroyWindow();
	Items.Empty();
	return FReply::Handled();
}

TSharedRef<ITableRow> SListWidgetConsolidate::OnGenerateRowForList(TSharedPtr<FListItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	// Verify if Asset still exist
	if (Item->GetObject()->IsValidLowLevel())
	{
		//Create the row
		return
			SNew(STableRow< TSharedPtr<FString> >, OwnerTable)
			.Padding(2.0f)
			[
				SNew(SCheckBox)
				//.Style(FEditorStyle::Get(), "Menu.RadioButton")
				.IsChecked(Item.ToSharedRef(), &FListItem::IAssetSelectedState)
				.OnCheckStateChanged(Item.ToSharedRef(), &FListItem::OnAssetSelected)
				[
					SNew(STextBlock)
					.Text(FText::FromString((Item->GetObjectName())))
				]
			];
	}
	else
	{
		Items.Remove(Item);
		return SNew(STableRow<TSharedPtr<FString>>, OwnerTable);
	}
}

bool SListWidgetConsolidate::IsConsolidateButtonEnabled() const
{
	return SelectedItemCount != 0;
}

void SListWidgetConsolidate::RefreshList()
{
	TArray<TSharedPtr<FListItem>> ItemsToRemove = TArray<TSharedPtr<FListItem>>();
	for (TSharedPtr<FListItem> Item : Items)
	{
		if (!Item->GetObject()->IsValidLowLevel())
		{
			ItemsToRemove.Add(Item);
		}
	}
	for (TSharedPtr<FListItem> Item : ItemsToRemove)
	{
		Items.Remove(Item);
	}
	ListViewWidget->RequestListRefresh();
}

void SListWidgetConsolidate::OnMouseEnter(const FGeometry & MyGeometry, const FPointerEvent & MouseEvent)
{
	RefreshList();
}

FAssetInfos::FAssetInfos(FAssetData AssetData)
{
	FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
	// Create a text file (may need to delete it after use)
	TempFile = AssetToolsModule.Get().DumpAssetToTempFile(AssetData.GetAsset());
	// Retrieve the asset name
	Name = AssetData.ToSoftObjectPath().GetAssetName();
	// Retrieve the file size
	FFileHelper::LoadFileToStringArray(FileData, *TempFile);
	FileSize = IFileManager::Get().FileSize(*TempFile);
}

FAssetInfos::FAssetInfos()
{
}


#undef LOCTEXT_NAMESPACE