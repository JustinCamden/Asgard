// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#include "SMExtendedPropertyHelpers.h"


void USMExtendedGraphPropertyHelpers::BreakTextGraphProperty(const FSMTextGraphProperty& GraphProperty, FText& Result)
{
	const_cast<FSMTextGraphProperty&>(GraphProperty).Execute();
	Result = GraphProperty.Result;
}
