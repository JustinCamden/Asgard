// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "EditorStyle.h"
#include "Framework/Commands/Commands.h"


class FSMEditorCommands : public TCommands<FSMEditorCommands>
{
public:
	/** Constructor */
	FSMEditorCommands()
		: TCommands<FSMEditorCommands>(TEXT("SMEditor"), NSLOCTEXT("Contexts", "SMEditor", "State Machine Editor"),
			NAME_None, FEditorStyle::GetStyleSetName())
	{
	}

	// TCommand
	void RegisterCommands() override;
	FORCENOINLINE static const FSMEditorCommands& Get();
	// ~TCommand

	/** Go to the graph for this node. */
	TSharedPtr<FUICommandInfo> GoToGraph;

	/** Go to the blueprint for this node. */
	TSharedPtr<FUICommandInfo> GoToNodeBlueprint;
	
	/** Create transition from Node A - Node A. */
	TSharedPtr<FUICommandInfo> CreateSelfTransition;

	/** Create a nested state machine from existing nodes. */
	TSharedPtr<FUICommandInfo> CollapseToStateMachine;

	/** Converts a nested state machine to a referenced state machine. */
	TSharedPtr<FUICommandInfo> ConvertToStateMachineReference;

	/** Replace the reference a state machine points to. */
	TSharedPtr<FUICommandInfo> ChangeStateMachineReference;

	/** Open the reference blueprint. */
	TSharedPtr<FUICommandInfo> JumpToStateMachineReference;

	/** Signals to use an intermediate graph. */
	TSharedPtr<FUICommandInfo> EnableIntermediateGraph;

	/** Signals to stop using an intermediate graph. */
	TSharedPtr<FUICommandInfo> DisableIntermediateGraph;

	/** Replace node with state machine. */
	TSharedPtr<FUICommandInfo> ReplaceWithStateMachine;
	
	/** Replace node with state machine reference. */
	TSharedPtr<FUICommandInfo> ReplaceWithStateMachineReference;

	/** Replace node with state machine parent. */
	TSharedPtr<FUICommandInfo> ReplaceWithStateMachineParent;
	
	/** Replace node with state. */
	TSharedPtr<FUICommandInfo> ReplaceWithState;

	/** Replace node with state conduit. */
	TSharedPtr<FUICommandInfo> ReplaceWithConduit;

	/** Clear a graph property. */
	TSharedPtr<FUICommandInfo> ResetGraphProperty;

	/** Go to a graph property. */
	TSharedPtr<FUICommandInfo> GoToPropertyGraph;

	/** Use the graph to edit. */
	TSharedPtr<FUICommandInfo> ConvertPropertyToGraphEdit;

	/** Use the node to edit. */
	TSharedPtr<FUICommandInfo> RevertPropertyToNodeEdit;
};
