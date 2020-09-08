// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#include "SMGraphNode_Base.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Engine/Engine.h"
#include "UObject/UObjectThreadContext.h"
#include "SMBlueprintEditorUtils.h"
#include "SMBlueprint.h"
#include "SMUtils.h"
#include "Graph/SMGraph.h"
#include "Graph/Schema/SMGraphSchema.h"
#include "Graph/SMPropertyGraph.h"
#include "EdGraphUtilities.h"
#include "Customization/SMEditorCustomization.h"
#include "SMBlueprintEditor.h"
#include "SMGraphNode_StateNode.h"
#include "SMNodeInstanceUtils.h"

#define LOCTEXT_NAMESPACE "SMGraphNodeBase"

/** Log a message to the message log up to 4 arguments long. */
#define LOG_MESSAGE(LOG_TYPE, MESSAGE, ARGS, ARGS_COUNT)						\
	do {																		\
		if(ARGS_COUNT == 0)														\
			MessageLog.LOG_TYPE(MESSAGE);										\
		else if(ARGS_COUNT == 1)												\
			MessageLog.LOG_TYPE(MESSAGE, ARGS[0]);								\
		else if(ARGS_COUNT == 2)												\
			MessageLog.LOG_TYPE(MESSAGE, ARGS[0], ARGS[1]);						\
		else if(ARGS_COUNT == 3)												\
			MessageLog.LOG_TYPE(MESSAGE, ARGS[0], ARGS[1], ARGS[2]);			\
		else if(ARGS_COUNT >= 4)												\
			MessageLog.LOG_TYPE(MESSAGE, ARGS[0], ARGS[1], ARGS[2], ARGS[3]);	\
																				\
	} while(0)

USMGraphNode_Base::USMGraphNode_Base(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCanRenameNode = true;
	DebugTotalTime = 0.f;
	bIsDebugActive = false;
	bWasDebugActive = false;
	bCreatePropertyGraphsOnPropertyChange = true;
	MaxTimeToShowDebug = 1.f;
	BoundGraph = nullptr;
	NodeInstanceTemplate = nullptr;
	CachedBrush = FSlateNoResource();
	bJustPasted = false;
	bGenerateTemplateOnNodePlacement = true;
}

void USMGraphNode_Base::PostLoad()
{
	Super::PostLoad();
	
	ConvertToCurrentVersion();
}

void USMGraphNode_Base::DestroyNode()
{
	Super::DestroyNode();
	DestroyAllPropertyGraphs();
}

void USMGraphNode_Base::PostPasteNode()
{
	bJustPasted = true;
	Super::PostPasteNode();

	if (UEdGraph* Graph = GetBoundGraph())
	{
		// Add the new graph as a child of our parent graph
		UEdGraph* ParentGraph = GetGraph();

		if (ParentGraph->SubGraphs.Find(Graph) == INDEX_NONE)
		{
			ParentGraph->SubGraphs.Add(Graph);
		}

		// Restore transactional flag that is lost during copy/paste process
		Graph->SetFlags(RF_Transactional);

		UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraphChecked(ParentGraph);
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	}

	InitTemplate();
	CreateGraphPropertyGraphs(true);

	bJustPasted = false;
}

void USMGraphNode_Base::PostEditUndo()
{
	Super::PostEditUndo();

	if (NodeInstanceTemplate)
	{
		NodeInstanceTemplate->ClearFlags(RF_Transient);
	}
	
	// No bound graph prevents the property graphs from finding their blueprint. This could happen if a graph deletion was being redone.
	if(BoundGraph == nullptr)
	{
		return;
	}
	RefreshAllProperties(false);
}

void USMGraphNode_Base::PostPlacedNewNode()
{
	SetToCurrentVersion();

	Super::PostPlacedNewNode();
}

void USMGraphNode_Base::OnRenameNode(const FString& NewName)
{
	FBlueprintEditorUtils::RenameGraph(GetBoundGraph(), NewName);
}

UObject* USMGraphNode_Base::GetJumpTargetForDoubleClick() const
{
	return BoundGraph;
}

bool USMGraphNode_Base::CanJumpToDefinition() const
{
	return GetJumpTargetForDoubleClick() != nullptr;
}

void USMGraphNode_Base::JumpToDefinition() const
{
	if (UObject* HyperlinkTarget = GetJumpTargetForDoubleClick())
	{
		FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(HyperlinkTarget);
	}
}

bool USMGraphNode_Base::CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const
{
	return Schema->IsA(USMGraphSchema::StaticClass());
}

void USMGraphNode_Base::ReconstructNode()
{
	Super::ReconstructNode();

	for (auto& KeyVal : GraphPropertyGraphs)
	{
		Cast<USMPropertyGraph>(KeyVal.Value)->RefreshProperty(false);
	}

	if (GraphPropertyGraphs.Num())
	{
		UBlueprint* Blueprint = FSMBlueprintEditorUtils::FindBlueprintForNodeChecked(this);
		FSMBlueprintEditorUtils::ConditionallyCompileBlueprint(Blueprint);
	}
}

void USMGraphNode_Base::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	/* BoundGraph could be null if undoing/redoing deletion. */
	if (bCreatePropertyGraphsOnPropertyChange && BoundGraph)
	{
		CreateGraphPropertyGraphs();
	}
}

void USMGraphNode_Base::ValidateNodeDuringCompilation(FCompilerResultsLog& MessageLog) const
{
	Super::ValidateNodeDuringCompilation(MessageLog);

	for (const FSMGraphNodeLog& Log : CollectedLogs)
	{
		switch (Log.LogType)
		{
		case EMessageSeverity::Info:
		{
			LOG_MESSAGE(Note, *Log.ConsoleMessage, Log.ReferenceList, Log.ReferenceList.Num());
			break;
		}
		case EMessageSeverity::Warning:
		{
			LOG_MESSAGE(Warning, *Log.ConsoleMessage, Log.ReferenceList, Log.ReferenceList.Num());
			break;
		}
		case EMessageSeverity::Error:
		{
			LOG_MESSAGE(Error, *Log.ConsoleMessage, Log.ReferenceList, Log.ReferenceList.Num());
			break;
		}
		}
	}
}

void USMGraphNode_Base::CreateGraphPropertyGraphs(bool bGenerateNewGuids)
{
	TSet<FGuid> LiveGuids;

	bool bHasChanged = false;
	if(NodeInstanceTemplate && SupportsPropertyGraphs())
	{
		Modify();
		if(BoundGraph)
		{
			BoundGraph->Modify();
		}

		UClass* TemplateClass = NodeInstanceTemplate->GetClass();
		for (TFieldIterator<FProperty> It(TemplateClass); It; ++It)
		{
			FProperty* Property = *It;
			bool bIsActualGraphProperty = false;
			
			FName VarName = Property->GetFName();
			if (VarName == GET_MEMBER_NAME_CHECKED(USMNodeInstance, ExposedPropertyOverrides))
			{
				continue;
			}
			
			// So custom graph details can be displayed.
			if (FStructProperty* StructProperty = FSMNodeInstanceUtils::IsPropertyGraphProperty(Property))
			{
				bIsActualGraphProperty = true;
				FSMGraphPropertyCustomization::RegisterNewStruct(StructProperty->Struct->GetFName());
			}
			
			// Only properties that are instance editable.
			if(!bIsActualGraphProperty && !FSMNodeInstanceUtils::IsPropertyExposedToGraphNode(Property))
			{
				continue;
			}

			FProperty* TargetProperty = Property;
			int32 ArraySize = 1;
			FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property);
			if(ArrayProperty)
			{
				TargetProperty = ArrayProperty->Inner;
				
				FScriptArrayHelper Helper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<uint8>(NodeInstanceTemplate));
				ArraySize = Helper.Num();

				// No array initialized yet.
				if(ArraySize == 0)
				{
					continue;
				}
			}
			
			// Storage for properties initialized only within this scope.
			TArray<FSMGraphProperty> TempGraphProperties;
			TempGraphProperties.Reserve(ArraySize);

			// Contains temp properties or pointers to stored properties.
			TArray<FSMGraphProperty_Base*> GraphProperties;
			if(bIsActualGraphProperty)
			{
				// This property itself is a graph property.
				USMUtils::BlueprintPropertyToNativeProperty(Property, NodeInstanceTemplate, GraphProperties);

				// Set the variable information. This may still be used for lookup later.
				for(int32 Idx = 0; Idx < GraphProperties.Num(); ++Idx)
				{
					FSMNodeInstanceUtils::SetGraphPropertyFromProperty(*GraphProperties[Idx], TargetProperty, Idx, false);
				}
			}
			else
			{
				// Look for an override.
				FSMGraphProperty* MatchedGraphProperty = NodeInstanceTemplate->ExposedPropertyOverrides.FindByPredicate([&](const FSMGraphProperty_Base& GraphProperty)
				{
					return GraphProperty.VariableName == VarName;
				});

				for (int32 Idx = 0; Idx < ArraySize; ++Idx)
				{
					// Check if the array has been modified. This requires special handling for adding or removing elements.
					if (NodeInstanceTemplate->WasArrayPropertyModified(Property->GetFName()) && NodeInstanceTemplate->ArrayIndexChanged == Idx)
					{
						if (NodeInstanceTemplate->ArrayChangeType == EPropertyChangeType::ArrayRemove)
						{
							HandlePropertyGraphArrayRemoval(GraphProperties, TempGraphProperties, TargetProperty, Idx, ArraySize, MatchedGraphProperty);
						}
						else if (NodeInstanceTemplate->ArrayChangeType == EPropertyChangeType::ArrayAdd)
						{
							HandlePropertyGraphArrayInsertion(GraphProperties, TempGraphProperties, TargetProperty, Idx, ArraySize, MatchedGraphProperty);
						}

						// Always trigger an update if the array was modified.
						bHasChanged = true;
						break;
					}

					// Default handling with no array modification.
					{
						FSMGraphProperty TempProperty;

						if (MatchedGraphProperty)
						{
							// Assign override defaults before assigning a guid.
							TempProperty = *MatchedGraphProperty;
						}

						FSMNodeInstanceUtils::SetGraphPropertyFromProperty(TempProperty, TargetProperty, Idx);

						TempGraphProperties.Add(TempProperty);
						GraphProperties.Add(&TempGraphProperties.Last());
					}
				}
			}
			
			for (int32 i = 0; i < GraphProperties.Num(); ++i)
			{
				FSMGraphProperty_Base* GraphProperty = GraphProperties[i];
				check(GraphProperty);
				GraphProperty->RealDisplayName = Property->GetDisplayNameText();
				GraphProperty->ArrayIndex = i;

				FGuid Guid = GraphProperty->GenerateNewGuidIfNotValid();

				if(bGenerateNewGuids)
				{
					// Make sure reference is up to date.. if this was a copy paste operation it won't be.
					if (USMGraphK2Node_PropertyNode_Base* ExistingGraphPropertyNode = GraphPropertyNodes.FindRef(Guid))
					{
						ExistingGraphPropertyNode->SyncWithContainer();
					}
				}
				
				UEdGraph* OldGraph = GraphPropertyGraphs.FindRef(Guid);
				if (bGenerateNewGuids && OldGraph)
				{
					// Remove these so they don't get deleted later, we just want to reassign the guid.
					GraphPropertyGraphs.Remove(Guid);
					GraphPropertyNodes.Remove(Guid);

					// Create a new guid if requested.
					if(!GraphProperty->ShouldGenerateGuidFromVariable())
					{
						Guid = GraphProperty->GenerateNewGuid();
					}
				}

				LiveGuids.Add(Guid);

				// Refresh the runtime node property in case it has changed.
				if (USMGraphK2Node_PropertyNode_Base* ExistingGraphPropertyNode = GraphPropertyNodes.FindRef(Guid))
				{
					ExistingGraphPropertyNode->SetPropertyNode(GraphProperty);
				}

				if (GraphPropertyGraphs.Contains(Guid))
				{
					continue;
				}

				UEdGraph* PropertyGraph = nullptr;
				if (bGenerateNewGuids && OldGraph)
				{
					PropertyGraph = OldGraph;
					CastChecked<USMPropertyGraph>(PropertyGraph)->RefreshProperty();
				}
				else
				{
					// Load the package for this module. This is needed to find the correct class to load.
					UPackage* Package = GraphProperty->GetEditorModule();
					if(!Package)
					{
						FSMGraphNodeLog NodeLog(EMessageSeverity::Error);
						NodeLog.ConsoleMessage = TEXT("Could not find editor module for node @@.");
						NodeLog.NodeMessage = FString::Printf(TEXT("Could not find editor module for node @@!"));
						NodeLog.ReferenceList.Add(this);
						AddNodeLogMessage(NodeLog);
						continue;
					}
					UClass* GraphClass = GraphProperty->GetGraphClass(Package);
					if(!GraphClass)
					{
						FSMGraphNodeLog NodeLog(EMessageSeverity::Error);
						NodeLog.ConsoleMessage = TEXT("Could not find graph class for node @@.");
						NodeLog.NodeMessage = FString::Printf(TEXT("Could not find graph class for node @@!"));
						NodeLog.ReferenceList.Add(this);
						AddNodeLogMessage(NodeLog);
						continue;
					}
					UClass* SchemaClass = GraphProperty->GetGraphSchemaClass(Package);
					if (!SchemaClass)
					{
						FSMGraphNodeLog NodeLog(EMessageSeverity::Error);
						NodeLog.ConsoleMessage = TEXT("Could not find schema class for node @@.");
						NodeLog.NodeMessage = FString::Printf(TEXT("Could not find schema class for node @@!"));
						NodeLog.ReferenceList.Add(this);
						AddNodeLogMessage(NodeLog);
						continue;
					}

					PropertyGraph = FBlueprintEditorUtils::CreateNewGraph(
						BoundGraph,
						NAME_None,
						GraphClass,
						SchemaClass);
					check(PropertyGraph);

					FEdGraphUtilities::RenameGraphToNameOrCloseToName(PropertyGraph, *GraphProperty->GetDisplayName().ToString());

					CastChecked<USMPropertyGraph>(PropertyGraph)->TempGraphProperty = GraphProperty;
					
					// Initialize the property graph
					const UEdGraphSchema* Schema = PropertyGraph->GetSchema();
					Schema->CreateDefaultNodesForGraph(*PropertyGraph);
				}

				BoundGraph->SubGraphs.AddUnique(PropertyGraph);

				// Look for placed property nodes and link them.
				InitPropertyGraphNodes(PropertyGraph, GraphProperty);

				// Clear out temp property as it wont be valid after this scope.
				CastChecked<USMPropertyGraph>(PropertyGraph)->TempGraphProperty = nullptr;
				
				GraphPropertyGraphs.Add(Guid, PropertyGraph);
				bHasChanged = true;
			}
		}

		NodeInstanceTemplate->ResetArrayCheck();
	}

	// Remove graphs no longer used.
	TArray<FGuid> CurrentKeys;
	GraphPropertyGraphs.GetKeys(CurrentKeys);

	for(const FGuid& Guid : CurrentKeys)
	{
		if(!LiveGuids.Contains(Guid))
		{
			UEdGraph* GraphToRemove = GraphPropertyGraphs[Guid];
			RemovePropertyGraph(Cast<USMPropertyGraph>(GraphToRemove), false);
			GraphPropertyGraphs.Remove(Guid);
			GraphPropertyNodes.Remove(Guid);
			bHasChanged = true;
		}
	}

	if(bHasChanged)
	{
		ReconstructNode();
	}
}

UEdGraph* USMGraphNode_Base::GetGraphPropertyGraph(const FGuid& Guid) const
{
	return GraphPropertyGraphs.FindRef(Guid);
}

USMGraphK2Node_PropertyNode_Base* USMGraphNode_Base::GetGraphPropertyNode(const FGuid& Guid) const
{
	return GraphPropertyNodes.FindRef(Guid);
}

USMGraphK2Node_PropertyNode_Base* USMGraphNode_Base::GetGraphPropertyNode(const FName& VariableName) const
{
	for (const auto& KeyVal : GraphPropertyNodes)
	{
		if (KeyVal.Value && KeyVal.Value->GetPropertyNodeChecked()->VariableName == VariableName)
		{
			return KeyVal.Value;
		}
	}

	return nullptr;
}

TArray<USMGraphK2Node_PropertyNode_Base*> USMGraphNode_Base::GetAllPropertyGraphNodesAsArray() const
{
	TArray<USMGraphK2Node_PropertyNode_Base*> Nodes;

	for(const auto& KeyVal : GraphPropertyNodes)
	{
		if (KeyVal.Value)
		{
			Nodes.Add(KeyVal.Value);
		}
	}

	return Nodes;
}

void USMGraphNode_Base::InitPropertyGraphNodes(UEdGraph* PropertyGraph, FSMGraphProperty_Base* Property)
{
	Modify();
	
	TArray<USMGraphK2Node_PropertyNode_Base*> PropertyNodes;
	FSMBlueprintEditorUtils::GetAllNodesOfClassNested<USMGraphK2Node_PropertyNode_Base>(PropertyGraph, PropertyNodes);
	for (USMGraphK2Node_PropertyNode_Base* PlacedPropertyNode : PropertyNodes)
	{
		PlacedPropertyNode->Modify();
		PlacedPropertyNode->OwningGraphNode = this;
		PlacedPropertyNode->SetPropertyNode(Property);
		PlacedPropertyNode->SetPinValueFromPropertyDefaults(bJustPasted);
		GraphPropertyNodes.Add(Property->GetGuid(), PlacedPropertyNode);
	}
}

void USMGraphNode_Base::RefreshAllProperties(bool bModify)
{
	for(const auto& KeyVal : GetAllPropertyGraphs())
	{
		if(USMPropertyGraph* PropertyGraph = Cast<USMPropertyGraph>(KeyVal.Value))
		{
			PropertyGraph->RefreshProperty(bModify);
		}
	}
}

void USMGraphNode_Base::ForceRecreateProperties()
{
	CreateGraphPropertyGraphs();
	RefreshAllProperties(false);
}

USMGraphK2Node_PropertyNode_Base* USMGraphNode_Base::GetPropertyNodeUnderMouse() const
{
	for(const auto& KeyVal : GetAllPropertyGraphNodes())
	{
		if(KeyVal.Value->bMouseOverNodeProperty)
		{
			return KeyVal.Value;
		}
	}

	return nullptr;
}

UEdGraphPin* USMGraphNode_Base::GetInputPin() const
{
	if(Pins.Num() == 0 || Pins[INDEX_PIN_INPUT]->Direction == EGPD_Output)
	{
		return nullptr;
	}

	return Pins[INDEX_PIN_INPUT];
}

UEdGraphPin* USMGraphNode_Base::GetOutputPin() const
{
	for(UEdGraphPin* Pin : Pins)
	{
		if(Pin->Direction == EGPD_Output)
		{
			return Pin;
		}
	}

	return nullptr;
}

UEdGraphNode* USMGraphNode_Base::GetOutputNode() const
{
	UEdGraphPin* OutputPin = GetOutputPin();

	if (OutputPin)
	{
		if (OutputPin->LinkedTo.Num() > 0 && OutputPin->LinkedTo[INDEX_PIN_INPUT]->GetOwningNode() != nullptr)
		{
			return OutputPin->LinkedTo[INDEX_PIN_INPUT]->GetOwningNode();
		}
	}

	return nullptr;
}

void USMGraphNode_Base::GetAllOutputNodes(TArray<UEdGraphNode*>& OutNodes) const
{
	UEdGraphPin* OutputPin = GetOutputPin();

	if (OutputPin)
	{
		for (int32 Idx = 0; Idx < OutputPin->LinkedTo.Num(); ++Idx)
		{
			OutNodes.Add(OutputPin->LinkedTo[Idx]->GetOwningNode());
		}
	}
}

void USMGraphNode_Base::PreCompile(FSMKismetCompilerContext& CompilerContext)
{
	if (!BoundGraph)
	{
		return;
	}

	ConvertToCurrentVersion();

	ResetLogMessages();

	TMap<FGuid, UEdGraph*> PropertyGraphs = GetAllPropertyGraphs();

	for (const auto& KeyVal : PropertyGraphs)
	{
		if (KeyVal.Value == nullptr)
		{
			GraphPropertyGraphs.Remove(KeyVal.Key);
			GraphPropertyNodes.Remove(KeyVal.Key);

			CompilerContext.MessageLog.Error(TEXT("Property graph missing on load on node @@."), this);
		}
	}
	
	ForceRecreateProperties();
}

void USMGraphNode_Base::OnCompile(FSMKismetCompilerContext& CompilerContext)
{
	if(!BoundGraph)
	{
		return;
	}

	FSMNode_Base* RuntimeNode = FSMBlueprintEditorUtils::GetRuntimeNodeFromGraph(BoundGraph);
	check(RuntimeNode);
	RuntimeNode->SetNodeInstanceClass(GetNodeClass());
	if(NodeInstanceTemplate && !IsUsingDefaultNodeClass())
	{
		// We don't need the default template at runtime.
		CompilerContext.DefaultObjectTemplates.Add(RuntimeNode->GetNodeGuid(), NodeInstanceTemplate);
	}
}

void USMGraphNode_Base::UpdateTime(float DeltaTime)
{
	if (const FSMNode_Base* DebugNode = GetDebugNode())
	{
		MaxTimeToShowDebug = GetMaxDebugTime();
		
		// Toggle active status and reset time if switching active states.
		if(DebugNode->IsActive() || (DebugNode->bWasActive && !WasDebugNodeActive()))
		{
			bWasDebugActive = false;

			// Was active is debug only data and exists to help us determine if we should draw an active state.
			const_cast<FSMNode_Base*>(DebugNode)->bWasActive = false;
			if (!IsDebugNodeActive())
			{
				bIsDebugActive = true;
				DebugTotalTime = 0.f;
			}
		}
		else if (IsDebugNodeActive())
		{
			bWasDebugActive = true;
			bIsDebugActive = false;
			DebugTotalTime = 0.f;
		}
		// In the event a node is no longer active but is still being reported it is and we don't want to display it any more.
		else if (WasDebugNodeActive() && !IsDebugNodeActive() && DebugTotalTime >= MaxTimeToShowDebug)
		{
			bWasDebugActive = false;
		}
		else
		{
			DebugTotalTime += DeltaTime;
		}
	}
}

void USMGraphNode_Base::ResetLogMessages()
{
	CollectedLogs.Empty();
	bHasCompilerMessage = false;
}

void USMGraphNode_Base::UpdateErrorMessageFromLogs()
{
	bHasCompilerMessage = TryGetNodeLogMessage(ErrorMsg, ErrorType);
}

void USMGraphNode_Base::AddNodeLogMessage(const FSMGraphNodeLog& Message)
{
	CollectedLogs.Add(Message);
}

bool USMGraphNode_Base::TryGetNodeLogMessage(FString& OutMessage, int32& OutSeverity) const
{
	int32 Severity = EMessageSeverity::Info;
	FString Message;
	for (const FSMGraphNodeLog& Log : CollectedLogs)
	{
		if (!Message.IsEmpty())
		{
			Message += "\n";
		}

		Message += Log.NodeMessage;

		if (Log.LogType < Severity)
		{
			Severity = Log.LogType;
		}
	}

	OutMessage = Message;
	OutSeverity = Severity;

	return CollectedLogs.Num() > 0;
}

void USMGraphNode_Base::InitTemplate()
{
	UClass* NodeClass = GetNodeClass();
	UClass* DefaultNodeClass = GetDefaultNodeClass();
	if (NodeClass == nullptr)
	{
		/* No longer allow null classes.
		 * The default class is used to configure shared properties for all states.
		 * A default template is not needed at runtime and won't be copied to the CDO. */
		if (!DefaultNodeClass)
		{
			ensureAlways(GetClass() == USMGraphNode_StateMachineEntryNode::StaticClass() || GetClass() == USMGraphNode_AnyStateNode::StaticClass());
			return;
		}
		
		SetNodeClass(DefaultNodeClass);
		return;
	}

	Modify();
	
	const FString TemplateName = FString::Printf(TEXT("NODE_TEMPLATE_%s_%s_%s"), *GetName(), *NodeClass->GetName(), *FGuid::NewGuid().ToString());
	USMNodeInstance* NewTemplate = NodeClass ? NewObject<USMNodeInstance>(this, NodeClass, *TemplateName, RF_ArchetypeObject | RF_Transactional | RF_Public) : nullptr;

	if (NodeInstanceTemplate)
	{
		NodeInstanceTemplate->Modify();
		
		if (NewTemplate)
		{
			UEngine::CopyPropertiesForUnrelatedObjects(NodeInstanceTemplate, NewTemplate);
		}

		// Original template isn't needed any more.
		DestroyTemplate();
	}

	NodeInstanceTemplate = NewTemplate;

	// We only want a template for default classes.
	if (NodeClass != DefaultNodeClass)
	{
		if (NodeInstanceTemplate)
		{
			if (!FUObjectThreadContext::Get().IsRoutingPostLoad)
			{
				NodeInstanceTemplate->ConstructionScript();
			}
		}
	}
	
	// Need to recreate property graphs before reconstructing the node otherwise properties will mismatch and cause a crash.
	if (BoundGraph)
	{
		CreateGraphPropertyGraphs();
	}

	if (NodeClass != DefaultNodeClass)
	{
		PlaceDefaultInstanceNodes();
	}
	
	// Template may have new widgets to display.
	ReconstructNode();
}

void USMGraphNode_Base::DestroyTemplate()
{
	if (NodeInstanceTemplate)
	{
		NodeInstanceTemplate->Modify();
		FSMBlueprintEditorUtils::TrashObject(NodeInstanceTemplate);
		NodeInstanceTemplate = nullptr;
	}
}

void USMGraphNode_Base::DestroyAllPropertyGraphs()
{
	Modify();
	
	for (const auto& KeyVal : GetAllPropertyGraphNodes())
	{
		if (KeyVal.Value)
		{
			USMPropertyGraph* Graph = KeyVal.Value->GetPropertyGraph();
			RemovePropertyGraph(Graph, false);
		}
	}

	GraphPropertyNodes.Empty();
	GraphPropertyGraphs.Empty();
}

void USMGraphNode_Base::PlaceDefaultInstanceNodes()
{
	Modify();

	if (BoundGraph)
	{
		BoundGraph->Modify();
	}
}

void USMGraphNode_Base::SetNodeClass(UClass* Class)
{
	InitTemplate();
}

UClass* USMGraphNode_Base::GetDefaultNodeClass() const
{
	if (FSMNode_Base* RuntimeNode = FindRuntimeNode())
	{
		return RuntimeNode->GetDefaultNodeInstanceClass();
	}

	return nullptr;
}

USMGraph* USMGraphNode_Base::GetStateMachineGraph() const
{
	return Cast<USMGraph>(GetGraph());
}

FSMNode_Base* USMGraphNode_Base::FindRuntimeNode() const
{
	return FSMBlueprintEditorUtils::GetRuntimeNodeFromGraph(GetBoundGraph());
}

const FSMNode_Base* USMGraphNode_Base::GetDebugNode() const
{
	USMBlueprint* Blueprint = CastChecked<USMBlueprint>(FSMBlueprintEditorUtils::FindBlueprintForNode(this));

	if (USMInstance* Instance = Cast<USMInstance>(Blueprint->GetObjectBeingDebugged()))
	{
		if (FSMNode_Base* RuntimeNode = FindRuntimeNode())
		{
			const FSMDebugStateMachine& DebugMachine = Instance->GetDebugStateMachineConst();

			// Find the real runtime node being debugged.
			return DebugMachine.GetRuntimeNode(RuntimeNode->GetNodeGuid());
		}
	}

	return nullptr;
}

float USMGraphNode_Base::GetMaxDebugTime() const
{
	const USMEditorSettings* Settings = FSMBlueprintEditorUtils::GetEditorSettings();
	return Settings->TimeToDisplayLastActiveState + Settings->TimeToFadeLastActiveState;
}

FLinearColor USMGraphNode_Base::GetBackgroundColor() const
{
	const FLinearColor* CustomColor = GetCustomBackgroundColor();
	const FLinearColor BaseColor = Internal_GetBackgroundColor() * (CustomColor ? *CustomColor : FLinearColor(1.f, 1.f, 1.f, 1.f));
	const FLinearColor ActiveColor = GetActiveBackgroundColor();

	if (GetDebugNode() == nullptr)
	{
		return BaseColor;
	}

	if (IsDebugNodeActive())
	{
		return ActiveColor;
	}

	const float TimeToFade = 0.7f;
	const float DebugTime = GetDebugTime();

	if (bWasDebugActive && DebugTime < TimeToFade)
	{
		return FLinearColor::LerpUsingHSV(ActiveColor, BaseColor, DebugTime / TimeToFade);
	}

	return BaseColor;
}

FLinearColor USMGraphNode_Base::GetActiveBackgroundColor() const
{
	const USMEditorSettings* Settings = FSMBlueprintEditorUtils::GetEditorSettings();
	return Settings->ActiveStateColor;
}

const FSlateBrush* USMGraphNode_Base::GetNodeIcon()
{
	if(USMNodeInstance* Instance = GetNodeTemplate())
	{
		if(Instance->HasCustomIcon())
		{
			UTexture2D* Texture = Instance->GetNodeIcon();
			const FString TextureName = Texture ? Texture->GetFullName() : FString();
			const FVector2D Size = Instance->GetNodeIconSize();
			const FLinearColor TintColor = Instance->GetNodeIconTintColor();
			if(CachedTexture != TextureName || CachedTextureSize != Size || CachedNodeTintColor != TintColor)
			{
				CachedTexture = TextureName;
				CachedTextureSize = Size;
				CachedNodeTintColor = TintColor;
				FSlateBrush Brush;
				if (Texture)
				{
					Brush.SetResourceObject(Texture);
					Brush.ImageSize = Size.GetMax() > 0 ? Size : FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
					Brush.TintColor = TintColor;
				}
				else
				{
					Brush = FSlateNoResource();
				}
				
				CachedBrush = Brush;
			}

			return &CachedBrush;
		}
	}

	return nullptr;
}

void USMGraphNode_Base::ConvertToCurrentVersion(bool bOnlyOnLoad, bool bResetVersion)
{
	if (bResetVersion)
	{
		LoadedVersion = 0;
	}
	
	if ((!IsTemplate() && GetLinker() && GetLinker()->IsPersistent() && GetLinker()->IsLoading()) || !bOnlyOnLoad)
	{
		if (GetDefaultNodeClass())
		{
			if (NodeInstanceTemplate == nullptr)
			{
				// Configure pre 2.3 nodes that are missing node instance templates.
				InitTemplate();
			}
			if (LoadedVersion < TEMPLATE_PROPERTY_VERSION)
			{
				// Pre 2.3 nodes need to have their properties imported to the node instance template.
				ImportDeprecatedProperties();
			}
		}

		SetToCurrentVersion();
	}
}

void USMGraphNode_Base::SetToCurrentVersion()
{
	LoadedVersion = CURRENT_VERSION;
}

void USMGraphNode_Base::ForceSetVersion(int32 NewVersion)
{
	LoadedVersion = NewVersion;
}

FLinearColor USMGraphNode_Base::Internal_GetBackgroundColor() const
{
	return FLinearColor::Black;
}

const FLinearColor* USMGraphNode_Base::GetCustomBackgroundColor() const
{
	if(!NodeInstanceTemplate || !NodeInstanceTemplate->HasCustomColor())
	{
		return nullptr;
	}

	return &NodeInstanceTemplate->GetNodeColor();
}

void USMGraphNode_Base::RemovePropertyGraph(USMPropertyGraph* PropertyGraph, bool RemoveFromMaps)
{
	if(!PropertyGraph)
	{
		return;
	}

	PropertyGraph->Modify();
	PropertyGraph->ResultNode->Modify();
	
	if(RemoveFromMaps)
	{
		const FGuid& Guid = PropertyGraph->ResultNode->GetPropertyNode()->GetGuid();
		GraphPropertyGraphs.Remove(Guid);
		GraphPropertyNodes.Remove(Guid);
	}
	
	if (FSMBlueprintEditor* Editor = FSMBlueprintEditorUtils::GetStateMachineEditor(this))
	{
		Editor->CloseDocumentTab(PropertyGraph);
	}

	if (UEdGraph* ParentGraph = Cast<UEdGraph>(PropertyGraph->GetOuter()))
	{
		ParentGraph->Modify();
		ParentGraph->SubGraphs.Remove(PropertyGraph);
	}

	if(PropertyGraph->HasAnyFlags(RF_NeedLoad | RF_NeedPostLoad))
	{
		FSMBlueprintEditorUtils::TrashObject(PropertyGraph);
	}
	else
	{
		UBlueprint* Blueprint = FSMBlueprintEditorUtils::FindBlueprintForNodeChecked(this);
		FSMBlueprintEditorUtils::RemoveGraph(Blueprint, PropertyGraph, EGraphRemoveFlags::None);
	}
}

void USMGraphNode_Base::HandlePropertyGraphArrayRemoval(TArray<FSMGraphProperty_Base*>& GraphProperties,
	TArray<FSMGraphProperty>& TempGraphProperties, FProperty* TargetProperty, int32 RemovalIndex, int32 ArraySize, FSMGraphProperty* OverrideGraphProperty)
{
	FSMGraphProperty TempProperty;

	if (OverrideGraphProperty)
	{
		// Assign override defaults before assigning a guid.
		TempProperty = *OverrideGraphProperty;
	}

	// The property index being removed.
	FSMNodeInstanceUtils::SetGraphPropertyFromProperty(TempProperty, TargetProperty, RemovalIndex);
	UEdGraph* PropertyGraphToRemove = GraphPropertyGraphs.FindRef(TempProperty.GetGuid());
	
	// Remove the graph for the deleted index.
	if (PropertyGraphToRemove)
	{
		RemovePropertyGraph(Cast<USMPropertyGraph>(PropertyGraphToRemove), true);
	}

	// The current graph array size hasn't been adjusted for the removal yet and we need to iterate through everything.
	ArraySize++; 

	// Reassign all guids that follow next in the array to their current index - 1.
	for (int32 NextIdx = RemovalIndex + 1; NextIdx < ArraySize; ++NextIdx)
	{
		FSMGraphProperty NextGraphProperty;

		if (OverrideGraphProperty)
		{
			// Assign override defaults before assigning a guid.
			NextGraphProperty = *OverrideGraphProperty;
		}

		FSMNodeInstanceUtils::SetGraphPropertyFromProperty(NextGraphProperty, TargetProperty, NextIdx);
		const FGuid OldGuid = NextGraphProperty.GetGuid();

		UEdGraph* NextGraph = GraphPropertyGraphs.FindRef(OldGuid);
		USMGraphK2Node_PropertyNode_Base* NextNode = GraphPropertyNodes.FindRef(OldGuid);
		
		// Remove the old guids.
		GraphPropertyGraphs.Remove(OldGuid);
		GraphPropertyNodes.Remove(OldGuid);

		// Update the new guids.
		FSMNodeInstanceUtils::SetGraphPropertyFromProperty(NextGraphProperty, TargetProperty, NextIdx - 1);
		if (NextGraph && NextNode)
		{
			const FGuid& NewGuid = NextGraphProperty.GetGuid();
			
			GraphPropertyGraphs.Add(NewGuid, NextGraph);
			GraphPropertyNodes.Add(NewGuid, NextNode);
		}

		TempGraphProperties.Add(NextGraphProperty);
		GraphProperties.Add(&TempGraphProperties.Last());
	}
}

void USMGraphNode_Base::HandlePropertyGraphArrayInsertion(TArray<FSMGraphProperty_Base*>& GraphProperties,
	TArray<FSMGraphProperty>& TempGraphProperties, FProperty* TargetProperty, int32 InsertionIndex, int32 ArraySize,
	FSMGraphProperty* OverrideGraphProperty)
{
	// Reassign this guid and all guids that follow next in the array to their current index + 1.
	// Go backwards since the previous index would overwrite the next index.
	for (int32 NextIdx = ArraySize - 1; NextIdx >= InsertionIndex; --NextIdx)
	{
		FSMGraphProperty NextGraphProperty;

		if (OverrideGraphProperty)
		{
			// Assign override defaults before assigning a guid.
			NextGraphProperty = *OverrideGraphProperty;
		}

		FSMNodeInstanceUtils::SetGraphPropertyFromProperty(NextGraphProperty, TargetProperty, NextIdx);
		const FGuid OldGuid = NextGraphProperty.GetGuid();

		UEdGraph* NextGraph = GraphPropertyGraphs.FindRef(OldGuid);
		USMGraphK2Node_PropertyNode_Base* NextNode = GraphPropertyNodes.FindRef(OldGuid);

		// Remove the old guids.
		GraphPropertyGraphs.Remove(OldGuid);
		GraphPropertyNodes.Remove(OldGuid);

		// Update the new guids.
		FSMNodeInstanceUtils::SetGraphPropertyFromProperty(NextGraphProperty, TargetProperty, NextIdx + 1);
		if (NextGraph && NextNode)
		{
			const FGuid& NewGuid = NextGraphProperty.GetGuid();

			GraphPropertyGraphs.Add(NewGuid, NextGraph);
			GraphPropertyNodes.Add(NewGuid, NextNode);
		}
	}

	// Add the temp graph properties including the insertion property now that their guids have been updated properly.
	for (int32 Idx = InsertionIndex; Idx < ArraySize; ++Idx)
	{
		FSMGraphProperty NextGraphProperty;

		if (OverrideGraphProperty)
		{
			// Assign override defaults before assigning a guid.
			NextGraphProperty = *OverrideGraphProperty;
		}
		
		FSMNodeInstanceUtils::SetGraphPropertyFromProperty(NextGraphProperty, TargetProperty, Idx);
		TempGraphProperties.Add(NextGraphProperty);
		GraphProperties.Add(&TempGraphProperties.Last());
	}
}

#undef LOCTEXT_NAMESPACE
