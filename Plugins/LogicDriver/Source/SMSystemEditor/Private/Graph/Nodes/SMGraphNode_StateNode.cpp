// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#include "SMGraphNode_StateNode.h"
#include "Kismet2/Kismet2NameValidators.h"
#include "Utilities/SMBlueprintEditorUtils.h"
#include "SMGraphNode_TransitionEdge.h"
#include "Utilities/SMNodeInstanceUtils.h"
#include "SMUtils.h"
#include "Engine/Engine.h"
#include "Graph/SMGraph.h"
#include "Graph/SMStateGraph.h"
#include "Graph/Schema/SMStateGraphSchema.h"
#include "Helpers/SMGraphK2Node_FunctionNodes_NodeInstance.h"
#include "RootNodes/SMGraphK2Node_StateUpdateNode.h"
#include "RootNodes/SMGraphK2Node_StateEndNode.h"
#include "RootNodes/SMGraphK2Node_IntermediateNodes.h"
#include "RootNodes/SMGraphK2Node_TransitionInitializedNode.h"
#include "RootNodes/SMGraphK2Node_TransitionShutdownNode.h"

#define LOCTEXT_NAMESPACE "SMGraphStateNode"

class FSMStateNodeNameValidator : public FStringSetNameValidator
{
public:
	FSMStateNodeNameValidator(const USMGraphNode_StateNodeBase* InStateNode)
		: FStringSetNameValidator(FString())
	{
		TArray<USMGraphNode_StateNodeBase*> Nodes;
		USMGraph* StateMachine = CastChecked<USMGraph>(InStateNode->GetOuter());

		StateMachine->GetNodesOfClass<USMGraphNode_StateNodeBase>(Nodes);
		for (auto NodeIt = Nodes.CreateIterator(); NodeIt; ++NodeIt)
		{
			USMGraphNode_StateNodeBase* Node = *NodeIt;
			if (Node != InStateNode)
			{
				Names.Add(Node->GetStateName());
			}
		}
	}

	// Begin FSMStateNodeNameValidator
	virtual EValidatorResult IsValid(const FString& Name, bool bOriginal) override
	{
		EValidatorResult Result = FStringSetNameValidator::IsValid(Name, bOriginal);

		if (Result == EValidatorResult::Ok)
		{
			if (Name.Len() > 100)
			{
				Result = EValidatorResult::TooLong;
			}
		}

		return Result;
	}
	// End FSMStateNodeNameValidator
};

USMGraphNode_StateNodeBase::USMGraphNode_StateNodeBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), bAlwaysUpdate_DEPRECATED(false), bDisableTickTransitionEvaluation_DEPRECATED(false),
	  bEvalTransitionsOnStart_DEPRECATED(false), bExcludeFromAnyState_DEPRECATED(false),
	  bCanTransitionToSelf(false)
{
}

void USMGraphNode_StateNodeBase::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, TEXT("Transition"), TEXT("In"));
	CreatePin(EGPD_Output, TEXT("Transition"), TEXT("Out"));
}

FString USMGraphNode_StateNodeBase::GetStateName() const
{
	return BoundGraph ? *(BoundGraph->GetName()) : TEXT("(null)");
}

bool USMGraphNode_StateNodeBase::IsEndState(bool bCheckAnyState) const
{
	// Must have entry.
	if(!HasInputConnections())
	{
		return false;
	}

	// Check any states since they add transitions to this node on compile.
	if (bCheckAnyState && FSMBlueprintEditorUtils::IsNodeImpactedFromAnyStateNode(this))
	{
		return false;
	}

	
	// If no output definitely end state.
	if(GetOutputPin()->LinkedTo.Num() == 0)
	{
		return true;
	}

	for (UEdGraphPin* Pin : GetOutputPin()->LinkedTo)
	{
		if (USMGraphNode_TransitionEdge* Transition = Cast<USMGraphNode_TransitionEdge>(Pin->GetOwningNode()))
		{
			// Transitioning to self doesn't count.
			if(Transition->GetFromState() == Transition->GetToState())
			{
				continue;
			}

			// There has to be some way out of here...
			if (Transition->PossibleToTransition())
			{
				return false;
			}
		}
	}

	return true;
}

bool USMGraphNode_StateNodeBase::HasInputConnections() const
{
	if (UEdGraphPin* Pin = GetInputPin())
	{
		if (Pin->LinkedTo.Num() == 0)
		{
			return false;
		}

		for (UEdGraphPin* InputPin : Pin->LinkedTo)
		{
			if (InputPin->GetOwningNode()->IsA<USMGraphNode_StateMachineEntryNode>())
			{
				return true;
			}

			if (USMGraphNode_TransitionEdge* Transition = Cast<USMGraphNode_TransitionEdge>(InputPin->GetOwningNode()))
			{
				// Ignore self and input connections which can't transition.
				if (Transition->GetFromState() == Transition->GetToState() || !Transition->PossibleToTransition())
				{
					continue;
				}

				return true;
			}
		}
	}
	
	return false;
}

bool USMGraphNode_StateNodeBase::HasOutputConnections() const
{
	if (UEdGraphPin* Pin = GetOutputPin())
	{
		return Pin->LinkedTo.Num() > 0;
	}

	return false;
}

bool USMGraphNode_StateNodeBase::ShouldDefaultTransitionsToParallel() const
{
	if (USMStateInstance_Base* StateInstance = Cast<USMStateInstance_Base>(GetNodeTemplate()))
	{
		return StateInstance->bDefaultToParallel;
	}

	return false;
}

bool USMGraphNode_StateNodeBase::ShouldExcludeFromAnyState() const
{
	if (USMStateInstance_Base* StateInstance = Cast<USMStateInstance_Base>(GetNodeTemplate()))
	{
		return StateInstance->bExcludeFromAnyState;
	}

	return false;
}

bool USMGraphNode_StateNodeBase::HasTransitionToNode(UEdGraphNode* Node) const
{
	for (UEdGraphPin* OutputPin : GetOutputPin()->LinkedTo)
	{
		if (USMGraphNode_TransitionEdge* Transition = Cast<USMGraphNode_TransitionEdge>(OutputPin->GetOwningNode()))
		{
			if (Transition->GetToState() == Node)
			{
				return true;
			}
		}
	}

	return false;
}

bool USMGraphNode_StateNodeBase::HasTransitionFromNode(UEdGraphNode* Node) const
{
	if (UEdGraphPin* InputPin = GetInputPin())
	{
		for (UEdGraphPin* Pin : InputPin->LinkedTo)
		{
			if (USMGraphNode_TransitionEdge* Transition = Cast<USMGraphNode_TransitionEdge>(Pin->GetOwningNode()))
			{
				if (Transition->GetFromState() == Node)
				{
					return true;
				}
			}
		}
	}

	return false;
}

USMGraphNode_StateNodeBase* USMGraphNode_StateNodeBase::GetPreviousNode(int32 Index /*= 0*/) const
{
	if (USMGraphNode_TransitionEdge* Transition = GetPreviousTransition(Index))
	{
		return Transition->GetFromState();
	}

	return nullptr;
}

USMGraphNode_StateNodeBase* USMGraphNode_StateNodeBase::GetNextNode(int32 Index /*= 0*/) const
{
	if (USMGraphNode_TransitionEdge* Transition = GetNextTransition(Index))
	{
		return Transition->GetToState();
	}

	return nullptr;
}

USMGraphNode_TransitionEdge* USMGraphNode_StateNodeBase::GetPreviousTransition(int32 Index) const
{
	if (UEdGraphPin* InputPin = GetInputPin())
	{
		if (InputPin->LinkedTo.Num() <= Index)
		{
			return nullptr;
		}

		if (USMGraphNode_TransitionEdge* Transition = Cast<USMGraphNode_TransitionEdge>(InputPin->LinkedTo[Index]->GetOwningNode()))
		{
			return Transition;
		}
	}
	return nullptr;
}

USMGraphNode_TransitionEdge* USMGraphNode_StateNodeBase::GetNextTransition(int32 Index) const
{
	if (UEdGraphPin* OutputPin = GetOutputPin())
	{
		if (OutputPin->LinkedTo.Num() <= Index)
		{
			return nullptr;
		}

		if (USMGraphNode_TransitionEdge* Transition = Cast<USMGraphNode_TransitionEdge>(OutputPin->LinkedTo[Index]->GetOwningNode()))
		{
			return Transition;
		}
	}

	return nullptr;
}

void USMGraphNode_StateNodeBase::GetInputTransitions(TArray<USMGraphNode_TransitionEdge*>& OutTransitions) const
{
	if (UEdGraphPin* InputPin = GetInputPin())
	{
		for (int32 Idx = 0; Idx < InputPin->LinkedTo.Num(); ++Idx)
		{
			if (USMGraphNode_TransitionEdge* Transition = Cast<USMGraphNode_TransitionEdge>(InputPin->LinkedTo[Idx]->GetOwningNode()))
			{
				OutTransitions.AddUnique(Transition);
			}
		}
	}
}

void USMGraphNode_StateNodeBase::GetOutputTransitions(TArray<USMGraphNode_TransitionEdge*>& OutTransitions) const
{
	if (UEdGraphPin* OutputPin = GetOutputPin())
	{
		for (int32 Idx = 0; Idx < OutputPin->LinkedTo.Num(); ++Idx)
		{
			if (USMGraphNode_TransitionEdge* Transition = Cast<USMGraphNode_TransitionEdge>(OutputPin->LinkedTo[Idx]->GetOwningNode()))
			{
				OutTransitions.AddUnique(Transition);
			}
		}
	}
}

UEdGraphPin* USMGraphNode_StateNodeBase::GetConnectedEntryPin() const
{
	if (UEdGraphPin* InputPin = GetInputPin())
	{
		for (int32 Idx = 0; Idx < InputPin->LinkedTo.Num(); ++Idx)
		{
			if (InputPin->LinkedTo[Idx]->GetOwningNode()->IsA<USMGraphNode_StateMachineEntryNode>())
			{
				return InputPin->LinkedTo[Idx];
			}
		}
	}

	return nullptr;
}

FLinearColor USMGraphNode_StateNodeBase::GetBackgroundColorForNodeInstance(USMNodeInstance* NodeInstance) const
{
	const USMEditorSettings* Settings = FSMBlueprintEditorUtils::GetEditorSettings();
	const FLinearColor* CustomColor = GetCustomBackgroundColor(NodeInstance);
	const FLinearColor ColorModifier = !CustomColor ? FLinearColor(0.6f, 0.6f, 0.6f, 0.5f) : *CustomColor;
	const FLinearColor EndStateColor = !CustomColor ? Settings->EndStateColor * ColorModifier : CastChecked<USMStateInstance>(NodeInstance)->GetEndStateColor();

	if (IsEndState())
	{
		return EndStateColor;
	}

	const FLinearColor DefaultColor = Settings->StateDefaultColor;

	// No input -- node unreachable.
	if (!HasInputConnections())
	{
		return DefaultColor * ColorModifier;
	}

	// State is active
	if (FSMBlueprintEditorUtils::GraphHasAnyLogicConnections(BoundGraph))
	{
		return CustomColor ? *CustomColor * FLinearColor(1.f, 1.f, 1.f, 1.2f) : Settings->StateWithLogicColor * ColorModifier;
	}

	return DefaultColor * ColorModifier;
}

FText USMGraphNode_StateNodeBase::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(GetStateName());
}

TSharedPtr<INameValidatorInterface> USMGraphNode_StateNodeBase::MakeNameValidator() const
{
	return MakeShareable(new FSMStateNodeNameValidator(this));
}

void USMGraphNode_StateNodeBase::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	// Template has been changed.
	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(USMGraphNode_StateNodeBase, NodeInstanceTemplate))
	{
		//  Check if it's a property we care about.
		FEditPropertyChain::TDoubleLinkedListNode* MemberNode = PropertyChangedEvent.PropertyChain.GetActiveMemberNode();
		if (MemberNode && MemberNode->GetNextNode() && MemberNode->GetValue())
		{
			const FName Name = PropertyChangedEvent.PropertyChain.GetActiveMemberNode()->GetNextNode()->GetValue()->GetFName();
			if (Name == GET_MEMBER_NAME_CHECKED(USMStateInstance_Base, bDefaultToParallel))
			{
				for (int32 Idx = 0; Idx < GetOutputPin()->LinkedTo.Num(); ++Idx)
				{
					if (USMGraphNode_TransitionEdge* Transition = GetNextTransition(Idx))
					{
						if (USMTransitionInstance* Instance = Transition->GetNodeTemplateAs<USMTransitionInstance>())
						{
							Instance->bRunParallel = ShouldDefaultTransitionsToParallel();
						}
					}
				}
			}
		}
	}
}

void USMGraphNode_StateNodeBase::ImportDeprecatedProperties()
{
	Super::ImportDeprecatedProperties();

	if (USMStateInstance_Base* StateInstance = Cast<USMStateInstance_Base>(GetNodeTemplate()))
	{
		StateInstance->bAlwaysUpdate = bAlwaysUpdate_DEPRECATED;
		StateInstance->bDisableTickTransitionEvaluation = bDisableTickTransitionEvaluation_DEPRECATED;
		StateInstance->bEvalTransitionsOnStart = bEvalTransitionsOnStart_DEPRECATED;
		StateInstance->bExcludeFromAnyState = bExcludeFromAnyState_DEPRECATED;
	}
}

void USMGraphNode_StateNodeBase::AutowireNewNode(UEdGraphPin* FromPin)
{
	Super::AutowireNewNode(FromPin);

	if (FromPin != nullptr)
	{
		UEdGraphPin* InputPin = GetInputPin();
		
		if (InputPin && GetSchema()->TryCreateConnection(FromPin, InputPin))
		{
			FromPin->GetOwningNode()->NodeConnectionListChanged();
		}
	}
}

void USMGraphNode_StateNodeBase::PostPlacedNewNode()
{
	SetToCurrentVersion();
	
	// Create a new state machine graph
	check(BoundGraph == NULL);
	BoundGraph = FBlueprintEditorUtils::CreateNewGraph(
		this,
		NAME_None,
		USMStateGraph::StaticClass(),
		USMStateGraphSchema::StaticClass());
	check(BoundGraph);

	// Find an interesting name
	TSharedPtr<INameValidatorInterface> NameValidator = FNameValidatorFactory::MakeValidator(this);
	FBlueprintEditorUtils::RenameGraphWithSuggestion(BoundGraph, NameValidator, TEXT("State"));

	// Initialize the state machine graph
	const UEdGraphSchema* Schema = BoundGraph->GetSchema();
	Schema->CreateDefaultNodesForGraph(*BoundGraph);

	// Add the new graph as a child of our parent graph
	UEdGraph* ParentGraph = GetGraph();

	if (ParentGraph->SubGraphs.Find(BoundGraph) == INDEX_NONE)
	{
		ParentGraph->SubGraphs.Add(BoundGraph);
	}

	if (bGenerateTemplateOnNodePlacement)
	{
		InitTemplate();
	}
}

void USMGraphNode_StateNodeBase::PostPasteNode()
{
	// Find an interesting name, but try to keep the same if possible
	TSharedPtr<INameValidatorInterface> NameValidator = FNameValidatorFactory::MakeValidator(this);
	FBlueprintEditorUtils::RenameGraphWithSuggestion(BoundGraph, NameValidator, GetStateName());

	for (UEdGraphNode* GraphNode : BoundGraph->Nodes)
	{
		GraphNode->CreateNewGuid();
		GraphNode->PostPasteNode();
		// Required to correct context display issues.
		GraphNode->ReconstructNode();
	}

	Super::PostPasteNode();
}

void USMGraphNode_StateNodeBase::DestroyNode()
{
	Modify();
	if(BoundGraph)
	{
		BoundGraph->Modify();
	}
	
	UEdGraph* GraphToRemove = BoundGraph;

	BoundGraph = nullptr;
	Super::DestroyNode();

	if (GraphToRemove)
	{
		UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNodeChecked(this);
		FBlueprintEditorUtils::RemoveGraph(Blueprint, GraphToRemove, EGraphRemoveFlags::Recompile);
	}
}

void USMGraphNode_StateNodeBase::SetRuntimeDefaults(FSMState_Base& State) const
{
	State.SetNodeName(GetStateName());

	if (USMStateInstance_Base* StateInstance = Cast<USMStateInstance_Base>(GetNodeTemplate()))
	{
		State.bAlwaysUpdate = StateInstance->bAlwaysUpdate;
		State.bDisableTickTransitionEvaluation = StateInstance->bDisableTickTransitionEvaluation;
		State.bAllowParallelReentry = StateInstance->bAllowParallelReentry;
		State.bStayActiveOnStateChange = StateInstance->bStayActiveOnStateChange;
		State.bEvalTransitionsOnStart = StateInstance->bEvalTransitionsOnStart;
	}
}

FLinearColor USMGraphNode_StateNodeBase::Internal_GetBackgroundColor() const
{
	return GetBackgroundColorForNodeInstance(NodeInstanceTemplate);
}

void FStateStackContainer::InitTemplate(UObject* Owner, bool bForceInit, bool bForceNewGuid)
{
	if (StateStackClass == nullptr)
	{
		DestroyTemplate();
		return;
	}

	if (!bForceInit && NodeStackInstanceTemplate && NodeStackInstanceTemplate->GetClass() == StateStackClass)
	{
		return;
	}

	Owner->Modify();

	if (bForceNewGuid || !TemplateGuid.IsValid())
	{
		TemplateGuid = FGuid::NewGuid();
	}

	FString NodeName = Owner->GetName();
	NodeName = FSMBlueprintEditorUtils::GetSafeName(NodeName);
	
	const FString TemplateName = FString::Printf(TEXT("NODE_STACK_TEMPLATE_%s_%s_%s"), *NodeName, *StateStackClass->GetName(), *TemplateGuid.ToString());
	USMStateInstance* NewTemplate = StateStackClass ? NewObject<USMStateInstance>(Owner, StateStackClass, *TemplateName, RF_ArchetypeObject | RF_Transactional | RF_Public) : nullptr;

	if (NodeStackInstanceTemplate)
	{
		if (NewTemplate && NewTemplate->GetClass() == NodeStackInstanceTemplate->GetClass())
		{
			// Only copy when they're the same class. Causes problems when there's a common base class between the new node template and original template. Packaging won't find the template.
			UEngine::CopyPropertiesForUnrelatedObjects(NodeStackInstanceTemplate, NewTemplate);
		}

		// Original template isn't needed any more.
		DestroyTemplate();
	}

	NodeStackInstanceTemplate = NewTemplate;
	if (NodeStackInstanceTemplate)
	{
		NodeStackInstanceTemplate->SetTemplateGuid(TemplateGuid);
		NodeStackInstanceTemplate->ConstructionScript();
	}
}

void FStateStackContainer::DestroyTemplate()
{
	if (NodeStackInstanceTemplate)
	{
		NodeStackInstanceTemplate->Modify();
		FSMBlueprintEditorUtils::TrashObject(NodeStackInstanceTemplate);
		NodeStackInstanceTemplate = nullptr;
	}
}


USMGraphNode_StateNode::USMGraphNode_StateNode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void USMGraphNode_StateNode::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	// Enable templates
	bool bStateChange = false;
	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(USMGraphNode_StateNode, StateClass))
	{
		InitTemplate();
		// Disable property graph refresh because InitTemplate handles it.
		bCreatePropertyGraphsOnPropertyChange = false;

		bStateChange = true;
	}
	else if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(USMGraphNode_StateNode, StateStack) || PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(FStateStackContainer, StateStackClass))
	{
		if (PropertyChangedEvent.ChangeType == EPropertyChangeType::Duplicate)
		{
			// Array element duplication requires a new template generated.
			const int32 ArrayIndex = PropertyChangedEvent.GetArrayIndex(PropertyChangedEvent.GetPropertyName().ToString());
			if (ArrayIndex >= 0 && ArrayIndex + 1 < StateStack.Num())
			{
				const FStateStackContainer& OriginalStateStack = StateStack[ArrayIndex];
				FStateStackContainer& NewStateStack = StateStack[ArrayIndex + 1];

				NewStateStack.TemplateGuid = FGuid::NewGuid();
				if (OriginalStateStack.NodeStackInstanceTemplate && OriginalStateStack.NodeStackInstanceTemplate->GetClass() != GetDefaultNodeClass())
				{
					if (NewStateStack.NodeStackInstanceTemplate != OriginalStateStack.NodeStackInstanceTemplate)
					{
						// This state *shouldn't* exist because the object isn't deep copied, but who knows if USTRUCT UPROPERTY UObject handling changes?
						NewStateStack.DestroyTemplate();
					}
					
					NewStateStack.NodeStackInstanceTemplate = Cast<USMNodeInstance>(StaticDuplicateObject(OriginalStateStack.NodeStackInstanceTemplate, OriginalStateStack.NodeStackInstanceTemplate->GetOuter()));
					UEngine::CopyPropertiesForUnrelatedObjects(OriginalStateStack.NodeStackInstanceTemplate, NewStateStack.NodeStackInstanceTemplate);
					NewStateStack.NodeStackInstanceTemplate->SetTemplateGuid(NewStateStack.TemplateGuid);

					for (TFieldIterator<FProperty> It(NewStateStack.NodeStackInstanceTemplate->GetClass()); It; ++It)
					{
						// Look for real graph properties (not auto generated from a variable), they won't have had their guids cleared.
						
						FProperty* Property = *It;

						FName VarName = Property->GetFName();
						if (VarName == GET_MEMBER_NAME_CHECKED(USMNodeInstance, ExposedPropertyOverrides))
						{
							continue;
						}

						if (FStructProperty* StructProperty = FSMNodeInstanceUtils::IsPropertyGraphProperty(Property))
						{
							TArray<FSMGraphProperty_Base*> GraphProperties;
							USMUtils::BlueprintPropertyToNativeProperty(Property, NewStateStack.NodeStackInstanceTemplate, GraphProperties);
							for (FSMGraphProperty_Base* GraphProperty : GraphProperties)
							{
								GraphProperty->InvalidateGuid();
							}
						}
					}

					FSMBlueprintEditorUtils::DuplicateStackTemplatePropertyGraphs(this, this, NewStateStack, OriginalStateStack.TemplateGuid);
				}
			}
		}
		
		InitStateStack();
	}
	
	Super::PostEditChangeProperty(PropertyChangedEvent);
	bCreatePropertyGraphsOnPropertyChange = true;

	if (bStateChange)
	{
		FSMBlueprintEditorUtils::ConditionallyCompileBlueprint(FSMBlueprintEditorUtils::FindBlueprintForNodeChecked(this), false);
	}
}

void USMGraphNode_StateNode::PlaceDefaultInstanceNodes()
{
	Super::PlaceDefaultInstanceNodes();

	USMGraphK2Node_StateEntryNode* EntryNode = FSMBlueprintEditorUtils::GetSingleNodeFromGraph<USMGraphK2Node_StateEntryNode>(BoundGraph);
	FSMBlueprintEditorUtils::PlaceNodeIfNotSet<USMGraphK2Node_StateInstance_Begin>(BoundGraph, EntryNode);

	USMGraphK2Node_StateUpdateNode* UpdateNode = FSMBlueprintEditorUtils::GetSingleNodeFromGraph<USMGraphK2Node_StateUpdateNode>(BoundGraph);
	FSMBlueprintEditorUtils::PlaceNodeIfNotSet<USMGraphK2Node_StateInstance_Update>(BoundGraph, UpdateNode);

	USMGraphK2Node_StateEndNode* EndNode = FSMBlueprintEditorUtils::GetSingleNodeFromGraph<USMGraphK2Node_StateEndNode>(BoundGraph);
	FSMBlueprintEditorUtils::PlaceNodeIfNotSet<USMGraphK2Node_StateInstance_End>(BoundGraph, EndNode);

	// Optional nodes.
	FSMBlueprintEditorUtils::SetupDefaultPassthroughNodes<USMGraphK2Node_IntermediateStateMachineStartNode, USMGraphK2Node_StateInstance_StateMachineStart>(BoundGraph);
	FSMBlueprintEditorUtils::SetupDefaultPassthroughNodes<USMGraphK2Node_IntermediateStateMachineStopNode, USMGraphK2Node_StateInstance_StateMachineStop>(BoundGraph);

	FSMBlueprintEditorUtils::SetupDefaultPassthroughNodes<USMGraphK2Node_TransitionInitializedNode, USMGraphK2Node_StateInstance_OnStateInitialized>(BoundGraph);
	FSMBlueprintEditorUtils::SetupDefaultPassthroughNodes<USMGraphK2Node_TransitionShutdownNode, USMGraphK2Node_StateInstance_OnStateShutdown>(BoundGraph);
}

void USMGraphNode_StateNode::SetNodeClass(UClass* Class)
{
	StateClass = Class;
	Super::SetNodeClass(Class);
}

void USMGraphNode_StateNode::InitTemplate()
{
	Super::InitTemplate();
}

void USMGraphNode_StateNode::OnCompile(FSMKismetCompilerContext& CompilerContext)
{
	Super::OnCompile(CompilerContext);

	const TArray<FStateStackContainer>& Templates = GetAllNodeStackTemplates();

	if (Templates.Num() > 0)
	{
		FSMNode_Base* RuntimeNode = FSMBlueprintEditorUtils::GetRuntimeNodeFromGraph(BoundGraph);
		check(RuntimeNode);

		for (const FStateStackContainer& Template : Templates)
		{
			if (Template.NodeStackInstanceTemplate && GetDefaultNodeClass() != Template.StateStackClass)
			{
				CompilerContext.AddDefaultObjectTemplate(RuntimeNode->GetNodeGuid(), Template.NodeStackInstanceTemplate, FTemplateContainer::StackTemplate, Template.TemplateGuid);
			}
		}
	}
}

const TArray<FStateStackContainer>& USMGraphNode_StateNode::GetAllNodeStackTemplates() const
{
	return StateStack;
}

int32 USMGraphNode_StateNode::GetIndexOfTemplate(const FGuid& TemplateGuid) const
{
	for (int32 Idx = 0; Idx < StateStack.Num(); ++Idx)
	{
		if (StateStack[Idx].TemplateGuid == TemplateGuid)
		{
			return Idx;
		}
	}

	return -1;
}

USMNodeInstance* USMGraphNode_StateNode::GetTemplateFromIndex(int32 Index) const
{
	if (Index >= 0 && Index < StateStack.Num())
	{
		return StateStack[Index].NodeStackInstanceTemplate;
	}

	return nullptr;
}

void USMGraphNode_StateNode::InitStateStack()
{
	for (FStateStackContainer& StateContainer : StateStack)
	{
		StateContainer.InitTemplate(this);
	}
}

void USMGraphNode_StateNode::DestroyStateStack()
{
	for (FStateStackContainer& StateContainer : StateStack)
	{
		StateContainer.DestroyTemplate();
	}

	StateStack.Reset();
}


USMGraphNode_AnyStateNode::USMGraphNode_AnyStateNode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), bAllowInitialReentry(false)
{
	NodeName = LOCTEXT("AnyStateNodeTitle", "Any State");
}

void USMGraphNode_AnyStateNode::AllocateDefaultPins()
{
	CreatePin(EGPD_Output, TEXT("Transition"), TEXT("Out"));
}

void USMGraphNode_AnyStateNode::PostPlacedNewNode()
{
	// Skip state base so we don't create a graph.
	USMGraphNode_Base::PostPlacedNewNode();
}

void USMGraphNode_AnyStateNode::PostPasteNode()
{
	// Skip state because it relies on a graph being present.
	USMGraphNode_Base::PostPasteNode();
}

FText USMGraphNode_AnyStateNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return NodeName;
}

void USMGraphNode_AnyStateNode::OnRenameNode(const FString& NewName)
{
	NodeName = FText::FromString(NewName);
}

FString USMGraphNode_AnyStateNode::GetStateName() const
{
	return NodeName.ToString();
}

FLinearColor USMGraphNode_AnyStateNode::Internal_GetBackgroundColor() const
{
	const USMEditorSettings* Settings = FSMBlueprintEditorUtils::GetEditorSettings();
	const FLinearColor DefaultColor = Settings->AnyStateDefaultColor;

	if (IsEndState())
	{
		return DefaultColor * FLinearColor(1.f, 1.f, 1.f, 0.5f);
	}

	return DefaultColor;
}

#undef LOCTEXT_NAMESPACE
