// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "Graph/Nodes/PropertyNodes/SMGraphK2Node_PropertyNode.h"
#include "SMTextGraphProperty.h"
#include "SMGraphK2Node_TextPropertyNode.generated.h"

UCLASS()
class SMEXTENDEDEDITOR_API USMGraphK2Node_TextPropertyNode : public USMGraphK2Node_PropertyNode_Base
{
	GENERATED_UCLASS_BODY()

public:
	
	UPROPERTY(EditAnywhere, Category = "Graph Property")
	FSMTextGraphProperty TextProperty;
	
	// UEdGraphNode
	void AllocateDefaultPins() override;
	// ~UedGraphNode
	
	// USMGraphK2Node_PropertyNode
	FSMGraphProperty_Base* GetPropertyNode() override { return &TextProperty; }
	void SetPropertyNode(FSMGraphProperty_Base* NewNode) override { TextProperty = *(FSMTextGraphProperty*)NewNode; }
	TSharedPtr<SSMGraphProperty_Base> GetGraphNodeWidget() const override;
	bool IsConsideredForDefaultProperty() const override { return TextProperty.WidgetInfo.bConsiderForDefaultWidget; }
	void DefaultPropertyActionWhenPlaced(TSharedPtr<SWidget> Widget) override;
	void ResetProperty() override;
	void SetPropertyDefaultsFromPin() override;
	void SetPinValueFromPropertyDefaults(bool bUpdateTemplateDefaults) override;
protected:
	void Internal_GetContextMenuActionsForOwningNode(const UEdGraph* CurrentGraph, const UEdGraphNode* InGraphNode, const UEdGraphPin* InGraphPin, FToolMenuSection& MenuSection, bool bIsDebugging) const override;
	// ~USMGraphK2Node_PropertyNode
};