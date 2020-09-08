// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "SMGraphK2Schema.h"
#include "SMPropertyGraphSchema.generated.h"


UCLASS()
class SMSYSTEMEDITOR_API USMPropertyGraphSchema : public USMGraphK2Schema
{
	GENERATED_UCLASS_BODY()

public:
	// UEdGraphSchema_K2
	void CreateDefaultNodesForGraph(UEdGraph& Graph) const override;
	bool CanDuplicateGraph(UEdGraph* InSourceGraph) const override { return false; }
	void HandleGraphBeingDeleted(UEdGraph& GraphBeingRemoved) const override;
	/** This isn't currently called by UE4. */
	void GetGraphDisplayInformation(const UEdGraph& Graph, /*out*/ FGraphDisplayInfo& DisplayInfo) const override;
	bool TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const override;
	// ~UEdGraphSchema_K2

};

