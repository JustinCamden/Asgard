// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "EdGraphSchema_K2.h"
#include "SMGraphK2Schema.generated.h"


UCLASS()
class SMSYSTEMEDITOR_API USMGraphK2Schema : public UEdGraphSchema_K2
{
	GENERATED_UCLASS_BODY()

public:
	static const FName PC_StateMachine;
	static const FName GN_StateMachineDefinitionGraph;

	// UEdGraphSchema_K2
	void GetContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;
	void CreateDefaultNodesForGraph(UEdGraph& Graph) const override;
	bool CanDuplicateGraph(UEdGraph* InSourceGraph) const override { return false; }
	void HandleGraphBeingDeleted(UEdGraph& GraphBeingRemoved) const override;
	const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* PinA, const UEdGraphPin* PinB) const override;
	/** This isn't currently called by UE4. */
	bool CanEncapuslateNode(UEdGraphNode const& TestNode) const override;
	void GetGraphDisplayInformation(const UEdGraph& Graph, /*out*/ FGraphDisplayInfo& DisplayInfo) const override;
	// ~UEdGraphSchema_K2

	static UEdGraphPin* GetThenPin(UEdGraphNode* Node);
	static bool IsThenPin(UEdGraphPin* Pin);
};

