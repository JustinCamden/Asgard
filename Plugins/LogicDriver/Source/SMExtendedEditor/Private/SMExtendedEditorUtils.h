// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once
#include "SMBlueprintEditorUtils.h"

class FSMExtendedEditorUtils : public FSMBlueprintEditorUtils
{
public:

	static void GetAllNodesWithTextPropertiesNested(UBlueprint* Blueprint, TArray<USMGraphNode_Base*>& NodesOut);

	static void GetAllTextPropertiesNested(UBlueprint* Blueprint, TArray<class USMGraphK2Node_TextPropertyNode*>& TextPropertyNodesOut);

	/** Resets text property graphs and reconstructs text property nodes. */
	static void RefreshTextProperties(UBlueprint* InBlueprint);

	// Blueprint events
	static void HandleRenameVariableReferencesEvent(UBlueprint* InBlueprint, UClass* InVariableClass, const FName& InOldVarName, const FName& InNewVarName);
	static void HandleRenameGraphEvent(UBlueprint* InBlueprint, UEdGraph* InVariableClass, const FName& InOldVarName, const FName& InNewVarName);
	
	static UK2Node_CallFunction* CreateTextConversionNode(UEdGraph* Graph, UEdGraphPin* FromPin, UEdGraphPin* ToPin, const TArray<FName>& CustomFunctionNames, bool bWireConnection = true);
	static UEdGraphPin* GetConversionInputPin(UEdGraphNode* Node, bool bCheckSelfPin = false);
	static UEdGraphPin* GetConversionOutputPin(UEdGraphNode* Node);
	static FName FindTextConversionFunctionName(FName FromType, UObject* Object);
};