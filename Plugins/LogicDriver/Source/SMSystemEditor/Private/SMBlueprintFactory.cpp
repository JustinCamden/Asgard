// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#include "SMBlueprintFactory.h"
#include "Modules/ModuleManager.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/MessageDialog.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/SClassPickerDialog.h"
#include "BlueprintEditorSettings.h"
#include "Graph/SMGraphK2.h"
#include "Graph/Schema/SMGraphK2Schema.h"
#include "Graph/Schema/SMGraphSchema.h"
#include "Blueprints/SMBlueprint.h"
#include "SMInstance.h"
#include "SMConduitInstance.h"
#include "Utilities/SMBlueprintEditorUtils.h"
#include "ClassViewer/Private/UnloadedBlueprintData.h"
#include "SMStateMachineInstance.h"
#include "SMTransitionInstance.h"
#include "Utilities/SMVersionUtils.h"


#define LOCTEXT_NAMESPACE "SMBlueprintFactory"

/**
 * Copied out of Factories/EditorFactories.cpp
 */
class FAssetClassParentFilter : public IClassViewerFilter
{
public:
	FAssetClassParentFilter()
		: DisallowedClassFlags(CLASS_None), bDisallowBlueprintBase(false)
	{}

	/** All children of these classes will be included unless filtered out by another setting. */
	TSet< const UClass* > AllowedChildrenOfClasses;

	/** Disallowed class flags. */
	EClassFlags DisallowedClassFlags;

	/** Disallow blueprint base classes. */
	bool bDisallowBlueprintBase;

	virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
	{
		bool bAllowed = !InClass->HasAnyClassFlags(DisallowedClassFlags)
			&& InFilterFuncs->IfInChildOfClassesSet(AllowedChildrenOfClasses, InClass) != EFilterReturn::Failed;

		if (bAllowed && bDisallowBlueprintBase)
		{
			if (FKismetEditorUtilities::CanCreateBlueprintOfClass(InClass))
			{
				return false;
			}
		}

		return bAllowed;
	}

	virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef< const IUnloadedBlueprintData > InUnloadedClassData, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
	{
		if (bDisallowBlueprintBase)
		{
			return false;
		}

		return !InUnloadedClassData->HasAnyClassFlags(DisallowedClassFlags)
			&& InFilterFuncs->IfInChildOfClassesSet(AllowedChildrenOfClasses, InUnloadedClassData) != EFilterReturn::Failed;
	}
};


USMBlueprintFactory::USMBlueprintFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), BlueprintType()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = USMBlueprint::StaticClass();
	ParentClass = USMInstance::StaticClass();
}

UObject* USMBlueprintFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
	// Make sure we are trying to factory a SM Blueprint, then create and init one
	check(Class->IsChildOf(USMBlueprint::StaticClass()));

	// If they selected an interface, force the parent class to be UInterface
	if (BlueprintType == BPTYPE_Interface)
	{
		ParentClass = UInterface::StaticClass();
	}

	if (!ParentClass || !FKismetEditorUtilities::CanCreateBlueprintOfClass(ParentClass) || !ParentClass->IsChildOf(USMInstance::StaticClass()))
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("ClassName"), (ParentClass != NULL) ? FText::FromString(ParentClass->GetName()) : LOCTEXT("Null", "(null)"));
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("CannotCreateStateMachineBlueprint", "Cannot create a State Machine Blueprint based on the class '{ClassName}'."), Args));
		return nullptr;
	}

	USMBlueprint* NewStateMachineBP = CastChecked<USMBlueprint>(FKismetEditorUtilities::CreateBlueprint(ParentClass, InParent, Name, BlueprintType, USMBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass(), CallingContext));
	CreateGraphsForNewBlueprint(NewStateMachineBP);

	return NewStateMachineBP;
}

UObject* USMBlueprintFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return FactoryCreateNew(Class, InParent, Name, Flags, Context, Warn, NAME_None);
}

bool USMBlueprintFactory::DoesSupportClass(UClass* Class)
{
	return Class == USMBlueprint::StaticClass();
}

FString USMBlueprintFactory::GetDefaultNewAssetName() const
{
	return "BP_StateMachine";
}

void USMBlueprintFactory::CreateGraphsForNewBlueprint(USMBlueprint* Blueprint)
{
	// New blueprints should always be on the latest version.
	FSMVersionUtils::SetToLatestVersion(Blueprint);
	
	// Locate the blueprint's event graph or create a new one.
	UEdGraph* EventGraph = FindObject<UEdGraph>(Blueprint, *(UEdGraphSchema_K2::GN_EventGraph.ToString()));

	if(!EventGraph)
	{
#if WITH_EDITORONLY_DATA
		if (Blueprint->UbergraphPages.Num())
		{
			FBlueprintEditorUtils::RemoveGraphs(Blueprint, Blueprint->UbergraphPages);
		}
#endif
		EventGraph = FBlueprintEditorUtils::CreateNewGraph(Blueprint, UEdGraphSchema_K2::GN_EventGraph, UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());

		FBlueprintEditorUtils::AddUbergraphPage(Blueprint, EventGraph);
		EventGraph->bAllowDeletion = false;

		const UEdGraphSchema* EventGraphSchema = EventGraph->GetSchema();
		EventGraphSchema->CreateDefaultNodesForGraph(*EventGraph);
	}

	UBlueprintEditorSettings* Settings = GetMutableDefault<UBlueprintEditorSettings>();
	if (Settings && Settings->bSpawnDefaultBlueprintNodes)
	{
		// Create default events.
		const int32 NodePositionX = 255;
		int32 NodePositionY = 0;
		
		// OnStateMachineStart
		UK2Node_Event* OnStateMachineStartedNode = FKismetEditorUtilities::AddDefaultEventNode(Blueprint, EventGraph, GET_FUNCTION_NAME_CHECKED(USMInstance, OnStateMachineStart), USMInstance::StaticClass(), NodePositionY);
		if (USMGraphK2Schema::GetThenPin(OnStateMachineStartedNode)->LinkedTo.Num() == 0)
		{
			FSMBlueprintEditorUtils::CreateParentFunctionCall(EventGraph, USMInstance::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(USMInstance, OnStateMachineStart)), OnStateMachineStartedNode, NodePositionX);
		}
		
		// Tick
		UK2Node_Event* TickFunctionNode = FKismetEditorUtilities::AddDefaultEventNode(Blueprint, EventGraph, GET_FUNCTION_NAME_CHECKED(USMInstance, Tick), USMInstance::StaticClass(), NodePositionY);
		if (USMGraphK2Schema::GetThenPin(TickFunctionNode)->LinkedTo.Num() == 0)
		{
			FSMBlueprintEditorUtils::CreateParentFunctionCall(EventGraph, USMInstance::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(USMInstance, Tick)), TickFunctionNode, NodePositionX);
		}

		int32 SafeXPosition = 0;
		int32 SafeYPosition = 0;

		if (EventGraph->Nodes.Num() != 0)
		{
			// Place right under OnStateMachineStart node.
			SafeXPosition = EventGraph->Nodes[0]->NodePosX;
			SafeYPosition = EventGraph->Nodes[0]->NodePosY + EventGraph->Nodes[0]->NodeHeight + 70;
		}

		// Add a getter for the context for the state machine.
		UK2Node_CallFunction* GetOwnerNode = NewObject<UK2Node_CallFunction>(EventGraph);
		UFunction* MakeNodeFunction = USMInstance::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(USMInstance, GetContext));
		GetOwnerNode->CreateNewGuid();
		GetOwnerNode->PostPlacedNewNode();
		GetOwnerNode->SetFromFunction(MakeNodeFunction);
		GetOwnerNode->SetFlags(RF_Transactional);
		GetOwnerNode->AllocateDefaultPins();
		GetOwnerNode->NodePosX = SafeXPosition;
		GetOwnerNode->NodePosY = SafeYPosition;
		UEdGraphSchema_K2::SetNodeMetaData(GetOwnerNode, FNodeMetadata::DefaultGraphNode);
		GetOwnerNode->MakeAutomaticallyPlacedGhostNode();

		EventGraph->AddNode(GetOwnerNode);
	}

	// Default top level state machine graph
	USMGraphK2* NewTopLevelGraph = CastChecked<USMGraphK2>(FBlueprintEditorUtils::CreateNewGraph(Blueprint, USMGraphK2Schema::GN_StateMachineDefinitionGraph, USMGraphK2::StaticClass(), USMGraphK2Schema::StaticClass()));
	NewTopLevelGraph->bAllowDeletion = false;
	FBlueprintEditorUtils::AddUbergraphPage(Blueprint, NewTopLevelGraph);

	const UEdGraphSchema* StateMachineGraphSchema = NewTopLevelGraph->GetSchema();
	StateMachineGraphSchema->CreateDefaultNodesForGraph(*NewTopLevelGraph);

	// Set the first graph to the new state machine.
	TArray<USMGraphK2Node_StateMachineNode*> StateMachineNodes;
	FBlueprintEditorUtils::GetAllNodesOfClass<USMGraphK2Node_StateMachineNode>(Blueprint, StateMachineNodes);
	check(StateMachineNodes.Num() == 1);

	USMGraph* StateMachineGraph = StateMachineNodes[0]->GetStateMachineGraph();
	Blueprint->LastEditedDocuments.Reset();
	Blueprint->LastEditedDocuments.Add(StateMachineGraph);
}


USMNodeBlueprintFactory::USMNodeBlueprintFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), BlueprintType()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = USMNodeBlueprint::StaticClass();
	ParentClass = nullptr;
}

bool USMNodeBlueprintFactory::ConfigureProperties()
{
	// nullptr the DataAssetClass so we can check for selection
	ParentClass = nullptr;

	// Load the classviewer module to display a class picker
	FClassViewerModule& ClassViewerModule = FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer");

	// Fill in options
	FClassViewerInitializationOptions Options;
	Options.Mode = EClassViewerMode::ClassPicker;
	Options.NameTypeToDisplay = EClassViewerNameTypeToDisplay::DisplayName;
	Options.DisplayMode = EClassViewerDisplayMode::TreeView;
	
	TSharedPtr<FAssetClassParentFilter> Filter = MakeShareable(new FAssetClassParentFilter);
	Options.ClassFilter = Filter;

	Filter->DisallowedClassFlags = CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists;
	Filter->AllowedChildrenOfClasses.Add(USMStateMachineInstance::StaticClass());
	Filter->AllowedChildrenOfClasses.Add(USMStateInstance::StaticClass());
	Filter->AllowedChildrenOfClasses.Add(USMConduitInstance::StaticClass());
	Filter->AllowedChildrenOfClasses.Add(USMTransitionInstance::StaticClass());
	
	const FText TitleText = LOCTEXT("CreateDataAssetOptions", "Pick Data Asset Class");
	UClass* ChosenClass = nullptr;
	const bool bPressedOk = SClassPickerDialog::PickClass(TitleText, Options, ChosenClass, USMNodeInstance::StaticClass());

	if (bPressedOk)
	{
		ParentClass = ChosenClass;
	}

	return bPressedOk;
}

UObject* USMNodeBlueprintFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags,
                                                   UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
	// Make sure we are trying to factory a SM Blueprint, then create and init one
	check(Class->IsChildOf(USMNodeBlueprint::StaticClass()));

	// If they selected an interface, force the parent class to be UInterface
	if (BlueprintType == BPTYPE_Interface)
	{
		ParentClass = UInterface::StaticClass();
	}

	if (!ParentClass || !FKismetEditorUtilities::CanCreateBlueprintOfClass(ParentClass) || !ParentClass->IsChildOf(USMNodeInstance::StaticClass()))
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("ClassName"), (ParentClass != NULL) ? FText::FromString(ParentClass->GetName()) : LOCTEXT("Null", "(null)"));
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("CannotCreateStateMachineBlueprint", "Cannot create a State Machine Node Blueprint based on the class '{ClassName}'."), Args));
		return nullptr;
	}

	USMNodeBlueprint* NewStateMachineBP = CastChecked<USMNodeBlueprint>(FKismetEditorUtilities::CreateBlueprint(ParentClass, InParent, Name, BlueprintType, USMNodeBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass(), CallingContext));

	return NewStateMachineBP;
}

UObject* USMNodeBlueprintFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags,
	UObject* Context, FFeedbackContext* Warn)
{
	return FactoryCreateNew(Class, InParent, Name, Flags, Context, Warn, NAME_None);
}

bool USMNodeBlueprintFactory::DoesSupportClass(UClass* Class)
{
	return Class->IsChildOf(ParentClass);
}

FString USMNodeBlueprintFactory::GetDefaultNewAssetName() const
{
	FString NewName = ParentClass ? *ParentClass->GetName() : TEXT("Node");
	NewName.RemoveFromEnd("_C");
	
	return FString::Printf(TEXT("BP_%s"), *NewName);
}

void USMNodeBlueprintFactory::SetupNewBlueprint(USMNodeBlueprint* Blueprint)
{
	UBlueprintEditorSettings* Settings = GetMutableDefault<UBlueprintEditorSettings>();
	if (Settings && Settings->bSpawnDefaultBlueprintNodes)
	{
		if (UEdGraph* EventGraph = FindObject<UEdGraph>(Blueprint, *(UEdGraphSchema_K2::GN_EventGraph.ToString())))
		{
			int32 NodePositionY = 0;

			// Default state event nodes. Don't add default for conduits.
			if (Blueprint->GeneratedClass && Blueprint->GeneratedClass->IsChildOf(USMStateInstance_Base::StaticClass()) && !Blueprint->GeneratedClass->IsChildOf(USMConduitInstance::StaticClass()))
			{
				FKismetEditorUtilities::AddDefaultEventNode(Blueprint, EventGraph, GET_FUNCTION_NAME_CHECKED(USMStateInstance_Base, OnStateBegin), USMStateInstance_Base::StaticClass(), NodePositionY);
				FKismetEditorUtilities::AddDefaultEventNode(Blueprint, EventGraph, GET_FUNCTION_NAME_CHECKED(USMStateInstance_Base, OnStateUpdate), USMStateInstance_Base::StaticClass(), NodePositionY);
				FKismetEditorUtilities::AddDefaultEventNode(Blueprint, EventGraph, GET_FUNCTION_NAME_CHECKED(USMStateInstance_Base, OnStateEnd), USMStateInstance_Base::StaticClass(), NodePositionY);
			}
		}

		if (Blueprint->GeneratedClass)
		{
			if(Blueprint->GeneratedClass->IsChildOf(USMNodeInstance::StaticClass()))
			{
				// Construction script.
				UFunction* Function = USMNodeInstance::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(USMNodeInstance, ConstructionScript));
				UClass* const OverrideFuncClass = CastChecked<UClass>(Function->GetOuter())->GetAuthoritativeClass();

				UEdGraph* const NewGraph = FBlueprintEditorUtils::CreateNewGraph(Blueprint, Function->GetFName(), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
				NewGraph->bAllowDeletion = false;
				FBlueprintEditorUtils::AddFunctionGraph(Blueprint, NewGraph, false, OverrideFuncClass);

				// Make super call.
				TArray<UK2Node*> Nodes;
				FSMBlueprintEditorUtils::GetAllNodesOfClassNested<UK2Node>(NewGraph, Nodes);
				if (Nodes.Num() == 1) // If more then maybe UE4 is autowiring the parent LIKE IT SHOULD.
				{
					FSMBlueprintEditorUtils::CreateParentFunctionCall(NewGraph, USMNodeInstance::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(USMNodeInstance, ConstructionScript)), Nodes[0]);
				}

			}
			if (Blueprint->GeneratedClass->IsChildOf(USMTransitionInstance::StaticClass()))
			{
				// Default transition function.
				UFunction* Function = USMTransitionInstance::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(USMTransitionInstance, CanEnterTransition));
				UClass* const OverrideFuncClass = CastChecked<UClass>(Function->GetOuter())->GetAuthoritativeClass();

				UEdGraph* const NewGraph = FBlueprintEditorUtils::CreateNewGraph(Blueprint, Function->GetFName(), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
				FBlueprintEditorUtils::AddFunctionGraph(Blueprint, NewGraph, false, OverrideFuncClass);

				// Remove the parent call and leave the result false by default.
				{
					TArray<UK2Node_CallParentFunction*> ParentNodes;
					FSMBlueprintEditorUtils::GetAllNodesOfClassNested<UK2Node_CallParentFunction>(NewGraph, ParentNodes);

					if (ParentNodes.Num() > 0)
					{
						FSMBlueprintEditorUtils::RemoveNode(Blueprint, ParentNodes[0]);
					}
				}
				
				// Make the graph the first opened document of the new blueprint.
				Blueprint->LastEditedDocuments.Reset();
				Blueprint->LastEditedDocuments.Add(NewGraph);
			}
			else if (Blueprint->GeneratedClass->IsChildOf(USMConduitInstance::StaticClass()))
			{
				// Default transition function.
				UFunction* Function = USMConduitInstance::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(USMConduitInstance, CanEnterTransition));
				UClass* const OverrideFuncClass = CastChecked<UClass>(Function->GetOuter())->GetAuthoritativeClass();

				UEdGraph* const NewGraph = FBlueprintEditorUtils::CreateNewGraph(Blueprint, Function->GetFName(), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
				FBlueprintEditorUtils::AddFunctionGraph(Blueprint, NewGraph, false, OverrideFuncClass);

				// Remove the parent call and leave the result false by default.
				{
					TArray<UK2Node_CallParentFunction*> ParentNodes;
					FSMBlueprintEditorUtils::GetAllNodesOfClassNested<UK2Node_CallParentFunction>(NewGraph, ParentNodes);

					if (ParentNodes.Num() > 0)
					{
						FSMBlueprintEditorUtils::RemoveNode(Blueprint, ParentNodes[0]);
					}
				}

				if (USMConduitInstance* ConduitInstance = Cast<USMConduitInstance>(Blueprint->GeneratedClass->ClassDefaultObject))
				{
					ConduitInstance->bEvalWithTransitions = FSMBlueprintEditorUtils::GetProjectEditorSettings()->bConfigureNewConduitsAsTransitions;
				}
				
				// Make the graph the first opened document of the new blueprint.
				Blueprint->LastEditedDocuments.Reset();
				Blueprint->LastEditedDocuments.Add(NewGraph);
			}
		}
		// Needed for event nodes to be setup correctly and allow parent node to be selectable in context menu.
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
	}
}

void USMNodeBlueprintFactory::SetParentClass(TSubclassOf<USMNodeInstance> Class)
{
	ParentClass = Class;
}

#undef LOCTEXT_NAMESPACE
