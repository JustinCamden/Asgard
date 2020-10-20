// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#include "Utilities/SMNodeInstanceUtils.h"
#include "ScopedTransaction.h"
#include "IAssetTools.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/Text/STextBlock.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Configuration/SMEditorStyle.h"
#include "SMNodeInstance.h"
#include "SMUtils.h"


#define LOCTEXT_NAMESPACE "SMNodeInstanceUtils"


bool FSMNodeInstanceUtils::IsWidgetChildOf(TSharedPtr<SWidget> Parent, TSharedPtr<SWidget> PossibleChild)
{
	FChildren* Children = Parent->GetChildren();
	for (int32 i = 0; i < Children->Num(); ++i)
	{
		TSharedRef<SWidget> Child = Children->GetChildAt(i);
		if (Child == PossibleChild)
		{
			return true;
		}
		return IsWidgetChildOf(Child, PossibleChild);
	}

	return false;
}

TSharedPtr<SWidget> FSMNodeInstanceUtils::CreateNodeClassWidgetDisplay(USMNodeInstance* NodeInstance)
{
	const FSMNodeDescription& Description = NodeInstance->GetNodeDescription();
	const FString Name = Description.Name.IsNone() ? NodeInstance->GetClass()->GetName() : Description.Name.ToString();
	const FText TextFormat = FText::FromString(Description.Description.IsEmpty() ? "{0}" : "{0} - {1}");
	TSharedPtr<SWidget> Result = SNew(STextBlock)
		.TextStyle(FSMEditorStyle::Get(), "SMGraph.Tooltip.Info")
		.Text(FText::Format(TextFormat, FText::FromString(Name), Description.Description));
	return Result;
}

const FGuid& FSMNodeInstanceUtils::SetGraphPropertyFromProperty(FSMGraphProperty_Base& GraphProperty,
	FProperty* Property, USMNodeInstance* NodeInstance, int32 Index, bool bSetGuid, bool bUseTemplateInGuid)
{
	check(NodeInstance)
	
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	
	GraphProperty.bIsInArray = Property->IsA<FArrayProperty>() || Property->GetOwnerProperty()->IsA<FArrayProperty>();
	
	GraphProperty.VariableName = Property->GetFName();
	GraphProperty.MemberReference.SetFromField<FProperty>(Property, false);
	K2Schema->ConvertPropertyToPinType(Property, GraphProperty.VariableType);

	// TemplateGuid is used to calculate final guid.
	GraphProperty.SetTemplateGuid(NodeInstance->GetTemplateGuid());

	if(!bSetGuid)
	{
		return GraphProperty.GetGuid();
	}
	
	if (GraphProperty.MemberReference.GetMemberGuid().IsValid())
	{
		// Blueprint variable
		return GraphProperty.SetGuid(GraphProperty.MemberReference.GetMemberGuid(), Index, bUseTemplateInGuid);
	}

	//  Native variable.
	const FString SearchString = GraphProperty.MemberReference.GetReferenceSearchString(Property->GetOwnerClass());
	return GraphProperty.SetGuid(USMUtils::PathToGuid(SearchString), Index, bUseTemplateInGuid);
}

bool FSMNodeInstanceUtils::IsPropertyExposedToGraphNode(FProperty* Property)
{
	return !Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance) && Property->HasAllPropertyFlags(CPF_BlueprintVisible) &&
		!Property->HasAnyPropertyFlags(CPF_BlueprintReadOnly) && !Property->HasMetaData("HideOnNode");
}

FStructProperty* FSMNodeInstanceUtils::IsPropertyGraphProperty(FProperty* Property)
{
	if (Property->HasMetaData("HideOnNode"))
	{
		// Assume this node never wants to be displayed.
		return nullptr;
	}
	
	if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
	{
		if (StructProperty->Struct->IsChildOf(FSMGraphProperty_Base_Runtime::StaticStruct()))
		{
			return StructProperty;
		}
	}

	if(FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
	{
		if (FStructProperty* StructProperty = CastField<FStructProperty>(ArrayProperty->Inner))
		{
			if (StructProperty->Struct->IsChildOf(FSMGraphProperty_Base_Runtime::StaticStruct()))
			{
				return StructProperty;
			}
		}
	}

	return nullptr;
}


#undef LOCTEXT_NAMESPACE
