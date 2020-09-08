// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#include "SMBlueprint.h"
#include "SMTestHelpers.h"
#include "SMBlueprintGeneratedClass.h"
#include "SMBlueprintFactory.h"
#include "SMBlueprintEditorUtils.h"
#include "SMTestContext.h"
#include "EdGraph/EdGraph.h"
#include "K2Node_CallFunction.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Graph/SMGraphK2.h"
#include "Graph/SMGraph.h"
#include "Graph/SMStateGraph.h"
#include "Graph/SMIntermediateGraph.h"
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


#if WITH_DEV_AUTOMATION_TESTS

#if PLATFORM_DESKTOP

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreateAssetTest, "SMTests.CreateAsset", EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

	bool FCreateAssetTest::RunTest(const FString& Parameters)
{
	FAssetHandler NewAsset;
	if (!TestHelpers::TryCreateNewStateMachineAsset(this, NewAsset, true))
	{
		return false;
	}

	// Verify correct type created.
	USMBlueprint* NewBP = NewAsset.GetObjectAs<USMBlueprint>();
	TestNotNull("New asset object should be USMBlueprint", NewBP);

	{
		USMBlueprintGeneratedClass* GeneratedClass = Cast<USMBlueprintGeneratedClass>(NewBP->GetGeneratedClass());
		TestNotNull("Generated Class should match expected class", GeneratedClass);

		// Verify new version set correctly.
		TestEqual("Instance version is correctly created", STATEMACHINE_BLUEPRINT_VERSION, NewBP->GetBlueprintVersion());
	}

	bool bReverify = false;

Verify:

	// Verify event graph.
	{
		UEdGraph** FoundGraph = NewBP->UbergraphPages.FindByPredicate([&](UEdGraph* Graph)
		{
			return Graph->GetFName() == USMGraphK2Schema::GN_EventGraph;
		});
		TestNotNull("Event Graph should exist", FoundGraph);
	}

	// Verify state machine graph.
	{
		USMGraphK2* TopLevelGraph = FSMBlueprintEditorUtils::GetTopLevelStateMachineGraph(NewBP);
		TestNotNull("State Machine Graph should exist", TopLevelGraph);

		TArray<USMGraphK2Node_StateMachineSelectNode*> SelectNodes;
		TopLevelGraph->GetNodesOfClass<USMGraphK2Node_StateMachineSelectNode>(SelectNodes);

		TestTrue("One state machine select node should exist", SelectNodes.Num() == 1);

		USMGraphK2Node_StateMachineSelectNode* SelectNode = SelectNodes[0];
		TestTrue("Select node should have a single default wire", SelectNode->GetInputPin()->LinkedTo.Num() == 1);
		TestTrue("SelectNode should be wired to a state machine definition node", SelectNode->GetInputPin()->LinkedTo[0]->GetOwningNode()->IsA<USMGraphK2Node_StateMachineNode>());
	}

	// Verify reloading asset works properly.
	if (!bReverify)
	{
		if (!NewAsset.LoadAsset(this))
		{
			return false;
		}

		NewBP = NewAsset.GetObjectAs<USMBlueprint>();
		TestNotNull("New asset object should be USMBlueprint", NewBP);

		USMBlueprintGeneratedClass* GeneratedClass = Cast<USMBlueprintGeneratedClass>(NewBP->GetGeneratedClass());
		TestNotNull("Generated Class should match expected class", GeneratedClass);

		//////////////////////////////////////////////////////////////////////////
		// ** If changing instance version number change this test. **
		//////////////////////////////////////////////////////////////////////////
		// Verify version matches.
		TestEqual("Instance version is correctly created", STATEMACHINE_BLUEPRINT_VERSION, NewBP->GetBlueprintVersion());

		bReverify = true;
		goto Verify;
	}

	return NewAsset.DeleteAsset(this);
}

/**
 * Test deleting by both node and graph.
 * Deletion has some circular logic involved so we want to make sure we don't get stuck in a loop and that everything cleans up properly.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDeleteDeleteNodeTest, "SMTests.DeleteNode", EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

	bool FDeleteDeleteNodeTest::RunTest(const FString& Parameters)
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
	int32 TotalStates = 0;
	UEdGraphPin* LastStatePin = nullptr;

	// Build a state machine of three states.
	{
		const int32 CurrentStates = 3;
		TestHelpers::BuildLinearStateMachine(this, StateMachineGraph, CurrentStates, &LastStatePin);
		if (!NewAsset.SaveAsset(this))
		{
			return false;
		}
		TotalStates += CurrentStates;
	}

	// Verify works before deleting.
	{
		const int32 ExpectedValue = TotalStates;
		int32 EntryHits = 0; int32 UpdateHits = 0; int32 EndHits = 0;
		TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits);
		TestEqual("State Machine generated value", EntryHits, ExpectedValue);
		TestEqual("State Machine generated value", UpdateHits, 0);
		TestEqual("State Machine generated value", EndHits, ExpectedValue);
	}


	// Test deleting the last node by deleting the node.
	{
		USMGraphNode_StateNode* LastStateNode = CastChecked<USMGraphNode_StateNode>(LastStatePin->GetOwningNode());
		USMStateGraph* StateGraph = CastChecked<USMStateGraph>(LastStateNode->GetBoundGraph());
		USMGraph* OwningGraph = LastStateNode->GetStateMachineGraph();

		// Set last state pin to the previous state.
		LastStatePin = CastChecked<USMGraphNode_TransitionEdge>(LastStateNode->GetInputPin()->LinkedTo[0]->GetOwningNode())->GetInputPin()->LinkedTo[0];

		TestTrue("State Machine Graph should own State Node", OwningGraph->Nodes.Contains(LastStateNode));
		TestTrue("State Machine Graph should have a State Graph subgraph", OwningGraph->SubGraphs.Contains(StateGraph));

		FSMBlueprintEditorUtils::RemoveNode(NewBP, LastStateNode, true);

		TestTrue("State Machine Graph should not own State Node", !OwningGraph->Nodes.Contains(LastStateNode));
		TestTrue("State Machine Graph should not have a State Graph subgraph", !OwningGraph->SubGraphs.Contains(StateGraph));

		TotalStates--;

		// Verify runs without last state.
		{
			const int32 ExpectedValue = TotalStates;
			int32 EntryHits = 0; int32 UpdateHits = 0; int32 EndHits = 0;
			TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits);
			TestEqual("State Machine generated value", EntryHits, ExpectedValue);
			TestEqual("State Machine generated value", UpdateHits, 0);
			TestEqual("State Machine generated value", EndHits, ExpectedValue);
		}
	}

	// Test deleting the last node by deleting the graph.
	{
		USMGraphNode_StateNode* LastStateNode = CastChecked<USMGraphNode_StateNode>(LastStatePin->GetOwningNode());
		USMStateGraph* StateGraph = CastChecked<USMStateGraph>(LastStateNode->GetBoundGraph());
		USMGraph* OwningGraph = LastStateNode->GetStateMachineGraph();

		TestTrue("State Machine Graph should own State Node", OwningGraph->Nodes.Contains(LastStateNode));
		TestTrue("State Machine Graph should have a State Graph subgraph", OwningGraph->SubGraphs.Contains(StateGraph));

		FSMBlueprintEditorUtils::RemoveGraph(NewBP, StateGraph);

		TestTrue("State Machine Graph should not own State Node", !OwningGraph->Nodes.Contains(LastStateNode));
		TestTrue("State Machine Graph should not have a State Graph subgraph", !OwningGraph->SubGraphs.Contains(StateGraph));

		TotalStates--;

		// Verify runs without last state.
		{
			const int32 ExpectedValue = TotalStates;
			int32 EntryHits = 0; int32 UpdateHits = 0; int32 EndHits = 0;
			TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits);
			TestEqual("State Machine generated value", EntryHits, ExpectedValue);
			TestEqual("State Machine generated value", UpdateHits, 0);
			TestEqual("State Machine generated value", EndHits, ExpectedValue);
		}
	}

	return NewAsset.DeleteAsset(this);
}

/**
 * Assemble and run a hierarchical state machine.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAssembleStateMachineTest, "SMTests.AssembleStateMachine", EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

	bool FAssembleStateMachineTest::RunTest(const FString& Parameters)
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
	TestHelpers::BuildLinearStateMachine(this, StateMachineGraph, 1, &LastStatePin);
	if (!NewAsset.SaveAsset(this))
	{
		return false;
	}
	TestHelpers::TestLinearStateMachine(this, NewBP, TotalStates);

	// Add on a second state.
	TestHelpers::BuildLinearStateMachine(this, StateMachineGraph, 1, &LastStatePin, USMStateTestInstance::StaticClass(), USMTransitionTestInstance::StaticClass());
	if (!NewAsset.SaveAsset(this))
	{
		return false;
	}
	TotalStates++;
	TestHelpers::TestLinearStateMachine(this, NewBP, TotalStates);

	// Build a nested state machine.
	UEdGraphPin* EntryPointForNestedStateMachine = LastStatePin;
	USMGraphNode_StateMachineStateNode* NestedStateMachineNode = TestHelpers::CreateNewNode<USMGraphNode_StateMachineStateNode>(this, StateMachineGraph, EntryPointForNestedStateMachine);

	UEdGraphPin* LastNestedPin = nullptr;
	{
		TestHelpers::BuildLinearStateMachine(this, Cast<USMGraph>(NestedStateMachineNode->GetBoundGraph()), 1, &LastNestedPin);
		LastStatePin = NestedStateMachineNode->GetOutputPin();
	}

	// Add logic to the state machine transition.
	USMGraphNode_TransitionEdge* TransitionToNestedStateMachine = CastChecked<USMGraphNode_TransitionEdge>(NestedStateMachineNode->GetInputPin()->LinkedTo[0]->GetOwningNode());
	TestHelpers::AddTransitionResultLogic(this, TransitionToNestedStateMachine);

	TotalStates += 1; // Nested machine is a single state.
	TestHelpers::TestLinearStateMachine(this, NewBP, TotalStates);

	// Add more top level.
	TestHelpers::BuildLinearStateMachine(this, StateMachineGraph, 10, &LastStatePin);
	if (!NewAsset.SaveAsset(this))
	{
		return false;
	}
	TotalStates += 10;

	// This will run the nested machine only up to the first state.
	TestHelpers::TestLinearStateMachine(this, NewBP, TotalStates);

	int32 ExpectedEntryValue = TotalStates;
	// Run the same machine until an end state is reached.
	{
		int32 EntryHits = 0; int32 UpdateHits = 0; int32 EndHits = 0;
		TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits);

		TestEqual("State Machine generated value", EntryHits, ExpectedEntryValue);
		TestEqual("State Machine generated value", UpdateHits, 0);
		TestEqual("State Machine generated value", EndHits, ExpectedEntryValue);
	}

	// Add to the nested state machine
	{
		TestHelpers::BuildLinearStateMachine(this, Cast<USMGraph>(NestedStateMachineNode->GetBoundGraph()), 10, &LastNestedPin);
		TotalStates += 10;
	}

	// Run the same machine until an end state is reached. The result should be the same as the top level machine won't wait for the nested machine.
	{
		int32 EntryHits = 0; int32 UpdateHits = 0; int32 EndHits = 0;
		TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits);

		TestEqual("State Machine generated value", EntryHits, ExpectedEntryValue);
		TestEqual("State Machine generated value", UpdateHits, 0);
		TestEqual("State Machine generated value", EndHits, ExpectedEntryValue);
	}

	// Run the same machine until an end state is reached. This time we force states to update when ending.
	{
		TArray<USMGraphNode_StateNode*> TopLevelStates;
		FSMBlueprintEditorUtils::GetAllNodesOfClassNested<USMGraphNode_StateNode>(StateMachineGraph, TopLevelStates);
		check(TopLevelStates.Num() > 0);

		for (USMGraphNode_StateNode* State : TopLevelStates)
		{
			State->GetNodeTemplateAs<USMStateInstance_Base>()->bAlwaysUpdate = true;
		}

		int32 EntryHits = 0; int32 UpdateHits = 0; int32 EndHits = 0;
		TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits);

		TestEqual("State Machine generated value", EntryHits, ExpectedEntryValue);
		// Last update entry is called after stopping meaning UpdateTime is 0, which we used to test updates.
		TestEqual("State Machine generated value", UpdateHits, ExpectedEntryValue - 1);
		TestEqual("State Machine generated value", EndHits, ExpectedEntryValue);
	}

	return NewAsset.DeleteAsset(this);
}

/**
 * Test a single tick vs double tick to start state and evaluate transitions.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStateNodeSingleTickTest, "SMTests.SingleTick", EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

	bool FStateNodeSingleTickTest::RunTest(const FString& Parameters)
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

	TestHelpers::BuildLinearStateMachine(this, StateMachineGraph, TotalStates, &LastStatePin);
	if (!NewAsset.SaveAsset(this))
	{
		return false;
	}
	
	TestHelpers::TestLinearStateMachine(this, NewBP, TotalStates);

	int32 ExpectedEntryValue = TotalStates;
	// Run with normal tick approach.
	{
		int32 EntryHits = 0; int32 UpdateHits = 0; int32 EndHits = 0;
		USMInstance* TestedStateMachine = TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits, 0, false, false);

		TestFalse("State machine not in last state", TestedStateMachine->IsInEndState());
		
		TestNotEqual("State Machine generated value", EntryHits, ExpectedEntryValue);
		TestNotEqual("State Machine generated value", EndHits, ExpectedEntryValue);
	}
	{
		TArray<USMGraphNode_StateNodeBase*> StateNodes;
		FSMBlueprintEditorUtils::GetAllNodesOfClassNested<USMGraphNode_StateNodeBase>(StateMachineGraph, StateNodes);
		StateNodes[0]->GetNodeTemplateAs<USMStateInstance_Base>()->bEvalTransitionsOnStart = true;
		
		int32 EntryHits = 0; int32 UpdateHits = 0; int32 EndHits = 0;
		USMInstance* TestedStateMachine = TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits, 0, true, true);

		TestTrue("State machine in last state", TestedStateMachine->IsInEndState());

		TestEqual("State Machine generated value", EntryHits, ExpectedEntryValue);
		TestEqual("State Machine generated value", EndHits, ExpectedEntryValue);

		// Test custom transition values.
		USMGraphNode_TransitionEdge* Transition = StateNodes[0]->GetNextTransition(0);
		Transition->GetNodeTemplateAs<USMTransitionInstance>()->bCanEvalWithStartState = false;
		TestedStateMachine = TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits, 0, false, false);

		TestFalse("State machine in last state", TestedStateMachine->IsInEndState());

		TestNotEqual("State Machine generated value", EntryHits, ExpectedEntryValue);
		TestNotEqual("State Machine generated value", EndHits, ExpectedEntryValue);
	}
	// Test larger on same tick
	{
		FSMBlueprintEditorUtils::RemoveAllNodesFromGraph(StateMachineGraph);
		ExpectedEntryValue = TotalStates = 10;
		
		LastStatePin = nullptr;
		TestHelpers::BuildLinearStateMachine(this, StateMachineGraph, TotalStates, &LastStatePin);

		TArray<USMGraphNode_StateNodeBase*> StateNodes;
		FSMBlueprintEditorUtils::GetAllNodesOfClassNested<USMGraphNode_StateNodeBase>(StateMachineGraph, StateNodes);

		for (USMGraphNode_StateNodeBase* Node : StateNodes)
		{
			Node->GetNodeTemplateAs<USMStateInstance_Base>()->bEvalTransitionsOnStart = true;
		}
		
		int32 EntryHits = 0; int32 UpdateHits = 0; int32 EndHits = 0;
		USMInstance* TestedStateMachine = TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits, 0, true, true);

		TestTrue("State machine in last state", TestedStateMachine->IsInEndState());

		TestEqual("State Machine generated value", EntryHits, ExpectedEntryValue);
		TestEqual("State Machine generated value", EndHits, ExpectedEntryValue);
	}
	
	return NewAsset.DeleteAsset(this);
}

/**
 * Test scenario where state machine has duplicate state and transition runtime guids and that they are properly fixed.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDuplicateRuntimeNodeTest, "SMTests.DuplicateRuntimeNode", EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

	bool FDuplicateRuntimeNodeTest::RunTest(const FString& Parameters)
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
	int32 TotalStates = 5;

	UEdGraphPin* LastStatePin = nullptr;

	TestHelpers::BuildLinearStateMachine(this, StateMachineGraph, TotalStates, &LastStatePin);
	if (!NewAsset.SaveAsset(this))
	{
		return false;
	}
	int32 TotalDuplicated = FSMBlueprintEditorUtils::FixUpDuplicateRuntimeGuids(NewBP);

	TestEqual("No duplicates", TotalDuplicated, 0);

	// Set duplicate state nodes.
	{
		TArray<USMGraphNode_StateNodeBase*> StateNodes;
		FSMBlueprintEditorUtils::GetAllNodesOfClassNested<USMGraphNode_StateNodeBase>(StateMachineGraph, StateNodes);
		FSMNode_Base* OriginalRuntimeNode = FSMBlueprintEditorUtils::GetRuntimeNodeFromGraph(StateNodes[0]->GetBoundGraph());

		FSMNode_Base* DupeRuntimeNode = FSMBlueprintEditorUtils::GetRuntimeNodeFromGraph(StateNodes[1]->GetBoundGraph());
		DupeRuntimeNode->SetNodeGuid(OriginalRuntimeNode->GetNodeGuid());
		FSMBlueprintEditorUtils::UpdateRuntimeNodeForGraph(DupeRuntimeNode, StateNodes[1]->GetBoundGraph());
	}

	// Set duplicate transition nodes.
	{
		TArray<USMGraphNode_TransitionEdge*> TransitionNodes;
		FSMBlueprintEditorUtils::GetAllNodesOfClassNested<USMGraphNode_TransitionEdge>(StateMachineGraph, TransitionNodes);
		FSMNode_Base* OriginalRuntimeNode = FSMBlueprintEditorUtils::GetRuntimeNodeFromGraph(TransitionNodes[0]->GetBoundGraph());

		FSMNode_Base* DupeRuntimeNode = FSMBlueprintEditorUtils::GetRuntimeNodeFromGraph(TransitionNodes[1]->GetBoundGraph());
		DupeRuntimeNode->SetNodeGuid(OriginalRuntimeNode->GetNodeGuid());
		FSMBlueprintEditorUtils::UpdateRuntimeNodeForGraph(DupeRuntimeNode, TransitionNodes[1]->GetBoundGraph());

		DupeRuntimeNode = FSMBlueprintEditorUtils::GetRuntimeNodeFromGraph(TransitionNodes[3]->GetBoundGraph());
		DupeRuntimeNode->SetNodeGuid(OriginalRuntimeNode->GetNodeGuid());
		FSMBlueprintEditorUtils::UpdateRuntimeNodeForGraph(DupeRuntimeNode, TransitionNodes[3]->GetBoundGraph());
	}

	TotalDuplicated = FSMBlueprintEditorUtils::FixUpDuplicateRuntimeGuids(NewBP);
	TestEqual("Duplicates", TotalDuplicated, 3);

	TotalDuplicated = FSMBlueprintEditorUtils::FixUpDuplicateRuntimeGuids(NewBP);
	TestEqual("All duplicates fixed", TotalDuplicated, 0);
	
	TestHelpers::TestLinearStateMachine(this, NewBP, TotalStates);

	int32 ExpectedEntryValue = TotalStates;
	{
		int32 EntryHits = 0; int32 UpdateHits = 0; int32 EndHits = 0;
		USMInstance* TestedStateMachine = TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits, 0, false, false);

		TestFalse("State machine not in last state", TestedStateMachine->IsInEndState());

		TestNotEqual("State Machine generated value", EntryHits, ExpectedEntryValue);
		TestNotEqual("State Machine generated value", EndHits, ExpectedEntryValue);
	}

	// Set more this time and test fix using BP compile instead.
	
	// Set duplicate state nodes.
	{
		TArray<USMGraphNode_StateNodeBase*> StateNodes;
		FSMBlueprintEditorUtils::GetAllNodesOfClassNested<USMGraphNode_StateNodeBase>(StateMachineGraph, StateNodes);
		FSMNode_Base* OriginalRuntimeNode = FSMBlueprintEditorUtils::GetRuntimeNodeFromGraph(StateNodes[0]->GetBoundGraph());

		for (int32 i = 1; i < TotalStates; ++i)
		{
			FSMNode_Base* DupeRuntimeNode = FSMBlueprintEditorUtils::GetRuntimeNodeFromGraph(StateNodes[i]->GetBoundGraph());
			DupeRuntimeNode->SetNodeGuid(OriginalRuntimeNode->GetNodeGuid());
			FSMBlueprintEditorUtils::UpdateRuntimeNodeForGraph(DupeRuntimeNode, StateNodes[i]->GetBoundGraph());
		}
	}

	// Set duplicate transition nodes.
	{
		TArray<USMGraphNode_TransitionEdge*> TransitionNodes;
		FSMBlueprintEditorUtils::GetAllNodesOfClassNested<USMGraphNode_TransitionEdge>(StateMachineGraph, TransitionNodes);
		FSMNode_Base* OriginalRuntimeNode = FSMBlueprintEditorUtils::GetRuntimeNodeFromGraph(TransitionNodes[0]->GetBoundGraph());

		for (int32 i = 1; i < TotalStates - 1; ++i)
		{
			FSMNode_Base* DupeRuntimeNode = FSMBlueprintEditorUtils::GetRuntimeNodeFromGraph(TransitionNodes[i]->GetBoundGraph());
			DupeRuntimeNode->SetNodeGuid(OriginalRuntimeNode->GetNodeGuid());
			FSMBlueprintEditorUtils::UpdateRuntimeNodeForGraph(DupeRuntimeNode, TransitionNodes[i]->GetBoundGraph());
		}
	}

	// This will compile BP.
	AddExpectedError("has duplicate runtime GUID with", EAutomationExpectedErrorFlags::Contains, 7);
	TestHelpers::TestLinearStateMachine(this, NewBP, TotalStates);
	{
		int32 EntryHits = 0; int32 UpdateHits = 0; int32 EndHits = 0;
		USMInstance* TestedStateMachine = TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits, 0, false, false);

		TestFalse("State machine not in last state", TestedStateMachine->IsInEndState());

		TestNotEqual("State Machine generated value", EntryHits, ExpectedEntryValue);
		TestNotEqual("State Machine generated value", EndHits, ExpectedEntryValue);
	}
	
	TotalDuplicated = FSMBlueprintEditorUtils::FixUpDuplicateRuntimeGuids(NewBP);
	TestEqual("All duplicates fixed", TotalDuplicated, 0);

	return NewAsset.DeleteAsset(this);
}


bool TestParentStateMachines(FAutomationTestBase* Test, int32 NumParentsCallsInChild = 1, int32 NumChildCallsInGrandChild = 1, int32 NumGrandChildCallsInReference = 1)
{
	FAssetHandler ParentAsset;
	if (!TestHelpers::TryCreateNewStateMachineAsset(Test, ParentAsset, false))
	{
		return false;
	}

	int32 TotalParentStates = 3;

	USMBlueprint* ParentBP = ParentAsset.GetObjectAs<USMBlueprint>();
	{
		// Find root state machine.
		USMGraphK2Node_StateMachineNode* ParentRootStateMachineNode = FSMBlueprintEditorUtils::GetRootStateMachineNode(ParentBP);

		// Find the state machine graph.
		USMGraph* ParentStateMachineGraph = ParentRootStateMachineNode->GetStateMachineGraph();

		UEdGraphPin* LastParentTopLevelStatePin = nullptr;

		// Build single state - state machine.
		TestHelpers::BuildLinearStateMachine(Test, ParentStateMachineGraph, TotalParentStates, &LastParentTopLevelStatePin);
		if (!ParentAsset.SaveAsset(Test))
		{
			return false;
		}
		USMGraphNode_TransitionEdge* TransitionEdge =
			CastChecked<USMGraphNode_TransitionEdge>(Cast<USMGraphNode_StateNode>(LastParentTopLevelStatePin->GetOwningNode())->GetInputPin()->LinkedTo[0]->GetOwningNode());

		// Event check so we can test if the parent machine was triggered.
		TestHelpers::AddEventWithLogic<USMGraphK2Node_TransitionEnteredNode>(Test, TransitionEdge,
			USMTestContext::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(USMTestContext, IncreaseTransitionTaken)));

		FKismetEditorUtilities::CompileBlueprint(ParentBP);
	}

	FAssetHandler ChildAsset;
	if (!TestHelpers::TryCreateNewStateMachineAsset(Test, ChildAsset, false))
	{
		return false;
	}
	int32 TotalChildStates = 1 + (NumParentsCallsInChild * 2);
	USMBlueprint* ChildBP = ChildAsset.GetObjectAs<USMBlueprint>();
	{
		ChildBP->ParentClass = ParentBP->GetGeneratedClass();

		// Find root state machine.
		USMGraphK2Node_StateMachineNode* RootStateMachineNode = FSMBlueprintEditorUtils::GetRootStateMachineNode(ChildBP);

		// Find the state machine graph.
		USMGraph* StateMachineGraph = RootStateMachineNode->GetStateMachineGraph();

		UEdGraphPin* LastTopLevelStatePin = nullptr;

		// Build single state - state machine.
		TestHelpers::BuildLinearStateMachine(Test, StateMachineGraph, 1, &LastTopLevelStatePin, USMStateTestInstance::StaticClass(), USMTransitionTestInstance::StaticClass());
		if (!ChildAsset.SaveAsset(Test))
		{
			return false;
		}

		for (int32 i = 0; i < NumParentsCallsInChild; ++i)
		{
			USMGraphNode_StateMachineParentNode* ParentNode = TestHelpers::CreateNewNode<USMGraphNode_StateMachineParentNode>(Test, StateMachineGraph, LastTopLevelStatePin);

			Test->TestNotNull("Parent Node created", ParentNode);
			Test->TestEqual("Correct parent class defaulted", Cast<USMBlueprintGeneratedClass>(ParentNode->ParentClass), ParentBP->GetGeneratedClass());
			ParentNode->GetNodeTemplateAs<USMStateMachineInstance>()->bReuseCurrentState = true;
			
			TArray<USMBlueprintGeneratedClass*> ParentClasses;
			FSMBlueprintEditorUtils::TryGetParentClasses(ChildBP, ParentClasses);

			Test->TestTrue("Correct parent class found for child", ParentClasses.Num() == 1 && ParentClasses[0] == ParentBP->GetGeneratedClass());

			// Transition before parent node.
			USMGraphNode_TransitionEdge* TransitionToParent = CastChecked<USMGraphNode_TransitionEdge>(ParentNode->GetInputPin()->LinkedTo[0]->GetOwningNode());
			TestHelpers::AddTransitionResultLogic(Test, TransitionToParent);

			// Add one more state so we can wait for the parent to complete.
			LastTopLevelStatePin = ParentNode->GetOutputPin();

			TestHelpers::BuildLinearStateMachine(Test, StateMachineGraph, 1, &LastTopLevelStatePin);
			{
				// Signal the state after the first nested state machine to wait for its completion.
				USMGraphNode_TransitionEdge* TransitionFromParent = CastChecked<USMGraphNode_TransitionEdge>(ParentNode->GetOutputPin()->LinkedTo[0]->GetOwningNode());
				TestHelpers::OverrideTransitionResultLogic<USMGraphK2Node_StateMachineReadNode_InEndState>(Test, TransitionFromParent);
			}
		}

		int32 EntryHits = 0; int32 UpdateHits = 0; int32 EndHits = 0;
		USMInstance* TestedStateMachine = TestHelpers::RunStateMachineToCompletion(Test, ChildBP, EntryHits, UpdateHits, EndHits);

		int32 ExpectedHits = (TotalParentStates * NumParentsCallsInChild) + TotalChildStates - NumParentsCallsInChild; // No hit for parent node, but instead for each node in parent.

		Test->TestTrue("State machine in last state", TestedStateMachine->IsInEndState());

		Test->TestEqual("State Machine generated value", EntryHits, ExpectedHits);
		Test->TestEqual("State Machine generated value", EndHits, ExpectedHits);
		USMTestContext* Context = Cast<USMTestContext>(TestedStateMachine->GetContext());

		Test->TestEqual("State Machine parent state hit", Context->TestTransitionEntered.Count, NumParentsCallsInChild);
	}

	// Test empty parent
	{
		USMGraphK2Node_StateMachineNode* ParentRootStateMachineNode = FSMBlueprintEditorUtils::GetRootStateMachineNode(ParentBP);
		USMGraphK2Node_StateMachineSelectNode* CachedOutputNode = CastChecked<USMGraphK2Node_StateMachineSelectNode>(ParentRootStateMachineNode->GetOutputNode());
		ParentRootStateMachineNode->BreakAllNodeLinks();

		Test->AddExpectedError("is not connected to any state machine", EAutomationExpectedErrorFlags::Contains, 0);
		Test->AddExpectedError("has no root state machine graph in parent", EAutomationExpectedErrorFlags::Contains, 0); // Will be hit for every time compiled.
		FKismetEditorUtilities::CompileBlueprint(ParentBP);

		int32 ExpectedHits = TotalChildStates - NumParentsCallsInChild; // Empty parent node.
		int32 EntryHits = 0; int32 UpdateHits = 0; int32 EndHits = 0;

		USMInstance* TestedStateMachine = TestHelpers::RunStateMachineToCompletion(Test, ChildBP, EntryHits, UpdateHits, EndHits);
		
		Test->TestEqual("State Machine generated value", EntryHits, ExpectedHits);
		Test->TestEqual("State Machine generated value", EndHits, ExpectedHits);
		USMTestContext* Context = Cast<USMTestContext>(TestedStateMachine->GetContext());

		Test->TestEqual("State Machine parent state hit", Context->TestTransitionEntered.Count, 0);

		// Re-establish link.
		ParentRootStateMachineNode->GetSchema()->TryCreateConnection(ParentRootStateMachineNode->GetOutputPin(), CachedOutputNode->GetInputPin());
		FKismetEditorUtilities::CompileBlueprint(ParentBP);
		// Re-run original test.
		{
			TestedStateMachine = TestHelpers::RunStateMachineToCompletion(Test, ChildBP, EntryHits, UpdateHits, EndHits);

			ExpectedHits = (TotalParentStates * NumParentsCallsInChild) + TotalChildStates - NumParentsCallsInChild; // No hit for parent node, but instead for each node in parent.

			Test->TestTrue("State machine in last state", TestedStateMachine->IsInEndState());
			Test->TestEqual("State Machine generated value", EntryHits, ExpectedHits);
			Test->TestEqual("State Machine generated value", EndHits, ExpectedHits);
			Context = Cast<USMTestContext>(TestedStateMachine->GetContext());

			Test->TestEqual("State Machine parent state hit", Context->TestTransitionEntered.Count, NumParentsCallsInChild);
		}
	}

	// Create GrandChild
	FAssetHandler GrandChildAsset;
	if (!TestHelpers::TryCreateNewStateMachineAsset(Test, GrandChildAsset, false))
	{
		return false;
	}

	int32 TotalGrandChildStates = 1 + (NumChildCallsInGrandChild * 2);
	USMBlueprint* GrandChildBP = GrandChildAsset.GetObjectAs<USMBlueprint>();
	{
		GrandChildBP->ParentClass = ChildBP->GetGeneratedClass();

		// Find root state machine.
		USMGraphK2Node_StateMachineNode* RootStateMachineNode = FSMBlueprintEditorUtils::GetRootStateMachineNode(GrandChildBP);

		// Find the state machine graph.
		USMGraph* StateMachineGraph = RootStateMachineNode->GetStateMachineGraph();

		UEdGraphPin* LastTopLevelStatePin = nullptr;

		// Build single state - state machine.
		TestHelpers::BuildLinearStateMachine(Test, StateMachineGraph, 1, &LastTopLevelStatePin);
		if (!GrandChildAsset.SaveAsset(Test))
		{
			return false;
		}

		for (int32 i = 0; i < NumChildCallsInGrandChild; ++i)
		{
			USMGraphNode_StateMachineParentNode* ParentNode = TestHelpers::CreateNewNode<USMGraphNode_StateMachineParentNode>(Test, StateMachineGraph, LastTopLevelStatePin);
			ParentNode->GetNodeTemplateAs<USMStateMachineInstance>()->bReuseCurrentState = true;
			
			Test->TestNotNull("Parent Node created", ParentNode);
			Test->TestEqual("Correct parent class defaulted", Cast<USMBlueprintGeneratedClass>(ParentNode->ParentClass), ChildBP->GetGeneratedClass());

			TArray<USMBlueprintGeneratedClass*> ParentClasses;
			FSMBlueprintEditorUtils::TryGetParentClasses(GrandChildBP, ParentClasses);

			Test->TestTrue("Correct parent class found for child", ParentClasses.Num() == 2 && ParentClasses.Contains(ChildBP->GetGeneratedClass()) && ParentClasses.Contains(ParentBP->GetGeneratedClass()));

			// Transition before parent node.
			USMGraphNode_TransitionEdge* TransitionToParent = CastChecked<USMGraphNode_TransitionEdge>(ParentNode->GetInputPin()->LinkedTo[0]->GetOwningNode());
			TestHelpers::AddTransitionResultLogic(Test, TransitionToParent);

			// Add one more state so we can wait for the parent to complete.
			LastTopLevelStatePin = ParentNode->GetOutputPin();

			TestHelpers::BuildLinearStateMachine(Test, StateMachineGraph, 1, &LastTopLevelStatePin, USMStateTestInstance::StaticClass(), USMTransitionTestInstance::StaticClass());
			{
				// Signal the state after the first nested state machine to wait for its completion.
				USMGraphNode_TransitionEdge* TransitionFromParent = CastChecked<USMGraphNode_TransitionEdge>(ParentNode->GetOutputPin()->LinkedTo[0]->GetOwningNode());
				TestHelpers::OverrideTransitionResultLogic<USMGraphK2Node_StateMachineReadNode_InEndState>(Test, TransitionFromParent);
			}
		}

		int32 EntryHits = 0; int32 UpdateHits = 0; int32 EndHits = 0;
		USMInstance* TestedStateMachine = TestHelpers::RunStateMachineToCompletion(Test, GrandChildBP, EntryHits, UpdateHits, EndHits);

		int32 ExpectedHits = ((TotalParentStates * NumParentsCallsInChild) + (TotalChildStates - NumParentsCallsInChild)) * NumChildCallsInGrandChild + TotalGrandChildStates - NumChildCallsInGrandChild; // No hit for either parent nodes, but instead for each node in parent.

		Test->TestTrue("State machine in last state", TestedStateMachine->IsInEndState());

		Test->TestEqual("State Machine generated value", EntryHits, ExpectedHits);
		Test->TestEqual("State Machine generated value", EndHits, ExpectedHits);
		USMTestContext* Context = Cast<USMTestContext>(TestedStateMachine->GetContext());

		Test->TestEqual("State Machine parent state hit", Context->TestTransitionEntered.Count, NumParentsCallsInChild * NumChildCallsInGrandChild); // From grand parent.

		// Test maintaining node guids that were generated from being duplicates.
		{
			TestedStateMachine = TestHelpers::CreateNewStateMachineInstanceFromBP(Test, GrandChildBP, Context);
			TMap<FGuid, FSMNode_Base*> OldNodeMap = TestedStateMachine->GetNodeMap();

			// Recompile which recalculate NodeGuids on duped nodes.
			FKismetEditorUtilities::CompileBlueprint(GrandChildBP);
			Context = NewObject<USMTestContext>();
			TestedStateMachine = TestHelpers::CreateNewStateMachineInstanceFromBP(Test, GrandChildBP, Context);

			const TMap<FGuid, FSMNode_Base*>& NewNodeMap = TestedStateMachine->GetNodeMap();
			for (auto& KeyVal : NewNodeMap)
			{
				Test->TestTrue("Dupe node guids haven't changed", OldNodeMap.Contains(KeyVal.Key));
			}
		}

		// Test saving / restoring
		{
			for (int32 i = 0; i < ExpectedHits; ++i)
			{
				TestedStateMachine = TestHelpers::RunStateMachineToCompletion(Test, GrandChildBP, EntryHits, UpdateHits, EndHits, i, true, false, false);
				TArray<FGuid> CurrentGuids;
				TestedStateMachine->GetAllActiveStateGuids(CurrentGuids);

				TestedStateMachine = TestHelpers::CreateNewStateMachineInstanceFromBP(Test, GrandChildBP, Context);
				TestedStateMachine->LoadFromMultipleStates(CurrentGuids);

				TArray<FGuid> ReloadedGuids;
				TestedStateMachine->GetAllActiveStateGuids(ReloadedGuids);

				int32 Match = TestHelpers::ArrayContentsInArray(ReloadedGuids, CurrentGuids);
				Test->TestEqual("State machine states reloaded", Match, CurrentGuids.Num());
			}
		}
	}

	// Test creating reference to the grand child
	FAssetHandler ReferenceAsset;
	if (!TestHelpers::TryCreateNewStateMachineAsset(Test, ReferenceAsset, false))
	{
		return false;
	}
	int32 TotalRefStates = 1 + NumGrandChildCallsInReference * 2;
	USMBlueprint* RefrenceBP = ReferenceAsset.GetObjectAs<USMBlueprint>();
	{
		// Find root state machine.
		USMGraphK2Node_StateMachineNode* RootStateMachineNode = FSMBlueprintEditorUtils::GetRootStateMachineNode(RefrenceBP);

		// Find the state machine graph.
		USMGraph* StateMachineGraph = RootStateMachineNode->GetStateMachineGraph();

		UEdGraphPin* LastTopLevelStatePin = nullptr;

		// Build single state - state machine.
		TestHelpers::BuildLinearStateMachine(Test, StateMachineGraph, 1, &LastTopLevelStatePin);
		if (!ChildAsset.SaveAsset(Test))
		{
			return false;
		}

		for (int32 i = 0; i < NumGrandChildCallsInReference; ++i)
		{
			USMGraphNode_StateMachineStateNode* ReferenceNode = TestHelpers::CreateNewNode<USMGraphNode_StateMachineStateNode>(Test, StateMachineGraph, LastTopLevelStatePin);
			ReferenceNode->ReferenceStateMachine(GrandChildBP);
			ReferenceNode->GetNodeTemplateAs<USMStateMachineInstance>()->bReuseCurrentState = true;

			// Transition before reference node.
			{
				USMGraphNode_TransitionEdge* TransitionToReference = CastChecked<USMGraphNode_TransitionEdge>(ReferenceNode->GetInputPin()->LinkedTo[0]->GetOwningNode());
				TestHelpers::AddTransitionResultLogic(Test, TransitionToReference);
			}
			
			// Add one more state so we can wait for the reference to complete.
			LastTopLevelStatePin = ReferenceNode->GetOutputPin();

			TestHelpers::BuildLinearStateMachine(Test, StateMachineGraph, 1, &LastTopLevelStatePin);
			{
				// Signal the state after the first nested state machine to wait for its completion.
				USMGraphNode_TransitionEdge* TransitionFromReference = CastChecked<USMGraphNode_TransitionEdge>(ReferenceNode->GetOutputPin()->LinkedTo[0]->GetOwningNode());
				TestHelpers::OverrideTransitionResultLogic<USMGraphK2Node_StateMachineReadNode_InEndState>(Test, TransitionFromReference);
			}
		}

		int32 EntryHits = 0; int32 UpdateHits = 0; int32 EndHits = 0;
		USMInstance* TestedStateMachine = TestHelpers::RunStateMachineToCompletion(Test, RefrenceBP, EntryHits, UpdateHits, EndHits);

		int32 ExpectedHits = (((TotalParentStates * NumParentsCallsInChild) + (TotalChildStates - NumParentsCallsInChild)) * NumChildCallsInGrandChild + TotalGrandChildStates - NumChildCallsInGrandChild) *
			NumGrandChildCallsInReference + TotalRefStates - NumGrandChildCallsInReference;

		Test->TestTrue("State machine in last state", TestedStateMachine->IsInEndState());

		Test->TestEqual("State Machine generated value", EntryHits, ExpectedHits);
		Test->TestEqual("State Machine generated value", EndHits, ExpectedHits);
		USMTestContext* Context = Cast<USMTestContext>(TestedStateMachine->GetContext());

		Test->TestEqual("State Machine parent state hit", Context->TestTransitionEntered.Count, NumParentsCallsInChild * NumChildCallsInGrandChild * NumGrandChildCallsInReference);


		// Test saving / restoring
		{
			for (int32 i = 0; i < ExpectedHits; ++i)
			{
				TestedStateMachine = TestHelpers::RunStateMachineToCompletion(Test, RefrenceBP, EntryHits, UpdateHits, EndHits, i, true, false, false);
				TArray<FGuid> CurrentGuids;
				TestedStateMachine->GetAllActiveStateGuids(CurrentGuids);

				TestedStateMachine = TestHelpers::CreateNewStateMachineInstanceFromBP(Test, RefrenceBP, Context);
				TestedStateMachine->LoadFromMultipleStates(CurrentGuids);

				TArray<FGuid> ReloadedGuids;
				TestedStateMachine->GetAllActiveStateGuids(ReloadedGuids);

				int32 Match = TestHelpers::ArrayContentsInArray(ReloadedGuids, CurrentGuids);
				Test->TestEqual("State machine states reloaded", Match, CurrentGuids.Num());
			}
		}
	}

	ReferenceAsset.DeleteAsset(Test);
	GrandChildAsset.DeleteAsset(Test);
	ChildAsset.DeleteAsset(Test);
	return ParentAsset.DeleteAsset(Test);
}

/**
 * Assemble and run a hierarchical state machine.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStateMachineParentTest, "SMTests.StateMachineParent", EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

	bool FStateMachineParentTest::RunTest(const FString& Parameters)
{
	for(int32 c = 1; c < 4; ++c)
	{
		for(int32 gc = 1; gc < 4; ++gc)
		{
			for (int32 ref = 1; ref < 4; ++ref)
			{
				if (!TestParentStateMachines(this, c, gc, ref))
				{
					return false;
				}
			}
		}
	}

	return true;
}

bool TestSaveStateMachineState(FAutomationTestBase* Test, bool bCreateReferences, bool bReuseStates, bool bCreateIntermediateReferenceGraphs = false, bool bReuseReferences = false)
{
	FAssetHandler NewAsset;
	if (!TestHelpers::TryCreateNewStateMachineAsset(Test, NewAsset, false))
	{
		return false;
	}

	USMBlueprint* NewBP = NewAsset.GetObjectAs<USMBlueprint>();
	
	// Find root state machine.
	USMGraphK2Node_StateMachineNode* RootStateMachineNode = FSMBlueprintEditorUtils::GetRootStateMachineNode(NewBP);

	// Find the state machine graph.
	USMGraph* StateMachineGraph = RootStateMachineNode->GetStateMachineGraph();

	UEdGraphPin* LastTopLevelStatePin = nullptr;

	// Build single state - state machine.
	TestHelpers::BuildLinearStateMachine(Test, StateMachineGraph, 2, &LastTopLevelStatePin);
	if (!NewAsset.SaveAsset(Test))
	{
		return false;
	}

	// Used to keep track of nested state machines and to convert them to references.
	TArray<USMGraphNode_StateMachineStateNode*> StateMachineStateNodes;

	// Build a top level state machine node. When converting to references this will be replaced with a copy of the next reference.
	// Don't add to StateMachineStateNodes list yet this will have special handling.
	UEdGraphPin* EntryPointForNestedStateMachine = LastTopLevelStatePin;
	USMGraphNode_StateMachineStateNode* NestedStateMachineNodeToUseDuplicateReference = TestHelpers::BuildNestedStateMachine(Test, StateMachineGraph, 4, &EntryPointForNestedStateMachine, nullptr);
	NestedStateMachineNodeToUseDuplicateReference->GetNodeTemplateAs<USMStateMachineInstance>()->bReuseCurrentState = bReuseStates;
	NestedStateMachineNodeToUseDuplicateReference->bReuseReference = bReuseReferences;
	LastTopLevelStatePin = NestedStateMachineNodeToUseDuplicateReference->GetOutputPin();

	// Build a nested state machine.

	UEdGraphPin* LastNestedPin = nullptr;
	EntryPointForNestedStateMachine = LastTopLevelStatePin;
	USMGraphNode_StateMachineStateNode* NestedStateMachineNode = TestHelpers::BuildNestedStateMachine(Test, StateMachineGraph, 4, &EntryPointForNestedStateMachine, &LastNestedPin);
	NestedStateMachineNode->GetNodeTemplateAs<USMStateMachineInstance>()->bReuseCurrentState = bReuseStates;
	NestedStateMachineNode->bReuseReference = bReuseReferences;
	StateMachineStateNodes.Add(NestedStateMachineNode);
	LastTopLevelStatePin = NestedStateMachineNode->GetOutputPin();

	{
		// Signal the state after the first nested state machine to wait for its completion.
		USMGraphNode_TransitionEdge* TransitionFromNestedStateMachine = CastChecked<USMGraphNode_TransitionEdge>(NestedStateMachineNodeToUseDuplicateReference->GetOutputPin()->LinkedTo[0]->GetOwningNode());
		TestHelpers::OverrideTransitionResultLogic<USMGraphK2Node_StateMachineReadNode_InEndState>(Test, TransitionFromNestedStateMachine);
	}
	
	// Add two second level nested state machines.
	UEdGraphPin* EntryPointFor2xNested = LastNestedPin;
	USMGraphNode_StateMachineStateNode* NestedStateMachineNode_2 = TestHelpers::BuildNestedStateMachine(Test, Cast<USMGraph>(NestedStateMachineNode->GetBoundGraph()),
		4, &EntryPointFor2xNested, nullptr);
	NestedStateMachineNode_2->GetNodeTemplateAs<USMStateMachineInstance>()->bReuseCurrentState = bReuseStates;
	NestedStateMachineNode_2->bReuseReference = bReuseReferences;
	StateMachineStateNodes.Insert(NestedStateMachineNode_2, 0);
	{
		UEdGraphPin* Nested1xPinOut = NestedStateMachineNode_2->GetOutputPin();
		// Add more 1x states nested level (states leading from the second nested state machine).
		{
			TestHelpers::BuildLinearStateMachine(Test, Cast<USMGraph>(NestedStateMachineNode->GetBoundGraph()), 2, &Nested1xPinOut);
			if (!NewAsset.SaveAsset(Test))
			{
				return false;
			}

			// Signal the state after the nested state machine to wait for its completion.
			USMGraphNode_TransitionEdge* TransitionFromNestedStateMachine = CastChecked<USMGraphNode_TransitionEdge>(NestedStateMachineNode_2->GetOutputPin()->LinkedTo[0]->GetOwningNode());
			TestHelpers::OverrideTransitionResultLogic<USMGraphK2Node_StateMachineReadNode_InEndState>(Test, TransitionFromNestedStateMachine);
		}
		// Add another second level nested state machine. (Sibling to above state machine)
		{
			USMGraphNode_StateMachineStateNode* NestedStateMachineNode_2_2 = TestHelpers::BuildNestedStateMachine(Test, Cast<USMGraph>(NestedStateMachineNode->GetBoundGraph()),
				4, &Nested1xPinOut, nullptr);
			NestedStateMachineNode_2_2->GetNodeTemplateAs<USMStateMachineInstance>()->bReuseCurrentState = bReuseStates;
			NestedStateMachineNode_2_2->bReuseReference = bReuseReferences;
			StateMachineStateNodes.Insert(NestedStateMachineNode_2_2, 0);
			// Add more 1x states nested level (states leading from the second nested state machine).
			{
				UEdGraphPin* Nested1x_2PinOut = NestedStateMachineNode_2_2->GetOutputPin();
				TestHelpers::BuildLinearStateMachine(Test, Cast<USMGraph>(NestedStateMachineNode->GetBoundGraph()), 2, &Nested1x_2PinOut);
				if (!NewAsset.SaveAsset(Test))
				{
					return false;
				}

				// Signal the state after the nested state machine to wait for its completion.
				USMGraphNode_TransitionEdge* TransitionFromNestedStateMachine = CastChecked<USMGraphNode_TransitionEdge>(NestedStateMachineNode_2_2->GetOutputPin()->LinkedTo[0]->GetOwningNode());
				TestHelpers::OverrideTransitionResultLogic<USMGraphK2Node_StateMachineReadNode_InEndState>(Test, TransitionFromNestedStateMachine);
			}
		}
	}

	// Add more top level (states leading from nested state machine).
	{
		TestHelpers::BuildLinearStateMachine(Test, StateMachineGraph, 2, &LastTopLevelStatePin);
		if (!NewAsset.SaveAsset(Test))
		{
			return false;
		}

		// Signal the state after the second nested state machine to wait for its completion.
		USMGraphNode_TransitionEdge* TransitionFromNestedStateMachine = CastChecked<USMGraphNode_TransitionEdge>(NestedStateMachineNode->GetOutputPin()->LinkedTo[0]->GetOwningNode());
		TestHelpers::OverrideTransitionResultLogic<USMGraphK2Node_StateMachineReadNode_InEndState>(Test, TransitionFromNestedStateMachine);
	}

	TArray<FAssetHandler> ExtraAssets;
	const FName TestStringVarName = "TestStringVar";
	const FString DefaultStringValue = "BaseValue";
	const FString NewStringValue = "OverValue";

	int32 TotalReferences = 0;
	TSet<UClass*> GeneratedReferenceClasses;
	// Convert nested state machines to references.
	if (bCreateReferences)
	{
		// This loop has to run backwards (StateMachineStateNodes should be sorted in reverse) because if a top level sm is converted to a reference first
		// that would invalidate the nested state machine graphs and make converting to references more complicated.
		int32 Index = 0;
		for (USMGraphNode_StateMachineStateNode* NestedSM : StateMachineStateNodes)
		{
			// Now convert the state machine to a reference.
			FString AssetName = "SaveRef_" + FString::FromInt(Index);
			USMBlueprint* NewReferencedBlueprint = FSMBlueprintEditorUtils::ConvertStateMachineToReference(NestedSM, false, &AssetName, nullptr);
			Test->TestNotNull("New referenced blueprint created", NewReferencedBlueprint);
			Test->TestTrue("Nested state machine has had all nodes removed.", NestedSM->GetBoundGraph()->Nodes.Num() == 1);

			TotalReferences++;
			GeneratedReferenceClasses.Add(NewReferencedBlueprint->GeneratedClass);
			
			if(bCreateIntermediateReferenceGraphs)
			{
				NestedSM->SetUseIntermediateGraph(true);
			}

			// Add a variable to this blueprint so we can test reading it from the template.
			FEdGraphPinType StringPinType(UEdGraphSchema_K2::PC_String, NAME_None, nullptr, EPinContainerType::None, false, FEdGraphTerminalType());
			FBlueprintEditorUtils::AddMemberVariable(NewReferencedBlueprint, TestStringVarName, StringPinType, DefaultStringValue);
			
			FKismetEditorUtilities::CompileBlueprint(NewReferencedBlueprint);

			if (!bReuseReferences)
			{
				USMInstance* ReferenceTemplate = NestedSM->GetStateMachineReferenceTemplateDirect();
				Test->TestNull("Template null because it is not enabled", ReferenceTemplate);

				// Manually set and update template value. Normally checking the box will trigger the state machine reference to init.
				NestedSM->bUseTemplate = true;
				NestedSM->InitStateMachineReferenceTemplate();

				ReferenceTemplate = NestedSM->GetStateMachineReferenceTemplateDirect();
				Test->TestNotNull("Template not null because it is enabled", ReferenceTemplate);

				Test->TestNotNull("New referenced template instantiated", ReferenceTemplate);
				Test->TestEqual("Direct template has owner of nested node", ReferenceTemplate->GetOuter(), (UObject*)NestedSM);

				TestHelpers::TestSetTemplate(Test, ReferenceTemplate, DefaultStringValue, NewStringValue);
			}
			
			// Store handler information so we can delete the object.
			FString ReferencedPath = NewReferencedBlueprint->GetPathName();
			FAssetHandler ReferencedAsset(NewReferencedBlueprint->GetName(), USMBlueprint::StaticClass(), NewObject<USMBlueprintFactory>(), &ReferencedPath);
			ReferencedAsset.Object = NewReferencedBlueprint;

			UPackage* Package = FAssetData(NewReferencedBlueprint).GetPackage();
			ReferencedAsset.Package = Package;

			ExtraAssets.Add(ReferencedAsset);

			Index++;
		}

		// Replace the second nested state machine node with a copy of the first reference.
		{
			NestedStateMachineNodeToUseDuplicateReference->ReferenceStateMachine(NestedStateMachineNode->GetStateMachineReference());

			// Set template value.
			if (!bReuseReferences)
			{
				NestedStateMachineNodeToUseDuplicateReference->bUseTemplate = true;
				NestedStateMachineNodeToUseDuplicateReference->InitStateMachineReferenceTemplate();
				USMInstance* ReferenceTemplate = NestedStateMachineNodeToUseDuplicateReference->GetStateMachineReferenceTemplateDirect();
				TestHelpers::TestSetTemplate(Test, ReferenceTemplate, DefaultStringValue, NewStringValue);
			}
			
			// This is a reference which contains all of the created references so far.
			TotalReferences += StateMachineStateNodes.Num();
			
			// Now add so it can be tested.
			StateMachineStateNodes.Add(NestedStateMachineNodeToUseDuplicateReference);
		}
	}
	
	FKismetEditorUtilities::CompileBlueprint(NewBP);

	// Test running normally, then test manually evaluating transitions.
	{
		int32 EntryVal, UpdateVal, EndVal;
		TestHelpers::RunStateMachineToCompletion(Test, NewBP, EntryVal, UpdateVal, EndVal);

		USMTestContext* Context = NewObject<USMTestContext>();
		USMInstance* StateMachineInstance = TestHelpers::CreateNewStateMachineInstanceFromBP(Test, NewBP, Context);

		StateMachineInstance->Start();
		while (!StateMachineInstance->IsInEndState())
		{
			StateMachineInstance->EvaluateTransitions();
		}
		
		StateMachineInstance->Stop();

		int32 CompareEntry = Context->GetEntryInt();
		int32 CompareUpdate = Context->GetUpdateInt();
		int32 CompareEnd = Context->GetEndInt();

		Test->TestEqual("Manual transition evaluation matches normal tick", CompareEntry, EntryVal);
		Test->TestEqual("Manual transition evaluation matches normal tick", CompareUpdate, UpdateVal);
		Test->TestEqual("Manual transition evaluation matches normal tick", CompareEnd, EndVal);
	}
	
	// Now increment its states testing that active/current state retrieval works properly.
	{
		// Create a context we will run the state machine for.
		USMTestContext* Context = NewObject<USMTestContext>();
		USMInstance* StateMachineInstance = TestHelpers::CreateNewStateMachineInstanceFromBP(Test, NewBP, Context);

		// Validate instances retrievable
		TArray<USMInstance*> AllReferences = StateMachineInstance->GetAllReferencedInstances(true);
		if(!bCreateReferences)
		{
			Test->TestEqual("", AllReferences.Num(), 0);
		}
		else
		{
			TSet<UClass*> ReferenceClasses;
			for(USMInstance* Ref : AllReferences)
			{
				ReferenceClasses.Add(Ref->GetClass());
			}

			int32 Match = TestHelpers::ArrayContentsInArray(ReferenceClasses.Array(), GeneratedReferenceClasses.Array());
			Test->TestEqual("Unique reference classes found", Match, GeneratedReferenceClasses.Num());

			Test->TestEqual("All nested references found", AllReferences.Num(), bReuseReferences ? GeneratedReferenceClasses.Num() : TotalReferences);

			AllReferences = StateMachineInstance->GetAllReferencedInstances(false);
			Test->TestEqual("Direct references", AllReferences.Num(), bReuseReferences ? 1 : 2);

			// Templates should only be in CDO.
			Test->TestEqual("Instance doesn't have templates stored", StateMachineInstance->ReferenceTemplates.Num(), 0);
			
			// Validate template stored in default object correctly.
			USMInstance* DefaultObject = Cast<USMInstance>(StateMachineInstance->GetClass()->GetDefaultObject(false));
			const int32 TotalTemplates = bReuseReferences ? 0 : 2; // Reuse references don't support templates.
			Test->TestEqual("Reference template in CDO", DefaultObject->ReferenceTemplates.Num(), TotalTemplates);
			int32 TemplatesVerified = 0;
			for(UObject* TemplateObject : DefaultObject->ReferenceTemplates)
			{
				if (USMInstance* Template = Cast<USMInstance>(TemplateObject))
				{
					Test->TestEqual("Template outered to instance default object", Template->GetOuter(), (UObject*)DefaultObject);
					bool bStringDefaultValueVerified = false;
					for (TFieldIterator<FStrProperty> It(Template->GetClass(), EFieldIteratorFlags::ExcludeSuper); It; ++It)
					{
						FString* Ptr = (*It)->ContainerPtrToValuePtr<FString>(Template);
						FString StrValue = *Ptr;
						Test->TestEqual("Instance has template override string value", StrValue, NewStringValue);
						bStringDefaultValueVerified = true;
					}
					Test->TestTrue("Template has string property from template verified.", bStringDefaultValueVerified);
					TemplatesVerified++;
				}
			}
			Test->TestEqual("Templates verified in CDO", TemplatesVerified, TotalTemplates);

			if (!bReuseReferences)
			{
				// Validate template has applied default values to referenced instance.
				for (USMInstance* ReferenceInstance : AllReferences)
				{
					bool bStringDefaultValueVerified = false;
					for (TFieldIterator<FStrProperty> It(ReferenceInstance->GetClass(), EFieldIteratorFlags::ExcludeSuper); It; ++It)
					{
						FString* Ptr = (*It)->ContainerPtrToValuePtr<FString>(ReferenceInstance);
						FString StrValue = *Ptr;
						Test->TestEqual("Instance has template override string value", StrValue, NewStringValue);
						bStringDefaultValueVerified = true;
					}
					Test->TestTrue("Instance has string property from template verified.", bStringDefaultValueVerified);
				}
			}
		}
		
		Test->TestNull("No nested active state", StateMachineInstance->GetSingleNestedActiveState());

		FSMState_Base* OriginalInitialState = StateMachineInstance->GetRootStateMachine().GetSingleInitialState();
		Test->TestNotNull("Initial state set", OriginalInitialState);

		// This will test retrieving the nested active state thoroughly.
		int32 StatesHit = TestHelpers::RunAllStateMachinesToCompletion(Test, StateMachineInstance, &StateMachineInstance->GetRootStateMachine(), -1, 0);
		int32 TotalStatesHit = StatesHit;
		
		FSMState_Base* ActiveNestedState = StateMachineInstance->GetSingleNestedActiveState();
		FSMState_Base* SavedActiveState = nullptr;
		Test->TestNotNull("Active nested state not null", ActiveNestedState);
		Test->TestNotEqual("Current active nested state not equal to original", OriginalInitialState, ActiveNestedState);

		StateMachineInstance->Stop();
		ActiveNestedState = StateMachineInstance->GetSingleNestedActiveState();
		Test->TestNull("Active nested state null after stop", ActiveNestedState);

		TArray<FGuid> SavedStateGuids;

		const int32 StatesNotHit = 5;
		// Re-instantiate and abort sooner.
		{
			USMTestContext* NewContext = NewObject<USMTestContext>();
			USMInstance* NewStateMachineInstance = TestHelpers::CreateNewStateMachineInstanceFromBP(Test, NewBP, NewContext);

			OriginalInitialState = NewStateMachineInstance->GetRootStateMachine().GetSingleInitialState();

			NewStateMachineInstance->Start();
			Test->TestTrue("State Machine should have started", NewStateMachineInstance->IsActive());

			// Run but not through all states.
			TestHelpers::RunAllStateMachinesToCompletion(Test, NewStateMachineInstance, &NewStateMachineInstance->GetRootStateMachine(), StatesHit - StatesNotHit, -1);
			SavedActiveState = ActiveNestedState = NewStateMachineInstance->GetSingleNestedActiveState();
			Test->TestNotEqual("Nested state shouldn't equal original state", ActiveNestedState, OriginalInitialState);

			// Top level, nested_1 (exited already) nested_2, nested_2_1 (exited already), nested_2_2
			NewStateMachineInstance->GetAllActiveStateGuids(SavedStateGuids);

			// One state machine is inactive so the states restored depend on if the current state is reused. Reusing states increases count of second nested state machine.
			// First nested state machine is replaced with a reference to second nested state machine when using references.
			int32 ExpectedStates = 3;
			if(bReuseStates)
			{
				ExpectedStates += 2;
				if(bCreateReferences)
				{
					if(bReuseReferences)
					{
						ExpectedStates -= 1; // Can't save properly since guids are duplicated.
					}
					else
					{
						ExpectedStates += 2;
					}
				}
			}
			Test->TestEqual("Current states tracked", SavedStateGuids.Num(), ExpectedStates);
		}

		// Re-instantiate and restore state.
		{
			USMTestContext* NewContext = NewObject<USMTestContext>();
			USMInstance* NewStateMachineInstance = TestHelpers::CreateNewStateMachineInstanceFromBP(Test, NewBP, NewContext);

			OriginalInitialState = NewStateMachineInstance->GetRootStateMachine().GetSingleInitialState();

			// Should be able to locate this instance's active state struct.
			ActiveNestedState = NewStateMachineInstance->FindStateByGuid(ActiveNestedState->GetGuid());
			Test->TestNotNull("Active state found by guid", ActiveNestedState);

			// Restore the state machine active state.
			NewStateMachineInstance->LoadFromState(ActiveNestedState->GetGuid());
			bool bIsStateMachine = NewStateMachineInstance->GetRootStateMachine().GetSingleInitialState()->IsStateMachine();
			Test->TestTrue("Initial top level state should be state machine", bIsStateMachine);
			if(!bIsStateMachine)
			{
				return false;
			}
			
			ActiveNestedState = ((FSMStateMachine*)NewStateMachineInstance->GetRootStateMachine().GetSingleInitialState())->FindState(ActiveNestedState->GetGuid());
			Test->TestNotNull("Active state found by guid from within top level initial state.", ActiveNestedState);

			NewStateMachineInstance->Start();
			Test->TestTrue("State Machine should have started", NewStateMachineInstance->IsActive());
			Test->TestEqual("The first state to start should be equal to the previous saved active state", NewStateMachineInstance->GetSingleNestedActiveState(), ActiveNestedState);

			// Run to the very last state. References won't have states remaining and non references will have the end state left.
			StatesHit = TestHelpers::RunAllStateMachinesToCompletion(Test, NewStateMachineInstance, &NewStateMachineInstance->GetRootStateMachine(), -1, -1);

			// No point of testing with reusing references. It won't be guaranteed to work properly as the Guid Path needs to be unique for each reference. The state that loads
			// may be the wrong 'instance' of this state.
			if (!bReuseReferences)
			{
				Test->TestEqual("Correct number of states hit", StatesHit, StatesNotHit);
			}
			
			ActiveNestedState = NewStateMachineInstance->GetSingleNestedActiveState();
			Test->TestNotEqual("Nested state shouldn't equal initial state", ActiveNestedState, NewStateMachineInstance->GetRootStateMachine().GetSingleInitialState());
		}

		// Re-instantiate and restore all states.
		{
			USMTestContext* NewContext = NewObject<USMTestContext>();
			USMInstance* NewStateMachineInstance = TestHelpers::CreateNewStateMachineInstanceFromBP(Test, NewBP, NewContext);

			OriginalInitialState = NewStateMachineInstance->GetRootStateMachine().GetSingleInitialState();

			TArray<FGuid> OriginalStateGuids;
			NewStateMachineInstance->GetAllActiveStateGuids(OriginalStateGuids);

			int32 Match = TestHelpers::ArrayContentsInArray(OriginalStateGuids, SavedStateGuids);
			Test->TestEqual("Original guids don't match saved guids", Match, 0);

			// Should be able to locate this instance's active state struct.
			SavedActiveState = NewStateMachineInstance->FindStateByGuid(SavedActiveState->GetGuid());
			Test->TestNotNull("Active state found by guid", SavedActiveState);

			// Restore the state machine states.
			NewStateMachineInstance->LoadFromMultipleStates(SavedStateGuids);
			Test->TestTrue("Initial top level state should be state machine", NewStateMachineInstance->GetRootStateMachine().GetSingleInitialState()->IsStateMachine());

			SavedActiveState = ((FSMStateMachine*)NewStateMachineInstance->GetRootStateMachine().GetSingleInitialState())->FindState(SavedActiveState->GetGuid());
			Test->TestNotNull("Active state found by guid from within top level initial state.", SavedActiveState);

			NewStateMachineInstance->Start();
			Test->TestTrue("State Machine should have started", NewStateMachineInstance->IsActive());
			Test->TestEqual("The first state to start should be equal to the previous saved active state", NewStateMachineInstance->GetSingleNestedActiveState(), SavedActiveState);

			// Validate all states restored.
			Match = TestHelpers::ArrayContentsInArray(NewStateMachineInstance->GetAllActiveStateGuidsCopy(), SavedStateGuids);
			Test->TestEqual("Restored guids match saved guids", Match, SavedStateGuids.Num());

			// Run to the very last state.
			StatesHit = TestHelpers::RunAllStateMachinesToCompletion(Test, NewStateMachineInstance, &NewStateMachineInstance->GetRootStateMachine(), -1, -1);
			Test->TestEqual("Correct number of states hit", StatesHit, StatesNotHit);

			SavedActiveState = NewStateMachineInstance->GetSingleNestedActiveState();
			Test->TestNotEqual("Nested state shouldn't equal initial state", SavedActiveState, NewStateMachineInstance->GetRootStateMachine().GetSingleInitialState());
		}

		// One last test checking incrementing every state, saving, and reloading.
		{
			for (int32 i = 0; i < TotalStatesHit; ++i)
			{
				int32 EntryHits, UpdateHits, EndHits;
				USMInstance* TestedStateMachine = TestHelpers::RunStateMachineToCompletion(Test, NewBP, EntryHits, UpdateHits, EndHits, i, true, false, false);
				TArray<FGuid> CurrentGuids;
				TestedStateMachine->GetAllActiveStateGuids(CurrentGuids);

				TestedStateMachine = TestHelpers::CreateNewStateMachineInstanceFromBP(Test, NewBP, Context);
				TestedStateMachine->LoadFromMultipleStates(CurrentGuids);

				TArray<FGuid> ReloadedGuids;
				TestedStateMachine->GetAllActiveStateGuids(ReloadedGuids);

				int32 Match = TestHelpers::ArrayContentsInArray(ReloadedGuids, CurrentGuids);
				Test->TestEqual("State machine states reloaded", Match, CurrentGuids.Num());
			}
		}
	}

	for (FAssetHandler& Asset : ExtraAssets)
	{
		Asset.DeleteAsset(Test);
	}

	return NewAsset.DeleteAsset(Test);
}

/**
 * Save and restore the state of a hierarchical state machine, then do it again with bReuseCurrentState.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveStateMachineStateTest, "SMTests.SaveRestoreReuseStateMachineState", EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

	bool FSaveStateMachineStateTest::RunTest(const FString& Parameters)
{
	const bool bUseReferences = false;
	return TestSaveStateMachineState(this, bUseReferences, false) &&
		TestSaveStateMachineState(this, bUseReferences, true);
}

/**
 * Save and restore the state of a hierarchical state machine with references, then do it again with bReuseCurrentState.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveStateMachineStateWithReferencesTest, "SMTests.SaveRestoreReuseStateMachineStateWithReferences", EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

	bool FSaveStateMachineStateWithReferencesTest::RunTest(const FString& Parameters)
{
	const bool bUseReferences = true;
	return TestSaveStateMachineState(this, bUseReferences, false) &&
		TestSaveStateMachineState(this, bUseReferences, true) &&
		TestSaveStateMachineState(this, bUseReferences, false, false, true) &&
		TestSaveStateMachineState(this, bUseReferences, true, false, true);
}

/**
 * Save and restore the state of a hierarchical state machine with references which use intermediate graphs, then do it again with bReuseCurrentState.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveStateMachineStateWithIntermediateReferencesTest, "SMTests.SaveRestoreReuseStateMachineStateWithIntermediateReferences", EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

	bool FSaveStateMachineStateWithIntermediateReferencesTest::RunTest(const FString& Parameters)
{
	const bool bUseReferences = true;
	return TestSaveStateMachineState(this, bUseReferences, false, true) &&
		TestSaveStateMachineState(this, bUseReferences, true, true) &&
		TestSaveStateMachineState(this, bUseReferences, false, true, true) &&
		TestSaveStateMachineState(this, bUseReferences, true, true, true);
}

/**
 * Tests GetStateMachineReference in the intermediate graph and validates it returns the correct reference.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStateRead_GetStateMachineReferenceTest, "SMTests.StateRead_GetStateMachineReference", EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

	bool FStateRead_GetStateMachineReferenceTest::RunTest(const FString& Parameters)
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

	UEdGraphPin* LastStatePin = nullptr;

	// Build top level state machine.
	{
		TestHelpers::BuildLinearStateMachine(this, StateMachineGraph, 2, &LastStatePin);
		if (!NewAsset.SaveAsset(this))
		{
			return false;
		}
	}

	// Build a nested state machine.
	UEdGraphPin* EntryPointForNestedStateMachine = LastStatePin;
	UEdGraphPin* LastNestedPin = nullptr;
	USMGraphNode_StateMachineStateNode* NestedStateMachineNode = TestHelpers::BuildNestedStateMachine(this, StateMachineGraph, 4, &EntryPointForNestedStateMachine, &LastNestedPin);

	// Add more top level.
	{
		LastStatePin = NestedStateMachineNode->GetOutputPin();
		TestHelpers::BuildLinearStateMachine(this, StateMachineGraph, 2, &LastStatePin);
		if (!NewAsset.SaveAsset(this))
		{
			return false;
		}

		// Signal the state after the nested state machine to wait for its completion.
		USMGraphNode_TransitionEdge* TransitionFromNestedStateMachine = CastChecked<USMGraphNode_TransitionEdge>(NestedStateMachineNode->GetOutputPin()->LinkedTo[0]->GetOwningNode());
		TestHelpers::OverrideTransitionResultLogic<USMGraphK2Node_StateMachineReadNode_InEndState>(this, TransitionFromNestedStateMachine);
	}
	TestTrue("Nested state machine has correct node count", NestedStateMachineNode->GetBoundGraph()->Nodes.Num() > 1);

	USMBlueprint* NewReferencedBlueprint = FSMBlueprintEditorUtils::ConvertStateMachineToReference(NestedStateMachineNode, false, nullptr, nullptr);
	FKismetEditorUtilities::CompileBlueprint(NewReferencedBlueprint);
	
	// Store handler information so we can delete the object.
	FString ReferencedPath = NewReferencedBlueprint->GetPathName();
	FAssetHandler ReferencedAsset(NewReferencedBlueprint->GetName(), USMBlueprint::StaticClass(), NewObject<USMBlueprintFactory>(), &ReferencedPath);
	ReferencedAsset.Object = NewReferencedBlueprint;

	UPackage* Package = FAssetData(NewReferencedBlueprint).GetPackage();
	ReferencedAsset.Package = Package;
	
	TestNotNull("New referenced blueprint created", NewReferencedBlueprint);
	TestTrue("Nested state machine has had all nodes removed.", NestedStateMachineNode->GetBoundGraph()->Nodes.Num() == 1);

	// Now convert the state machine to a reference.
	NestedStateMachineNode->SetUseIntermediateGraph(true);
	
	// Find the intermediate graph which should have been created.
	TSet<USMIntermediateGraph*> Graphs;
	FSMBlueprintEditorUtils::GetAllGraphsOfClassNested<USMIntermediateGraph>(NestedStateMachineNode->GetBoundGraph(), Graphs);

	TestTrue("Intermediate Graph Found", Graphs.Num() == 1);

	USMGraphK2Node_StateMachineRef_Stop* StopNode = nullptr;
	USMIntermediateGraph* IntermediateGraph = nullptr;
	for(USMIntermediateGraph* Graph : Graphs)
	{
		IntermediateGraph = Graph;
		TArray< USMGraphK2Node_StateMachineRef_Stop*> StopNodes;
		FSMBlueprintEditorUtils::GetAllNodesOfClassNested<USMGraphK2Node_StateMachineRef_Stop>(Graph, StopNodes);
		TestTrue("Stop Node Found", StopNodes.Num() == 1);
		StopNode = StopNodes[0];
	}

	if(StopNode == nullptr)
	{
		return false;
	}

	UEdGraphPin* ContextOutPin = nullptr;
	UK2Node_CallFunction* GetContextNode = TestHelpers::CreateContextGetter(this, IntermediateGraph, &ContextOutPin);

	// Add a call to read from the context.
	UK2Node_CallFunction* SetReference = TestHelpers::CreateFunctionCall(IntermediateGraph, USMTestContext::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(USMTestContext, SetTestReference)));

	USMGraphK2Node_StateReadNode_GetStateMachineReference* GetReference = TestHelpers::CreateNewNode<USMGraphK2Node_StateReadNode_GetStateMachineReference>(this, IntermediateGraph, SetReference->FindPin(FName("Instance")), false);
	TestNotNull("Expected helper node to be created", GetReference);

	UK2Node_DynamicCast* CastNode = TestHelpers::CreateAndLinkPureCastNode(this, IntermediateGraph, ContextOutPin, SetReference->FindPin(TEXT("self"), EGPD_Input));
	TestNotNull("Context linked to member function set reference", CastNode);

	const bool bWired = IntermediateGraph->GetSchema()->TryCreateConnection(StopNode->FindPin(FName("then")), SetReference->GetExecPin());
	TestTrue("Wired execution from stop node to set reference", bWired);

	FKismetEditorUtilities::CompileBlueprint(NewBP);
	
	USMTestContext* Context = NewObject<USMTestContext>();
	USMInstance* StateMachineInstance = TestHelpers::CreateNewStateMachineInstanceFromBP(this, NewBP, Context);

	const FGuid PathGuid = FSMBlueprintEditorUtils::TryCreatePathGuid(IntermediateGraph);
	
	USMInstance* ReferenceInstance = StateMachineInstance->GetReferencedInstanceByGuid(PathGuid);
	TestNotNull("Real reference exists", ReferenceInstance);
	TestNull("TestReference not set", Context->TestReference);

	TestHelpers::RunAllStateMachinesToCompletion(this, StateMachineInstance, &StateMachineInstance->GetRootStateMachine());

	TestNotNull("TestReference set from blueprint graph", Context->TestReference);
	TestNotEqual("Test reference is not the root instance", StateMachineInstance, Context->TestReference);
	TestEqual("Found reference equals real reference", Context->TestReference, ReferenceInstance);
	
	ReferencedAsset.DeleteAsset(this);
	return NewAsset.DeleteAsset(this);
}

/**
 * Assemble and run a hierarchical state machine and wait for the internal state machine to finish.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStateRead_InEndStateTest, "SMTests.StateRead_InEndState", EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

	bool FStateRead_InEndStateTest::RunTest(const FString& Parameters)
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
	int32 TotalStates = 0;
	int32 TotalTopLevelStates = 0;
	UEdGraphPin* LastStatePin = nullptr;

	// Build top level state machine.
	{
		const int32 CurrentStates = 2;
		TestHelpers::BuildLinearStateMachine(this, StateMachineGraph, CurrentStates, &LastStatePin);
		if (!NewAsset.SaveAsset(this))
		{
			return false;
		}
		TotalStates += CurrentStates;
		TotalTopLevelStates += CurrentStates;
	}

	// Build a nested state machine.
	UEdGraphPin* EntryPointForNestedStateMachine = LastStatePin;
	USMGraphNode_StateMachineStateNode* NestedStateMachineNode = TestHelpers::CreateNewNode<USMGraphNode_StateMachineStateNode>(this, StateMachineGraph, EntryPointForNestedStateMachine);

	UEdGraphPin* LastNestedPin = nullptr;
	{
		const int32 CurrentStates = 10;
		TestHelpers::BuildLinearStateMachine(this, Cast<USMGraph>(NestedStateMachineNode->GetBoundGraph()), CurrentStates, &LastNestedPin);
		LastStatePin = NestedStateMachineNode->GetOutputPin();

		TotalStates += CurrentStates;
		TotalTopLevelStates += 1;
	}

	// Add logic to the state machine transition.
	USMGraphNode_TransitionEdge* TransitionToNestedStateMachine = CastChecked<USMGraphNode_TransitionEdge>(NestedStateMachineNode->GetInputPin()->LinkedTo[0]->GetOwningNode());
	TestHelpers::AddTransitionResultLogic(this, TransitionToNestedStateMachine);

	// Add more top level (states leading from nested state machine).
	{
		const int32 CurrentStates = 10;
		TestHelpers::BuildLinearStateMachine(this, StateMachineGraph, CurrentStates, &LastStatePin);
		if (!NewAsset.SaveAsset(this))
		{
			return false;
		}
		TotalStates += CurrentStates;
		TotalTopLevelStates += CurrentStates;
	}

	// This will run the nested machine only up to the first state.
	TestHelpers::TestLinearStateMachine(this, NewBP, TotalTopLevelStates);

	int32 ExpectedEntryValue = TotalTopLevelStates;

	// Run the same machine until an end state is reached. The result should be the same as the top level machine won't wait for the nested machine.
	{
		int32 EntryHits = 0; int32 UpdateHits = 0; int32 EndHits = 0;
		TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits);

		TestEqual("State Machine generated value", EntryHits, ExpectedEntryValue);
		TestEqual("State Machine generated value", UpdateHits, 0);
		TestEqual("State Machine generated value", EndHits, ExpectedEntryValue);
	}

	// Now let's try waiting for the nested state machine. Clear the graph except for the result node.
	{
		USMGraphNode_TransitionEdge* TransitionFromNestedStateMachine = CastChecked<USMGraphNode_TransitionEdge>(NestedStateMachineNode->GetOutputPin()->LinkedTo[0]->GetOwningNode());
		TestHelpers::OverrideTransitionResultLogic<USMGraphK2Node_StateMachineReadNode_InEndState>(this, TransitionFromNestedStateMachine);

		ExpectedEntryValue = TotalStates;

		// Run the same machine until an end state is reached. This time the result should be modified by all nested states.
		int32 EntryHits = 0; int32 UpdateHits = 0; int32 EndHits = 0;
		TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits);

		TestEqual("State Machine generated value", EntryHits, ExpectedEntryValue);
		TestEqual("State Machine generated value", UpdateHits, 0);
		TestEqual("State Machine generated value", EndHits, ExpectedEntryValue);
	}

	return NewAsset.DeleteAsset(this);
}

/**
 * Test transitioning from a state after a time period.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStateRead_TimeInStateTest, "SMTests.StateRead_TimeInState", EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

	bool FStateRead_TimeInStateTest::RunTest(const FString& Parameters)
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
	int32 TotalStates = 0;
	UEdGraphPin* LastStatePin = nullptr;

	// Build a state machine of only two states.
	{
		const int32 CurrentStates = 2;
		TestHelpers::BuildLinearStateMachine(this, StateMachineGraph, CurrentStates, &LastStatePin);
		if (!NewAsset.SaveAsset(this))
		{
			return false;
		}
		TotalStates += CurrentStates;
	}

	int32 ExpectedEntryValue = TotalStates;

	// Run as normal.
	{
		int32 EntryHits = 0; int32 UpdateHits = 0; int32 EndHits = 0;
		TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits);
	}

	// Now let's try waiting for the first state. Each tick increments the update count by one.
	{
		USMGraphNode_TransitionEdge* TransitionFromNestedStateMachine =
			CastChecked<USMGraphNode_TransitionEdge>(Cast<USMGraphNode_StateNode>(LastStatePin->GetOwningNode())->GetInputPin()->LinkedTo[0]->GetOwningNode());

		UEdGraph* TransitionGraph = TransitionFromNestedStateMachine->GetBoundGraph();
		TransitionGraph->Nodes.Empty();
		TransitionGraph->GetSchema()->CreateDefaultNodesForGraph(*TransitionGraph);

		TestHelpers::AddSpecialFloatTransitionLogic<USMGraphK2Node_StateReadNode_TimeInState>(this, TransitionFromNestedStateMachine);

		// Run again. By default the transition will wait until time in state is greater than the value of USMTestContext::GreaterThanTest.
		int32 EntryHits = 0; int32 UpdateHits = 0; int32 EndHits = 0;
		TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits);

		TestEqual("State Machine generated value", EntryHits, ExpectedEntryValue);
		TestEqual("State Machine generated value", UpdateHits, (int32)USMTestContext::GreaterThanTest + 1);
		TestEqual("State Machine generated value", EndHits, ExpectedEntryValue);
	}

	return NewAsset.DeleteAsset(this);
}

/**
 * Collapse states down to a nested state machine.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCollapseStateMachineTest, "SMTests.CollapseStateMachine", EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

	bool FCollapseStateMachineTest::RunTest(const FString& Parameters)
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
	int32 TotalStates = 5;

	UEdGraphPin* LastStatePin = nullptr;

	TestHelpers::BuildLinearStateMachine(this, StateMachineGraph, TotalStates, &LastStatePin);
	if (!NewAsset.SaveAsset(this))
	{
		return false;
	}
	TestHelpers::TestLinearStateMachine(this, NewBP, TotalStates);

	// Let the last node on the graph be the node after the new state machine.
	USMGraphNode_StateNodeBase* AfterNode = CastChecked<USMGraphNode_StateNodeBase>(LastStatePin->GetOwningNode());

	// Let the second node from the beginning be the node leading to the new state machine.
	USMGraphNode_StateNodeBase* BeforeNode = AfterNode->GetPreviousNode()->GetPreviousNode()->GetPreviousNode();
	check(BeforeNode);

	// The two states in between will become a state machine.
	TSet<UObject*> SelectedNodes;
	USMGraphNode_StateNodeBase* SMStartNode = BeforeNode->GetNextNode();
	USMGraphNode_StateNodeBase* SMEndNode = SMStartNode->GetNextNode();
	SelectedNodes.Add(SMStartNode);
	SelectedNodes.Add(SMEndNode);

	TestEqual("Start SM Node connects from before node", BeforeNode, SMStartNode->GetPreviousNode());
	TestEqual("Before Node connects to start SM node", SMStartNode, BeforeNode->GetNextNode());

	TestEqual("End SM Node connects from after node", AfterNode, SMEndNode->GetNextNode());
	TestEqual("After Node connects to end SM node", SMEndNode, AfterNode->GetPreviousNode());

	FSMBlueprintEditorUtils::CollapseNodesAndCreateStateMachine(SelectedNodes);

	TotalStates -= 1;

	TestNotEqual("Start SM Node no longer connects to before node", BeforeNode, SMStartNode->GetPreviousNode());
	TestNotEqual("Before Node no longer connects to start SM node", SMStartNode, BeforeNode->GetNextNode());

	TestNotEqual("End SM Node no longer connects from after node", AfterNode, SMEndNode->GetNextNode());
	TestNotEqual("After Node no longer connects to end SM node", SMEndNode, AfterNode->GetPreviousNode());

	USMGraphNode_StateMachineStateNode* NewSMNode = Cast<USMGraphNode_StateMachineStateNode>(BeforeNode->GetNextNode());
	TestNotNull("State Machine node created in proper location", NewSMNode);

	if (NewSMNode == nullptr)
	{
		return false;
	}

	TestEqual("New SM Node connects to correct node", NewSMNode->GetNextNode(), AfterNode);

	TestHelpers::TestLinearStateMachine(this, NewBP, TotalStates);

	return NewAsset.DeleteAsset(this);
}

/**
 * Assemble a hierarchical state machine and convert the nested state machine to a reference, then run and wait for the referenced state machine to finish.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReferenceStateMachineTest, "SMTests.ReferenceStateMachine", EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

	bool FReferenceStateMachineTest::RunTest(const FString& Parameters)
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
	int32 TotalStates = 0;
	int32 TotalTopLevelStates = 0;
	UEdGraphPin* LastStatePin = nullptr;

	// Build top level state machine.
	{
		const int32 CurrentStates = 2;
		TestHelpers::BuildLinearStateMachine(this, StateMachineGraph, CurrentStates, &LastStatePin);
		if (!NewAsset.SaveAsset(this))
		{
			return false;
		}
		TotalStates += CurrentStates;
		TotalTopLevelStates += CurrentStates;
	}

	// Build a nested state machine.
	UEdGraphPin* EntryPointForNestedStateMachine = LastStatePin;
	USMGraphNode_StateMachineStateNode* NestedStateMachineNode = TestHelpers::CreateNewNode<USMGraphNode_StateMachineStateNode>(this, StateMachineGraph, EntryPointForNestedStateMachine);

	UEdGraphPin* LastNestedPin = nullptr;
	{
		const int32 CurrentStates = 10;
		TestHelpers::BuildLinearStateMachine(this, Cast<USMGraph>(NestedStateMachineNode->GetBoundGraph()), CurrentStates, &LastNestedPin, USMStateTestInstance::StaticClass(), USMTransitionTestInstance::StaticClass());
		NestedStateMachineNode->GetBoundGraph()->Rename(TEXT("Nested_State_Machine_For_Reference"));
		LastStatePin = NestedStateMachineNode->GetOutputPin();

		TotalStates += CurrentStates;
		TotalTopLevelStates += 1;
	}

	// Add logic to the state machine transition.
	USMGraphNode_TransitionEdge* TransitionToNestedStateMachine = CastChecked<USMGraphNode_TransitionEdge>(NestedStateMachineNode->GetInputPin()->LinkedTo[0]->GetOwningNode());
	TestHelpers::AddTransitionResultLogic(this, TransitionToNestedStateMachine);

	// Add more top level.
	{
		const int32 CurrentStates = 10;
		TestHelpers::BuildLinearStateMachine(this, StateMachineGraph, CurrentStates, &LastStatePin);
		if (!NewAsset.SaveAsset(this))
		{
			return false;
		}
		TotalStates += CurrentStates;
		TotalTopLevelStates += CurrentStates;
	}

	TestTrue("Nested state machine has correct node count", NestedStateMachineNode->GetBoundGraph()->Nodes.Num() > 1);

	// Now convert the state machine to a reference.
	USMBlueprint* NewReferencedBlueprint = FSMBlueprintEditorUtils::ConvertStateMachineToReference(NestedStateMachineNode, false, nullptr, nullptr);
	TestNotNull("New referenced blueprint created", NewReferencedBlueprint);
	TestTrue("Nested state machine has had all nodes removed.", NestedStateMachineNode->GetBoundGraph()->Nodes.Num() == 1);

	FKismetEditorUtilities::CompileBlueprint(NewReferencedBlueprint);

	// Store handler information so we can delete the object.
	FString ReferencedPath = NewReferencedBlueprint->GetPathName();
	FAssetHandler ReferencedAsset(NewReferencedBlueprint->GetName(), USMBlueprint::StaticClass(), NewObject<USMBlueprintFactory>(), &ReferencedPath);
	ReferencedAsset.Object = NewReferencedBlueprint;

	UPackage* Package = FAssetData(NewReferencedBlueprint).GetPackage();
	ReferencedAsset.Package = Package;

	// This will run the nested machine only up to the first state.
	TestHelpers::TestLinearStateMachine(this, NewBP, TotalTopLevelStates);

	int32 ExpectedEntryValue = TotalTopLevelStates;

	// Run the same machine until an end state is reached. The result should be the same as the top level machine won't wait for the nested machine.
	{
		int32 EntryHits = 0; int32 UpdateHits = 0; int32 EndHits = 0;
		TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits);

		TestEqual("State Machine generated value", EntryHits, ExpectedEntryValue);
		TestEqual("State Machine generated value", UpdateHits, 0);
		TestEqual("State Machine generated value", EndHits, ExpectedEntryValue);
	}

	// Now let's try waiting for the nested state machine. Clear the graph except for the result node.
	{
		USMGraphNode_TransitionEdge* TransitionFromNestedStateMachine = CastChecked<USMGraphNode_TransitionEdge>(NestedStateMachineNode->GetOutputPin()->LinkedTo[0]->GetOwningNode());
		UEdGraph* TransitionGraph = TransitionFromNestedStateMachine->GetBoundGraph();
		TransitionGraph->Nodes.Empty();
		TransitionGraph->GetSchema()->CreateDefaultNodesForGraph(*TransitionGraph);

		TestHelpers::AddSpecialBooleanTransitionLogic<USMGraphK2Node_StateMachineReadNode_InEndState>(this, TransitionFromNestedStateMachine);
		ExpectedEntryValue = TotalStates;

		// Run the same machine until an end state is reached. This time the result should be modified by all nested states.
		int32 EntryHits = 0; int32 UpdateHits = 0; int32 EndHits = 0;
		TestHelpers::RunStateMachineToCompletion(this, NewBP, EntryHits, UpdateHits, EndHits);

		TestEqual("State Machine generated value", EntryHits, ExpectedEntryValue);
		TestEqual("State Machine generated value", UpdateHits, 0);
		TestEqual("State Machine generated value", EndHits, ExpectedEntryValue);
	}

	// Verify it can't reference itself.
	bool bReferenceSelf = NestedStateMachineNode->ReferenceStateMachine(NewBP, true);
	TestFalse("State Machine should not have been allowed to reference itself", bReferenceSelf);

	// Finally let's check circular references and make sure it doesn't stack overflow.
	bReferenceSelf = NestedStateMachineNode->ReferenceStateMachine(NewBP, false);
	TestTrue("State Machine has been overridden to reference itself", bReferenceSelf);

	FKismetEditorUtilities::CompileBlueprint(NewBP);

	// As long as generating the state machine is successful we are fine. Running it would cause a stack overflow because we have no exit conditions.
	// That doesn't need to be tested as that is up to user implementation.
	USMTestContext* Context = NewObject<USMTestContext>();

	AddExpectedError(FString("Attempted to generate state machine with circular referencing"));
	USMInstance* ReferencesNoReuse = TestHelpers::CreateNewStateMachineInstanceFromBP(this, NewBP, Context, false); // Don't test node map that will stack overflow.
	const int32 NoReuse = ReferencesNoReuse->GetAllReferencedInstances(true).Num();
	
	// Now check legacy behavior if the same reference is reused.
	NestedStateMachineNode->bUseTemplate = false;
	NestedStateMachineNode->bReuseReference = true;

	FKismetEditorUtilities::CompileBlueprint(NewBP);

	/*
	if (PLATFORM_WINDOWS) // Linux may stack overflow here. Really this should be investigated more but this is around unsupported behavior as it is.
	{
		USMInstance* ReferencesWithReuse = TestHelpers::CreateNewStateMachineInstanceFromBP(this, NewBP, Context, false);
		// Didn't crash? Great!

		const int32 WithReuse = ReferencesWithReuse->GetAllReferencedInstances(true).Num();
		// We expect more references because with circular referencing and instancing state machine generation aborts with an error message.
		// When reusing instances this gets around that behavior.
		// ------------
		// Changed with guid paths... Specifically CalculatePathGuid under FSMStateMachine helps prevent stack overflow.
		TestTrue("References reused", WithReuse < NoReuse);
	}
	*/
	ReferencedAsset.DeleteAsset(this);
	return NewAsset.DeleteAsset(this);
}

/**
 * Test optional transition event nodes.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTransitionEventTest, "SMTests.TransitionEvents", EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

	bool FTransitionEventTest::RunTest(const FString& Parameters)
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
		if (!NewAsset.SaveAsset(this))
		{
			return false;
		}
	}

	{
		USMGraphNode_TransitionEdge* TransitionEdge =
			CastChecked<USMGraphNode_TransitionEdge>(Cast<USMGraphNode_StateNode>(LastStatePin->GetOwningNode())->GetInputPin()->LinkedTo[0]->GetOwningNode());

		TestHelpers::AddEventWithLogic<USMGraphK2Node_TransitionInitializedNode>(this, TransitionEdge,
			USMTestContext::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(USMTestContext, IncreaseTransitionInit)));

		TestHelpers::AddEventWithLogic<USMGraphK2Node_TransitionShutdownNode>(this, TransitionEdge,
			USMTestContext::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(USMTestContext, IncreaseTransitionShutdown)));

		TestHelpers::AddEventWithLogic<USMGraphK2Node_TransitionPreEvaluateNode>(this, TransitionEdge,
			USMTestContext::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(USMTestContext, IncreaseTransitionPreEval)));

		TestHelpers::AddEventWithLogic<USMGraphK2Node_TransitionPostEvaluateNode>(this, TransitionEdge,
			USMTestContext::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(USMTestContext, IncreaseTransitionPostEval)));

		TestHelpers::AddEventWithLogic<USMGraphK2Node_TransitionEnteredNode>(this, TransitionEdge,
			USMTestContext::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(USMTestContext, IncreaseTransitionTaken)));

		FKismetEditorUtilities::CompileBlueprint(NewBP);

		// Create a context we will run the state machine for.
		USMTestContext* Context = NewObject<USMTestContext>();
		USMInstance* StateMachineInstance = TestHelpers::CreateNewStateMachineInstanceFromBP(this, NewBP, Context);

		// Test initial values

		TestEqual("InitValue", Context->TestTransitionInit.Count, 0);
		TestEqual("ShutdownValue", Context->TestTransitionShutdown.Count, 0);
		TestEqual("PreEvalValue", Context->TestTransitionPreEval.Count, 0);
		TestEqual("PostEvalValue", Context->TestTransitionPostEval.Count, 0);
		TestEqual("TransitionEntered", Context->TestTransitionEntered.Count, 0);

		// Test in start state.

		StateMachineInstance->Start();
		TestTrue("State Machine should have started", StateMachineInstance->IsActive());

		TestEqual("InitValue", Context->TestTransitionInit.Count, 1);
		TestEqual("ShutdownValue", Context->TestTransitionShutdown.Count, 0);
		TestEqual("PreEvalValue", Context->TestTransitionPreEval.Count, 0);
		TestEqual("PostEvalValue", Context->TestTransitionPostEval.Count, 0);
		TestEqual("TransitionEntered", Context->TestTransitionEntered.Count, 0);

		// Test pre/post eval
		TArray<TArray<FSMTransition*>> TransitionChain;
		const bool bFoundTransition = StateMachineInstance->GetRootStateMachine().GetSingleActiveState()->GetValidTransition(TransitionChain);
		TestTrue("Transition found", bFoundTransition);
		FSMTransition* ValidTransition = TransitionChain[0][0];
		
		TestEqual("InitValue", Context->TestTransitionInit.Count, 1);
		TestEqual("ShutdownValue", Context->TestTransitionShutdown.Count, 0);
		TestEqual("PreEvalValue", Context->TestTransitionPreEval.Count, 1);
		TestEqual("PostEvalValue", Context->TestTransitionPostEval.Count, 1);
		TestEqual("TransitionEntered", Context->TestTransitionEntered.Count, 0);

		TestTrue("PostEval should have occurred after PreEval", Context->TestTransitionPreEval.TimeStamp < Context->TestTransitionPostEval.TimeStamp);

		// Test after taking the transition.

		StateMachineInstance->GetRootStateMachine().ProcessTransition(ValidTransition, nullptr, 1.f);

		TestEqual("InitValue", Context->TestTransitionInit.Count, 1);
		TestEqual("ShutdownValue", Context->TestTransitionShutdown.Count, 1);
		TestEqual("PreEvalValue", Context->TestTransitionPreEval.Count, 1);
		TestEqual("PostEvalValue", Context->TestTransitionPostEval.Count, 1);
		TestEqual("TransitionEntered", Context->TestTransitionEntered.Count, 1);

		TestTrue("TransitionEntered should have occurred after Shutdown", Context->TestTransitionShutdown.TimeStamp < Context->TestTransitionEntered.TimeStamp);

		// Should shut down the state machine now.
		StateMachineInstance->Update(1.f);

		TestTrue("State Machine should be in end state", StateMachineInstance->IsInEndState());

		TestEqual("InitValue", Context->TestTransitionInit.Count, 1);
		TestEqual("ShutdownValue", Context->TestTransitionShutdown.Count, 1);
		TestEqual("PreEvalValue", Context->TestTransitionPreEval.Count, 1);
		TestEqual("PostEvalValue", Context->TestTransitionPostEval.Count, 1);
		TestEqual("TransitionEntered", Context->TestTransitionEntered.Count, 1);

		StateMachineInstance->Shutdown();
		TestFalse("State Machine should have stopped", StateMachineInstance->IsActive());
	}

	return NewAsset.DeleteAsset(this);
}

/**
 * Test optional transition event nodes.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTransitionCanEvaluateTest, "SMTests.TransitionCanEvaluate", EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

	bool FTransitionCanEvaluateTest::RunTest(const FString& Parameters)
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
		if (!NewAsset.SaveAsset(this))
		{
			return false;
		}
	}

	{
		USMGraphNode_TransitionEdge* TransitionEdge =
			CastChecked<USMGraphNode_TransitionEdge>(Cast<USMGraphNode_StateNode>(LastStatePin->GetOwningNode())->GetInputPin()->LinkedTo[0]->GetOwningNode());

		UK2Node_CallFunction* PreEvalNode = TestHelpers::AddEventWithLogic<USMGraphK2Node_TransitionPreEvaluateNode>(this, TransitionEdge,
			USMTestContext::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(USMTestContext, IncreaseTransitionPreEval)));

		TestHelpers::AddEventWithLogic<USMGraphK2Node_TransitionPostEvaluateNode>(this, TransitionEdge,
			USMTestContext::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(USMTestContext, IncreaseTransitionPostEval)));

		USMGraphK2Node_StateWriteNode_CanEvaluate* SetCanEvaluate = TestHelpers::CreateNewNode<USMGraphK2Node_StateWriteNode_CanEvaluate>(this,
			TransitionEdge->GetBoundGraph(), PreEvalNode->GetThenPin(), false);

		TestNotNull("Can Evaluate Write not should exist", SetCanEvaluate);
		if (!SetCanEvaluate)
		{
			return false;
		}

		// First run will never swith states.
		{
			FKismetEditorUtilities::CompileBlueprint(NewBP);

			// Create a context we will run the state machine for.
			USMTestContext* Context = NewObject<USMTestContext>();
			USMInstance* StateMachineInstance = TestHelpers::CreateNewStateMachineInstanceFromBP(this, NewBP, Context);

			// Test initial values

			TestEqual("PreEvalValue", Context->TestTransitionPreEval.Count, 0);
			TestEqual("PostEvalValue", Context->TestTransitionPostEval.Count, 0);

			// Test in start state.

			StateMachineInstance->Start();
			TestTrue("State Machine should have started", StateMachineInstance->IsActive());

			TestEqual("PreEvalValue", Context->TestTransitionPreEval.Count, 0);
			TestEqual("PostEvalValue", Context->TestTransitionPostEval.Count, 0);

			// Test pre/post eval after trying to change. But eval is never run.

			TArray<TArray<FSMTransition*>> TransitionChain;
			const bool bFoundTransition = StateMachineInstance->GetRootStateMachine().GetSingleActiveState()->GetValidTransition(TransitionChain);

			TestFalse("No valid transition should exist", bFoundTransition);
			TestEqual("PreEvalValue", Context->TestTransitionPreEval.Count, 1);
			TestEqual("PostEvalValue", Context->TestTransitionPostEval.Count, 1);

			TestTrue("PostEval should have occurred after PreEval", Context->TestTransitionPreEval.TimeStamp < Context->TestTransitionPostEval.TimeStamp);

			StateMachineInstance->Shutdown();
		}

		SetCanEvaluate->GetInputPin()->DefaultValue = "true";

		// Second run should work normally.
		{
			FKismetEditorUtilities::CompileBlueprint(NewBP);

			// Create a context we will run the state machine for.
			USMTestContext* Context = NewObject<USMTestContext>();
			USMInstance* StateMachineInstance = TestHelpers::CreateNewStateMachineInstanceFromBP(this, NewBP, Context);

			// Test initial values

			TestEqual("PreEvalValue", Context->TestTransitionPreEval.Count, 0);
			TestEqual("PostEvalValue", Context->TestTransitionPostEval.Count, 0);

			// Test in start state.

			StateMachineInstance->Start();
			TestTrue("State Machine should have started", StateMachineInstance->IsActive());

			TestEqual("PreEvalValue", Context->TestTransitionPreEval.Count, 0);
			TestEqual("PostEvalValue", Context->TestTransitionPostEval.Count, 0);

			// Test pre/post eval after trying to change. But eval is never run.

			TArray<TArray<FSMTransition*>> TransitionChain;
			const bool bFoundTransition = StateMachineInstance->GetRootStateMachine().GetSingleActiveState()->GetValidTransition(TransitionChain);
			TestTrue("Transition found", bFoundTransition);
			FSMTransition* ValidTransition = TransitionChain[0][0];

			TestNotNull("Transition should evaluate", ValidTransition);
			TestEqual("PreEvalValue", Context->TestTransitionPreEval.Count, 1);
			TestEqual("PostEvalValue", Context->TestTransitionPostEval.Count, 1);

			TestTrue("PostEval should have occurred after PreEval", Context->TestTransitionPreEval.TimeStamp < Context->TestTransitionPostEval.TimeStamp);

			StateMachineInstance->Shutdown();
		}
	}

	return NewAsset.DeleteAsset(this);
}

/**
 * Replace a node in the state machine.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReplaceNodesTest, "SMTests.ReplaceNodes", EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

	bool FReplaceNodesTest::RunTest(const FString& Parameters)
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
	int32 TotalStates = 5;

	UEdGraphPin* LastStatePin = nullptr;

	TestHelpers::BuildLinearStateMachine(this, StateMachineGraph, TotalStates, &LastStatePin);
	if (!NewAsset.SaveAsset(this))
	{
		return false;
	}
	TestHelpers::TestLinearStateMachine(this, NewBP, TotalStates);

	// Let the last node on the graph be the node after the new node.
	USMGraphNode_StateNodeBase* AfterNode = CastChecked<USMGraphNode_StateNodeBase>(LastStatePin->GetOwningNode());

	// Let node prior to the one we are replacing.
	USMGraphNode_StateNodeBase* BeforeNode = AfterNode->GetPreviousNode()->GetPreviousNode();
	check(BeforeNode);

	// The node we are replacing is the second to last node.
	USMGraphNode_StateNodeBase* NodeToReplace = AfterNode->GetPreviousNode();
	TestTrue("Node is state", NodeToReplace->IsA<USMGraphNode_StateNode>());

	// State machine -- can't easily test converting to reference but that is just setting a null reference.
	USMGraphNode_StateMachineStateNode* StateMachineNode = FSMBlueprintEditorUtils::ConvertNodeTo<USMGraphNode_StateMachineStateNode>(NodeToReplace);
	TestTrue("Node removed", NodeToReplace->GetNextNode() == nullptr && NodeToReplace->GetPreviousNode() == nullptr && NodeToReplace->GetBoundGraph() == nullptr);
	TestTrue("Node is state machine", StateMachineNode->IsA<USMGraphNode_StateMachineStateNode>());
	TestFalse("Node is not reference", StateMachineNode->IsStateMachineReference());
	TestEqual("Connected to original next node", StateMachineNode->GetNextNode(), AfterNode);
	TestEqual("Connected to original previous node", StateMachineNode->GetPreviousNode(), BeforeNode);

	int32 A, B;
	TestHelpers::RunStateMachineToCompletion(this, NewBP, TotalStates, A, B);
	
	// Conduit
	NodeToReplace = StateMachineNode;
	USMGraphNode_ConduitNode* ConduitNode = FSMBlueprintEditorUtils::ConvertNodeTo<USMGraphNode_ConduitNode>(NodeToReplace);
	TestTrue("Node removed", NodeToReplace->GetNextNode() == nullptr && NodeToReplace->GetPreviousNode() == nullptr && NodeToReplace->GetBoundGraph() == nullptr);
	TestTrue("Node is conduit", ConduitNode->IsA<USMGraphNode_ConduitNode>());
	TestEqual("Connected to original next node", ConduitNode->GetNextNode(), AfterNode);
	TestEqual("Connected to original previous node", ConduitNode->GetPreviousNode(), BeforeNode);

	// Back to state
	NodeToReplace = ConduitNode;
	USMGraphNode_StateNode* StateNode = FSMBlueprintEditorUtils::ConvertNodeTo<USMGraphNode_StateNode>(NodeToReplace);
	TestTrue("Node removed", NodeToReplace->GetNextNode() == nullptr && NodeToReplace->GetPreviousNode() == nullptr && NodeToReplace->GetBoundGraph() == nullptr);
	TestTrue("Node is state", StateNode->IsA<USMGraphNode_StateNode>());
	TestEqual("Connected to original next node", StateNode->GetNextNode(), AfterNode);
	TestEqual("Connected to original previous node", StateNode->GetPreviousNode(), BeforeNode);
	TestHelpers::RunStateMachineToCompletion(this, NewBP, TotalStates, A, B);

	
	return NewAsset.DeleteAsset(this);
}

#endif

#endif //WITH_DEV_AUTOMATION_TESTS