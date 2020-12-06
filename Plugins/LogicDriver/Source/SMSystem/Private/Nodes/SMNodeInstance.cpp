// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#include "SMNodeInstance.h"
#include "CoreMinimal.h"
#include "SMInstance.h"


DEFINE_STAT(STAT_NodeInstances);

USMNodeInstance::USMNodeInstance() : Super(), bAutoEvalExposedProperties(true), OwningNode(nullptr)
{
	INC_DWORD_STAT(STAT_NodeInstances)
	
	NodeIconTintColor = FLinearColor(1.f, 1.f, 1.f, 1.f);
	
#if WITH_EDITORONLY_DATA
	// TODO: Read editor settings.
	NodeColor = FLinearColor(1.f, 1.f, 1.f, 0.7f);
	NodeDescription.Category = FText::FromString("User");
#endif

#if WITH_EDITOR
	ResetArrayCheck();
#endif
}

UWorld* USMNodeInstance::GetWorld() const
{
	if(UObject* Context = GetContext())
	{
		return Context->GetWorld();
	}

	return nullptr;
}

void USMNodeInstance::BeginDestroy()
{
	Super::BeginDestroy();
	DEC_DWORD_STAT(STAT_NodeInstances)
}

UObject* USMNodeInstance::GetContext() const
{
	if(USMInstance* SMInstance = GetStateMachineInstance())
	{
		return SMInstance->GetContext();
	}

	return nullptr;
}

USMInstance* USMNodeInstance::GetStateMachineInstance(bool bTopMostInstance) const
{
	if(USMInstance* Instance = Cast<USMInstance>(GetOuter()))
	{
		if(bTopMostInstance)
		{
			return Instance->GetMasterReferenceOwner();
		}

		return Instance;
	}

	return nullptr;
}

void USMNodeInstance::SetOwningNode(FSMNode_Base* Node)
{
	OwningNode = Node;
}

USMStateMachineInstance* USMNodeInstance::GetOwningStateMachineNodeInstance() const
{
	if (const FSMNode_Base* Node = GetOwningNode())
	{
		if (FSMNode_Base* NodeOwner = Node->GetOwnerNode())
		{
			return Cast<USMStateMachineInstance>(NodeOwner->GetNodeInstance());
		}
	}

	return nullptr;
}

float USMNodeInstance::GetTimeInState() const
{
	return OwningNode ? OwningNode->TimeInState : 0.f;
}

bool USMNodeInstance::IsInEndState() const
{
	return OwningNode ? OwningNode->bIsInEndState : false;
}

bool USMNodeInstance::HasUpdated() const
{
	return OwningNode ? OwningNode->bHasUpdated : false;
}

bool USMNodeInstance::IsActive() const
{
	return OwningNode ? OwningNode->IsActive() : false;
}

const FString& USMNodeInstance::GetNodeName() const
{
	static FString EmptyString;
	return OwningNode ? OwningNode->GetNodeName() : EmptyString;
}

const FGuid& USMNodeInstance::GetGuid() const
{
	static FGuid BlankGuid;
	return OwningNode ? OwningNode->GetGuid() : BlankGuid;
}

void USMNodeInstance::EvaluateGraphProperties()
{
	if (const FSMNode_Base* Node = GetOwningNode())
	{
		const_cast<FSMNode_Base*>(Node)->ExecuteGraphProperties();
	}
}

UTexture2D* USMNodeInstance::GetNodeIcon_Implementation() const
{
	return NodeIcon;
}

FVector2D USMNodeInstance::GetNodeIconSize_Implementation() const
{
	return NodeIconSize;
}

FLinearColor USMNodeInstance::GetNodeIconTintColor_Implementation() const
{
	return NodeIconTintColor;
}

#if WITH_EDITORONLY_DATA
FString USMNodeInstance::GetNodeDisplayName() const
{
	if(NodeDescription.Name.IsNone())
	{
		FString ClassName = GetClass()->GetName();
		ClassName.RemoveFromEnd(TEXT("_C"));
		return ClassName;
	}
	return NodeDescription.Name.ToString();
}
#endif

#if WITH_EDITOR

void USMNodeInstance::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (PropertyChangedEvent.ChangeType == EPropertyChangeType::ArrayRemove || PropertyChangedEvent.ChangeType == EPropertyChangeType::ArrayAdd)
	{
		ArrayPropertyChanged = PropertyChangedEvent.GetPropertyName();
		ArrayChangeType = PropertyChangedEvent.ChangeType;
		ArrayIndexChanged = PropertyChangedEvent.GetArrayIndex(ArrayPropertyChanged.ToString());
	}
	else
	{
		ResetArrayCheck();
	}
}

bool USMNodeInstance::WasArrayPropertyModified(const FName& PropertyName) const
{
	return ArrayPropertyChanged == PropertyName && (ArrayChangeType == EPropertyChangeType::ArrayRemove || ArrayChangeType == EPropertyChangeType::ArrayAdd) && ArrayIndexChanged >= 0;
}

void USMNodeInstance::ResetArrayCheck()
{
	ArrayPropertyChanged = NAME_None;
	ArrayChangeType = EPropertyChangeType::Unspecified;
	ArrayIndexChanged = -1;
}
#endif