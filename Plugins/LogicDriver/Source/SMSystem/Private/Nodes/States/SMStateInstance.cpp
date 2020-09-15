// Copyright Recursoft LLC 2019-2020. All Rights Reserved.

#include "SMStateInstance.h"
#include "SMInstance.h"
#include "SMState.h"
#include "SMLogging.h"
#include "SMStateMachine.h"
#include "SMTransitionInstance.h"
#if WITH_EDITORONLY_DATA
#include "Engine/BlueprintGeneratedClass.h"
#endif

USMStateInstance_Base::USMStateInstance_Base() : Super(),
bEvalGraphsOnStart(true), bEvalGraphsOnUpdate(false), bEvalGraphsOnEnd(false), bEvalGraphsOnInitialize(true), bEvalGraphsOnRootStateMachineStart(false), bEvalGraphsOnRootStateMachineStop(false)
{
#if WITH_EDITORONLY_DATA
	// TODO: Read editor settings.
	NodeEndStateColor = FLinearColor(1.f, 1.f, 1.f, 0.7f);
	bDisplayNameWidget = true;
	bRegisterWithContextMenu = true;
	bHideFromContextMenuIfRulesFail = false;
#endif
}

bool USMStateInstance_Base::IsInEndState() const
{
	if (FSMState_Base* State = (FSMState_Base*)GetOwningNode())
	{
		return State->IsInEndState();
	}

	return false;
}

void USMStateInstance_Base::GetStateInfo(FSMStateInfo& State) const
{
	if (FSMState_Base* StateNode = (FSMState_Base*)GetOwningNode())
	{
		State = FSMStateInfo(*StateNode);
	}
	else
	{
		State = FSMStateInfo();
	}
}

bool USMStateInstance_Base::IsStateMachine() const
{
	if (FSMState_Base* State = (FSMState_Base*)GetOwningNode())
	{
		return State->IsStateMachine();
	}

	return false;
}

void USMStateInstance_Base::SetActive(bool bValue)
{
	if (FSMState_Base* State = (FSMState_Base*)GetOwningNode())
	{
		if (FSMStateMachine* StateMachine = (FSMStateMachine*)State->GetOwnerNode())
		{
			if (bValue)
			{
				StateMachine->AddActiveState(State, true);
			}
			else
			{
				StateMachine->RemoveActiveState(State, true);
			}
		}
	}
}

void USMStateInstance_Base::EvaluateTransitions()
{
	if (USMInstance* StateMachineInstance = GetStateMachineInstance(true))
	{
		StateMachineInstance->EvaluateTransitions();
	}
}

bool USMStateInstance_Base::GetOutgoingTransitions(TArray<USMTransitionInstance*>& Transitions, bool bExcludeAlwaysFalse) const
{
	Transitions.Reset();

	if (const FSMState_Base* Node = (FSMState_Base*)GetOwningNodeContainer())
	{
		for (FSMTransition* Transition : Node->GetOutgoingTransitions())
		{
			if (Transition->bAlwaysFalse && bExcludeAlwaysFalse)
			{
				continue;
			}
			if (USMTransitionInstance* TransitionInstance = Cast<USMTransitionInstance>(Transition->GetNodeInstance()))
			{
				Transitions.AddUnique(TransitionInstance);
			}
		}
	}

	return Transitions.Num() > 0;
}

bool USMStateInstance_Base::GetIncomingTransitions(TArray<USMTransitionInstance*>& Transitions,
	bool bExcludeAlwaysFalse) const
{
	Transitions.Reset();

	if (const FSMState_Base* Node = (FSMState_Base*)GetOwningNodeContainer())
	{
		for (FSMTransition* Transition : Node->GetIncomingTransitions())
		{
			if (Transition->bAlwaysFalse && bExcludeAlwaysFalse)
			{
				continue;
			}
			if (USMTransitionInstance* TransitionInstance = Cast<USMTransitionInstance>(Transition->GetNodeInstance()))
			{
				Transitions.AddUnique(TransitionInstance);
			}
		}
	}

	return Transitions.Num() > 0;
}

USMTransitionInstance* USMStateInstance_Base::GetTransitionToTake() const
{
	if (FSMState_Base* State = (FSMState_Base*)GetOwningNodeContainer())
	{
		if (const FSMTransition* Transition = State->GetTransitionToTake())
		{
			return Cast<USMTransitionInstance>(Transition->GetNodeInstance());
		}
	}

	return nullptr;
}

bool USMStateInstance_Base::SwitchToLinkedState(USMStateInstance_Base* NextStateInstance, bool bRequireTransitionToPass)
{
	if (const FSMState_Base* Node = (FSMState_Base*)GetOwningNode())
	{
		if (!Node->IsActive())
		{
			LOG_WARNING(TEXT("Attempted to switch to linked state %s but this node %s is not currently active."), *NextStateInstance->GetName(), *Node->GetNodeName());
			return false;
		}

		// Find the state in the available transitions.
		for (FSMTransition* Transition : Node->GetOutgoingTransitions())
		{
			if (NextStateInstance == Transition->GetToState()->GetNodeInstance())
			{
				if (bRequireTransitionToPass && !Transition->DoesTransitionPass())
				{
					return false;
				}

				// Notify the owning state machine to take this transition.
				((FSMStateMachine*)Transition->GetOwnerNode())->ProcessTransition(Transition, nullptr, 0.f);
				return true;
			}
		}

		LOG_WARNING(TEXT("Attempted to switch to linked state %s from %s but the node could not be found."), *NextStateInstance->GetName(), *Node->GetNodeName());
	}

	return false;
}

USMTransitionInstance* USMStateInstance_Base::GetTransitionByIndex(int32 Index) const
{
	TArray<USMTransitionInstance*> Transitions;
	if (GetOutgoingTransitions(Transitions, false))
	{
		if (Index >= 0 && Index < Transitions.Num())
		{
			return Transitions[Index];
		}
	}

	return nullptr;
}

USMStateInstance_Base* USMStateInstance_Base::GetNextStateByTransitionIndex(int32 Index) const
{
	USMTransitionInstance* FoundTransition = GetTransitionByIndex(Index);
	return FoundTransition ? FoundTransition->GetNextStateInstance() : nullptr;
}

void USMStateInstance_Base::GetAllNodesOfType(TArray<USMNodeInstance*>& OutNodes, TSubclassOf<USMNodeInstance> NodeClass, bool bIncludeChildren,
                                              const TArray<UClass*>& StopIfTypeIsNot) const
{
	// If true then the node cycles back to itself at some point.
	if (OutNodes.Contains(this))
	{
		return;
	}

	// Only count if this node is of the right type.
	if ((bIncludeChildren && GetClass()->IsChildOf(NodeClass)) || GetClass() == NodeClass)
	{
		OutNodes.Add(const_cast<USMStateInstance_Base*>(this));
	}

	// Look for all connected nodes.
	if (FSMState_Base* Node = (FSMState_Base*)GetOwningNode())
	{
		for (FSMTransition* Transition : Node->GetOutgoingTransitions())
		{
			FSMState_Base* NextState = Transition->GetToState();
			if (USMStateInstance_Base* Instance = Cast<USMStateInstance_Base>(NextState->GetNodeInstance()))
			{
				// We break the search when a forbidden type is hit.
				if (StopIfTypeIsNot.Num() > 0)
				{
					bool bAllowed = false;
					for (UClass* Class : StopIfTypeIsNot)
					{
						if (Instance->GetClass()->IsChildOf(Class))
						{
							bAllowed = true;
							break;
						}
					}

					if (!bAllowed)
					{
						continue;
					}
				}

				Instance->GetAllNodesOfType(OutNodes, NodeClass, bIncludeChildren, StopIfTypeIsNot);
			}
		}
	}
}

#if WITH_EDITORONLY_DATA
bool USMStateInstance_Base::IsRegisteredWithContextMenu() const
{
	return bRegisterWithContextMenu && Cast<UBlueprintGeneratedClass>(GetClass());
}
#endif

USMStateInstance::USMStateInstance() : Super()
{
}

void USMStateInstance::GetAllStateStackInstances(TArray<USMStateInstance_Base*>& StateStackInstances) const
{
	StateStackInstances.Reset();
	
	if (const FSMNode_Base* State = GetOwningNode())
	{
		const TArray<USMNodeInstance*>& StateStack = State->GetStackInstances();
		StateStackInstances.Reserve(StateStack.Num());
		
		for (USMNodeInstance* Node : StateStack)
		{
			if (USMStateInstance_Base* StateInstance = Cast<USMStateInstance_Base>(Node))
			{
				StateStackInstances.Add(StateInstance);
			}
		}
	}
}

USMStateInstance_Base* USMStateInstance::GetStateInStack(int32 Index) const
{
	if (const FSMNode_Base* State = GetOwningNode())
	{
		const TArray<USMNodeInstance*>& StateStack = State->GetStackInstances();
		if (Index >= 0 && Index < StateStack.Num())
		{
			return Cast<USMStateInstance_Base>(StateStack[Index]);
		}
	}

	return nullptr;
}

USMStateInstance_Base* USMStateInstance::GetStateInStackByClass(TSubclassOf<USMStateInstance> StateClass,
	bool bIncludeChildren) const
{
	if (const FSMNode_Base* State = GetOwningNode())
	{
		const TArray<USMNodeInstance*>& StateStack = State->GetStackInstances();
		for (USMNodeInstance* Node : StateStack)
		{
			if ((bIncludeChildren && Node->GetClass()->IsChildOf(StateClass)) || Node->GetClass() == StateClass)
			{
				return Cast<USMStateInstance_Base>(Node);
			}
		}
	}

	return nullptr;
}

USMStateInstance_Base* USMStateInstance::GetStackOwnerInstance() const
{
	if (const FSMNode_Base* State = GetOwningNode())
	{
		return Cast<USMStateInstance_Base>(State->GetNodeInstance());
	}

	return nullptr;
}

void USMStateInstance::GetAllStatesInStackOfClass(TSubclassOf<USMStateInstance> StateClass, TArray<USMStateInstance_Base*>& StateStackInstances, bool bIncludeChildren) const
{
	StateStackInstances.Reset();
	
	if (const FSMNode_Base* State = GetOwningNode())
	{
		const TArray<USMNodeInstance*>& StateStack = State->GetStackInstances();
		for (USMNodeInstance* Node : StateStack)
		{
			if ((bIncludeChildren && Node->GetClass()->IsChildOf(StateClass)) || Node->GetClass() == StateClass)
			{
				if (USMStateInstance_Base* StateNode = Cast<USMStateInstance_Base>(Node))
				{
					StateStackInstances.Add(StateNode);
				}
			}
		}
	}
}

int32 USMStateInstance::GetStateStackCount() const
{
	if (const FSMNode_Base* State = GetOwningNode())
	{
		return State->GetStackInstances().Num();
	}

	return 0;
}
