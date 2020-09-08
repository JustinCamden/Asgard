// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class FSMAssetTypeActions_Base : public FAssetTypeActions_Base
{
public:
	FSMAssetTypeActions_Base(uint32 Categories);
	uint32 GetCategories() override;
	
protected:
	uint32 MyAssetCategory;
};

class FSMBlueprintAssetTypeActions : public FSMAssetTypeActions_Base
{
public:
	FSMBlueprintAssetTypeActions(uint32 InAssetCategory);

	// FAssetTypeActions_Base
	FText GetName() const override;
	FColor GetTypeColor() const override;
	UClass* GetSupportedClass() const override;
	void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
	// ~FAssetTypeActions_Base

};

/** Wrapper just to hide base instance class from being created in the asset browser. */
class FSMInstanceAssetTypeActions : public FSMAssetTypeActions_Base
{
public:
	FSMInstanceAssetTypeActions(EAssetTypeCategories::Type InAssetCategory);

	// FAssetTypeActions_Base
	FText GetName() const override;
	FColor GetTypeColor() const override;
	UClass* GetSupportedClass() const override;
	// ~FAssetTypeActions_Base
};

/** For editing node classes. */
class FSMNodeInstanceAssetTypeActions : public FSMAssetTypeActions_Base
{
public:
	FSMNodeInstanceAssetTypeActions(uint32 InAssetCategory);

	// FAssetTypeActions_Base
	FText GetName() const override;
	FColor GetTypeColor() const override;
	UClass* GetSupportedClass() const override;
	void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
	// ~FAssetTypeActions_Base

};