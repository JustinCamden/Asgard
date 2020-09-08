// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#include "SMNode_Base.h"
#include "SMUtils.h"
#include "SMLogging.h"
#include "SMNodeInstance.h"


FSMNode_Base::FSMNode_Base() : TimeInState(0), bIsInEndState(false), bHasUpdated(false), DuplicateId(0),
OwnerNode(nullptr),
OwningInstance(nullptr), NodeInstance(nullptr), NodeInstanceClass(nullptr),
bInitialized(false), bIsActive(false)
{
	/*
	 * Originally the Guid was initialized here. This caused warnings to show up during packaging because
	 * Unreal does safety checks on struct native constructors by comparing multiple initializations with different
	 * addresses and verifying each property matches. That doesn't work with a Guid because it is guaranteed to
	 * be unique each time.
	 */
}

void FSMNode_Base::Initialize(UObject* Instance)
{
	OwningInstance = Cast<USMInstance>(Instance);
	bInitialized = true;
	GraphEvaluator.Initialize(Instance);

	CreateNodeInstance();
}

void FSMNode_Base::Reset()
{
	GraphEvaluator.Reset();
	ResetGraphProperties();
}

const FGuid& FSMNode_Base::GetNodeGuid() const
{
	return Guid;
}

void FSMNode_Base::GenerateNewNodeGuid()
{
	SetNodeGuid(FGuid::NewGuid());
}

const FGuid& FSMNode_Base::GetGuid() const
{
	return PathGuid;
}

void FSMNode_Base::CalculatePathGuid(TMap<FString, int32>& MappedPaths)
{
	USMUtils::PathToGuid(GetGuidPath(MappedPaths), &PathGuid);
}

FString FSMNode_Base::GetGuidPath(TMap<FString, int32>& MappedPaths) const
{
	TArray<const FSMNode_Base*> Owners;
	USMUtils::TryGetAllOwners(this, Owners);
	return USMUtils::BuildGuidPathFromNodes(Owners, &MappedPaths);
}

void FSMNode_Base::GenerateNewNodeGuidIfNotSet()
{
	if (Guid.IsValid())
	{
		return;
	}

	GenerateNewNodeGuid();
}

void FSMNode_Base::SetNodeGuid(const FGuid& NewGuid)
{
	Guid = NewGuid;
}

void FSMNode_Base::SetOwnerNodeGuid(const FGuid& NewGuid)
{
	OwnerGuid = NewGuid;
}

void FSMNode_Base::SetOwnerNode(FSMNode_Base* Owner)
{
	OwnerNode = Owner;
}

void FSMNode_Base::CreateNodeInstance()
{
	GraphProperties.Reset();

	if (!NodeInstanceClass)
	{
		SetNodeInstanceClass(GetDefaultNodeInstanceClass());

		if (!NodeInstanceClass)
		{
			return;
		}
	}

	UObject* TemplateInstance = nullptr;
	if (TemplateName != NAME_None && OwningInstance)
	{
		TemplateInstance = USMUtils::FindTemplateFromInstance(OwningInstance, TemplateName);
		if (TemplateInstance == nullptr)
		{
			LOG_ERROR(TEXT("Could not find node template %s for use on node %s from package %s. Loading defaults."), *TemplateName.ToString(), *GetNodeName(), *OwningInstance->GetName());
		}
	}

	NodeInstance = NewObject<USMNodeInstance>(OwningInstance, NodeInstanceClass, NAME_None, RF_NoFlags, TemplateInstance);
	NodeInstance->SetOwningNode(this);

	CreateGraphProperties();

	NodeInstance->ConstructionScript();
}

void FSMNode_Base::SetNodeInstanceClass(UClass* NewNodeInstanceClass)
{
	if (NewNodeInstanceClass && !IsNodeInstanceClassCompatible(NewNodeInstanceClass))
	{
		LOG_ERROR(TEXT("Could not set node instance class %s on node %s. The types are not compatible."), *NewNodeInstanceClass->GetName(), *GetNodeName());
		return;
	}

	NodeInstanceClass = NewNodeInstanceClass;
}

bool FSMNode_Base::IsNodeInstanceClassCompatible(UClass* NewNodeInstanceClass) const
{
	ensureMsgf(false, TEXT("FSMNode_Base IsNodeInstanceClassCompatible hit for node %s and instance class %s. This should always be overidden in child classes."),
		*GetNodeName(), NewNodeInstanceClass ? *NewNodeInstanceClass->GetName() : TEXT("None"));
	return false;
}

void FSMNode_Base::AddVariableGraphProperty(const FSMGraphProperty_Base& GraphProperty)
{
	VariableGraphProperties.Add(GraphProperty);
}

void FSMNode_Base::SetNodeName(const FString& Name)
{
	NodeName = Name;
}

void FSMNode_Base::SetTemplateName(const FName& Name)
{
	TemplateName = Name;
}

void FSMNode_Base::Execute()
{
	if (!bInitialized)
	{
		return;
	}

	UpdateReadStates();

	GraphEvaluator.Execute();
}

void FSMNode_Base::SetActive(bool bValue)
{
#if WITH_EDITORONLY_DATA
	bWasActive = bIsActive;
#endif
	bIsActive = bValue;
}

void FSMNode_Base::ExecuteGraphProperties(bool bVariablesOnly)
{
	for (FSMGraphProperty_Base& GraphProperty : VariableGraphProperties)
	{
		GraphProperty.Execute();
	}
	
	if (bVariablesOnly)
	{
		return;
	}

	for (FSMGraphProperty_Base* GraphProperty : GraphProperties)
	{
		GraphProperty->Execute();
	}
}

void FSMNode_Base::ResetGraphProperties()
{
	for (FSMGraphProperty_Base& GraphProperty : VariableGraphProperties)
	{
		GraphProperty.Reset();
	}
	
	for (FSMGraphProperty_Base* GraphProperty : GraphProperties)
	{
		GraphProperty->Reset();
	}
}

void FSMNode_Base::CreateGraphProperties()
{
	TSet<FProperty*> GraphStructPropertiesForStateMachine;
	USMUtils::TryGetGraphPropertiesForClass(OwningInstance->GetClass(), GraphStructPropertiesForStateMachine);

	/* Looks for the real property stored on the owning instance. */
	auto GetRealProperty = [&](TSet<FProperty*>& Properties, const FSMGraphProperty_Base* GraphProperty) -> FSMGraphProperty_Base*
	{
		for (FProperty* Prop : Properties)
		{
			TArray<FSMGraphProperty_Base*> GraphPropertyInstances;
			USMUtils::BlueprintPropertyToNativeProperty(Prop, OwningInstance, GraphPropertyInstances);

			for (FSMGraphProperty_Base* FoundInstance : GraphPropertyInstances)
			{
				// The real property is the owner guid of the instanced property.
				if (FoundInstance->GetGuid() == GraphProperty->GetOwnerGuid())
				{
					return FoundInstance;
				}
			}
		}
		return nullptr;
	};

	TSet<FProperty*> GraphStructProperties;
	if (USMUtils::TryGetGraphPropertiesForClass(NodeInstanceClass, GraphStructProperties))
	{
		for (FProperty* GraphStructProperty : GraphStructProperties)
		{
			TArray<FSMGraphProperty_Base*> GraphPropertyInstances;
			USMUtils::BlueprintPropertyToNativeProperty(GraphStructProperty, NodeInstance, GraphPropertyInstances);

			for (FSMGraphProperty_Base* GraphProperty : GraphPropertyInstances)
			{
				// The graph property being executed is actually in the template, but the graph has a duplicate created on the owning instance,
				// so we need to link them to get the owning instance result properly to template.
				GraphProperty->LinkedProperty = GetRealProperty(GraphStructPropertiesForStateMachine, GraphProperty);
				GraphProperty->Initialize(OwningInstance);
				GraphProperties.AddUnique(GraphProperty);
			}
		}
	}

	// Variable properties already have everything they need and just need to be initialized.
	for(FSMGraphProperty_Base& VariableProperty : VariableGraphProperties)
	{
		VariableProperty.Initialize(OwningInstance);
	}
}