// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Graph/Schema/SMPropertyGraphSchema.h"
#include "SMTextPropertyGraphSchema.generated.h"


UCLASS()
class SMEXTENDEDEDITOR_API USMTextPropertyGraphSchema : public USMPropertyGraphSchema
{
	GENERATED_UCLASS_BODY()

public:
	// UEdGraphSchema_K2
	void CreateDefaultNodesForGraph(UEdGraph& Graph) const override;
	bool CanDuplicateGraph(UEdGraph* InSourceGraph) const override { return false; }
	/** This isn't currently called by UE4. */
	void GetGraphDisplayInformation(const UEdGraph& Graph, /*out*/ FGraphDisplayInfo& DisplayInfo) const override;
	// ~UEdGraphSchema_K2
};

