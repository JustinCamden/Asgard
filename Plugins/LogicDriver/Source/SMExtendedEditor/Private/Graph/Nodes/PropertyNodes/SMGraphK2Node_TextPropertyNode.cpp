// Copyright Recursoft LLC 2019-2020. All Rights Reserved.

#include "SMGraphK2Node_TextPropertyNode.h"
#include "Widgets/Text/SSMTextProperty.h"
#include "Widgets/Input/SButton.h"
#include "Components/HorizontalBox.h"
#include "Graph/Nodes/SMGraphNode_Base.h"
#include "Graph/SMTextPropertyGraph.h"
#include "ScopedTransaction.h"
#include "SMBlueprintEditor.h"
#include "Commands/SMExtendedEditorCommands.h"
#include "ToolMenuSection.h"


#define LOCTEXT_NAMESPACE "SMTextPropertyNode"

USMGraphK2Node_TextPropertyNode::USMGraphK2Node_TextPropertyNode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void USMGraphK2Node_TextPropertyNode::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Text, TEXT("Result"));
}

void USMGraphK2Node_TextPropertyNode::ConfigureRuntimePropertyNode()
{
	RuntimeTextProperty.GraphEvaluator = TextProperty.GraphEvaluator;
	RuntimeTextProperty.SetGuid(TextProperty.GetGuid());
	RuntimeTextProperty.SetOwnerGuid(TextProperty.GetOwnerGuid());
	RuntimeTextProperty.TextSerializer = TextProperty.TextSerializer;
	RuntimeTextProperty.Result = TextProperty.Result;
}

FSMGraphProperty_Base_Runtime* USMGraphK2Node_TextPropertyNode::GetRuntimePropertyNode()
{
	return &RuntimeTextProperty;
}

TSharedPtr<SSMGraphProperty_Base> USMGraphK2Node_TextPropertyNode::GetGraphNodeWidget() const
{
	return SNew(SSMTextProperty)
		.GraphNode(const_cast<USMGraphK2Node_TextPropertyNode*>(this))
		.WidgetInfo(&TextProperty.WidgetInfo);
}

void USMGraphK2Node_TextPropertyNode::DefaultPropertyActionWhenPlaced(TSharedPtr<SWidget> Widget)
{
	if (SSMTextProperty* TextWidget = (SSMTextProperty*)Widget.Get())
	{
		TextWidget->ToggleTextEdit(true);
	}
}

void USMGraphK2Node_TextPropertyNode::ResetProperty()
{
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "ResetProperty", "Reset Property"));
	Super::ResetProperty();
	Cast<USMTextPropertyGraph>(GetPropertyGraph())->SetTextBody(FText::GetEmpty());
}

void USMGraphK2Node_TextPropertyNode::SetPropertyDefaultsFromPin()
{
	// Empty because setting defaults doesn't work properly here. The Result pin will receive the entire graph property struct as text.
}

void USMGraphK2Node_TextPropertyNode::SetPinValueFromPropertyDefaults(bool bUpdateTemplateDefaults)
{
	// Empty because setting defaults doesn't work properly here. The Result pin will receive the entire graph property struct as text.
}

void USMGraphK2Node_TextPropertyNode::Internal_GetContextMenuActionsForOwningNode(const UEdGraph* CurrentGraph,
	const UEdGraphNode* InGraphNode, const UEdGraphPin* InGraphPin, FToolMenuSection& MenuSection, bool bIsDebugging) const
{
	if (!GetPropertyGraph()->IsGraphBeingUsedToEdit() && !bIsDebugging)
	{
		MenuSection.AddMenuEntry(FSMExtendedEditorCommands::Get().StartTextPropertyEdit);
	}
	Super::Internal_GetContextMenuActionsForOwningNode(CurrentGraph, InGraphNode, InGraphPin, MenuSection, bIsDebugging);
}

#undef LOCTEXT_NAMESPACE
