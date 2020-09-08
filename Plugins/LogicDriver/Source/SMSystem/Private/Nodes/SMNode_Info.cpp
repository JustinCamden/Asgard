// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#include "SMNode_Info.h"
#include "SMTransition.h"


FSMInfo_Base::FSMInfo_Base(): NodeInstance(nullptr)
{
}

FSMInfo_Base::FSMInfo_Base(const FSMNode_Base& Node)
{
	this->Guid = Node.GetGuid();
	this->OwnerGuid = Node.GetOwnerNode() ? Node.GetOwnerNode()->GetGuid() : FGuid();
	this->NodeName = Node.GetNodeName();

	this->NodeGuid = Node.GetNodeGuid();
	this->OwnerNodeGuid = Node.GetOwnerNodeGuid();

	this->NodeInstance = Node.GetNodeInstance();
}

FSMTransitionInfo::FSMTransitionInfo() : Super()
{
	Priority = 0;
}

FSMTransitionInfo::FSMTransitionInfo(const FSMTransition& Transition) : Super(Transition)
{
	FromStateGuid = Transition.GetFromState()->GetGuid();
	ToStateGuid = Transition.GetToState()->GetGuid();
	Priority = Transition.Priority;
}

FSMStateInfo::FSMStateInfo() : Super(), bIsEndState(false)
{
}

FSMStateInfo::FSMStateInfo(const FSMState_Base& State) : Super(State)
{
	this->bIsEndState = State.IsEndState();
	
	for (FSMTransition* Transition : State.GetOutgoingTransitions())
	{
		OutgoingTransitions.Add(FSMTransitionInfo(*Transition));
	}
}
