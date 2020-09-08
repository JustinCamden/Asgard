// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Compilers/SMKismetCompiler.h"
#include "Graph/SMPropertyGraph.h"
#include "Graph/Nodes/RootNodes/SMGraphK2Node_RuntimeNodeContainer.h"
#include "SMGraphK2Node_PropertyNode.generated.h"


DECLARE_MULTICAST_DELEGATE(FForceVisualRefresh);

/**
 * Pure root reference nodes that are placed within a property graph returning a value.
 * USMGraphNode_Base -> K2 BoundGraph -> Property Graph -> Property Node (this)
 * Slate Node for USMGraphNode_Base -> this->GetGraphNodeWidget
 * Details Panel for USMGraphNode_Base -> this->GetGraphDetailWidget
 */
UCLASS(Abstract)
class SMSYSTEMEDITOR_API USMGraphK2Node_PropertyNode_Base : public USMGraphK2Node_RuntimeNodeReference
{
	GENERATED_UCLASS_BODY()

public:
	
	// UEdGraphNode
	bool IsNodePure() const override { return true; }
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FText GetTooltipText() const override;
	void PostPlacedNewNode() override;
	void PinDefaultValueChanged(UEdGraphPin* Pin) override;
	void NotifyPinConnectionListChanged(UEdGraphPin* Pin) override;
	bool HasExternalDependencies(TArray<UStruct*>* OptionalOutput) const override;
	bool CanCollapseNode() const override { return false; }
	// ~UedGraphNode

	// USMGraphK2Node_RuntimeNodeReference
	void PreCompileValidate(FCompilerResultsLog& MessageLog) override;
	// ~USMGraphK2Node_RuntimeNodeReference
	
	USMPropertyGraph* GetPropertyGraph() const { return Cast<USMPropertyGraph>(GetGraph()); }
	
	/** Get the runtime graph property. */
	virtual FSMGraphProperty_Base* GetPropertyNode() { return nullptr; }
	/** Get the runtime graph property. */
	FSMGraphProperty_Base* GetPropertyNodeChecked() { FSMGraphProperty_Base* Node = GetPropertyNode(); check(Node); return Node; }
	FSMGraphProperty_Base* GetPropertyNodeConst() const { return const_cast<USMGraphK2Node_PropertyNode_Base*>(this)->GetPropertyNode(); }
	FSMGraphProperty_Base* GetPropertyNodeConstChecked() const { FSMGraphProperty_Base* Node = GetPropertyNodeConst(); check(Node); return Node; }
	/** Sets the new node. Useful for refreshing the node with updated values from a template. */
	virtual void SetPropertyNode(FSMGraphProperty_Base* NewNode) {}

	/** Reset everything. */
	virtual void ResetProperty();
	/** Sets the property on the node template to match this pin. */
	virtual void SetPropertyDefaultsFromPin();
	/** Sets the pin default value from the property default value. */
	virtual void SetPinValueFromPropertyDefaults(bool bUpdateTemplateDefaults = false);
	/** Get the runtime graph property type. */
	UScriptStruct* GetPropertyNodeType() const;
	/** Get the runtime graph property as a FStructProperty. */
	FStructProperty* GetPropertyNodeProperty() const;
	/** The template which owns this property. */
	USMNodeInstance* GetOwningTemplate() const;
	/** The blueprint of the template owning this property. */
	USMNodeBlueprint* GetTemplateBlueprint() const;
	/** The property graph containing this property node. */
	UEdGraph* GetOwningGraph() const;
	/** The state machine graph node which owns this property. */
	class USMGraphNode_Base* GetOwningGraphNode() const;
	/** Open the property graph for this node. Should be the containing graph. */
	void JumpToPropertyGraph();
	
	/** Get the widget to represent this property node within a state machine graph node. */
	virtual TSharedPtr<class SSMGraphProperty_Base> GetGraphNodeWidget() const;
	/** Get the widget to represent the graph node in the details panel.*/
	virtual TSharedPtr<SWidget> GetGraphDetailWidget() const;
	/** Widget used to go to the graph. */
	virtual TSharedPtr<SWidget> GetViewGraphDetailWidget() const;
	/** Widget used to toggled edit mode of the graph. */
	virtual TSharedPtr<SWidget> GetToggleEditGraphDetailWidget() const;
	/** This forwards up context menu actions when the owning state machine graph node is right clicked on over this property. */
	void GetContextMenuActionsForOwningNode(const UEdGraph* CurrentGraph, const UEdGraphNode* InGraphNode, const UEdGraphPin* InGraphPin, class UToolMenu* ToolMenu, bool bIsDebugging) const;


	/** If the sgraphnode doesn't have a default name field then this property may be used as the default property to edit. */
	virtual bool IsConsideredForDefaultProperty() const { return false; }
	/** If the sgraphnode chooses this property as the default it will pass the already constructed widget in (should be the same type as GetGraphDetailWidget())
	 * so it can be used for a default action -- such as automatically editing text. */
	virtual void DefaultPropertyActionWhenPlaced(TSharedPtr<SWidget> Widget) {}

	/** Locates the result pin if one exists. */
	virtual UEdGraphPin* GetResultPin() const;
	UEdGraphPin* GetResultPinChecked() const { UEdGraphPin* Pin = GetResultPin(); check(Pin); return Pin; }
	
	UPROPERTY()
	class USMGraphNode_Base* OwningGraphNode;

	/** Set from slate widget representing this property. Used to help determine if the context menu creation should
	 * forward creation to this node as well.
	 * TODO: There has to be a better way of tracing the cursor to a widget. */
	bool bMouseOverNodeProperty;

	void ForceVisualRefresh() const { ForceVisualRefreshEvent.Broadcast(); }
	FForceVisualRefresh ForceVisualRefreshEvent;

protected:
	virtual void Internal_GetContextMenuActionsForOwningNode(const UEdGraph* CurrentGraph, const UEdGraphNode* InGraphNode, const UEdGraphPin* InGraphPin, FToolMenuSection& MenuSection, bool bIsDebugging) const;

protected:
	/** The last default autogenerated value. The pin value resets to this if the default value hasn't changed. */
	UPROPERTY()
	FString LastAutoGeneratedDefaultValue;

	/** True once the user has changed the default value. It cannot become false again. */
	UPROPERTY(NonTransactional)
	bool bDefaultValueChanged;

	/** True only while setting default value. */
	bool bGeneratedDefaultValueBeingSet;
};
