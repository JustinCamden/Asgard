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

	UPROPERTY()
	FSMTextGraphProperty_Runtime RuntimeTextProperty;
	
	UPROPERTY(EditAnywhere, Category = "Graph Property")
	FSMTextGraphProperty TextProperty;
	
	// UEdGraphNode
	virtual void AllocateDefaultPins() override;
	// ~UedGraphNode
	
	// USMGraphK2Node_PropertyNode
	virtual void ConfigureRuntimePropertyNode() override;
	virtual FSMGraphProperty_Base_Runtime* GetRuntimePropertyNode() override;
	virtual FSMGraphProperty_Base* GetPropertyNode() override { return &TextProperty; }
	virtual void SetPropertyNode(FSMGraphProperty_Base* NewNode) override { TextProperty = *(FSMTextGraphProperty*)NewNode; }
	virtual TSharedPtr<SSMGraphProperty_Base> GetGraphNodeWidget() const override;
	virtual bool IsConsideredForDefaultProperty() const override { return TextProperty.WidgetInfo.bConsiderForDefaultWidget; }
	virtual void DefaultPropertyActionWhenPlaced(TSharedPtr<SWidget> Widget) override;
	virtual void ResetProperty() override;
	virtual void SetPropertyDefaultsFromPin() override;
	virtual void SetPinValueFromPropertyDefaults(bool bUpdateTemplateDefaults) override;
protected:
	virtual void Internal_GetContextMenuActionsForOwningNode(const UEdGraph* CurrentGraph, const UEdGraphNode* InGraphNode, const UEdGraphPin* InGraphPin, FToolMenuSection& MenuSection, bool bIsDebugging) const override;
	// ~USMGraphK2Node_PropertyNode
};