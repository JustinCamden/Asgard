// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once
#include "Kismet2/BlueprintEditorUtils.h"
#include "Graph/SMGraph.h"


// Helpers for managing node instances and related objects.
class SMSYSTEMEDITOR_API FSMNodeInstanceUtils
{
public:
	/** Recursively checks a child to see if it belongs to a parent. */
	static bool IsWidgetChildOf(TSharedPtr<SWidget> Parent, TSharedPtr<SWidget> PossibleChild);

	static TSharedPtr<SWidget> CreateNodeClassWidgetDisplay(USMNodeInstance* NodeInstance);

	/** Sets all related internal properties. Returns the guid used. */
	static const FGuid& SetGraphPropertyFromProperty(FSMGraphProperty_Base& GraphProperty, FProperty* Property, int32 Index = 0, bool bSetGuid = true);

	static bool IsPropertyExposedToGraphNode(FProperty* Property);

	static FStructProperty* IsPropertyGraphProperty(FProperty* Property);
};

