// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#include "SMBlueprint.h"
#include "SMTestHelpers.h"
#include "SMBlueprintGeneratedClass.h"
#include "SMBlueprintFactory.h"
#include "SMBlueprintEditorUtils.h"
#include "SMTestContext.h"
#include "EdGraph/EdGraph.h"
#include "K2Node_CallFunction.h"
#include "Graph/SMConduitGraph.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Graph/SMGraphK2.h"
#include "Graph/SMGraph.h"
#include "Graph/SMStateGraph.h"
#include "Graph/SMIntermediateGraph.h"
#include "Graph/SMTextPropertyGraph.h"
#include "Graph/Schema/SMGraphK2Schema.h"
#include "Graph/Nodes/RootNodes/SMGraphK2Node_StateMachineSelectNode.h"
#include "Graph/Nodes/SMGraphK2Node_StateMachineNode.h"
#include "Graph/Nodes/SMGraphNode_StateNode.h"
#include "Graph/Nodes/SMGraphNode_StateMachineEntryNode.h"
#include "Graph/Nodes/SMGraphNode_TransitionEdge.h"
#include "Graph/Nodes/SMGraphNode_StateMachineStateNode.h"
#include "Graph/Nodes/SMGraphNode_StateMachineParentNode.h"
#include "Graph/Nodes/SMGraphNode_ConduitNode.h"
#include "Graph/Nodes/Helpers/SMGraphK2Node_StateReadNodes.h"
#include "Graph/Nodes/Helpers/SMGraphK2Node_StateWriteNodes.h"
#include "Graph/Nodes/RootNodes/SMGraphK2Node_TransitionInitializedNode.h"
#include "Graph/Nodes/RootNodes/SMGraphK2Node_TransitionShutdownNode.h"
#include "Graph/Nodes/RootNodes/SMGraphK2Node_TransitionPreEvaluateNode.h"
#include "Graph/Nodes/RootNodes/SMGraphK2Node_TransitionPostEvaluateNode.h"
#include "Graph/Nodes/RootNodes/SMGraphK2Node_TransitionEnteredNode.h"
#include "Graph/Nodes/Helpers/SMGraphK2Node_FunctionNodes.h"
#include "Graph/Nodes/PropertyNodes/SMGraphK2Node_TextPropertyNode.h"
#include "Graph/Nodes/RootNodes/SMGraphK2Node_StateEndNode.h"
#include "Graph/Nodes/RootNodes/SMGraphK2Node_StateUpdateNode.h"


#if WITH_DEV_AUTOMATION_TESTS

#if PLATFORM_DESKTOP

/**
 * Create node class blueprints.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreateNodeClassTest, "SMTests.NodeClassCreateBP", EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FCreateNodeClassTest::RunTest(const FString& Parameters)
{
	// Create node classes.
	FAssetHandler StateAsset;
	if (!TestHelpers::TryCreateNewNodeAsset(this, StateAsset, USMStateInstance::StaticClass(), true))
	{
		return false;
	}
	
	return StateAsset.DeleteAsset(this);
}

/**
 * Validate components import their deprecated values correctly.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FComponentTemplateTest, "SMTests.ComponentTemplateUpdate", EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

	bool FComponentTemplateTest::RunTest(const FString& Parameters)
{
	FAssetHandler NewAsset;
	if (!TestHelpers::TryCreateNewStateMachineAsset(this, NewAsset, false))
	{
		return false;
	}

	USMBlueprint* NewBP = NewAsset.GetObjectAs<USMBlueprint>();

	USMStateMachineTestComponent* TestComponent = NewObject<USMStateMachineTestComponent>(GetTransientPackage(), NAME_None, RF_ArchetypeObject | RF_Public);
	TestComponent->SetStateMachineClass(NewBP->GetGeneratedClass());

	bool bOverrideCanEverTick = true;
	bool bCanEverTick = false;
	
	bool bOverrideTickinterval = true;
	float TickInterval = 0.5f;

	// Test valid changes that will be imported.
	TestComponent->SetAllowTick(bOverrideCanEverTick, bCanEverTick);
	TestComponent->SetTickInterval(bOverrideTickinterval, TickInterval);

	TestComponent->ImportDeprecatedProperties_Public();

	USMInstance* Template = TestComponent->GetTemplateForInstance();
	TestNotNull("Instance template created", Template);
	
	TestEqual("CanTick", Template->CanEverTick(), bCanEverTick);
	TestEqual("TickInterval", Template->GetTickInterval(), TickInterval);

	// Prepare for changed values but without allowing override.
	bOverrideCanEverTick = false;
	TestComponent->SetAllowTick(bOverrideCanEverTick, bCanEverTick);
	bOverrideTickinterval = false;
	TestComponent->SetTickInterval(bOverrideTickinterval, TickInterval);

	// This shouldn't work and values should remain the same because we have a template and class set.
	TestComponent->ImportDeprecatedProperties_Public();
	Template = TestComponent->GetTemplateForInstance();
	TestNotNull("Instance template created", Template);
	TestEqual("CanTick", Template->CanEverTick(), bCanEverTick);
	TestEqual("TickInterval", Template->GetTickInterval(), TickInterval);

	// Clear and rerun, values should be default since overrides are disabled.
	TestComponent->ClearTemplateInstance();
	TestComponent->ImportDeprecatedProperties_Public();
	Template = TestComponent->GetTemplateForInstance();
	TestNotNull("Instance template created", Template);
	TestEqual("CanTick", Template->CanEverTick(), !bCanEverTick);
	TestEqual("TickInterval", Template->GetTickInterval(), 0.f);
	
	return NewAsset.DeleteAsset(this);
}

/**
 * Validate pre 2.3 nodes have their templates setup properly and deprecated node values are imported.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNodeTemplateUpdateTest, "SMTests.NodeTemplateUpdate", EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
	bool FNodeTemplateUpdateTest::RunTest(const FString& Parameters)
{
	FAssetHandler NewAsset;
	if (!TestHelpers::TryCreateNewStateMachineAsset(this, NewAsset, false))
	{
		return false;
	}

	USMBlueprint* NewBP = NewAsset.GetObjectAs<USMBlueprint>();

	// Find root state machine.
	USMGraphK2Node_StateMachineNode* RootStateMachineNode = FSMBlueprintEditorUtils::GetRootStateMachineNode(NewBP);

	// Find the state machine graph.
	USMGraph* StateMachineGraph = RootStateMachineNode->GetStateMachineGraph();

	// Total states to test.
	int32 TotalStates = 3;

	// Load default instances.
	UEdGraphPin* LastStatePin = nullptr;
	TestHelpers::BuildLinearStateMachine(this, StateMachineGraph, TotalStates, &LastStatePin);

	// Test importing state values.
	{
		USMGraphNode_StateNodeBase* FirstState = CastChecked<USMGraphNode_StateNodeBase>(StateMachineGraph->EntryNode->GetOutputNode());
		// Default templates
		{
			FirstState->DestroyTemplate();

			TestFalse("Default value correct", FirstState->bDisableTickTransitionEvaluation_DEPRECATED);
			TestFalse("Default value correct", FirstState->bEvalTransitionsOnStart_DEPRECATED);
			TestFalse("Default value correct", FirstState->bExcludeFromAnyState_DEPRECATED);
			TestFalse("Default value correct", FirstState->bAlwaysUpdate_DEPRECATED);

			FirstState->bDisableTickTransitionEvaluation_DEPRECATED = true;
			FirstState->bEvalTransitionsOnStart_DEPRECATED = true;
			FirstState->bExcludeFromAnyState_DEPRECATED = true;
			FirstState->bAlwaysUpdate_DEPRECATED = true;

			FirstState->ForceSetVersion(0);
			FirstState->ConvertToCurrentVersion(true);
			TestNull("Template still null since this wasn't a load.", FirstState->GetNodeTemplate());

			FirstState->ConvertToCurrentVersion(false);
			TestNotNull("Template created.", FirstState->GetNodeTemplate());

			TestTrue("Default value imported", FirstState->GetNodeTemplateAs<USMStateInstance_Base>(true)->bDisableTickTransitionEvaluation);
			TestTrue("Default value imported", FirstState->GetNodeTemplateAs<USMStateInstance_Base>(true)->bEvalTransitionsOnStart);
			TestTrue("Default value imported", FirstState->GetNodeTemplateAs<USMStateInstance_Base>(true)->bExcludeFromAnyState);
			TestTrue("Default value imported", FirstState->GetNodeTemplateAs<USMStateInstance_Base>(true)->bAlwaysUpdate);
			
			// Test runtime with default values.
			{
				FKismetEditorUtilities::CompileBlueprint(NewBP);
				USMTestContext* Context = NewObject<USMTestContext>();
				USMInstance* StateMachineInstance = TestHelpers::CreateNewStateMachineInstanceFromBP(this, NewBP, Context);

				USMStateInstance_Base* StateInstance = CastChecked<USMStateInstance_Base>(StateMachineInstance->GetRootStateMachine().GetSingleInitialState()->GetNodeInstance());

				// Default class templates don't get compiled into the CDO, so the values should still be default in runtime.
				TestFalse("Default value not imported to runtime", StateInstance->bDisableTickTransitionEvaluation);
				TestFalse("Default value not imported to runtime", StateInstance->bEvalTransitionsOnStart);
				TestFalse("Default value not imported to runtime", StateInstance->bExcludeFromAnyState);
				TestFalse("Default value not imported to runtime", StateInstance->bAlwaysUpdate);
			}
		}

		// Existing templates
		{
			const int32 TestInt = 7;
			{
				// Apply user template to a node that already has a default template created.
				FirstState->SetNodeClass(USMStateTestInstance::StaticClass());
				FirstState->GetNodeTemplateAs<USMStateTestInstance>(true)->ExposedInt = TestInt;

				// Defaults already set since we are applying the node class after the initial template was created. Old values should be copied to new template.
				TestTrue("Default value imported", FirstState->GetNodeTemplateAs<USMStateInstance_Base>(true)->bDisableTickTransitionEvaluation);
				TestTrue("Default value imported", FirstState->GetNodeTemplateAs<USMStateInstance_Base>(true)->bEvalTransitionsOnStart);
				TestTrue("Default value imported", FirstState->GetNodeTemplateAs<USMStateInstance_Base>(true)->bExcludeFromAnyState);
				TestTrue("Default value imported", FirstState->GetNodeTemplateAs<USMStateInstance_Base>(true)->bAlwaysUpdate);

				TestEqual("Edited value maintained", FirstState->GetNodeTemplateAs<USMStateTestInstance>(true)->ExposedInt, TestInt);
			}
			
			// Recreate so there are no existing values to be copied.
			{
				FirstState->DestroyTemplate();
				FirstState->SetNodeClass(USMStateTestInstance::StaticClass());
				FirstState->GetNodeTemplateAs<USMStateTestInstance>(true)->ExposedInt = TestInt;
			}
			
			FirstState->ConvertToCurrentVersion(true, true);
			TestFalse("Default value not imported since it's not load", FirstState->GetNodeTemplateAs<USMStateInstance_Base>(true)->bDisableTickTransitionEvaluation);
			TestFalse("Default value not imported since it's not load", FirstState->GetNodeTemplateAs<USMStateInstance_Base>(true)->bEvalTransitionsOnStart);
			TestFalse("Default value not imported since it's not load", FirstState->GetNodeTemplateAs<USMStateInstance_Base>(true)->bExcludeFromAnyState);
			TestFalse("Default value not imported since it's not load", FirstState->GetNodeTemplateAs<USMStateInstance_Base>(true)->bAlwaysUpdate);

			TestEqual("Edited value maintained", FirstState->GetNodeTemplateAs<USMStateTestInstance>(true)->ExposedInt, TestInt);

			FirstState->ConvertToCurrentVersion(false, true);
			TestNotNull("Template created.", FirstState->GetNodeTemplate());

			TestTrue("Default value imported", FirstState->GetNodeTemplateAs<USMStateInstance_Base>(true)->bDisableTickTransitionEvaluation);
			TestTrue("Default value imported", FirstState->GetNodeTemplateAs<USMStateInstance_Base>(true)->bEvalTransitionsOnStart);
			TestTrue("Default value imported", FirstState->GetNodeTemplateAs<USMStateInstance_Base>(true)->bExcludeFromAnyState);
			TestTrue("Default value imported", FirstState->GetNodeTemplateAs<USMStateInstance_Base>(true)->bAlwaysUpdate);

			TestEqual("Edited value maintained", FirstState->GetNodeTemplateAs<USMStateTestInstance>(true)->ExposedInt, TestInt);

			// Test runtime with default values.
			{
				FKismetEditorUtilities::CompileBlueprint(NewBP);
				USMTestContext* Context = NewObject<USMTestContext>();
				USMInstance* StateMachineInstance = TestHelpers::CreateNewStateMachineInstanceFromBP(this, NewBP, Context);

				USMStateTestInstance* StateInstance = CastChecked<USMStateTestInstance>(StateMachineInstance->GetRootStateMachine().GetSingleInitialState()->GetNodeInstance());

				// User templates get copied to the CDO so their values should match the node values.
				TestTrue("Default value imported to runtime", StateInstance->bDisableTickTransitionEvaluation);
				TestTrue("Default value imported to runtime", StateInstance->bEvalTransitionsOnStart);
				TestTrue("Default value imported to runtime", StateInstance->bExcludeFromAnyState);
				TestTrue("Default value imported to runtime", StateInstance->bAlwaysUpdate);

				TestEqual("Edited value maintained", StateInstance->ExposedInt, TestInt);
			}
		}
	}

	// Test importing transition values.
	{
		const int32 PriorityOrder = 4;
		USMGraphNode_TransitionEdge* Transition = CastChecked<USMGraphNode_TransitionEdge>(CastChecked<USMGraphNode_StateNodeBase>(StateMachineGraph->EntryNode->GetOutputNode())->GetNextTransition());
		// Default templates.
		{
			Transition->DestroyTemplate();

			TestEqual("Default value correct", Transition->PriorityOrder_DEPRECATED, 0);
			TestTrue("Default value correct", Transition->bCanEvaluate_DEPRECATED);
			TestTrue("Default value correct", Transition->bCanEvaluateFromEvent_DEPRECATED);
			TestTrue("Default value correct", Transition->bCanEvalWithStartState_DEPRECATED);

			Transition->PriorityOrder_DEPRECATED = PriorityOrder;
			Transition->bCanEvaluate_DEPRECATED = false;
			Transition->bCanEvaluateFromEvent_DEPRECATED = false;
			Transition->bCanEvalWithStartState_DEPRECATED = false;

			Transition->ForceSetVersion(0);
			Transition->ConvertToCurrentVersion(true);
			TestNull("Template still null since this wasn't a load.", Transition->GetNodeTemplate());

			Transition->ConvertToCurrentVersion(false);
			TestNotNull("Template created.", Transition->GetNodeTemplate());

			TestEqual("Default value imported", Transition->GetNodeTemplateAs<USMTransitionInstance>(true)->PriorityOrder, PriorityOrder);
			TestFalse("Default value imported", Transition->GetNodeTemplateAs<USMTransitionInstance>(true)->bCanEvaluate);
			TestFalse("Default value imported", Transition->GetNodeTemplateAs<USMTransitionInstance>(true)->bCanEvaluateFromEvent);
			TestFalse("Default value imported", Transition->GetNodeTemplateAs<USMTransitionInstance>(true)->bCanEvalWithStartState);

			// Test runtime with default values.
			{
				FKismetEditorUtilities::CompileBlueprint(NewBP);
				USMTestContext* Context = NewObject<USMTestContext>();
				USMInstance* StateMachineInstance = TestHelpers::CreateNewStateMachineInstanceFromBP(this, NewBP, Context);

				USMStateInstance_Base* StateInstance = CastChecked<USMStateInstance_Base>(StateMachineInstance->GetRootStateMachine().GetSingleInitialState()->GetNodeInstance());
				TArray<USMTransitionInstance*> Transitions;
				StateInstance->GetOutgoingTransitions(Transitions, false);
				USMTransitionInstance* TransitionInstance = Transitions[0];

				// Default class templates don't get compiled into the CDO, so the values should still be default in runtime.
				TestEqual("Default value not imported to runtime", TransitionInstance->PriorityOrder, 0);
				TestTrue("Default value not imported to runtime", TransitionInstance->bCanEvaluate);
				TestTrue("Default value not imported to runtime", TransitionInstance->bCanEvaluateFromEvent);
				TestTrue("Default value not imported to runtime", TransitionInstance->bCanEvalWithStartState);
			}
		}

		// Existing templates
		{
			const int32 TestInt = 7;
			{
				// Apply user template to a node that already has a default template created.
				Transition->SetNodeClass(USMTransitionTestInstance::StaticClass());
				Transition->GetNodeTemplateAs<USMTransitionTestInstance>(true)->IntValue = TestInt;

				// Defaults already set since we are applying the node class after the initial template was created. Old values should be copied to new template.
				TestEqual("Default value imported", Transition->GetNodeTemplateAs<USMTransitionInstance>(true)->PriorityOrder, PriorityOrder);
				TestFalse("Default value imported", Transition->GetNodeTemplateAs<USMTransitionInstance>(true)->bCanEvaluate);
				TestFalse("Default value imported", Transition->GetNodeTemplateAs<USMTransitionInstance>(true)->bCanEvaluateFromEvent);
				TestFalse("Default value imported", Transition->GetNodeTemplateAs<USMTransitionInstance>(true)->bCanEvalWithStartState);

				TestEqual("Edited value maintained", Transition->GetNodeTemplateAs<USMTransitionTestInstance>(true)->IntValue, TestInt);
			}

			// Recreate so there are no existing values to be copied.
			{
				Transition->DestroyTemplate();
				Transition->SetNodeClass(USMTransitionTestInstance::StaticClass());
				Transition->GetNodeTemplateAs<USMTransitionTestInstance>(true)->IntValue = TestInt;
			}

			Transition->ConvertToCurrentVersion(true, true);
			TestEqual("Default value not imported since it's not load", Transition->GetNodeTemplateAs<USMTransitionInstance>(true)->PriorityOrder, 0);
			TestTrue("Default value not imported since it's not load", Transition->GetNodeTemplateAs<USMTransitionInstance>(true)->bCanEvaluate);
			TestTrue("Default value not imported since it's not load", Transition->GetNodeTemplateAs<USMTransitionInstance>(true)->bCanEvaluateFromEvent);
			TestTrue("Default value not imported since it's not load", Transition->GetNodeTemplateAs<USMTransitionInstance>(true)->bCanEvalWithStartState);

			TestEqual("Edited value maintained", Transition->GetNodeTemplateAs<USMTransitionTestInstance>(true)->IntValue, TestInt);

			Transition->ConvertToCurrentVersion(false, true);
			TestNotNull("Template created.", Transition->GetNodeTemplate());

			TestEqual("Default value imported", Transition->GetNodeTemplateAs<USMTransitionInstance>(true)->PriorityOrder, PriorityOrder);
			TestFalse("Default value imported", Transition->GetNodeTemplateAs<USMTransitionInstance>(true)->bCanEvaluate);
			TestFalse("Default value imported", Transition->GetNodeTemplateAs<USMTransitionInstance>(true)->bCanEvaluateFromEvent);
			TestFalse("Default value imported", Transition->GetNodeTemplateAs<USMTransitionInstance>(true)->bCanEvalWithStartState);

			TestEqual("Edited value maintained", Transition->GetNodeTemplateAs<USMTransitionTestInstance>(true)->IntValue, TestInt);

			// Test runtime with default values.
			{
				FKismetEditorUtilities::CompileBlueprint(NewBP);
				USMTestContext* Context = NewObject<USMTestContext>();
				USMInstance* StateMachineInstance = TestHelpers::CreateNewStateMachineInstanceFromBP(this, NewBP, Context);

				USMStateInstance_Base* StateInstance = CastChecked<USMStateInstance_Base>(StateMachineInstance->GetRootStateMachine().GetSingleInitialState()->GetNodeInstance());
				TArray<USMTransitionInstance*> Transitions;
				StateInstance->GetOutgoingTransitions(Transitions, false);
				USMTransitionTestInstance* TransitionInstance = CastChecked< USMTransitionTestInstance>(Transitions[0]);

				// Default class templates don't get compiled into the CDO, so the values should still be default in runtime.
				TestEqual("Default value imported to runtime", TransitionInstance->PriorityOrder, PriorityOrder);
				TestFalse("Default value imported to runtime", TransitionInstance->bCanEvaluate);
				TestFalse("Default value imported to runtime", TransitionInstance->bCanEvaluateFromEvent);
				TestFalse("Default value imported to runtime", TransitionInstance->bCanEvalWithStartState);

				TestEqual("Edited value maintained", TransitionInstance->IntValue, TestInt);
			}
		}
	}

	// Test importing conduit values.
	{
		USMGraphNode_StateNodeBase* SecondState = CastChecked<USMGraphNode_StateNodeBase>(StateMachineGraph->EntryNode->GetOutputNode())->GetNextNode();
		USMGraphNode_ConduitNode* ConduitNode = FSMBlueprintEditorUtils::ConvertNodeTo<USMGraphNode_ConduitNode>(SecondState);

		// Default template.
		{
			ConduitNode->DestroyTemplate();

			TestFalse("Default value correct", ConduitNode->bDisableTickTransitionEvaluation_DEPRECATED);
			TestFalse("Default value correct", ConduitNode->bEvalTransitionsOnStart_DEPRECATED);
			TestFalse("Default value correct", ConduitNode->bExcludeFromAnyState_DEPRECATED);
			TestFalse("Default value correct", ConduitNode->bAlwaysUpdate_DEPRECATED);

			TestFalse("Default value correct", ConduitNode->bEvalWithTransitions_DEPRECATED);

			ConduitNode->bDisableTickTransitionEvaluation_DEPRECATED = true;
			ConduitNode->bEvalTransitionsOnStart_DEPRECATED = true;
			ConduitNode->bExcludeFromAnyState_DEPRECATED = true;
			ConduitNode->bAlwaysUpdate_DEPRECATED = true;
			ConduitNode->bEvalWithTransitions_DEPRECATED = true;

			ConduitNode->ForceSetVersion(0);
			ConduitNode->ConvertToCurrentVersion(true);
			TestNull("Template still null since this wasn't a load.", ConduitNode->GetNodeTemplate());

			ConduitNode->ConvertToCurrentVersion(false);
			TestNotNull("Template created.", ConduitNode->GetNodeTemplate());

			TestTrue("Default value imported", ConduitNode->GetNodeTemplateAs<USMStateInstance_Base>(true)->bDisableTickTransitionEvaluation);
			TestTrue("Default value imported", ConduitNode->GetNodeTemplateAs<USMStateInstance_Base>(true)->bEvalTransitionsOnStart);
			TestTrue("Default value imported", ConduitNode->GetNodeTemplateAs<USMStateInstance_Base>(true)->bExcludeFromAnyState);
			TestTrue("Default value imported", ConduitNode->GetNodeTemplateAs<USMStateInstance_Base>(true)->bAlwaysUpdate);

			TestTrue("Default value imported", ConduitNode->GetNodeTemplateAs<USMConduitInstance>(true)->bEvalWithTransitions);

			// Test runtime with default values.
			{
				FKismetEditorUtilities::CompileBlueprint(NewBP);
				USMTestContext* Context = NewObject<USMTestContext>();
				USMInstance* StateMachineInstance = TestHelpers::CreateNewStateMachineInstanceFromBP(this, NewBP, Context);

				USMStateInstance_Base* StateInstance = CastChecked<USMStateInstance_Base>(StateMachineInstance->GetRootStateMachine().GetSingleInitialState()->GetNodeInstance());
				USMConduitInstance* ConduitInstance = CastChecked<USMConduitInstance>(StateInstance->GetNextStateByTransitionIndex(0));

				// Default class templates don't get compiled into the CDO, so the values should still be default in runtime.
				TestFalse("Default value not imported to runtime", ConduitInstance->bDisableTickTransitionEvaluation);
				TestFalse("Default value not imported to runtime", ConduitInstance->bEvalTransitionsOnStart);
				TestFalse("Default value not imported to runtime", ConduitInstance->bExcludeFromAnyState);
				TestFalse("Default value not imported to runtime", ConduitInstance->bAlwaysUpdate);

				TestFalse("Default value not imported to runtime", ConduitInstance->bEvalWithTransitions);
			}
		}

		// Existing templates
		{
			const int32 TestInt = 7;
			{
				// Apply user template to a node that already has a default template created.
				ConduitNode->SetNodeClass(USMConduitTestInstance::StaticClass());
				ConduitNode->GetNodeTemplateAs<USMConduitTestInstance>(true)->IntValue = TestInt;

				// Defaults already set since we are applying the node class after the initial template was created. Old values should be copied to new template.
				TestTrue("Default value imported", ConduitNode->GetNodeTemplateAs<USMStateInstance_Base>(true)->bDisableTickTransitionEvaluation);
				TestTrue("Default value imported", ConduitNode->GetNodeTemplateAs<USMStateInstance_Base>(true)->bEvalTransitionsOnStart);
				TestTrue("Default value imported", ConduitNode->GetNodeTemplateAs<USMStateInstance_Base>(true)->bExcludeFromAnyState);
				TestTrue("Default value imported", ConduitNode->GetNodeTemplateAs<USMStateInstance_Base>(true)->bAlwaysUpdate);
				
				TestTrue("Default value imported", ConduitNode->GetNodeTemplateAs<USMConduitInstance>(true)->bEvalWithTransitions);
				
				TestEqual("Edited value maintained", ConduitNode->GetNodeTemplateAs<USMConduitTestInstance>(true)->IntValue, TestInt);
			}

			// Recreate so there are no existing values to be copied.
			{
				ConduitNode->DestroyTemplate();
				ConduitNode->SetNodeClass(USMConduitTestInstance::StaticClass());
				ConduitNode->GetNodeTemplateAs<USMConduitTestInstance>(true)->IntValue = TestInt;
			}

			ConduitNode->ConvertToCurrentVersion(true, true);
			TestFalse("Default value not imported since it's not load", ConduitNode->GetNodeTemplateAs<USMStateInstance_Base>(true)->bDisableTickTransitionEvaluation);
			TestFalse("Default value not imported since it's not load", ConduitNode->GetNodeTemplateAs<USMStateInstance_Base>(true)->bEvalTransitionsOnStart);
			TestFalse("Default value not imported since it's not load", ConduitNode->GetNodeTemplateAs<USMStateInstance_Base>(true)->bExcludeFromAnyState);
			TestFalse("Default value not imported since it's not load", ConduitNode->GetNodeTemplateAs<USMStateInstance_Base>(true)->bAlwaysUpdate);

			TestFalse("Default value not imported since it's not load", ConduitNode->GetNodeTemplateAs<USMConduitInstance>(true)->bEvalWithTransitions);
			
			TestEqual("Edited value maintained", ConduitNode->GetNodeTemplateAs<USMConduitTestInstance>(true)->IntValue, TestInt);

			ConduitNode->ConvertToCurrentVersion(false, true);
			TestNotNull("Template created.", ConduitNode->GetNodeTemplate());

			TestTrue("Default value imported", ConduitNode->GetNodeTemplateAs<USMStateInstance_Base>(true)->bDisableTickTransitionEvaluation);
			TestTrue("Default value imported", ConduitNode->GetNodeTemplateAs<USMStateInstance_Base>(true)->bEvalTransitionsOnStart);
			TestTrue("Default value imported", ConduitNode->GetNodeTemplateAs<USMStateInstance_Base>(true)->bExcludeFromAnyState);
			TestTrue("Default value imported", ConduitNode->GetNodeTemplateAs<USMStateInstance_Base>(true)->bAlwaysUpdate);

			TestTrue("Default value imported", ConduitNode->GetNodeTemplateAs<USMConduitInstance>(true)->bEvalWithTransitions);
			
			TestEqual("Edited value maintained", ConduitNode->GetNodeTemplateAs<USMConduitTestInstance>(true)->IntValue, TestInt);

			// Test runtime with default values.
			{
				FKismetEditorUtilities::CompileBlueprint(NewBP);
				USMTestContext* Context = NewObject<USMTestContext>();
				USMInstance* StateMachineInstance = TestHelpers::CreateNewStateMachineInstanceFromBP(this, NewBP, Context);

				USMStateInstance_Base* StateInstance = CastChecked<USMStateInstance_Base>(StateMachineInstance->GetRootStateMachine().GetSingleInitialState()->GetNodeInstance());
				USMConduitTestInstance* ConduitInstance = CastChecked<USMConduitTestInstance>(StateInstance->GetNextStateByTransitionIndex(0));

				// Default class templates don't get compiled into the CDO, so the values should still be default in runtime.
				TestTrue("Default value imported to runtime", ConduitInstance->bDisableTickTransitionEvaluation);
				TestTrue("Default value imported to runtime", ConduitInstance->bEvalTransitionsOnStart);
				TestTrue("Default value imported to runtime", ConduitInstance->bExcludeFromAnyState);
				TestTrue("Default value imported to runtime", ConduitInstance->bAlwaysUpdate);
				
				TestEqual("Edited value maintained", ConduitInstance->IntValue, TestInt);
				
				TestTrue("Default value imported to runtime", ConduitInstance->bEvalWithTransitions);
			}
		}
	}

	// Test importing state machine values.
	{
		USMGraphNode_StateNodeBase* ThirdState = CastChecked<USMGraphNode_StateNodeBase>(StateMachineGraph->EntryNode->GetOutputNode())->GetNextNode()->GetNextNode();
		USMGraphNode_StateMachineStateNode* FSMNode = FSMBlueprintEditorUtils::ConvertNodeTo<USMGraphNode_StateMachineStateNode>(ThirdState);

		// Default template.
		{
			FSMNode->DestroyTemplate();

			TestFalse("Default value correct", FSMNode->bDisableTickTransitionEvaluation_DEPRECATED);
			TestFalse("Default value correct", FSMNode->bEvalTransitionsOnStart_DEPRECATED);
			TestFalse("Default value correct", FSMNode->bExcludeFromAnyState_DEPRECATED);
			TestFalse("Default value correct", FSMNode->bAlwaysUpdate_DEPRECATED);

			TestFalse("Default value correct", FSMNode->bReuseIfNotEndState_DEPRECATED);
			TestFalse("Default value correct", FSMNode->bReuseCurrentState_DEPRECATED);

			FSMNode->bDisableTickTransitionEvaluation_DEPRECATED = true;
			FSMNode->bEvalTransitionsOnStart_DEPRECATED = true;
			FSMNode->bExcludeFromAnyState_DEPRECATED = true;
			FSMNode->bAlwaysUpdate_DEPRECATED = true;

			FSMNode->bReuseIfNotEndState_DEPRECATED = true;
			FSMNode->bReuseCurrentState_DEPRECATED = true;

			FSMNode->ForceSetVersion(0);
			FSMNode->ConvertToCurrentVersion(true);
			TestNull("Template still null since this wasn't a load.", FSMNode->GetNodeTemplate());

			FSMNode->ConvertToCurrentVersion(false);
			TestNotNull("Template created.", FSMNode->GetNodeTemplate());

			TestTrue("Default value imported", FSMNode->GetNodeTemplateAs<USMStateInstance_Base>(true)->bDisableTickTransitionEvaluation);
			TestTrue("Default value imported", FSMNode->GetNodeTemplateAs<USMStateInstance_Base>(true)->bEvalTransitionsOnStart);
			TestTrue("Default value imported", FSMNode->GetNodeTemplateAs<USMStateInstance_Base>(true)->bExcludeFromAnyState);
			TestTrue("Default value imported", FSMNode->GetNodeTemplateAs<USMStateInstance_Base>(true)->bAlwaysUpdate);

			TestTrue("Default value imported", FSMNode->GetNodeTemplateAs<USMStateMachineInstance>(true)->bReuseIfNotEndState);
			TestTrue("Default value imported", FSMNode->GetNodeTemplateAs<USMStateMachineInstance>(true)->bReuseCurrentState);

			// Test runtime with default values.
			{
				FKismetEditorUtilities::CompileBlueprint(NewBP);
				USMTestContext* Context = NewObject<USMTestContext>();
				USMInstance* StateMachineInstance = TestHelpers::CreateNewStateMachineInstanceFromBP(this, NewBP, Context);

				USMStateInstance_Base* StateInstance = CastChecked<USMStateInstance_Base>(StateMachineInstance->GetRootStateMachine().GetSingleInitialState()->GetNodeInstance());
				USMStateMachineInstance* FSMInstance = CastChecked<USMStateMachineInstance>(StateInstance->GetNextStateByTransitionIndex(0)->GetNextStateByTransitionIndex(0));

				// Default class templates don't get compiled into the CDO, so the values should still be default in runtime.
				TestFalse("Default value not imported to runtime", FSMInstance->bDisableTickTransitionEvaluation);
				TestFalse("Default value not imported to runtime", FSMInstance->bEvalTransitionsOnStart);
				TestFalse("Default value not imported to runtime", FSMInstance->bExcludeFromAnyState);
				TestFalse("Default value not imported to runtime", FSMInstance->bAlwaysUpdate);

				TestFalse("Default value not imported to runtime", FSMInstance->bReuseIfNotEndState);
				TestFalse("Default value not imported to runtime", FSMInstance->bReuseCurrentState);
			}
		}

		// Existing templates
		{
			const int32 TestInt = 7;
			{
				// Apply user template to a node that already has a default template created.
				FSMNode->SetNodeClass(USMStateMachineTestInstance::StaticClass());
				FSMNode->GetNodeTemplateAs<USMStateMachineTestInstance>(true)->ExposedInt = TestInt;

				// Defaults already set since we are applying the node class after the initial template was created. Old values should be copied to new template.
				TestTrue("Default value imported", FSMNode->GetNodeTemplateAs<USMStateMachineTestInstance>(true)->bDisableTickTransitionEvaluation);
				TestTrue("Default value imported", FSMNode->GetNodeTemplateAs<USMStateMachineTestInstance>(true)->bEvalTransitionsOnStart);
				TestTrue("Default value imported", FSMNode->GetNodeTemplateAs<USMStateMachineTestInstance>(true)->bExcludeFromAnyState);
				TestTrue("Default value imported", FSMNode->GetNodeTemplateAs<USMStateMachineTestInstance>(true)->bAlwaysUpdate);

				TestTrue("Default value imported", FSMNode->GetNodeTemplateAs<USMStateMachineInstance>(true)->bReuseIfNotEndState);
				TestTrue("Default value imported", FSMNode->GetNodeTemplateAs<USMStateMachineInstance>(true)->bReuseCurrentState);
				
				TestEqual("Edited value maintained", FSMNode->GetNodeTemplateAs<USMStateMachineTestInstance>(true)->ExposedInt, TestInt);
			}

			// Recreate so there are no existing values to be copied.
			{
				FSMNode->DestroyTemplate();
				FSMNode->SetNodeClass(USMStateMachineTestInstance::StaticClass());
				FSMNode->GetNodeTemplateAs<USMStateMachineTestInstance>(true)->ExposedInt = TestInt;
			}

			FSMNode->ConvertToCurrentVersion(true, true);
			TestFalse("Default value not imported since it's not load", FSMNode->GetNodeTemplateAs<USMStateMachineTestInstance>(true)->bDisableTickTransitionEvaluation);
			TestFalse("Default value not imported since it's not load", FSMNode->GetNodeTemplateAs<USMStateMachineTestInstance>(true)->bEvalTransitionsOnStart);
			TestFalse("Default value not imported since it's not load", FSMNode->GetNodeTemplateAs<USMStateMachineTestInstance>(true)->bExcludeFromAnyState);
			TestFalse("Default value not imported since it's not load", FSMNode->GetNodeTemplateAs<USMStateMachineTestInstance>(true)->bAlwaysUpdate);

			TestFalse("Default value not imported since it's not load", FSMNode->GetNodeTemplateAs<USMStateMachineTestInstance>(true)->bReuseIfNotEndState);
			TestFalse("Default value not imported since it's not load", FSMNode->GetNodeTemplateAs<USMStateMachineTestInstance>(true)->bReuseCurrentState);
			
			TestEqual("Edited value maintained", FSMNode->GetNodeTemplateAs<USMStateMachineTestInstance>(true)->ExposedInt, TestInt);

			FSMNode->ConvertToCurrentVersion(false, true);
			TestNotNull("Template created.", FSMNode->GetNodeTemplate());

			TestTrue("Default value imported", FSMNode->GetNodeTemplateAs<USMStateMachineTestInstance>(true)->bDisableTickTransitionEvaluation);
			TestTrue("Default value imported", FSMNode->GetNodeTemplateAs<USMStateMachineTestInstance>(true)->bEvalTransitionsOnStart);
			TestTrue("Default value imported", FSMNode->GetNodeTemplateAs<USMStateMachineTestInstance>(true)->bExcludeFromAnyState);
			TestTrue("Default value imported", FSMNode->GetNodeTemplateAs<USMStateMachineTestInstance>(true)->bAlwaysUpdate);

			TestTrue("Default value imported", FSMNode->GetNodeTemplateAs<USMStateMachineInstance>(true)->bReuseIfNotEndState);
			TestTrue("Default value imported", FSMNode->GetNodeTemplateAs<USMStateMachineInstance>(true)->bReuseCurrentState);
			
			TestEqual("Edited value maintained", FSMNode->GetNodeTemplateAs<USMStateMachineTestInstance>(true)->ExposedInt, TestInt);

			// Test runtime with default values.
			{
				FKismetEditorUtilities::CompileBlueprint(NewBP);
				USMTestContext* Context = NewObject<USMTestContext>();
				USMInstance* StateMachineInstance = TestHelpers::CreateNewStateMachineInstanceFromBP(this, NewBP, Context);

				USMStateInstance_Base* StateInstance = CastChecked<USMStateInstance_Base>(StateMachineInstance->GetRootStateMachine().GetSingleInitialState()->GetNodeInstance());
				USMStateMachineTestInstance* FSMInstance = CastChecked<USMStateMachineTestInstance>(StateInstance->GetNextStateByTransitionIndex(0)->GetNextStateByTransitionIndex(0));

				// Default class templates don't get compiled into the CDO, so the values should still be default in runtime.
				TestTrue("Default value imported to runtime", FSMInstance->bDisableTickTransitionEvaluation);
				TestTrue("Default value imported to runtime", FSMInstance->bEvalTransitionsOnStart);
				TestTrue("Default value imported to runtime", FSMInstance->bExcludeFromAnyState);
				TestTrue("Default value imported to runtime", FSMInstance->bAlwaysUpdate);

				TestTrue("Default value imported to runtime", FSMInstance->bReuseIfNotEndState);
				TestTrue("Default value imported to runtime", FSMInstance->bReuseCurrentState);

				TestEqual("Edited value maintained", FSMInstance->ExposedInt, TestInt);
			}
		}
	}
	
	return true;
}

/**
 * Disable tick on a state and manually evaluate from the state instance.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStateManualTransitionTest, "SMTests.StateManualTransition", EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FStateManualTransitionTest::RunTest(const FString& Parameters)
{
	FAssetHandler NewAsset;
	if (!TestHelpers::TryCreateNewStateMachineAsset(this, NewAsset, false))
	{
		return false;
	}

	USMBlueprint* NewBP = NewAsset.GetObjectAs<USMBlueprint>();

	// Find root state machine.
	USMGraphK2Node_StateMachineNode* RootStateMachineNode = FSMBlueprintEditorUtils::GetRootStateMachineNode(NewBP);

	// Find the state machine graph.
	USMGraph* StateMachineGraph = RootStateMachineNode->GetStateMachineGraph();

	// Total states to test.
	const int32 TotalStates = 2;

	UEdGraphPin* LastStatePin = nullptr;

	TestHelpers::BuildLinearStateMachine(this, StateMachineGraph, TotalStates, &LastStatePin);

	USMGraphNode_StateNode* FirstStateNode = CastChecked<USMGraphNode_StateNode>(StateMachineGraph->GetEntryNode()->GetOutputNode());
	FirstStateNode->GetNodeTemplateAs<USMStateInstance_Base>()->bDisableTickTransitionEvaluation = true;
	
	int32 EntryHits = 0; int32 UpdateHits = 0; int32 EndHits = 0;
	const int32 MaxIterations = 3;
	USMInstance* Instance = TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits, MaxIterations, false, false);

	TestTrue("State machine still active", Instance->IsActive());
	TestTrue("State machine shouldn't have been able to switch states.", !Instance->IsInEndState());

	TestEqual("State Machine generated value", EntryHits, 1);
	TestEqual("State Machine generated value", UpdateHits, MaxIterations);
	TestEqual("State Machine generated value", EndHits, 0);
	
	USMStateInstance* StateInstance = CastChecked<USMStateInstance>(Instance->GetSingleActiveState()->GetNodeInstance());
	StateInstance->EvaluateTransitions();
	
	TestTrue("State machine should have now switched states.", Instance->IsInEndState());
	TestTrue("State machine should still be active.", Instance->IsActive());

	USMTestContext* Context = CastChecked<USMTestContext>(StateInstance->GetContext());
	TestEqual("Update should NOT have been called from manual transition evaluation.", Context->GetUpdateInt(), MaxIterations);
	
	return true;
}

/**
 * Select a node class and test making sure instance nodes are set and hit properly.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSelectNodeClassTest, "SMTests.NodeClassEvalVariable", EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FSelectNodeClassTest::RunTest(const FString& Parameters)
{
	FAssetHandler NewAsset;
	if (!TestHelpers::TryCreateNewStateMachineAsset(this, NewAsset, false))
	{
		return false;
	}

	USMBlueprint* NewBP = NewAsset.GetObjectAs<USMBlueprint>();

	// Find root state machine.
	USMGraphK2Node_StateMachineNode* RootStateMachineNode = FSMBlueprintEditorUtils::GetRootStateMachineNode(NewBP);

	// Find the state machine graph.
	USMGraph* StateMachineGraph = RootStateMachineNode->GetStateMachineGraph();

	// Total states to test.
	int32 TotalStates = 1;

	UEdGraphPin* LastStatePin = nullptr;

	// Build single state - state machine.
	TestHelpers::BuildLinearStateMachine(this, StateMachineGraph, TotalStates, &LastStatePin, USMStateTestInstance::StaticClass(), USMTransitionTestInstance::StaticClass());
	if (!NewAsset.SaveAsset(this))
	{
		return false;
	}
	TestHelpers::TestLinearStateMachine(this, NewBP, TotalStates);

	////////////////////////
	// Test setting default value.
	////////////////////////
	const int32 TestDefaultInt = 12;
	
	USMGraphNode_StateNode* StateNode = CastChecked<USMGraphNode_StateNode>(StateMachineGraph->GetEntryNode()->GetOutputNode());
	auto PropertyNodes = StateNode->GetAllPropertyGraphNodesAsArray();
	PropertyNodes[0]->GetSchema()->TrySetDefaultValue(*PropertyNodes[0]->GetResultPinChecked(), FString::FromInt(TestDefaultInt)); // TrySet needed to trigger DefaultValueChanged

	USMInstance* Instance = TestHelpers::TestLinearStateMachine(this, NewBP, TotalStates, false);

	USMStateTestInstance* NodeInstance = CastChecked<USMStateTestInstance>(Instance->GetRootStateMachine().GetSingleInitialState()->GetNodeInstance());
	TestEqual("Default exposed value set and evaluated", NodeInstance->ExposedInt, TestDefaultInt + 1); // Default gets added to in the context.

	// Check manual evaluation. Alter the template directly rather than the class even though this isn't normally allowed.
	USMStateInstance* StateInstanceTemplate = CastChecked<USMStateInstance>(StateNode->GetNodeTemplate());
	// This will reset the begin evaluation.
	StateInstanceTemplate->bEvalGraphsOnUpdate = true;

	Instance = TestHelpers::TestLinearStateMachine(this, NewBP, TotalStates, false);
	Instance->Update(0.f); // One more update to trigger Update eval.
	NodeInstance = CastChecked<USMStateTestInstance>(Instance->GetRootStateMachine().GetSingleInitialState()->GetNodeInstance());
	TestEqual("Default exposed value set and evaluated", NodeInstance->ExposedInt, TestDefaultInt); // Verify the value matches the default.
	// 
	////////////////////////
	// Test graph evaluation -- needs to be done from a variable.
	////////////////////////
	FName VarName = "NewVar";
	FEdGraphPinType VarType;
	VarType.PinCategory = UEdGraphSchema_K2::PC_Int;

	// Create new variable.
	const int32 TestVarDefaultValue = 15;
	FBlueprintEditorUtils::AddMemberVariable(NewBP, VarName, VarType, FString::FromInt(TestVarDefaultValue));

	// Get class property from new variable.
	FProperty* NewProperty = FSMBlueprintEditorUtils::GetPropertyForVariable(NewBP, VarName);

	// Place variable getter and wire to result node.
	FSMBlueprintEditorUtils::PlacePropertyOnGraph(PropertyNodes[0]->GetGraph(), NewProperty, PropertyNodes[0]->GetResultPinChecked(), nullptr);

	Instance = TestHelpers::TestLinearStateMachine(this, NewBP, TotalStates);
	NodeInstance = CastChecked<USMStateTestInstance>(Instance->GetRootStateMachine().GetSingleInitialState()->GetNodeInstance());
	TestEqual("Default exposed value set and evaluated", NodeInstance->ExposedInt, TestVarDefaultValue); // Verify the value evaluated properly.

	// Turn update state eval off.
	StateInstanceTemplate->bEvalGraphsOnUpdate = false;
	Instance = TestHelpers::TestLinearStateMachine(this, NewBP, TotalStates);
	NodeInstance = CastChecked<USMStateTestInstance>(Instance->GetRootStateMachine().GetSingleInitialState()->GetNodeInstance());
	TestEqual("Default exposed value set and evaluated", NodeInstance->ExposedInt, TestVarDefaultValue + 1); // Verify the value evaluated properly and was modified.

	// Disable auto evaluation all together.
	StateInstanceTemplate->bAutoEvalExposedProperties = false;
	Instance = TestHelpers::TestLinearStateMachine(this, NewBP, TotalStates, false);

	NodeInstance = CastChecked<USMStateTestInstance>(Instance->GetRootStateMachine().GetSingleInitialState()->GetNodeInstance());
	TestNotEqual("Default exposed value not evaluated", NodeInstance->ExposedInt, TestVarDefaultValue);

	// Manual evaluation.
	NodeInstance->EvaluateGraphProperties();
	TestEqual("Default exposed value set and evaluated", NodeInstance->ExposedInt, TestVarDefaultValue); // Verify the value evaluated properly.
	
	return NewAsset.DeleteAsset(this);
}

/**
 * Verify node instance struct wrapper methods work properly.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNodeInstanceMethodsTest, "SMTests.NodeInstanceMethods", EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FNodeInstanceMethodsTest::RunTest(const FString& Parameters)
{
	FAssetHandler NewAsset;
	if (!TestHelpers::TryCreateNewStateMachineAsset(this, NewAsset, false))
	{
		return false;
	}

	USMBlueprint* NewBP = NewAsset.GetObjectAs<USMBlueprint>();

	// Find root state machine.
	USMGraphK2Node_StateMachineNode* RootStateMachineNode = FSMBlueprintEditorUtils::GetRootStateMachineNode(NewBP);

	// Find the state machine graph.
	USMGraph* StateMachineGraph = RootStateMachineNode->GetStateMachineGraph();

	// Total states to test.
	int32 TotalStates = 2;

	{
		UEdGraphPin* LastStatePin = nullptr;
		//Verify default instances load correctly.
		TestHelpers::BuildLinearStateMachine(this, StateMachineGraph, TotalStates, &LastStatePin, USMStateInstance::StaticClass(), USMTransitionInstance::StaticClass());
		int32 A, B, C;
		TestHelpers::RunStateMachineToCompletion(this, NewBP, A, B, C);
		FSMBlueprintEditorUtils::RemoveAllNodesFromGraph(StateMachineGraph);
	}
	
	// Load test instances.
	UEdGraphPin* LastStatePin = nullptr;
	TestHelpers::BuildLinearStateMachine(this, StateMachineGraph, TotalStates, &LastStatePin, USMStateTestInstance::StaticClass(), USMTransitionTestInstance::StaticClass());
	FKismetEditorUtilities::CompileBlueprint(NewBP);
	
	USMTestContext* Context = NewObject<USMTestContext>();
	USMInstance* StateMachineInstance = TestHelpers::CreateNewStateMachineInstanceFromBP(this, NewBP, Context);
	
	FSMState_Base* InitialState = StateMachineInstance->GetRootStateMachine().GetSingleInitialState();
	USMStateInstance_Base* NodeInstance = CastChecked<USMStateInstance_Base>(InitialState->GetNodeInstance());
	InitialState->bAlwaysUpdate = true; // Needed since we are manually switching states later.
	
	TestEqual("Correct state machine", NodeInstance->GetStateMachineInstance(), StateMachineInstance);
	TestEqual("Guids correct", NodeInstance->GetGuid(), InitialState->GetGuid());
	TestEqual("Name correct", NodeInstance->GetNodeName(), InitialState->GetNodeName());
	
	TestFalse("Initial state not active", NodeInstance->IsActive());
	StateMachineInstance->Start();
	TestTrue("Initial state active", NodeInstance->IsActive());

	InitialState->TimeInState = 3;
	TestEqual("Time correct", NodeInstance->GetTimeInState(), InitialState->TimeInState);

	FSMStateInfo StateInfo;
	NodeInstance->GetStateInfo(StateInfo);

	TestEqual("State info guids correct", StateInfo.Guid, InitialState->GetGuid());
	TestEqual("State info instance correct", StateInfo.NodeInstance, Cast<USMNodeInstance>(NodeInstance));
	TestFalse("Not a state machine", NodeInstance->IsStateMachine());
	TestFalse("Not in end state", NodeInstance->IsInEndState());
	TestFalse("Has not updated", NodeInstance->HasUpdated());
	TestNull("No transition to take", NodeInstance->GetTransitionToTake());

	USMStateInstance_Base* NextState = CastChecked<USMStateInstance_Base>(InitialState->GetOutgoingTransitions()[0]->GetToState()->GetNodeInstance());

	// Test searching nodes.
	TArray<USMNodeInstance*> FoundNodes;
	NodeInstance->GetAllNodesOfType(FoundNodes, USMStateInstance::StaticClass());

	TestEqual("All nodes found", FoundNodes.Num(), TotalStates);
	TestEqual("Correct state found", FoundNodes[0], Cast<USMNodeInstance>(NodeInstance));
	TestEqual("Correct state found", FoundNodes[1], Cast<USMNodeInstance>(NextState));

	// Verify state machine instance methods to retrieve node instances are correct.
	TArray<USMStateInstance_Base*> StateInstances;
	StateMachineInstance->GetAllStateInstances(StateInstances);
	TestEqual("All states found", StateInstances.Num(), StateMachineInstance->GetStateMap().Num());
	
	TArray<USMTransitionInstance*> TransitionInstances;
	StateMachineInstance->GetAllTransitionInstances(TransitionInstances);
	TestEqual("All transitions found", TransitionInstances.Num(), StateMachineInstance->GetTransitionMap().Num());
	
	// Test transition instance.
	USMTransitionInstance* NextTransition = CastChecked<USMTransitionInstance>(InitialState->GetOutgoingTransitions()[0]->GetNodeInstance());
	{
		TArray<USMTransitionInstance*> Transitions;
		NodeInstance->GetOutgoingTransitions(Transitions);

		TestEqual("One outgoing transition", Transitions.Num(), 1);
		USMTransitionInstance* TransitionInstance = Transitions[0];

		TestEqual("Transition instance correct", TransitionInstance, NextTransition);
		
		FSMTransitionInfo TransitionInfo;
		TransitionInstance->GetTransitionInfo(TransitionInfo);

		TestEqual("Transition info instance correct", TransitionInfo.NodeInstance, Cast<USMNodeInstance>(NextTransition));

		TestEqual("Prev state correct", TransitionInstance->GetPreviousStateInstance(), NodeInstance);
		TestEqual("Next state correct", TransitionInstance->GetNextStateInstance(), NextState);
	}
	
	NodeInstance->SwitchToLinkedState(NextState);

	TestFalse("State no longer active", NodeInstance->IsActive());
	TestTrue("Node has updated from bAlwaysUpdate", NodeInstance->HasUpdated());
	TestEqual("Transition to take set", NodeInstance->GetTransitionToTake(), NextTransition);
	
	USMTransitionInstance* PreviousTransition = CastChecked<USMTransitionInstance>(((FSMState_Base*)NextState->GetOwningNode())->GetIncomingTransitions()[0]->GetNodeInstance());
	{
		TestEqual("Previous transition is correct instance", PreviousTransition, NextTransition);
		
		TArray<USMTransitionInstance*> Transitions;
		NextState->GetIncomingTransitions(Transitions);

		TestEqual("One incoming transition", Transitions.Num(), 1);
		USMTransitionInstance* TransitionInstance = Transitions[0];

		TestEqual("Transition instance correct", TransitionInstance, PreviousTransition);

		FSMTransitionInfo TransitionInfo;
		TransitionInstance->GetTransitionInfo(TransitionInfo);

		TestEqual("Transition info instance correct", TransitionInfo.NodeInstance, Cast<USMNodeInstance>(PreviousTransition));

		TestEqual("Prev state correct", TransitionInstance->GetPreviousStateInstance(), NodeInstance);
		TestEqual("Next state correct", TransitionInstance->GetNextStateInstance(), NextState);
	}

	NodeInstance = CastChecked<USMStateInstance_Base>(StateMachineInstance->GetSingleActiveState()->GetNodeInstance());
	TestTrue("Is end state", NodeInstance->IsInEndState());

	//  Test nested reference FSM can retrieve transitions.
	{
		LastStatePin = nullptr;
		FSMBlueprintEditorUtils::RemoveAllNodesFromGraph(StateMachineGraph, NewBP);
		TestHelpers::BuildLinearStateMachine(this, StateMachineGraph, TotalStates, &LastStatePin);

		USMGraphNode_StateMachineStateNode* NestedFSM = FSMBlueprintEditorUtils::ConvertNodeTo<USMGraphNode_StateMachineStateNode>(CastChecked<USMGraphNode_StateNodeBase>(StateMachineGraph->EntryNode->GetOutputNode()));
		FKismetEditorUtilities::CompileBlueprint(NewBP);

		USMBlueprint* NewReferencedBlueprint = FSMBlueprintEditorUtils::ConvertStateMachineToReference(NestedFSM, false, nullptr, nullptr);

		Context = NewObject<USMTestContext>();
		StateMachineInstance = TestHelpers::CreateNewStateMachineInstanceFromBP(this, NewBP, Context);
		USMStateMachineInstance* FSMClass = CastChecked< USMStateMachineInstance>(StateMachineInstance->GetRootStateMachine().GetSingleInitialState()->GetNodeInstance());

		TArray<USMTransitionInstance*> Transitions;
		FSMClass->GetOutgoingTransitions(Transitions);
		TestEqual("Outgoing transitions found of reference FSM", Transitions.Num(), 1);
	}
	
	return true;
}

/**
 * Test nested state machines with a state machine class set evaluate graphs properly.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStateMachineClassInstanceTest, "SMTests.StateMachineClassInstance", EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

	bool FStateMachineClassInstanceTest::RunTest(const FString& Parameters)
{
	FAssetHandler NewAsset;
	if (!TestHelpers::TryCreateNewStateMachineAsset(this, NewAsset, false))
	{
		return false;
	}

	USMBlueprint* NewBP = NewAsset.GetObjectAs<USMBlueprint>();

	// Find root state machine.
	USMGraphK2Node_StateMachineNode* RootStateMachineNode = FSMBlueprintEditorUtils::GetRootStateMachineNode(NewBP);

	// Find the state machine graph.
	USMGraph* StateMachineGraph = RootStateMachineNode->GetStateMachineGraph();

	// Total states to test.
	int32 TotalStates = 2;

	UEdGraphPin* LastStatePin = nullptr;

	// Build state machine.
	TestHelpers::BuildLinearStateMachine(this, StateMachineGraph, TotalStates, &LastStatePin, USMStateTestInstance::StaticClass(), USMTransitionTestInstance::StaticClass());

	USMGraphNode_StateMachineStateNode* NestedFSMNode = FSMBlueprintEditorUtils::ConvertNodeTo<USMGraphNode_StateMachineStateNode>(CastChecked<USMGraphNode_Base>(StateMachineGraph->GetEntryNode()->GetOutputNode()));
	USMGraphNode_StateMachineStateNode* NestedFSMNode2 = FSMBlueprintEditorUtils::ConvertNodeTo<USMGraphNode_StateMachineStateNode>(NestedFSMNode->GetNextNode());

	TestHelpers::SetNodeClass(this, NestedFSMNode, USMStateMachineTestInstance::StaticClass());
	TestHelpers::SetNodeClass(this, NestedFSMNode2, USMStateMachineTestInstance::StaticClass());

	/*
	 * This part tests evaluating exposed variable blueprint graphs. There was a bug
	 * when more than one FSM was present that the graphs wouldn't evaluate properly, but default values would.
	 */
	
	// Create and wire a new variable to the first fsm.
	const int32 TestVarDefaultValue = 2;
	{
		FName VarName = "NewVar";
		FEdGraphPinType VarType;
		VarType.PinCategory = UEdGraphSchema_K2::PC_Int;

		// Create new variable.
		FBlueprintEditorUtils::AddMemberVariable(NewBP, VarName, VarType, FString::FromInt(TestVarDefaultValue));
		FProperty* NewProperty = FSMBlueprintEditorUtils::GetPropertyForVariable(NewBP, VarName);

		auto PropertyNode = NestedFSMNode->GetGraphPropertyNode("ExposedInt");

		// Place variable getter and wire to result node.
		TestTrue("Variable placed on graph", FSMBlueprintEditorUtils::PlacePropertyOnGraph(PropertyNode->GetGraph(), NewProperty, PropertyNode->GetResultPinChecked(), nullptr));
	}
	
	// Create and wire a second variable to the first fsm.
	const int32 TestVarDefaultValue2 = 4;
	{
		FName VarName = "NewVar2";
		FEdGraphPinType VarType;
		VarType.PinCategory = UEdGraphSchema_K2::PC_Int;

		// Create new variable.
		FBlueprintEditorUtils::AddMemberVariable(NewBP, VarName, VarType, FString::FromInt(TestVarDefaultValue2));
		FProperty* NewProperty = FSMBlueprintEditorUtils::GetPropertyForVariable(NewBP, VarName);

		auto PropertyNode = NestedFSMNode2->GetGraphPropertyNode("ExposedInt");

		// Place variable getter and wire to result node.
		TestTrue("Variable placed on graph", FSMBlueprintEditorUtils::PlacePropertyOnGraph(PropertyNode->GetGraph(), NewProperty, PropertyNode->GetResultPinChecked(), nullptr));
	}

	FKismetEditorUtilities::CompileBlueprint(NewBP);
	
	USMTestContext* Context = NewObject<USMTestContext>();
	USMInstance* StateMachineInstance = TestHelpers::CreateNewStateMachineInstanceFromBP(this, NewBP, Context);

	FSMState_Base* InitialState = StateMachineInstance->GetRootStateMachine().GetSingleInitialState();
	USMStateMachineTestInstance* NodeInstance = CastChecked<USMStateMachineTestInstance>(InitialState->GetNodeInstance());
	InitialState->bAlwaysUpdate = true; // Needed since we are manually switching states later.

	TestEqual("Correct state machine", NodeInstance->GetStateMachineInstance(), StateMachineInstance);
	TestEqual("Guids correct", NodeInstance->GetGuid(), InitialState->GetGuid());
	TestEqual("Name correct", NodeInstance->GetNodeName(), InitialState->GetNodeName());
	
	TestFalse("Initial state not active", NodeInstance->IsActive());
	
	TestEqual("Exposed var not set", NodeInstance->ExposedInt, 0);
	StateMachineInstance->Start();
	TestEqual("Exposed var set", NodeInstance->ExposedInt, TestVarDefaultValue);
	
	TestTrue("Initial state active", NodeInstance->IsActive());
	InitialState->TimeInState = 3;
	TestEqual("Time correct", NodeInstance->GetTimeInState(), InitialState->TimeInState);
	
	FSMStateInfo StateInfo;
	NodeInstance->GetStateInfo(StateInfo);

	TestEqual("State info guids correct", StateInfo.Guid, InitialState->GetGuid());
	TestEqual("State info instance correct", StateInfo.NodeInstance, Cast<USMNodeInstance>(NodeInstance));
	TestTrue("Is a state machine", NodeInstance->IsStateMachine());
	TestFalse("Has not updated", NodeInstance->HasUpdated());
	TestNull("No transition to take", NodeInstance->GetTransitionToTake());

	USMStateMachineTestInstance* NextState = CastChecked<USMStateMachineTestInstance>(InitialState->GetOutgoingTransitions()[0]->GetToState()->GetNodeInstance());

	// Test transition instance.
	USMTransitionInstance* NextTransition = CastChecked<USMTransitionInstance>(InitialState->GetOutgoingTransitions()[0]->GetNodeInstance());
	{
		TArray<USMTransitionInstance*> Transitions;
		NodeInstance->GetOutgoingTransitions(Transitions);

		TestEqual("One outgoing transition", Transitions.Num(), 1);
		USMTransitionInstance* TransitionInstance = Transitions[0];

		TestEqual("Transition instance correct", TransitionInstance, NextTransition);

		FSMTransitionInfo TransitionInfo;
		TransitionInstance->GetTransitionInfo(TransitionInfo);

		TestEqual("Transition info instance correct", TransitionInfo.NodeInstance, Cast<USMNodeInstance>(NextTransition));

		TestEqual("Prev state correct", Cast<USMStateMachineTestInstance>(TransitionInstance->GetPreviousStateInstance()), NodeInstance);
		TestEqual("Next state correct", Cast<USMStateMachineTestInstance>(TransitionInstance->GetNextStateInstance()), NextState);
	}
	
	TestEqual("Exposed var not set", NextState->ExposedInt, 0);
	StateMachineInstance->Update(0.f);
	TestEqual("Exposed var set", NextState->ExposedInt, TestVarDefaultValue2);

	TestFalse("State no longer active", NodeInstance->IsActive());
	TestTrue("Node has updated from bAlwaysUpdate", NodeInstance->HasUpdated());
	TestEqual("Transition to take set", NodeInstance->GetTransitionToTake(), NextTransition);

	TestEqual("State begin hit", NodeInstance->StateBeginHit.Count, 1);
	TestEqual("State update not hit", NodeInstance->StateUpdateHit.Count, 1);
	TestEqual("State end not hit", NodeInstance->StateEndHit.Count, 1);
	
	NodeInstance = CastChecked<USMStateMachineTestInstance>(StateMachineInstance->GetSingleActiveState()->GetNodeInstance());
	TestTrue("Is end state", NodeInstance->IsInEndState());

	TestEqual("State begin hit", NodeInstance->StateBeginHit.Count, 1);
	TestEqual("State update not hit", NodeInstance->StateUpdateHit.Count, 0);
	TestEqual("State end not hit", NodeInstance->StateEndHit.Count, 0);
	
	return true;
}

/**
 * Test nested state machines' bWaitForEndState flag.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaitForEndStateTest, "SMTests.WaitForEndState", EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

	bool FWaitForEndStateTest::RunTest(const FString& Parameters)
{
	FAssetHandler NewAsset;
	if (!TestHelpers::TryCreateNewStateMachineAsset(this, NewAsset, false))
	{
		return false;
	}

	USMBlueprint* NewBP = NewAsset.GetObjectAs<USMBlueprint>();

	// Find root state machine.
	USMGraphK2Node_StateMachineNode* RootStateMachineNode = FSMBlueprintEditorUtils::GetRootStateMachineNode(NewBP);

	// Find the state machine graph.
	USMGraph* StateMachineGraph = RootStateMachineNode->GetStateMachineGraph();

	// Total states to test.
	int32 TotalTopLevelStates = 3;
	int32 TotalNestedStates = 2;
	
	UEdGraphPin* LastStatePin = nullptr;

	// Build state machine first state.
	TestHelpers::BuildLinearStateMachine(this, StateMachineGraph, 1, &LastStatePin);

	// Connect nested FSM.
	UEdGraphPin* EntryPointForNestedStateMachine = LastStatePin;
	USMGraphNode_StateMachineStateNode* NestedFSM = TestHelpers::BuildNestedStateMachine(this, StateMachineGraph, TotalNestedStates, &EntryPointForNestedStateMachine, nullptr);
	LastStatePin = NestedFSM->GetOutputPin();
	
	NestedFSM->GetNodeTemplateAs<USMStateMachineInstance>()->bWaitForEndState = false;

	// Third state regular state.
	TestHelpers::BuildLinearStateMachine(this, StateMachineGraph, 1, &LastStatePin);

	// Test transition evaluation waiting for end state.
	// [A -> [A -> B] -> C
	{
		int32 EntryHits = 0; int32 UpdateHits = 0; int32 EndHits = 0;
		TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits);
		TestEqual("Didn't wait for end state.", EntryHits, TotalTopLevelStates);
		TestEqual("Didn't wait for end state.", EndHits, TotalTopLevelStates);

		NestedFSM->GetNodeTemplateAs<USMStateMachineInstance>()->bWaitForEndState = true;

		TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits);
		TestEqual("Waited for end state.", EntryHits, TotalTopLevelStates + TotalNestedStates - 1);
		TestEqual("Waited for end state.", EndHits, TotalTopLevelStates + TotalNestedStates - 1);
	}

	USMGraphNode_StateMachineStateNode* EndFSM = FSMBlueprintEditorUtils::ConvertNodeTo<USMGraphNode_StateMachineStateNode>(NestedFSM->GetNextNode());
	TestHelpers::BuildLinearStateMachine(this, CastChecked<USMGraph>(EndFSM->GetBoundGraph()), TotalNestedStates, nullptr);

	TotalNestedStates *= 2;
	
	// Test root end state not being considered until fsm is in end state.
	// [A -> [A -> B] -> [A -> B]
	{
		int32 EntryHits = 0; int32 UpdateHits = 0; int32 EndHits = 0;
		// Will hit all states of first FSM, then stop on first state of second fsm.
		TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits);
		TestEqual("Didn't wait for end state.", EntryHits, 4); // [A -> [A -> B] -> [A]
		TestEqual("Didn't wait for end state.", EndHits, 4);

		EndFSM->GetNodeTemplateAs<USMStateMachineInstance>()->bWaitForEndState = true;

		// Will hit all states of all FSMs. This test doesn't stop until the root state machine is in an end state.
		TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits);
		TestEqual("Waited for end state.", EntryHits, TotalTopLevelStates + TotalNestedStates - 2); // [A -> [A -> B] -> [A -> B]
		TestEqual("Waited for end state.", EndHits, TotalTopLevelStates + TotalNestedStates - 2);
	}
	
	return true;
}

/**
 * Test conduit functionality.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConduitTest, "SMTests.Conduit", EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

	bool FConduitTest::RunTest(const FString& Parameters)
{
	FAssetHandler NewAsset;
	if (!TestHelpers::TryCreateNewStateMachineAsset(this, NewAsset, false))
	{
		return false;
	}

	USMBlueprint* NewBP = NewAsset.GetObjectAs<USMBlueprint>();

	// Find root state machine.
	USMGraphK2Node_StateMachineNode* RootStateMachineNode = FSMBlueprintEditorUtils::GetRootStateMachineNode(NewBP);

	// Find the state machine graph.
	USMGraph* StateMachineGraph = RootStateMachineNode->GetStateMachineGraph();

	// Total states to test.
	const int32 TotalStates = 5;

	UEdGraphPin* LastStatePin = nullptr;

	TestHelpers::BuildLinearStateMachine(this, StateMachineGraph, TotalStates, &LastStatePin);

	USMGraphNode_StateNodeBase* FirstNode = CastChecked<USMGraphNode_StateNodeBase>(StateMachineGraph->GetEntryNode()->GetOutputNode());

	// This will become a conduit.
	USMGraphNode_StateNodeBase* SecondNode = CastChecked<USMGraphNode_StateNodeBase>(FirstNode->GetNextNode());
	USMGraphNode_ConduitNode* ConduitNode = FSMBlueprintEditorUtils::ConvertNodeTo<USMGraphNode_ConduitNode>(SecondNode);
	ConduitNode->GetNodeTemplateAs<USMConduitInstance>()->bEvalWithTransitions = false; // Settings make this true by default.
	
	// Eval with the conduit being considered a state. It will end with the active state becoming stuck on a conduit.
	int32 EntryHits = 0; int32 UpdateHits = 0; int32 EndHits = 0;
	int32 MaxIterations = TotalStates * 2;
	USMInstance* Instance = TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits, MaxIterations, false, false);

	TestTrue("State machine still active", Instance->IsActive());
	TestTrue("State machine shouldn't have been able to switch states.", !Instance->IsInEndState());

	TestTrue("Active state is conduit", Instance->GetSingleActiveState()->IsConduit());
	TestEqual("State Machine generated value", EntryHits, 1);
	TestEqual("State Machine generated value", UpdateHits, 0);
	TestEqual("State Machine generated value", EndHits, 1); // Ended state and switched to conduit.
	
	// Set conduit to true and try again.
	USMConduitGraph* Graph = CastChecked<USMConduitGraph>(ConduitNode->GetBoundGraph());
	UEdGraphPin* CanEvalPin = Graph->ResultNode->GetInputPin();
	CanEvalPin->DefaultValue = "True";

	// Eval normally.
	TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits, MaxIterations, true, true);

	// Configure conduit as transition and set to false.
	ConduitNode->GetNodeTemplateAs<USMConduitInstance>()->bEvalWithTransitions = true;
	CanEvalPin->DefaultValue = "False";
	Instance = TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits, MaxIterations, false, false);

	TestTrue("State machine still active", Instance->IsActive());
	TestTrue("State machine shouldn't have been able to switch states.", !Instance->IsInEndState());

	TestFalse("Active state is not conduit", Instance->GetSingleActiveState()->IsConduit());
	TestEqual("State Machine generated value", EntryHits, 1);
	TestEqual("State Machine generated value", UpdateHits, MaxIterations); // Updates because state not transitioning out.
	TestEqual("State Machine generated value", EndHits, 0); // State should never have ended.

	// Set conduit to true but set the next transition to false. Should have same result as when the conduit was false.
	CanEvalPin->DefaultValue = "True";
	USMGraphNode_TransitionEdge* Transition = CastChecked<USMGraphNode_TransitionEdge>(ConduitNode->GetOutputNode());
	USMTransitionGraph* TransitionGraph = CastChecked<USMTransitionGraph>(Transition->GetBoundGraph());
	UEdGraphPin* TransitionPin = TransitionGraph->ResultNode->GetInputPin();
	TransitionPin->BreakAllPinLinks(true);
	TransitionPin->DefaultValue = "False";
	Instance = TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits, MaxIterations, false, false);

	TestTrue("State machine still active", Instance->IsActive());
	TestTrue("State machine shouldn't have been able to switch states.", !Instance->IsInEndState());

	TestFalse("Active state is not conduit", Instance->GetSingleActiveState()->IsConduit());
	TestEqual("State Machine generated value", EntryHits, 1);
	TestEqual("State Machine generated value", UpdateHits, MaxIterations); // Updates because state not transitioning out.
	TestEqual("State Machine generated value", EndHits, 0); // State should never have ended.

	// Set transition to true and should eval normally.
	TransitionPin->DefaultValue = "True";
	TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits, MaxIterations, true, true);

	// Add another conduit node (false) after the last one configured to run as a transition. Result should be the same as the last failure.
	USMGraphNode_StateNodeBase* ThirdNode = CastChecked<USMGraphNode_StateNodeBase>(ConduitNode->GetNextNode());
	USMGraphNode_ConduitNode* NextConduitNode = FSMBlueprintEditorUtils::ConvertNodeTo<USMGraphNode_ConduitNode>(ThirdNode);
	NextConduitNode->GetNodeTemplateAs<USMConduitInstance>()->bEvalWithTransitions = true;
	Instance = TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits, MaxIterations, false, false);

	TestTrue("State machine still active", Instance->IsActive());
	TestTrue("State machine shouldn't have been able to switch states.", !Instance->IsInEndState());

	TestFalse("Active state is not conduit", Instance->GetSingleActiveState()->IsConduit());
	TestEqual("State Machine generated value", EntryHits, 1);
	TestEqual("State Machine generated value", UpdateHits, MaxIterations); // Updates because state not transitioning out.
	TestEqual("State Machine generated value", EndHits, 0); // State should never have ended.

	// Set new conduit to true and eval again.
	Graph = CastChecked<USMConduitGraph>(NextConduitNode->GetBoundGraph());
	CanEvalPin = Graph->ResultNode->GetInputPin();
	CanEvalPin->DefaultValue = "True";
	TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits, MaxIterations, true, true);

	// Test correct transition order.
	USMTestContext* Context = NewObject<USMTestContext>();
	Instance = TestHelpers::CreateNewStateMachineInstanceFromBP(this, NewBP, Context);
	Instance->Start();

	FSMState_Base* CurrentState = Instance->GetRootStateMachine().GetSingleActiveState();
	TArray<TArray<FSMTransition*>> TransitionChain;
	TestTrue("Valid transition found", CurrentState->GetValidTransition(TransitionChain));

	FSMState_Base* SecondState = CurrentState->GetOutgoingTransitions()[0]->GetToState();
	FSMState_Base* ThirdState = SecondState->GetOutgoingTransitions()[0]->GetToState();
	
	TestEqual("Transition to and after conduit found", TransitionChain[0].Num(), 3);
	TestEqual("Correct transition order", TransitionChain[0][0], CurrentState->GetOutgoingTransitions()[0]);
	TestEqual("Correct transition order", TransitionChain[0][1], SecondState->GetOutgoingTransitions()[0]);
	TestEqual("Correct transition order", TransitionChain[0][2], ThirdState->GetOutgoingTransitions()[0]);

	// Test conduit initialize & shutdown
	TestHelpers::AddEventWithLogic<USMGraphK2Node_TransitionInitializedNode>(this, ConduitNode,
		USMTestContext::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(USMTestContext, IncreaseTransitionInit)));

	TestHelpers::AddEventWithLogic<USMGraphK2Node_TransitionInitializedNode>(this, NextConduitNode,
		USMTestContext::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(USMTestContext, IncreaseTransitionInit)));

	TestHelpers::AddEventWithLogic<USMGraphK2Node_TransitionShutdownNode>(this, ConduitNode,
		USMTestContext::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(USMTestContext, IncreaseTransitionShutdown)));
	
	TestHelpers::AddEventWithLogic<USMGraphK2Node_TransitionShutdownNode>(this, NextConduitNode,
		USMTestContext::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(USMTestContext, IncreaseTransitionShutdown)));

	TestHelpers::AddEventWithLogic<USMGraphK2Node_TransitionEnteredNode>(this, ConduitNode,
		USMTestContext::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(USMTestContext, IncreaseTransitionTaken)));
	
	TestHelpers::AddEventWithLogic<USMGraphK2Node_TransitionEnteredNode>(this, NextConduitNode,
		USMTestContext::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(USMTestContext, IncreaseTransitionTaken)));
	
	FKismetEditorUtilities::CompileBlueprint(NewBP);
	
	Context = NewObject<USMTestContext>();
	Instance = TestHelpers::CreateNewStateMachineInstanceFromBP(this, NewBP, Context);
	Instance->Start();

	// All transition inits should be fired at once.
	TestEqual("InitValue", Context->TestTransitionInit.Count, 2);
	TestEqual("ShutdownValue", Context->TestTransitionShutdown.Count, 0);
	TestEqual("EnteredValue", Context->TestTransitionEntered.Count, 0);
	
	Instance = TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits, MaxIterations, true, false);
	Context = CastChecked<USMTestContext>(Instance->GetContext());
	
	TestEqual("InitValue", Context->TestTransitionInit.Count, 2);
	TestEqual("ShutdownValue", Context->TestTransitionShutdown.Count, 2);
	TestEqual("EnteredValue", Context->TestTransitionEntered.Count, 2);

	// Test having the second conduit go back to the first conduit. When both are set as transitions this caused a stack overflow. Check it's fixed.
	NextConduitNode->GetOutputPin()->BreakAllPinLinks(true);
	TestTrue("Next conduit wired to previous conduit", NextConduitNode->GetSchema()->TryCreateConnection(NextConduitNode->GetOutputPin(), ConduitNode->GetInputPin()));
	USMGraphNode_TransitionEdge* TransitionEdge = CastChecked<USMGraphNode_TransitionEdge>(NextConduitNode->GetOutputNode());
	TestHelpers::AddTransitionResultLogic(this, TransitionEdge);
	TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits, MaxIterations, true, false);
	
	return true;
}

/**
 * Test automatically binding to a multi-cast delegate.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTransitionEventAutoBindTest, "SMTests.TransitionEventsAutoBind", EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FTransitionEventAutoBindTest::RunTest(const FString& Parameters)
{
	FAssetHandler NewAsset;
	if (!TestHelpers::TryCreateNewStateMachineAsset(this, NewAsset, false))
	{
		return false;
	}

	USMBlueprint* NewBP = NewAsset.GetObjectAs<USMBlueprint>();

	// Find root state machine.
	USMGraphK2Node_StateMachineNode* RootStateMachineNode = FSMBlueprintEditorUtils::GetRootStateMachineNode(NewBP);

	// Find the state machine graph.
	USMGraph* StateMachineGraph = RootStateMachineNode->GetStateMachineGraph();

	// Total states to test.
	UEdGraphPin* LastStatePin = nullptr;
	
	// Build a state machine of only two states.
	{
		const int32 CurrentStates = 2;
		TestHelpers::BuildLinearStateMachine(this, StateMachineGraph, CurrentStates, &LastStatePin);
	}

	USMGraphNode_TransitionEdge* TransitionEdge =
		CastChecked<USMGraphNode_TransitionEdge>(Cast<USMGraphNode_StateNode>(LastStatePin->GetOwningNode())->GetInputPin()->LinkedTo[0]->GetOwningNode());
	TransitionEdge->GetNodeTemplateAs<USMTransitionInstance>()->bCanEvaluate = false;
	
	// Validate transition can't evaluate.
	{
		int32 a, b, c;
		USMInstance* Instance = TestHelpers::RunStateMachineToCompletion(this, NewBP, a, b, c, 5, false, false);
		TestFalse("State machine shouldn't have switched states due to transition evaluation being false.", Instance->IsInEndState());
	}
	
	// Setup a transition event binding.
	{
		TransitionEdge->DelegateOwnerInstance = ESMDelegateOwner::SMDO_Context;
		TransitionEdge->DelegateOwnerClass = USMTestContext::StaticClass();
		TransitionEdge->DelegatePropertyName = GET_MEMBER_NAME_CHECKED(USMTestContext, TransitionEvent);

		TransitionEdge->InitTransitionDelegate();

		FKismetEditorUtilities::CompileBlueprint(NewBP);

		// Create a context we will run the state machine for.
		USMTestContext* Context = NewObject<USMTestContext>();
		USMInstance* Instance = TestHelpers::CreateNewStateMachineInstanceFromBP(this, NewBP, Context);

		Instance->Start();
		Instance->Update(0.f);
		TestEqual("State machine still in initial state", Instance->GetRootStateMachine().GetSingleActiveState(), Instance->GetRootStateMachine().GetSingleInitialState());
		TestFalse("State machine shouldn't have switched states due to transition evaluation being false.", Instance->IsInEndState());
		
		Context->TransitionEvent.Broadcast();
		TestNotEqual("State machine switched states", Instance->GetRootStateMachine().GetSingleActiveState(), Instance->GetRootStateMachine().GetSingleInitialState());
		TestTrue("State machine now in end state.", Instance->IsInEndState());

		Instance->Shutdown();
		TestFalse("State Machine should have stopped", Instance->IsActive());
	}

	// Test disabling autobinded event evaluation.
	{
		TransitionEdge->GetNodeTemplateAs<USMTransitionInstance>()->bCanEvaluateFromEvent = false;
		
		FKismetEditorUtilities::CompileBlueprint(NewBP);

		// Create a context we will run the state machine for.
		USMTestContext* Context = NewObject<USMTestContext>();
		USMInstance* Instance = TestHelpers::CreateNewStateMachineInstanceFromBP(this, NewBP, Context);

		Instance->Start();
		Instance->Update(0.f);
		TestEqual("State machine still in initial state", Instance->GetRootStateMachine().GetSingleActiveState(), Instance->GetRootStateMachine().GetSingleInitialState());
		TestFalse("State machine shouldn't have switched states due to transition evaluation being false.", Instance->IsInEndState());

		// Shouldn't cause evaluation.
		Context->TransitionEvent.Broadcast();
		
		TestEqual("State machine still in initial state", Instance->GetRootStateMachine().GetSingleActiveState(), Instance->GetRootStateMachine().GetSingleInitialState());
		TestFalse("State machine shouldn't have switched states due to transition evaluation being false.", Instance->IsInEndState());
	}

	// Test disabling tick evaluation
	{
		TransitionEdge->GetNodeTemplateAs<USMTransitionInstance>()->bCanEvaluateFromEvent = true;
		
		USMGraphNode_StateNodeBase* FirstState = CastChecked<USMGraphNode_StateNodeBase>(StateMachineGraph->GetEntryNode()->GetOutputNode());
		FirstState->GetNodeTemplateAs<USMStateInstance_Base>()->bDisableTickTransitionEvaluation = true;
		FKismetEditorUtilities::CompileBlueprint(NewBP);

		// Create a context we will run the state machine for.
		USMTestContext* Context = NewObject<USMTestContext>();
		USMInstance* Instance = TestHelpers::CreateNewStateMachineInstanceFromBP(this, NewBP, Context);

		Instance->Start();
		Instance->Update(0.f);
		TestEqual("State machine still in initial state", Instance->GetRootStateMachine().GetSingleActiveState(), Instance->GetRootStateMachine().GetSingleInitialState());
		TestFalse("State machine shouldn't have switched states due to transition evaluation being false.", Instance->IsInEndState());

		// Should cause evaluation.
		Context->TransitionEvent.Broadcast();

		TestNotEqual("State machine switched states", Instance->GetRootStateMachine().GetSingleActiveState(), Instance->GetRootStateMachine().GetSingleInitialState());
		TestTrue("State machine now in end state.", Instance->IsInEndState());

		Instance->Shutdown();
		TestFalse("State Machine should have stopped", Instance->IsActive());
	}

	return true;
}

/**
 * Test creating an any state node.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAnyStateTest, "SMTests.AnyStateTest", EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FAnyStateTest::RunTest(const FString& Parameters)
{
	FAssetHandler NewAsset;
	if (!TestHelpers::TryCreateNewStateMachineAsset(this, NewAsset, false))
	{
		return false;
	}

	USMBlueprint* NewBP = NewAsset.GetObjectAs<USMBlueprint>();

	// Find root state machine.
	USMGraphK2Node_StateMachineNode* RootStateMachineNode = FSMBlueprintEditorUtils::GetRootStateMachineNode(NewBP);

	// Find the state machine graph.
	USMGraph* StateMachineGraph = RootStateMachineNode->GetStateMachineGraph();

	// Total states to test.
	UEdGraphPin* LastStatePin = nullptr;
	
	// Build a state machine of only two states.
	{
		const int32 CurrentStates = 2;
		TestHelpers::BuildLinearStateMachine(this, StateMachineGraph, CurrentStates, &LastStatePin);
	}

	USMGraphNode_StateNodeBase* LastNormalState = CastChecked<USMGraphNode_StateNodeBase>(LastStatePin->GetOwningNode());
	LastNormalState->GetNodeTemplateAs<USMStateInstance_Base>()->bExcludeFromAnyState = false;
	
	// Add any state.
	FGraphNodeCreator<USMGraphNode_AnyStateNode> AnyStateNodeCreator(*StateMachineGraph);
	USMGraphNode_AnyStateNode* AnyState = AnyStateNodeCreator.CreateNode();
	AnyStateNodeCreator.Finalize();

	FString AnyStateInitialStateName = "AnyState_Initial";
	{
		UEdGraphPin* InputPin = AnyState->GetOutputPin();

		// Connect a state to anystate.
		TestHelpers::BuildLinearStateMachine(this, StateMachineGraph, 1, &InputPin);

		AnyState->GetNextNode()->GetBoundGraph()->Rename(*AnyStateInitialStateName, nullptr, REN_DontCreateRedirectors);
	}

	USMGraphNode_TransitionEdge* TransitionEdge = AnyState->GetNextTransition();
	TransitionEdge->GetNodeTemplateAs<USMTransitionInstance>()->PriorityOrder = 1;

	{
		FKismetEditorUtilities::CompileBlueprint(NewBP);
		USMTestContext* Context = NewObject<USMTestContext>();
		USMInstance* Instance = TestHelpers::CreateNewStateMachineInstanceFromBP(this, NewBP, Context);

		Instance->Start();
		TestEqual("State machine still in initial state", Instance->GetRootStateMachine().GetSingleActiveState(), Instance->GetRootStateMachine().GetSingleInitialState());

		// Any state shouldn't be triggered because priority is lower.
		Instance->Update();
		TestNotEqual("Any state transition not called", Instance->GetRootStateMachine().GetSingleActiveState()->GetNodeName(), AnyStateInitialStateName);

		TestFalse("Not considered end state", Instance->IsInEndState());
		
		// No other transitions left except any state.
		Instance->Update();
		TestEqual("Any state transition called", Instance->GetRootStateMachine().GetSingleActiveState()->GetNodeName(), AnyStateInitialStateName);
		
		Instance->Shutdown();
	}
	
	TransitionEdge->GetNodeTemplateAs<USMTransitionInstance>()->PriorityOrder = -1;
	
	{
		FKismetEditorUtilities::CompileBlueprint(NewBP);
		USMTestContext* Context = NewObject<USMTestContext>();
		USMInstance* Instance = TestHelpers::CreateNewStateMachineInstanceFromBP(this, NewBP, Context);

		Instance->Start();
		TestEqual("State machine still in initial state", Instance->GetRootStateMachine().GetSingleActiveState(), Instance->GetRootStateMachine().GetSingleInitialState());

		// Any state should evaluate first.
		Instance->Update();
		TestEqual("Any state transition called", Instance->GetRootStateMachine().GetSingleActiveState()->GetNodeName(), AnyStateInitialStateName);

		Instance->Shutdown();
	}
	
	// Try reference nodes such as Time in State
	{
		TestHelpers::AddSpecialFloatTransitionLogic<USMGraphK2Node_StateReadNode_TimeInState>(this, TransitionEdge);
		FKismetEditorUtilities::CompileBlueprint(NewBP);
		USMTestContext* Context = NewObject<USMTestContext>();
		USMInstance* Instance = TestHelpers::CreateNewStateMachineInstanceFromBP(this, NewBP, Context);

		Instance->Start();
		TestEqual("State machine still in initial state", Instance->GetRootStateMachine().GetSingleActiveState(), Instance->GetRootStateMachine().GetSingleInitialState());

		// Any state shouldn't be triggered yet.
		Instance->Update(1);
		TestNotEqual("Any state transition not called", Instance->GetRootStateMachine().GetSingleActiveState()->GetNodeName(), AnyStateInitialStateName);
		TestFalse("Not considered end state because any state is not excluded from end.", Instance->IsInEndState());
		
		Instance->Update(3);
		TestNotEqual("Any state transition not called", Instance->GetRootStateMachine().GetSingleActiveState()->GetNodeName(), AnyStateInitialStateName);

		Instance->Update(3);
		TestNotEqual("Any state transition not called", Instance->GetRootStateMachine().GetSingleActiveState()->GetNodeName(), AnyStateInitialStateName);
		
		Instance->Update(1);
		TestEqual("Any state transition called", Instance->GetRootStateMachine().GetSingleActiveState()->GetNodeName(), AnyStateInitialStateName);

		Instance->Shutdown();
	}

	LastNormalState->GetNodeTemplateAs<USMStateInstance_Base>()->bExcludeFromAnyState = true;

	// Try reference nodes such as Time in State
	{
		TestHelpers::AddSpecialFloatTransitionLogic<USMGraphK2Node_StateReadNode_TimeInState>(this, TransitionEdge);
		FKismetEditorUtilities::CompileBlueprint(NewBP);
		USMTestContext* Context = NewObject<USMTestContext>();
		USMInstance* Instance = TestHelpers::CreateNewStateMachineInstanceFromBP(this, NewBP, Context);

		Instance->Start();
		TestEqual("State machine still in initial state", Instance->GetRootStateMachine().GetSingleActiveState(), Instance->GetRootStateMachine().GetSingleInitialState());

		// Any state shouldn't be triggered yet.
		Instance->Update(1);
		TestNotEqual("Any state transition not called", Instance->GetRootStateMachine().GetSingleActiveState()->GetNodeName(), AnyStateInitialStateName);
		TestTrue("Considered end state because any state is excluded from end.", Instance->IsInEndState());

		Instance->Update(3);
		TestNotEqual("Any state transition not called", Instance->GetRootStateMachine().GetSingleActiveState()->GetNodeName(), AnyStateInitialStateName);

		// Should not be called because last state is excluded.
		Instance->Update(5);
		TestNotEqual("Any state transition not called", Instance->GetRootStateMachine().GetSingleActiveState()->GetNodeName(), AnyStateInitialStateName);

		Instance->Shutdown();
	}
	
	return true;
}

/**
 * Test the extended editor text graph properties and make sure they format variables correctly.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSMTextGraphPropertyTest, "SMTests.TextGraphProperty", EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FSMTextGraphPropertyTest::RunTest(const FString& Parameters)
{
	FAssetHandler NewAsset;
	if (!TestHelpers::TryCreateNewStateMachineAsset(this, NewAsset, false))
	{
		return false;
	}

	USMBlueprint* NewBP = NewAsset.GetObjectAs<USMBlueprint>();

	// Create variables
	
	FName StrVar = "StrVar";
	FString StrVarValue = "TestString";
	FEdGraphPinType StrPinType;
	StrPinType.PinCategory = USMGraphK2Schema::PC_String;
	FBlueprintEditorUtils::AddMemberVariable(NewBP, StrVar, StrPinType, StrVarValue);
	
	FName IntVar = "IntVar";
	FString IntVarValue = "5";
	FEdGraphPinType IntPinType;
	IntPinType.PinCategory = USMGraphK2Schema::PC_Int;
	FBlueprintEditorUtils::AddMemberVariable(NewBP, IntVar, IntPinType, IntVarValue);

	USMTestObject* TestObject = NewObject<USMTestObject>(GetTransientPackage(), NAME_None, RF_MarkAsRootSet);  // This gets garbage collected before the test ends as of 4.25.0.
	
	FName ObjVar = "ObjVar";
	FEdGraphPinType ObjPinType;
	ObjPinType.PinCategory = USMGraphK2Schema::PC_Object;
	ObjPinType.PinSubCategoryObject = TestObject->GetClass();
	FBlueprintEditorUtils::AddMemberVariable(NewBP, ObjVar, ObjPinType);

	FText NewText = FText::FromString("Hello, {StrVar}! How about {IntVar}? What about no parsing like `{IntVar}? But can I parse the object with a custom to text method? Object: {ObjVar}");
	FText ExpectedText = FText::FromString(FString::Printf(TEXT("Hello, %s! How about %s? What about no parsing like {%s}? But can I parse the object with a custom to text method? Object: %s"),
		*StrVarValue, *IntVarValue, *IntVar.ToString(), *TestObject->CustomToText().ToString()));
	
	// Find root state machine.
	USMGraphK2Node_StateMachineNode* RootStateMachineNode = FSMBlueprintEditorUtils::GetRootStateMachineNode(NewBP);

	// Find the state machine graph.
	USMGraph* StateMachineGraph = RootStateMachineNode->GetStateMachineGraph();

	// Total states to test.
	int32 TotalStates = 1;

	UEdGraphPin* LastStatePin = nullptr;

	// Build single state - state machine.
	TestHelpers::BuildLinearStateMachine(this, StateMachineGraph, TotalStates, &LastStatePin, USMTextGraphState::StaticClass(), USMTransitionTestInstance::StaticClass());
	if (!NewAsset.SaveAsset(this))
	{
		return false;
	}

	USMGraphNode_StateNode* StateNode = CastChecked<USMGraphNode_StateNode>(StateMachineGraph->GetEntryNode()->GetOutputNode());
	auto PropertyNodes = StateNode->GetAllPropertyGraphNodesAsArray();

	TestEqual("Only one property exposed on node", PropertyNodes.Num(), 1);
	
	USMGraphK2Node_TextPropertyNode* TextPropertyNode = CastChecked<USMGraphK2Node_TextPropertyNode>(PropertyNodes[0]);
	USMTextPropertyGraph* PropertyGraph = CastChecked<USMTextPropertyGraph>(TextPropertyNode->GetPropertyGraph());

	((FSMTextGraphProperty*)TextPropertyNode->GetPropertyNodeChecked())->TextSerializer.ToTextFunctionNames.Add(GET_FUNCTION_NAME_CHECKED(USMTestObject, CustomToText));

	PropertyGraph->SetNewText(NewText);

	// Run and check results.
	FKismetEditorUtilities::CompileBlueprint(NewBP);
	USMTestContext* Context = NewObject<USMTestContext>();
	USMInstance* Instance = TestHelpers::CreateNewStateMachineInstanceFromBP(this, NewBP, Context);
	FProperty* ObjProperty = FindFProperty<FProperty>(Instance->GetClass(), ObjVar);

	uint8* SourcePtr = (uint8*)TestObject;
	uint8* DestPtr = ObjProperty->ContainerPtrToValuePtr<uint8>(Instance);
	ObjProperty->CopyCompleteValue(DestPtr, SourcePtr);
	
	Instance->Start();
	
	USMTextGraphState* NodeInstance = CastChecked<USMTextGraphState>(Instance->GetRootStateMachine().GetSingleInitialState()->GetNodeInstance());
	TestEqual("Text graph evaluated manually", NodeInstance->EvaluatedText.ToString(), ExpectedText.ToString());

	return NewAsset.DeleteAsset(this);
}

/**
 * Run multiple states in parallel.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FParallelStatesTest, "SMTests.ParallelStates", EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

	bool FParallelStatesTest::RunTest(const FString& Parameters)
{
	FAssetHandler NewAsset;
	if (!TestHelpers::TryCreateNewStateMachineAsset(this, NewAsset, false))
	{
		return false;
	}

	USMBlueprint* NewBP = NewAsset.GetObjectAs<USMBlueprint>();

	// Find root state machine.
	USMGraphK2Node_StateMachineNode* RootStateMachineNode = FSMBlueprintEditorUtils::GetRootStateMachineNode(NewBP);

	// Find the state machine graph.
	USMGraph* StateMachineGraph = RootStateMachineNode->GetStateMachineGraph();

	// Total states to test.
	int32 Rows = 2;
	int32 Branches = 2;
	TArray<UEdGraphPin*> LastStatePins;

	// A -> (B, C) Single
	TestHelpers::BuildBranchingStateMachine(this, StateMachineGraph, Rows, Branches, false, &LastStatePins);

	int32 EntryHits = 0; int32 UpdateHits = 0; int32 EndHits = 0;
	USMInstance* Instance = TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits);
	TestEqual("States hit linearly", EntryHits, Branches);

	// A -> (B, C) Parallel
	LastStatePins.Reset();
	FSMBlueprintEditorUtils::RemoveAllNodesFromGraph(StateMachineGraph, NewBP);
	TestHelpers::BuildBranchingStateMachine(this, StateMachineGraph, Rows, Branches, true, &LastStatePins);
	Instance = TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits, 1000, false);
	TestEqual("States hit parallel", EntryHits, Instance->GetStateMap().Num() - 1);

	// A -> (B, C, D, E) Parallel
	LastStatePins.Reset();
	FSMBlueprintEditorUtils::RemoveAllNodesFromGraph(StateMachineGraph, NewBP);
	Branches = 4;
	TestHelpers::BuildBranchingStateMachine(this, StateMachineGraph, Rows, Branches, true, &LastStatePins);
	Instance = TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits, 1000, false);
	TestEqual("States hit parallel", EntryHits, Instance->GetStateMap().Num() - 1);

	// A -> (B -> (B1 -> ..., B2-> ...), C -> ...) Parallel
	LastStatePins.Reset();
	FSMBlueprintEditorUtils::RemoveAllNodesFromGraph(StateMachineGraph, NewBP);
	Rows = 4;
	Branches = 2;
	TestHelpers::BuildBranchingStateMachine(this, StateMachineGraph, Rows, Branches, true, &LastStatePins);
	Instance = TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits, 1000, false);
	TestEqual("States hit parallel", EntryHits, Instance->GetStateMap().Num() - 1);

	LastStatePins.Reset();
	FSMBlueprintEditorUtils::RemoveAllNodesFromGraph(StateMachineGraph, NewBP);
	Rows = 3;
	Branches = 3;
	TestHelpers::BuildBranchingStateMachine(this, StateMachineGraph, Rows, Branches, true, &LastStatePins);
	Instance = TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits, 1000, false);
	TestEqual("States hit parallel", EntryHits, Instance->GetStateMap().Num() - 1);
	
	{
		TArray<FGuid> ActiveGuids;
		Instance->GetAllActiveStateGuids(ActiveGuids);
		const float TotalActiveEndStates = FMath::Pow(Branches, Rows); // Only end states are active.

		TestEqual("Active guids match end states.", ActiveGuids.Num(), (int32)TotalActiveEndStates);

		// Reset and reload. Only end states should be active.
		Instance->Shutdown();
		Instance = TestHelpers::CreateNewStateMachineInstanceFromBP(this, NewBP, NewObject<USMTestContext>());
		Instance->LoadFromMultipleStates(ActiveGuids);

		TestEqual("All initial states set", Instance->GetRootStateMachine().GetInitialStates().Num(), ActiveGuids.Num());
		Instance->Start();
		TestEqual("All states reloaded", TestHelpers::ArrayContentsInArray(Instance->GetAllActiveStateGuidsCopy(), ActiveGuids), ActiveGuids.Num());
	}

	// Test with leaving states active.
	{
		LastStatePins.Reset();
		FSMBlueprintEditorUtils::RemoveAllNodesFromGraph(StateMachineGraph, NewBP);
		Rows = 3;
		Branches = 3;
		TestHelpers::BuildBranchingStateMachine(this, StateMachineGraph, Rows, Branches, true, &LastStatePins, true);
		Instance = TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits, 1000, false);
		TestEqual("States hit parallel", EntryHits, Instance->GetStateMap().Num() - 1);

		TArray<FGuid> ActiveGuids;
		Instance->GetAllActiveStateGuids(ActiveGuids);

		TestEqual("Active guids match all states.", ActiveGuids.Num(), Instance->GetStateMap().Num() - 1);
		
		// Reset and reload.
		Instance->Shutdown();
		Instance = TestHelpers::CreateNewStateMachineInstanceFromBP(this, NewBP, NewObject<USMTestContext>());
		Instance->LoadFromMultipleStates(ActiveGuids);

		TestEqual("All initial states set", Instance->GetRootStateMachine().GetInitialStates().Num(), ActiveGuids.Num());
		Instance->Start();
		TestEqual("All states reloaded", TestHelpers::ArrayContentsInArray(Instance->GetAllActiveStateGuidsCopy(), ActiveGuids), ActiveGuids.Num());

		// Test manually deactivating states.
		{
			TArray<USMStateInstance_Base*> StateInstances;
			Instance->GetAllStateInstances(StateInstances);

			StateInstances[1]->SetActive(false);
			TestEqual("State active changed", TestHelpers::ArrayContentsInArray(Instance->GetAllActiveStateGuidsCopy(), ActiveGuids), ActiveGuids.Num() - 1);

			StateInstances[1]->SetActive(true);
			TestEqual("State active changed", TestHelpers::ArrayContentsInArray(Instance->GetAllActiveStateGuidsCopy(), ActiveGuids), ActiveGuids.Num());

			for (USMStateInstance_Base* StateInstance : StateInstances)
			{
				StateInstance->SetActive(false);
			}

			TestEqual("State active changed", Instance->GetAllActiveStateGuidsCopy().Num(), 0);

			for (USMStateInstance_Base* StateInstance : StateInstances)
			{
				StateInstance->SetActive(true);
			}

			TestEqual("State active changed", TestHelpers::ArrayContentsInArray(Instance->GetAllActiveStateGuidsCopy(), ActiveGuids), ActiveGuids.Num());
		}
	}

	// Test state re-entry
	{
		LastStatePins.Reset();
		FSMBlueprintEditorUtils::RemoveAllNodesFromGraph(StateMachineGraph, NewBP);

		TestHelpers::BuildBranchingStateMachine(this, StateMachineGraph, 2, 1, true, &LastStatePins, true, true);
		Instance = TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits, 1000, false);
		TestEqual("States hit parallel", EntryHits, Instance->GetStateMap().Num() - 1);
		TestEqual("States hit parallel", UpdateHits, 1);
		TestEqual("States hit parallel", EndHits, 0);

		Instance->Update(1.f);

		USMTestContext* Context = CastChecked<USMTestContext>(Instance->GetContext());
		EntryHits = Context->GetEntryInt();
		UpdateHits = Context->GetUpdateInt();
		EndHits = Context->GetEndInt();
		
		TestEqual("States hit parallel", EntryHits, Instance->GetStateMap().Num());
		TestEqual("States hit parallel", UpdateHits, 3); // Each state updates again. Currently we let a state that was re-entered run its update logic in the same tick.
		TestEqual("States hit parallel", EndHits, 0);

		// Without re-entry
		{
			LastStatePins.Reset();
			FSMBlueprintEditorUtils::RemoveAllNodesFromGraph(StateMachineGraph, NewBP);

			TestHelpers::BuildBranchingStateMachine(this, StateMachineGraph, 2, 1, true, &LastStatePins, true, false);
			Instance = TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits, 1000, false);
			
			TestEqual("States hit parallel", EntryHits, Instance->GetStateMap().Num() - 1);
			TestEqual("States hit parallel", UpdateHits, 1);
			TestEqual("States hit parallel", EndHits, 0);

			Instance->Update(1.f);

			Context = CastChecked<USMTestContext>(Instance->GetContext());
			EntryHits = Context->GetEntryInt();
			UpdateHits = Context->GetUpdateInt();
			EndHits = Context->GetEndInt();

			TestEqual("States hit parallel", EntryHits, Instance->GetStateMap().Num() - 1);
			TestEqual("States hit parallel", UpdateHits, 3); // Each state updates again.
			TestEqual("States hit parallel", EndHits, 0);
		}

		// Without transition evaluation connecting to an already active state.
		{
			LastStatePins.Reset();
			FSMBlueprintEditorUtils::RemoveAllNodesFromGraph(StateMachineGraph, NewBP);

			TestHelpers::BuildBranchingStateMachine(this, StateMachineGraph, 2, 1, true, &LastStatePins, true, true, false);
			Instance = TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits, 1000, false);

			TestEqual("States hit parallel", EntryHits, Instance->GetStateMap().Num() - 1);
			TestEqual("States hit parallel", UpdateHits, 1);
			TestEqual("States hit parallel", EndHits, 0);

			Instance->Update(1.f);

			Context = CastChecked<USMTestContext>(Instance->GetContext());
			EntryHits = Context->GetEntryInt();
			UpdateHits = Context->GetUpdateInt();
			EndHits = Context->GetEndInt();

			TestEqual("States hit parallel", EntryHits, Instance->GetStateMap().Num() - 1);
			TestEqual("States hit parallel", UpdateHits, 3); // Each state updates again.
			TestEqual("States hit parallel", EndHits, 0);
		}
	}
	
	return NewAsset.DeleteAsset(this);
}

/**
 * Test the new pin names load correctly.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSMPinConversionTest, "SMTests.PinConversion", EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FSMPinConversionTest::RunTest(const FString& Parameters)
{
	FAssetHandler NewAsset;
	if (!TestHelpers::TryCreateNewStateMachineAsset(this, NewAsset, false))
	{
		return false;
	}

	USMBlueprint* NewBP = NewAsset.GetObjectAs<USMBlueprint>();

	// Find root state machine.
	USMGraphK2Node_StateMachineNode* RootStateMachineNode = FSMBlueprintEditorUtils::GetRootStateMachineNode(NewBP);

	// Find the state machine graph.
	USMGraph* StateMachineGraph = RootStateMachineNode->GetStateMachineGraph();

	// Total states to test.
	UEdGraphPin* LastStatePin = nullptr;

	{
		TestHelpers::BuildLinearStateMachine(this, StateMachineGraph, 2, &LastStatePin, USMStateTestInstance::StaticClass(), USMTransitionTestInstance::StaticClass());
	}

	USMGraphNode_StateNodeBase* FirstNode = CastChecked<USMGraphNode_StateNodeBase>(StateMachineGraph->GetEntryNode()->GetOutputNode());
	{
		USMGraphNode_TransitionEdge* Transition = CastChecked<USMGraphNode_TransitionEdge>(FirstNode->GetOutputPin()->LinkedTo[0]->GetOwningNode());
		
		TestHelpers::AddEventWithLogic<USMGraphK2Node_TransitionPreEvaluateNode>(this, Transition,
			USMTestContext::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(USMTestContext, IncreaseTransitionPreEval)));

		TestHelpers::AddEventWithLogic<USMGraphK2Node_TransitionPostEvaluateNode>(this, Transition,
			USMTestContext::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(USMTestContext, IncreaseTransitionPostEval)));
	}
	
	// Use a conduit
	USMGraphNode_ConduitNode* SecondNodeConduit = FSMBlueprintEditorUtils::ConvertNodeTo<USMGraphNode_ConduitNode>(CastChecked<USMGraphNode_StateNodeBase>(FirstNode->GetNextNode()));
	{
		SecondNodeConduit->SetNodeClass(USMConduitTestInstance::StaticClass());
		USMConduitGraph* Graph = CastChecked<USMConduitGraph>(SecondNodeConduit->GetBoundGraph());
		UEdGraphPin* CanEvalPin = Graph->ResultNode->GetInputPin();
		CanEvalPin->BreakAllPinLinks();
		CanEvalPin->DefaultValue = "True";

		// Pin was cleared out during conversion.
		LastStatePin = SecondNodeConduit->GetOutputPin();
	}
	
	UEdGraphPin* LastNestedPin = nullptr;
	USMGraphNode_StateMachineStateNode* ThirdNodeRef = TestHelpers::BuildNestedStateMachine(this, StateMachineGraph, 1, &LastStatePin, &LastNestedPin);
	{
		FString AssetName = "PinTestRef_1";
		UBlueprint* RefBlueprint = FSMBlueprintEditorUtils::ConvertStateMachineToReference(ThirdNodeRef, false, &AssetName, nullptr);
		FKismetEditorUtilities::CompileBlueprint(RefBlueprint);
		LastStatePin = ThirdNodeRef->GetOutputPin();
	}
	
	USMGraphNode_StateMachineStateNode* FourthNodeIntermediateRef = TestHelpers::BuildNestedStateMachine(this, StateMachineGraph, 1, &LastStatePin, &LastNestedPin);
	{
		FString AssetName = "PinTestRef_2";
		UBlueprint* RefBlueprint = FSMBlueprintEditorUtils::ConvertStateMachineToReference(FourthNodeIntermediateRef, false, &AssetName, nullptr);
		FKismetEditorUtilities::CompileBlueprint(RefBlueprint);
		
		FourthNodeIntermediateRef->SetUseIntermediateGraph(true);
		LastStatePin = FourthNodeIntermediateRef->GetOutputPin();
		
		TestHelpers::AddEventWithLogic<USMGraphK2Node_IntermediateStateMachineStartNode>(this, FourthNodeIntermediateRef,
			USMTestContext::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(USMTestContext, IncreaseEntryInt)));
	}

	// Add one last state
	TestHelpers::BuildLinearStateMachine(this, StateMachineGraph, 1, &LastStatePin, USMStateTestInstance::StaticClass(), USMTransitionTestInstance::StaticClass());

	{
		USMGraphNode_TransitionEdge* Transition = CastChecked<USMGraphNode_TransitionEdge>(SecondNodeConduit->GetOutputPin()->LinkedTo[0]->GetOwningNode());
		TestHelpers::SetNodeClass(this, Transition, USMTransitionTestInstance::StaticClass());

		TestHelpers::AddTransitionResultLogic(this, Transition);
		
		TestHelpers::AddEventWithLogic<USMGraphK2Node_TransitionPreEvaluateNode>(this, Transition,
			USMTestContext::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(USMTestContext, IncreaseTransitionPreEval)));

		TestHelpers::AddEventWithLogic<USMGraphK2Node_TransitionPostEvaluateNode>(this, Transition,
			USMTestContext::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(USMTestContext, IncreaseTransitionPostEval)));
	}
	
	{
		// Signal the state after the nested state machine to wait for its completion.
		USMGraphNode_TransitionEdge* TransitionFromNestedStateMachine = CastChecked<USMGraphNode_TransitionEdge>(ThirdNodeRef->GetOutputPin()->LinkedTo[0]->GetOwningNode());

		TestHelpers::SetNodeClass(this, TransitionFromNestedStateMachine, USMTransitionTestInstance::StaticClass());
		TestHelpers::AddTransitionResultLogic(this, TransitionFromNestedStateMachine);
		
		TestHelpers::AddEventWithLogic<USMGraphK2Node_TransitionPreEvaluateNode>(this, TransitionFromNestedStateMachine,
			USMTestContext::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(USMTestContext, IncreaseTransitionPreEval)));

		TestHelpers::AddEventWithLogic<USMGraphK2Node_TransitionPostEvaluateNode>(this, TransitionFromNestedStateMachine,
			USMTestContext::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(USMTestContext, IncreaseTransitionPostEval)));
	}

	{
		// Signal the state after the nested state machine to wait for its completion.
		USMGraphNode_TransitionEdge* TransitionFromNestedStateMachine = CastChecked<USMGraphNode_TransitionEdge>(FourthNodeIntermediateRef->GetOutputPin()->LinkedTo[0]->GetOwningNode());

		TestHelpers::SetNodeClass(this, TransitionFromNestedStateMachine, USMTransitionTestInstance::StaticClass());
		TestHelpers::AddTransitionResultLogic(this, TransitionFromNestedStateMachine);
		
		TestHelpers::AddEventWithLogic<USMGraphK2Node_TransitionPreEvaluateNode>(this, TransitionFromNestedStateMachine,
			USMTestContext::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(USMTestContext, IncreaseTransitionPreEval)));

		TestHelpers::AddEventWithLogic<USMGraphK2Node_TransitionPostEvaluateNode>(this, TransitionFromNestedStateMachine,
			USMTestContext::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(USMTestContext, IncreaseTransitionPostEval)));
	}

	// Run as normal.
	int32 EntryHits = 0; int32 UpdateHits = 0; int32 EndHits = 0;
	USMInstance* Instance = TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits);

	USMTestContext* Context = CastChecked<USMTestContext>(Instance->GetContext());
	int32 PreEval = Context->TestTransitionPreEval.Count;
	int32 PostEval = Context->TestTransitionPostEval.Count;

	TestTrue("Pre/Post Evals hit", PreEval > 0 && PostEval > 0);
	
	// Rename all of the pins to pre 2.1 pin names.
	FName OldPinName = FName("");
	{
		{
			TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_StateEntryNode>(this, FirstNode->GetBoundGraph(), USMGraphK2Schema::PN_Then, &OldPinName);
			TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_StateUpdateNode>(this, FirstNode->GetBoundGraph(), USMGraphK2Schema::PN_Then, &OldPinName);
			TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_StateEndNode>(this, FirstNode->GetBoundGraph(), USMGraphK2Schema::PN_Then, &OldPinName);
		}

		{
			TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionEnteredNode>(this, FirstNode->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then, &OldPinName);
			TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionInitializedNode>(this, FirstNode->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then, &OldPinName);
			TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionShutdownNode>(this, FirstNode->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then, &OldPinName);
			TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionPreEvaluateNode>(this, FirstNode->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then, &OldPinName);
			TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionPostEvaluateNode>(this, FirstNode->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then, &OldPinName);
		}
		
		{
			TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_IntermediateStateMachineStartNode>(this, SecondNodeConduit->GetBoundGraph(), USMGraphK2Schema::PN_Then, &OldPinName);
		}

		{
			TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionEnteredNode>(this, SecondNodeConduit->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then, &OldPinName);
			TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionInitializedNode>(this, SecondNodeConduit->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then, &OldPinName);
			TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionShutdownNode>(this, SecondNodeConduit->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then, &OldPinName);
			TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionPreEvaluateNode>(this, SecondNodeConduit->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then, &OldPinName);
			TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionPostEvaluateNode>(this, SecondNodeConduit->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then, &OldPinName);
		}

		{
			TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionEnteredNode>(this, ThirdNodeRef->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then, &OldPinName);
			TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionInitializedNode>(this, ThirdNodeRef->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then, &OldPinName);
			TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionShutdownNode>(this, ThirdNodeRef->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then, &OldPinName);
			TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionPreEvaluateNode>(this, ThirdNodeRef->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then, &OldPinName);
			TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionPostEvaluateNode>(this, ThirdNodeRef->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then, &OldPinName);
		}
		
		{
			TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_IntermediateEntryNode>(this, FourthNodeIntermediateRef->GetBoundGraph(), USMGraphK2Schema::PN_Then, &OldPinName);
			TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_StateUpdateNode>(this, FourthNodeIntermediateRef->GetBoundGraph(), USMGraphK2Schema::PN_Then, &OldPinName);
			TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_StateEndNode>(this, FourthNodeIntermediateRef->GetBoundGraph(), USMGraphK2Schema::PN_Then, &OldPinName);

			TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_IntermediateStateMachineStartNode>(this, FourthNodeIntermediateRef->GetBoundGraph(), USMGraphK2Schema::PN_Then, &OldPinName);
		}

		{
			TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionEnteredNode>(this, FourthNodeIntermediateRef->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then, &OldPinName);
			TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionInitializedNode>(this, FourthNodeIntermediateRef->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then, &OldPinName);
			TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionShutdownNode>(this, FourthNodeIntermediateRef->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then, &OldPinName);
			TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionPreEvaluateNode>(this, FourthNodeIntermediateRef->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then, &OldPinName);
			TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionPostEvaluateNode>(this, FourthNodeIntermediateRef->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then, &OldPinName);
		}
	}

	// Verify it still works.
	int32 EntryHits2 = 0; int32 UpdateHits2 = 0; int32 EndHits2 = 0;
	Instance = TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits2, UpdateHits2, EndHits2);

	Context = CastChecked<USMTestContext>(Instance->GetContext());
	TestEqual("Hits match", Context->TestTransitionPreEval.Count, PreEval);
	TestEqual("Hits match", Context->TestTransitionPostEval.Count, PostEval);

	TestEqual("Hits match", EntryHits2, EntryHits);
	TestEqual("Hits match", UpdateHits2, UpdateHits);
	TestEqual("Hits match", EndHits2, EndHits);
	
	if (!NewAsset.SaveAsset(this))
	{
		return false;
	}

	if (!NewAsset.LoadAsset(this))
	{
		return false;
	}

	NewBP = NewAsset.GetObjectAs<USMBlueprint>();
	FSMBlueprintEditorUtils::ReconstructAllNodes(NewBP);
	
	RootStateMachineNode = FSMBlueprintEditorUtils::GetRootStateMachineNode(NewBP);
	StateMachineGraph = RootStateMachineNode->GetStateMachineGraph();
	FirstNode = CastChecked<USMGraphNode_StateNodeBase>(StateMachineGraph->GetEntryNode()->GetOutputNode());

	// Verify pins have been correctly renamed.
	{
		TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_StateEntryNode>(this, FirstNode->GetBoundGraph(), USMGraphK2Schema::PN_Then);
		TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_StateUpdateNode>(this, FirstNode->GetBoundGraph(), USMGraphK2Schema::PN_Then);
		TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_StateEndNode>(this, FirstNode->GetBoundGraph(), USMGraphK2Schema::PN_Then);
	}

	{
		TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionEnteredNode>(this, FirstNode->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then);
		TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionInitializedNode>(this, FirstNode->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then);
		TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionShutdownNode>(this, FirstNode->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then);
		TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionPreEvaluateNode>(this, FirstNode->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then);
		TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionPostEvaluateNode>(this, FirstNode->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then);
	}

	{
		TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_IntermediateStateMachineStartNode>(this, SecondNodeConduit->GetBoundGraph(), USMGraphK2Schema::PN_Then);
	}

	{
		TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionEnteredNode>(this, SecondNodeConduit->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then);
		TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionInitializedNode>(this, SecondNodeConduit->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then);
		TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionShutdownNode>(this, SecondNodeConduit->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then);
		TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionPreEvaluateNode>(this, SecondNodeConduit->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then);
		TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionPostEvaluateNode>(this, SecondNodeConduit->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then);
	}

	{
		TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionEnteredNode>(this, ThirdNodeRef->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then);
		TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionInitializedNode>(this, ThirdNodeRef->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then);
		TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionShutdownNode>(this, ThirdNodeRef->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then);
		TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionPreEvaluateNode>(this, ThirdNodeRef->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then);
		TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionPostEvaluateNode>(this, ThirdNodeRef->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then);
	}

	{
		TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_IntermediateEntryNode>(this, FourthNodeIntermediateRef->GetBoundGraph(), USMGraphK2Schema::PN_Then);
		TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_StateUpdateNode>(this, FourthNodeIntermediateRef->GetBoundGraph(), USMGraphK2Schema::PN_Then);
		TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_StateEndNode>(this, FourthNodeIntermediateRef->GetBoundGraph(), USMGraphK2Schema::PN_Then);

		TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_IntermediateStateMachineStartNode>(this, FourthNodeIntermediateRef->GetBoundGraph(), USMGraphK2Schema::PN_Then);
	}

	{
		TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionEnteredNode>(this, FourthNodeIntermediateRef->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then);
		TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionInitializedNode>(this, FourthNodeIntermediateRef->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then);
		TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionShutdownNode>(this, FourthNodeIntermediateRef->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then);
		TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionPreEvaluateNode>(this, FourthNodeIntermediateRef->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then);
		TestHelpers::VerifyNodeWiredFromPin<USMGraphK2Node_TransitionPostEvaluateNode>(this, FourthNodeIntermediateRef->GetNextTransition()->GetBoundGraph(), USMGraphK2Schema::PN_Then);
	}

	Instance = TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits2, UpdateHits2, EndHits2);

	Context = CastChecked<USMTestContext>(Instance->GetContext());
	TestEqual("Hits match", Context->TestTransitionPreEval.Count, PreEval);
	TestEqual("Hits match", Context->TestTransitionPostEval.Count, PostEval);
	
	TestEqual("Hits match", EntryHits2, EntryHits);
	TestEqual("Hits match", UpdateHits2, UpdateHits);
	TestEqual("Hits match", EndHits2, EndHits);
	
	return NewAsset.DeleteAsset(this);
}

#endif

#endif //WITH_DEV_AUTOMATION_TESTS