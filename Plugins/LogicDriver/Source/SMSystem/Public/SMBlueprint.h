// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "Engine/Blueprint.h"
#include "SMBlueprint.generated.h"

#define STATEMACHINE_BLUEPRINT_VERSION 1
#define STATEMACHINE_NODE_BLUEPRINT_VERSION 1

/**
 * State Machine Blueprints allow you to assemble a finite state machine which is capable of running normal Blueprint logic.
 * Any UObject instance may be passed as a context for the state machine.
 */
UCLASS(BlueprintType, meta = (DisplayName = "State Machine Blueprint"))
class SMSYSTEM_API USMBlueprint : public UBlueprint
{
	GENERATED_UCLASS_BODY()

public:

#if WITH_EDITOR
	// UBlueprint
	bool SupportedByDefaultBlueprintFactory() const override { return false; }
	UClass* GetBlueprintClass() const override;
	void GetReparentingRules(TSet<const UClass*>& AllowedChildrenOfClasses, TSet<const UClass*>& DisallowedChildrenOfClasses) const override;
	void NotifyGraphRenamed(UEdGraph* Graph, FName OldName, FName NewName) override;
	// ~UBlueprint

	USMBlueprint* FindOldestParentBlueprint() const;

	DECLARE_MULTICAST_DELEGATE_FourParams(FOnRenameGraph, UBlueprint* /*Blueprint*/, UEdGraph* /*VariableClass*/, const FName& /*OldVarName*/, const FName& /*NewVarName*/);
	/** Event fired when a graph in a state machine blueprint is renamed. */
	static FOnRenameGraph OnRenameGraphEvent;
#endif
	
	class USMBlueprintGeneratedClass* GetGeneratedClass() const;

	/** For load validation. */
	int32 GetBlueprintVersion() const { return InstanceVersion; }
private:
	UPROPERTY()
	int32 InstanceVersion = STATEMACHINE_BLUEPRINT_VERSION;
};


/**
 * State Machine Node Class Blueprints allow you to define custom classes for nodes within your State Machine Blueprint. Reusable logic can be encapsulated
 * here as well as the ability to take more direct control over state machines by gaining access to other node class instances. Additionally these classes
 * can be extended using C++.
 */
UCLASS(BlueprintType, meta = (DisplayName = "State Machine Node Class Blueprint"))
class SMSYSTEM_API USMNodeBlueprint : public UBlueprint
{
	GENERATED_UCLASS_BODY()

public:

#if WITH_EDITOR
	// UBlueprint
	bool SupportedByDefaultBlueprintFactory() const override { return false; }
	UClass* GetBlueprintClass() const override;
	void GetReparentingRules(TSet<const UClass*>& AllowedChildrenOfClasses, TSet<const UClass*>& DisallowedChildrenOfClasses) const override;
	// ~UBlueprint

#endif

	class USMNodeBlueprintGeneratedClass* GetGeneratedClass() const;

	/** For load validation. */
	int32 GetBlueprintVersion() const { return InstanceVersion; }
	
private:
	UPROPERTY()
	int32 InstanceVersion = STATEMACHINE_NODE_BLUEPRINT_VERSION;
};
