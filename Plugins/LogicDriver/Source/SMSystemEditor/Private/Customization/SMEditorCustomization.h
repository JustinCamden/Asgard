// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "DetailLayoutBuilder.h"
#include "SMBlueprintGeneratedClass.h"

template<typename T>
static T* GetObjectBeingCustomized(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);
	for(TWeakObjectPtr<UObject> Object : Objects)
	{
		if(T* CastedObject = Cast<T>(Object.Get()))
		{
			return CastedObject;
		}
	}

	return nullptr;
}

static EVisibility VisibilityConverter(bool bValue)
{
	return bValue ? EVisibility::Visible : EVisibility::Collapsed;
}

class FSMBaseCustomization : public IDetailCustomization
{
	// IDetailCustomization
	virtual void CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder) override;
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override {}
	// ~IDetailCustomization

protected:
	void ForceUpdate();
	
	TWeakPtr<IDetailLayoutBuilder> DetailBuilderPtr;
};

class FSMNodeCustomization : public FSMBaseCustomization {
public:
	// IDetailCustomization
	void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	// ~IDetailCustomization

	static TSharedRef<IDetailCustomization> MakeInstance();

protected:
	void GenerateGraphArrayWidget(TSharedRef<IPropertyHandle> PropertyHandle, int32 ArrayIndex, IDetailChildrenBuilder& ChildrenBuilder);
	TWeakObjectPtr<class USMGraphNode_Base> SelectedGraphNode;
};

class FSMStateMachineReferenceCustomization : public FSMNodeCustomization {
public:
	// IDetailCustomization
	void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	// ~IDetailCustomization

	static TSharedRef<IDetailCustomization> MakeInstance();

protected:
	void CustomizeParentSelection(IDetailLayoutBuilder& DetailBuilder);
	void OnUseTemplateChange();
	
	TArray<TSharedPtr<FName>> AvailableClasses;
	TMap<FName, USMBlueprintGeneratedClass*> MappedClasses;
};

class FSMTransitionEdgeCustomization : public FSMNodeCustomization {
public:
	// IDetailCustomization
	void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	// ~IDetailCustomization

	static TSharedRef<IDetailCustomization> MakeInstance();

protected:
	TArray<TSharedPtr<FName>> AvailableDelegates;
};

class FSMGraphPropertyCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	/** IPropertyTypeCustomization interface */
	virtual void CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

	/** Register the given struct with the Property Editor. */
	static void RegisterNewStruct(const FName& Name);

	/** Unregister all previously registered structs from the Property Editor. */
	static void UnregisterAllStructs();

	class USMGraphNode_Base* GetGraphNodeBeingCustomized(IPropertyTypeCustomizationUtils& StructCustomizationUtils) const;
private:
	TSharedPtr<IPropertyHandle> PropertyHandle;
};