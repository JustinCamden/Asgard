// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "BlueprintEditor.h"

class USMBlueprint;
class FGGAssetEditorToolbar;

class SMSYSTEMEDITOR_API FSMBlueprintEditor : public FBlueprintEditor
{
public:
	FSMBlueprintEditor();
	virtual ~FSMBlueprintEditor();

	void InitSMBlueprintEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, USMBlueprint* Blueprint);

	//  IToolkit interface
	FName GetToolkitFName() const override;
	FText GetBaseToolkitName() const override;
	FText GetToolkitName() const override;
	FText GetToolkitToolTipText() const override;
	FLinearColor GetWorldCentricTabColorScale() const override;
	FString GetWorldCentricTabPrefix() const override;
	FString GetDocumentationLink() const override;
	// ~ IToolkit interface

	// FBlueprintEditor
	void CreateDefaultCommands() override;
	void RefreshEditors(ERefreshBlueprintEditorReason::Type Reason /** = ERefreshBlueprintEditorReason::UnknownReason */) override;
	// ~FBlueprintEditor

	void CloseInvalidTabs();
	
	bool IsSelectedPropertyNodeValid() const;
	
	/** Set by property node. This isn't guaranteed to be valid unless used in a selected property command. */
	TWeakObjectPtr<class USMGraphK2Node_PropertyNode_Base> SelectedPropertyNode;

	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCreateGraphEditorCommands, FSMBlueprintEditor*, TSharedPtr<FUICommandList>);
	/** Event fired when a graph in a state machine blueprint is renamed. */
	static FOnCreateGraphEditorCommands OnCreateGraphEditorCommandsEvent;
protected:

	// FEditorUndoClient
	void PostUndo(bool bSuccess) override;
	// ~FEditorUndoClient

	/** Extend menu */
	void ExtendMenu();

	/** Extend toolbar */
	void ExtendToolbar();

	void BindCommands();

	/** FBlueprintEditor interface */
	void OnActiveTabChanged(TSharedPtr<SDockTab> PreviouslyActive, TSharedPtr<SDockTab> NewlyActivated) override;
	void OnSelectedNodesChangedImpl(const TSet<class UObject*>& NewSelection) override;
	void OnCreateGraphEditorCommands(TSharedPtr<FUICommandList> GraphEditorCommandsList) override;
	/** ~FBlueprintEditor interface */

	void GoToGraph();
	bool CanGoToGraph() const;

	void GoToNodeBlueprint();
	bool CanGoToNodeBlueprint() const;
	
	/** A self transition for the same state. */
	void CreateSingleNodeTransition();
	bool CanCreateSingleNodeTransition() const;

	/** Collapse selected nodes to a new nested state machine. */
	void CollapseNodesToStateMachine();
	bool CanCollapseNodesToStateMachine() const;

	void ConvertStateMachineToReference();
	bool CanConvertStateMachineToReference() const;

	void ChangeStateMachineReference();
	bool CanChangeStateMachineReference() const;

	void JumpToStateMachineReference();
	bool CanJumpToStateMachineReference() const;

	void EnableIntermediateGraph();
	bool CanEnableIntermediateGraph() const;

	void DisableIntermediateGraph();
	bool CanDisableIntermediateGraph() const;

	void ReplaceWithStateMachine();
	bool CanReplaceWithStateMachine() const;

	void ReplaceWithStateMachineReference();
	bool CanReplaceWithStateMachineReference() const;

	void ReplaceWithStateMachineParent();
	bool CanReplaceWithStateMachineParent() const;
	
	void ReplaceWithState();
	bool CanReplaceWithState() const;

	void ReplaceWithConduit();
	bool CanReplaceWithConduit() const;

	void GoToPropertyGraph();
	bool CanGoToPropertyGraph() const;

	void ClearGraphProperty();
	bool CanClearGraphProperty() const;

	void ToggleGraphPropertyEdit();
	bool CanToggleGraphPropertyEdit() const;
private:
	/** The extender to pass to the level editor to extend its window menu */
	TSharedPtr<FExtender> MenuExtender;

	/** Toolbar extender */
	TSharedPtr<FExtender> ToolbarExtender;

	/** The command list for this editor */
	TSharedPtr<FUICommandList> GraphEditorCommands;

	// selected state machine graph node 
	TWeakObjectPtr<class USMGraphK2Node_Base> SelectedStateMachineNode;
};


class SMSYSTEMEDITOR_API FSMNodeBlueprintEditor : public FBlueprintEditor
{
public:
	FSMNodeBlueprintEditor();
	virtual ~FSMNodeBlueprintEditor();

	// IToolkit interface
	void OnBlueprintChangedImpl(UBlueprint* InBlueprint, bool bIsJustBeingCompiled) override;
	FText GetBaseToolkitName() const override;
	FString GetDocumentationLink() const override;
	// ~IToolkit interface
};