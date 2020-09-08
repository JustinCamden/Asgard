// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "SMGraphK2.h"
#include "Nodes/RootNodes/SMGraphK2Node_ConduitResultNode.h"
#include "SMConduitGraph.generated.h"


UCLASS(MinimalAPI)
class USMConduitGraph : public USMGraphK2
{
	GENERATED_UCLASS_BODY()

public:
	// USMGraphK2
	bool HasAnyLogicConnections() const override;
	FSMNode_Base* GetRuntimeNode() const override { return ResultNode->GetRunTimeNode(); }
	// ~USMGraphK2

public:
	UPROPERTY()
	class USMGraphK2Node_ConduitResultNode* ResultNode;
};
