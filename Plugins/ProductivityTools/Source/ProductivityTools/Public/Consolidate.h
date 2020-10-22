// Copyright (c) 2019 Isara Technologies. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetData.h"
#include "Widgets/SWindow.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/SListView.h"
#include "ProductivityToolsSettings.h"

DECLARE_LOG_CATEGORY_EXTERN(LogProductivityToolsConsolidate, Log, All);

/**
 * The class for retrieving informations to compare with other assets
 */
class FAssetInfos
{
public:
	/**
	* Default Constructor
	* @param	AssetData	The Source Asset Data to extract the infos
	*/
	FAssetInfos();
	
	/**
	* Constructor
	* @param	AssetData	The Source Asset Data to extract the infos
	*/
	FAssetInfos(FAssetData AssetData);

	/** The full path of the generated .txt file representing the asset */
	FString TempFile;

	/** The Asset Name*/
	FString Name;

	/** The content of the generated .txt file representing the asset */
	TArray<FString> FileData;

	/** The Size of the .uasset file associated with the Asset */
	int64 FileSize;
};

/**
* The Widget Class for the Consolidate function
*/
class SListWidgetConsolidate : public SBorder
{
public:
	SLATE_BEGIN_ARGS(SListWidgetConsolidate)
		: _ParentWindow(), _AssetNameToConsolidateTo()
	{}

	SLATE_ATTRIBUTE(TSharedPtr<SWindow>, ParentWindow)
		SLATE_ATTRIBUTE(FString, AssetNameToConsolidateTo)

		SLATE_END_ARGS()

		void Construct(const FArguments& Args);

	/**
	* Class to support the list box
	*/
	class FListItem
	{
	public:
		/**
		* Constructor
		* @param	InParent	Parent Widget that holds the ListBox
		* @param	InObject	the UObject this list item represents in the ListBox
		*/
		FListItem(SListWidgetConsolidate *InParent, UObject* InObject) :
			Parent(InParent), Object(InObject), CheckState(ECheckBoxState::Unchecked)
		{
		};

		/**
		* Constructor
		* @param	InParent	Parent Widget that holds the ListBox
		* @param	InObject	the UObject this list item represents in the ListBox
		* @param	Checked		whether the item should be checked or not on start
		*/
		FListItem(SListWidgetConsolidate *InParent, UObject* InObject, bool Checked) : FListItem(InParent, InObject)
		{
			Checked ? CheckState = ECheckBoxState::Checked : CheckState = ECheckBoxState::Unchecked;
		};

		/**
		* Callback function to tell the new Checked State of the item
		*/
		void OnAssetSelected(ECheckBoxState NewCheckedState)
		{
			CheckState = NewCheckedState;
			CheckState == ECheckBoxState::Checked ?
				Parent->SelectedItemCount++ : Parent->SelectedItemCount--;

		};

		/** @return		The State of the CheckBox of this item*/
		ECheckBoxState IAssetSelectedState() const
		{
			return CheckState;
		}

		/** @return		Whether the item is selected */
		bool IsAssetSelected() const
		{
			return CheckState == ECheckBoxState::Checked;
		}

		/** @return		The full name of the object this item represents */
		FString GetObjectName() const
		{
			return Object->GetFullName();
		}
		/** @return		The UObject this item represents */
		UObject* GetObject()
		{
			return Object;
		}

	private:
		/** Parent Widget that holds the ListBox */
		SListWidgetConsolidate * Parent;

		/** The UObject this item represents */
		UObject *Object;

		/** The current check state of the item*/
		ECheckBoxState CheckState;
	};

	/** Callback to generate ListBoxRows */
	TSharedRef<ITableRow> OnGenerateRowForList(TSharedPtr<FListItem> Item, const TSharedRef<STableViewBase>& OwnerTable);

	/** The list of items to display */
	TArray<TSharedPtr<FListItem>> Items;

	/** ListBox for selecting which object to consolidate (UI list) */
	TSharedPtr< SListView< TSharedPtr<FListItem> > > ListViewWidget;

	/** The number of selected item in the list*/
	int32 SelectedItemCount;

	/** Asset to consolidate other Assets to*/
	UObject* ObjectToConsolidateTo;

	/**
	* Set the Object to consolidate to
	*
	* @param	ObjectToConsolidateTo		Object which other Objects will consolidate to
	*/
	void SetObjectToConsolidateTo(UObject* ObjectToConsolidateTo);

	/**
	* Add Objects to potentially consolidate to the item list
	*
	* @param	SelectedObjectsToConsolidate		Objects to add to the list (checked by default)
	* @param	OtherObjectsToConsolidate			Objects to add to the list (unchecked by default)
	*/
	void SetObjectsToConsolidate(TArray<UObject*> SelectedObjectsToConsolidate, TArray<UObject*> OtherObjectsToConsolidate);

	/** Called in response to the user clicking the "Consolidate Objects"/OK button; performs asset consolidation */
	FReply OnConsolidateButtonClicked();

	/** Called in response to the user clicking the cancel button; dismisses the panel w/o consolidating objects */
	FReply OnCancelButtonClicked();

	/** Callback for enabling/disabling the Consolidate Button */
	bool IsConsolidateButtonEnabled() const;

	void RefreshList();

private:

	/** A Pointer to our Parent Window */
	TWeakPtr<SWindow> ParentWindowPtr;

	void OnMouseEnter(const FGeometry & MyGeometry, const FPointerEvent & MouseEvent) override;
};

/**
* The class which hold the Consolidate related functions
*/
class FConsolidate
{
public:

	/**
	* Function called by the Consolidate button
	*
	* @param	SelectedAssets		The current Selected Assets in the content browser
	*/
	static void ConsolidateAsset(TArray<FAssetData> SelectedAssets);

	/**
	* Function called by the ConsolidateAll button
	* Loop through all assets in content browser and find duplicates of these assets
	*/
	static void ConsolidateAllAssets();

	/**
	* Calculate the percentage of resemblance between two assets
	* it compares the .uasset file names, the .uasset file sizes and the .txt file contents
	*
	* @param	SourceAssetInfos		The Infos of the Asset to compare to
	* @param	CompareAssetInfos		The Infos of the Asset to compare
	*
	* @return	The percentage of likeness between the two specified assets
	*/
	static float CalculateLikeness(FAssetInfos SourceAssetInfos, FAssetInfos CompareAssetInfos);

	/**
	* Retrieve asset infos from the specified Asset Infos Map
	* if it is not contained into the map, it creates new infos for the specified Asset Data
	*
	* @param	AssetInfosMap		The Map to retrieve the infos from
	* @param	AssetData			The Asset Data from which the infos come from
	*
	* @return	The percentage of likeness between the two specified assets
	*/
	static FAssetInfos RetrieveAssetInfos(TMap< FAssetData, FAssetInfos>* AssetInfosMap, FAssetData AssetData);

	/**
	* Create a window to ask the user which assets to consolidate
	*
	* @param	AssetToConsolidateTo			The Asset to consolidate to
	* @param	SelectedObjectsToConsolidate	Assets to eventually consolidate (checked by default)
	* @param	OtherObjectsToConsolidate		Assets to eventually consolidate (unchecked by default)
	* @param	LikenessMap						The Map to retrieve the likeness of each asset from (in order to sort the assets)
	* @param	MakeMainFrameWindow				Whether or not to make the window a child of Mainframe (will be always on top)
	*/
	static void CreateConsolidateWindow(UObject* AssetToConsolidateTo, 
		TArray<UObject*> SelectedObjectsToConsolidate, TArray<UObject*> OtherObjectsToConsolidate, TMap<FString, float> LikenessMap);

	/**
	* Retrieve a pointer to the widget's window for consolidating to the specified Asset
	* Allow to change an existing window instead of creating another one
	*
	* @param	AssetToConsolidateTo			The Asset to consolidate to
	*
	* @return	A pointer to the existing instance of the widget (if any)
	*/
	static TWeakPtr<SListWidgetConsolidate> RetrieveWidgetInstance(UObject* AssetToConsolidateTo);

private:
	/**
	* An array of pointers to existing instances of Widgets
	*/
	static TArray<TWeakPtr<SListWidgetConsolidate>> WidgetInstances;

};

/**
* Struct for sorting the objects
*/
struct FSortByLikeness {
	static TMap<FString, float> LikenessMap;
	static bool SortObjectArray(UObject* Asset1, UObject* Asset2)
	{
		if (Asset1 != nullptr && Asset2 != nullptr)
		{
			return LikenessMap[Asset1->GetFullName()] > LikenessMap[Asset2->GetFullName()];
		}
		else
		{
			return false;
		}
	};
};
TMap<FString, float> FSortByLikeness::LikenessMap = TMap<FString, float>();

/**
* Struct to make a two-keys Map
*/
struct FLikenessMapKey
{
	FAssetData a1;
	FAssetData a2;

	bool operator==(const FLikenessMapKey& k) const
	{
		return a1 == k.a1 && a2 == k.a2;
	}

	friend uint32 GetTypeHash(const FLikenessMapKey& k)
	{
		return GetTypeHash(k.a1.GetFullName() + k.a2.GetFullName());
	}
};



