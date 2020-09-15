// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#include "Utilities/SMExtendedEditorUtils.h"
#include "K2Node_VariableGet.h"
#include "Kismet/KismetTextLibrary.h"
#include "Graph/Nodes/PropertyNodes/SMGraphK2Node_TextPropertyNode.h"
#include "Graph/SMTextPropertyGraph.h"

void FSMExtendedEditorUtils::GetAllNodesWithTextPropertiesNested(UBlueprint* Blueprint, TArray<USMGraphNode_Base*>& NodesOut)
{
	TArray<USMGraphNode_Base*> AllNodes;
	GetAllNodesOfClassNested<USMGraphNode_Base>(GetTopLevelStateMachineGraph(Blueprint), AllNodes);

	for (USMGraphNode_Base* Node : AllNodes)
	{
		for (const auto& GraphPropertyNode : Node->GetAllPropertyGraphNodes())
		{
			if (GraphPropertyNode.Value->IsA<USMGraphK2Node_TextPropertyNode>())
			{
				NodesOut.Add(Node);
			}
		}
	}
}

void FSMExtendedEditorUtils::GetAllTextPropertiesNested(UBlueprint* Blueprint,
	TArray<USMGraphK2Node_TextPropertyNode*>& TextPropertyNodesOut)
{
	TArray<USMGraphNode_Base*> AllNodes;
	GetAllNodesOfClassNested<USMGraphNode_Base>(GetTopLevelStateMachineGraph(Blueprint), AllNodes);

	for (USMGraphNode_Base* Node : AllNodes)
	{
		for (const auto& GraphPropertyNode : Node->GetAllPropertyGraphNodes())
		{
			if (USMGraphK2Node_TextPropertyNode* PropertyNode = Cast<USMGraphK2Node_TextPropertyNode>(GraphPropertyNode.Value))
			{
				TextPropertyNodesOut.Add(PropertyNode);
			}
		}
	}
}

void FSMExtendedEditorUtils::RefreshTextProperties(UBlueprint* InBlueprint)
{
	if (InBlueprint->IsA<USMBlueprint>())
	{
		TArray<USMGraphNode_Base*> TextPropertyNodes;
		GetAllNodesWithTextPropertiesNested(InBlueprint, TextPropertyNodes);

		for (USMGraphNode_Base* Node : TextPropertyNodes)
		{
			// Will reset text body and repopulate graph which was reset above.
			Node->ReconstructNode();
		}
	}
}

void FSMExtendedEditorUtils::HandleRenameVariableReferencesEvent(UBlueprint* InBlueprint, UClass* InVariableClass,
	const FName& InOldVarName, const FName& InNewVarName)
{
	RefreshTextProperties(InBlueprint);
}

void FSMExtendedEditorUtils::HandleRenameGraphEvent(UBlueprint* InBlueprint, UEdGraph* InVariableClass,
	const FName& InOldVarName, const FName& InNewVarName)
{
	RefreshTextProperties(InBlueprint);
}

UK2Node_CallFunction* FSMExtendedEditorUtils::CreateTextConversionNode(UEdGraph* Graph, UEdGraphPin* FromPin,
	UEdGraphPin* ToPin, const TArray<FName>& CustomFunctionNames, bool bWireConnection)
{
	const UEdGraphSchema_K2* K2_Schema = Cast<const UEdGraphSchema_K2>(Graph->GetSchema());

	bool bUsingCustomFunction = false;
	UFunction* MakeNodeFunction = nullptr;
	UObject* Object = FromPin->PinType.PinSubCategoryObject.Get();
	if(Object && CustomFunctionNames.Num() > 0)
	{
		// Attempt look up of custom function first. This takes priority.
		for (const FName& CustomFunctionName : CustomFunctionNames)
		{
			if (CustomFunctionName == NAME_None)
			{
				continue;
			}
			
			if (UClass* Class = Cast<UClass>(Object))
			{
				MakeNodeFunction = Class->FindFunctionByName(CustomFunctionName);
			}
			else
			{
				MakeNodeFunction = Object->FindFunction(CustomFunctionName);
			}

			if (MakeNodeFunction)
			{
				break;
			}
		}
	}

	if (MakeNodeFunction == nullptr)
	{
		// No custom function found or provided, attempt default lookup.
		MakeNodeFunction = UKismetTextLibrary::StaticClass()->FindFunctionByName(FindTextConversionFunctionName(FromPin->PinType.PinCategory, Object));
	}
	else
	{
		bUsingCustomFunction = true;
	}
	
	if (MakeNodeFunction == nullptr)
	{
		return nullptr;
	}

	UK2Node_CallFunction* ConversionNode = NewObject<UK2Node_CallFunction>(Graph);

	ConversionNode->CreateNewGuid();
	ConversionNode->PostPlacedNewNode();
	ConversionNode->SetFromFunction(MakeNodeFunction);
	ConversionNode->SetFlags(RF_Transactional);
	ConversionNode->AllocateDefaultPins();
	ConversionNode->NodePosX = FromPin->GetOwningNode()->NodePosX;
	ConversionNode->NodePosY = FromPin->GetOwningNode()->NodePosY + 32;
	Graph->AddNode(ConversionNode, true);

	if (bWireConnection)
	{
		// CustomFunctions probably use a self pin.
		UEdGraphPin* ConversionInputPin = GetConversionInputPin(ConversionNode, bUsingCustomFunction);

		if (ConversionInputPin == nullptr)
		{
			return ConversionNode;
		}

		UEdGraphPin* ConversionOutputPin = GetConversionOutputPin(ConversionNode);

		if (ConversionOutputPin == nullptr)
		{
			return ConversionNode;
		}

		// Wire connections from argument node to the conversion node, and from the conversion node to the format node.
		K2_Schema->TryCreateConnection(FromPin, ConversionInputPin);
		K2_Schema->TryCreateConnection(ConversionOutputPin, ToPin);
	}

	return ConversionNode;
}

UEdGraphPin* FSMExtendedEditorUtils::GetConversionInputPin(UEdGraphNode* Node, bool bCheckSelfPin)
{
	const UEdGraphSchema_K2* K2_Schema = Cast<const UEdGraphSchema_K2>(Node->GetSchema());

	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin->Direction == EGPD_Input && !UEdGraphSchema_K2::IsExecPin(*Pin))
		{
			if (!K2_Schema->IsSelfPin(*Pin) || bCheckSelfPin)
			{
				return Pin;
			}
		}
	}

	return nullptr;
}

UEdGraphPin* FSMExtendedEditorUtils::GetConversionOutputPin(UEdGraphNode* Node)
{
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin->Direction == EGPD_Output && !UEdGraphSchema_K2::IsExecPin(*Pin))
		{
			return Pin;
		}
	}

	return nullptr;
}

FName FSMExtendedEditorUtils::FindTextConversionFunctionName(FName FromType, UObject* Object)
{
	if (FromType == UEdGraphSchema_K2::PC_Boolean)
	{
		return GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, Conv_BoolToText);
	}
	if (FromType == UEdGraphSchema_K2::PC_Byte)
	{
		return GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, Conv_ByteToText);
	}
	if (FromType == UEdGraphSchema_K2::PC_Int)
	{
		return GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, Conv_IntToText);
	}
	if (FromType == UEdGraphSchema_K2::PC_Float)
	{
		return GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, Conv_FloatToText);
	}
	if (FromType == UEdGraphSchema_K2::PC_Name)
	{
		return GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, Conv_NameToText);
	}
	if (FromType == UEdGraphSchema_K2::PC_String)
	{
		return GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, Conv_StringToText);
	}
	if (FromType == UEdGraphSchema_K2::PC_Object)
	{
		return GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, Conv_ObjectToText);
	}
	if (FromType == UEdGraphSchema_K2::PC_Struct)
	{
		if (Object)
		{
			if (UScriptStruct* ScriptStruct = Cast<UScriptStruct>(Object))
			{
				const FName Name = ScriptStruct->GetFName();

				if (Name == TEXT("Vector"))
				{
					return GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, Conv_VectorToText);
				}
				if (Name == TEXT("Vector2d"))
				{
					return GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, Conv_Vector2dToText);
				}
				if (Name == TEXT("Rotator"))
				{
					return GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, Conv_RotatorToText);
				}
				if (Name == TEXT("Transform"))
				{
					return GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, Conv_TransformToText);
				}
				if (Name == TEXT("Color"))
				{
					return GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, Conv_ColorToText);
				}
			}
		}
	}

	return NAME_None;
}