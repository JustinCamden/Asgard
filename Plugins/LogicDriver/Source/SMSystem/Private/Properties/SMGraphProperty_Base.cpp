// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#include "SMGraphProperty_Base.h"
#include "SMUtils.h"
#include "SMLogging.h"
#if WITH_EDITORONLY_DATA
#include "Misc/PackageName.h"
#endif

void FSMExposedFunctionHandler::Initialize(UObject* StateMachineObject)
{
	/* 
	 * See FExposedValueHandler under AnimNodeBase. This handler is effectively the same thing, minus the fast path options. 
	 * This will prepare the function setup at the entry point of this node.
	 */

	if (bInitialized || !StateMachineObject)
	{
		return;
	}

	OwnerObject = StateMachineObject;

	if (BoundFunction != NAME_None)
	{
		// Optimization from UE -- FProperty gets copied on new instances so we don't have to look up the function.
#if !WITH_EDITOR
		if (Function == nullptr)
#endif
		{
			// Only game thread is safe to call this on -- access shared map of the class.
			check(IsInGameThread());
			Function = StateMachineObject->FindFunction(BoundFunction);
			check(Function);
		}

		bInitialized = true;
	}
	else
	{
		Function = nullptr;
	}
}

void FSMExposedFunctionHandler::Reset()
{
	bInitialized = false;
	OwnerObject = nullptr;
}

void FSMExposedFunctionHandler::Execute(void* Parms) const
{
	if(!bInitialized || !IsValid(OwnerObject))
	{
		return;
	}

	if(OwnerObject->IsPendingKillOrUnreachable())
	{
		return;
	}

	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("SMGraphFunction::GraphEvaluation"), STAT_SMExposedFunctionHandler_Execute, STATGROUP_LogicDriver);
	OwnerObject->ProcessEvent(Function, Parms);
}


FSMGraphProperty_Base::FSMGraphProperty_Base(): LinkedProperty(nullptr), bIsInArray(false), GuidIndex(-1)
{
#if WITH_EDITORONLY_DATA
	GraphModuleClassName = "SMSystemEditor";
	GraphClassName = "SMPropertyGraph";
	GraphSchemaClassName = "SMPropertyGraphSchema";
	CachedGraphClass = nullptr;
	CachedSchemaClass = nullptr;
	ArrayIndex = 0;
#endif
}

void FSMGraphProperty_Base::Initialize(UObject* Instance)
{
	GraphEvaluator.Initialize(Instance);
}

void FSMGraphProperty_Base::Execute(void* Params)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("SMGraphProperty_Base::Execute"), STAT_SMGraphProperty_Base_Execute, STATGROUP_LogicDriver);
	
	GraphEvaluator.Execute(Params);

	// If set then the graph evaluator is actually executing a graph from the linked property.
	// We should copy the result value into this property.
	if(LinkedProperty)
	{
		SetResult(LinkedProperty->GetResult());
	}
}

void FSMGraphProperty_Base::Reset()
{
	GraphEvaluator.Reset();
}

void FSMGraphProperty_Base::InvalidateGuid()
{
	Guid.Invalidate();
}

const FGuid& FSMGraphProperty_Base::SetGuid(const FGuid& NewGuid)
{
	const FString GuidString = NewGuid.ToString();
	GuidUnmodified = NewGuid;
	GuidIndex = -1;
	
	const FString GuidWithTemplate = GuidString + TemplateGuid.ToString();
	USMUtils::PathToGuid(GuidWithTemplate, &Guid);
	return Guid;
}

const FGuid& FSMGraphProperty_Base::SetGuid(const FGuid& NewGuid, int32 Index, bool bCountTemplate)
{
	FString GuidWithIndex = NewGuid.ToString() + FString::FromInt(Index);
	GuidUnmodified = NewGuid;
	GuidIndex = Index;
	
	if (bCountTemplate)
	{
		GuidWithIndex += TemplateGuid.ToString();
	}
	
	USMUtils::PathToGuid(GuidWithIndex, &Guid);
	return Guid;
}

const FGuid& FSMGraphProperty_Base::GenerateNewGuid()
{
	SetGuid(FGuid::NewGuid());
	return Guid;
}

const FGuid& FSMGraphProperty_Base::GenerateNewGuidIfNotValid()
{
	if(!Guid.IsValid())
	{
		GenerateNewGuid();
	}

	return Guid;
}

const FGuid& FSMGraphProperty_Base::SetOwnerGuid(const FGuid& NewGuid)
{
	OwnerGuid = NewGuid;
	return OwnerGuid;
}

const FGuid& FSMGraphProperty_Base::GetOwnerGuid() const
{
	return OwnerGuid;
}

const FGuid& FSMGraphProperty_Base::SetTemplateGuid(const FGuid& NewGuid, bool bRefreshGuid)
{
	TemplateGuid = NewGuid;

	if (bRefreshGuid)
	{
		if (GuidIndex >= 0)
		{
			SetGuid(GetUnmodifiedGuid(), GuidIndex);
		}
		else
		{
			SetGuid(GetUnmodifiedGuid());
		}
	}
	
	return TemplateGuid;
}

#if WITH_EDITORONLY_DATA

UClass* FSMGraphProperty_Base::GetGraphClass(UObject* Outer) const
{
	if (!CachedGraphClass)
	{
		const_cast<FSMGraphProperty_Base*>(this)->CachedGraphClass = FindObject<UClass>(Outer, *GraphClassName.ToString());
	}

	return CachedGraphClass;
}

UClass* FSMGraphProperty_Base::GetGraphSchemaClass(UObject* Outer) const
{
	if (!CachedSchemaClass)
	{
		const_cast<FSMGraphProperty_Base*>(this)->CachedSchemaClass = FindObject<UClass>(Outer, *GraphSchemaClassName.ToString());
	}

	return CachedSchemaClass;
}

const FString& FSMGraphProperty_Base::GetGraphModuleName() const
{
	return GraphModuleClassName;
}

UPackage* FSMGraphProperty_Base::GetEditorModule() const
{
	const FString& ModuleName = GetGraphModuleName();
	const FString LongName = FPackageName::ConvertToLongScriptPackageName(*ModuleName);
	return Cast<UPackage>(StaticFindObjectFast(UPackage::StaticClass(), nullptr, *LongName, false, false));
}

const FString& FSMGraphProperty_Base::GetPropertyDisplayName() const
{
	static FString DefaultName("");
	return DefaultName;
}

FText FSMGraphProperty_Base::GetDisplayName() const
{
	const FString& DisplayName = GetPropertyDisplayName();
	if(!DisplayName.IsEmpty())
	{
		return FText::FromString(DisplayName);
	}

	return RealDisplayName;
}
#endif

FSMGraphProperty::FSMGraphProperty() : Super()
{
}