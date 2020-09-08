// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "SMGraphK2.h"
#include "SMPropertyGraph.generated.h"


UCLASS()
class SMSYSTEMEDITOR_API USMPropertyGraph : public USMGraphK2
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	class USMGraphK2Node_PropertyNode_Base* ResultNode;

	/** Temporarily set during graph initialization. */
	FSMGraphProperty_Base* TempGraphProperty;

	/** Called from owning state machine graph node. */
	virtual void RefreshProperty(bool bModify = true);

	/** Delete all nodes and recreate default nodes. */
	virtual void ResetGraph();

	/** Configure if the graph is editable and update the slate node. */
	virtual void SetUsingGraphToEdit(bool bValue);

	/** If the graph is the primary source of data. */
	virtual bool IsGraphBeingUsedToEdit() const { return bEditable; }

	/** Toggles the property edit value and updates the blueprint. */
	virtual void ToggleGraphPropertyEdit();

	virtual void SetPropertyOnGraph(FProperty* Property);
	virtual void SetFunctionOnGraph(UFunction* Function);

	/** Remove any nodes that aren't connected to the ResultNode. */
	void PruneDisconnectedNodes();

	TSet<UEdGraphPin*> PreventConnections;
};
