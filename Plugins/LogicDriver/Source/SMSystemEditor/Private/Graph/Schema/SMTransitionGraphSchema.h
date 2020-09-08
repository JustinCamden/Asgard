// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphSchema.h"
#include "EdGraphSchema_K2.h"
#include "SMGraphK2Schema.h"
#include "SMTransitionGraphSchema.generated.h"


UCLASS()
class SMSYSTEMEDITOR_API USMTransitionGraphSchema : public USMGraphK2Schema
{
	GENERATED_UCLASS_BODY()

	//~ Begin UEdGraphSchema Interface.
	void CreateDefaultNodesForGraph(UEdGraph& Graph) const override;
	void GetGraphDisplayInformation(const UEdGraph& Graph, /*out*/ FGraphDisplayInfo& DisplayInfo) const override;
	bool DoesSupportEventDispatcher() const	override { return false; }
	bool ShouldAlwaysPurgeOnModification() const override { return true; }
	bool CanDuplicateGraph(UEdGraph* InSourceGraph) const override { return false; }
	void HandleGraphBeingDeleted(UEdGraph& GraphBeingRemoved) const override;
	//~ End UEdGraphSchema Interface.
};