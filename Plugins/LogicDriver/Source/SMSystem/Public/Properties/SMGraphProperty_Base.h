// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphPin.h"
#include "Engine/MemberReference.h"
#include "SMNodeWidgetInfo.h"
#include "SMGraphProperty_Base.generated.h"


USTRUCT(BlueprintInternalUseOnly)
struct SMSYSTEM_API FSMExposedFunctionHandler
{
	GENERATED_USTRUCT_BODY()

	FSMExposedFunctionHandler()
		: BoundFunction(NAME_None)
		  , Function(nullptr)
		  , OwnerObject(nullptr), bInitialized(false)
	{
	}

	/** Name of the graph function we will be evaluating. */
	UPROPERTY()
	FName BoundFunction;

	/** Lookup the UFunction by the function name. */
	void Initialize(UObject* StateMachineObject);
	void Reset();

	/** Execute the function. */
	void Execute(void* Parms = nullptr) const;

private:
	UPROPERTY()
	UFunction* Function;

	UPROPERTY()
	UObject* OwnerObject;

	bool bInitialized;
};

/**
 * The base graph properties containing the bare essentials for run-time.
 * Any run-time graph property types should inherit from this.
 */
USTRUCT()
struct SMSYSTEM_API FSMGraphProperty_Base_Runtime
{
public:
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FSMExposedFunctionHandler GraphEvaluator;
	
	FSMGraphProperty_Base_Runtime();
	virtual ~FSMGraphProperty_Base_Runtime() = default;
	
	virtual void Initialize(UObject* Instance);
	virtual void Execute(void* Params = nullptr);
	virtual void Reset();
	
	virtual uint8* GetResult() const { return nullptr; }
	virtual void SetResult(uint8* Value) {}

	virtual const FGuid& SetGuid(const FGuid& NewGuid);
	const FGuid& GetGuid() const { return Guid; }
	virtual const FGuid& SetOwnerGuid(const FGuid& NewGuid);
	
	/** Returns the graph property owner of this node. Likely itself. */
	const FGuid& GetOwnerGuid() const { return OwnerGuid; }

	/** If set then the linked property is the one that is actually executing, but this struct is the one being read from. */
	FSMGraphProperty_Base_Runtime* LinkedProperty;
	
protected:
	UPROPERTY(meta = (IgnoreForMemberInitializationTest))
	FGuid Guid;

	/** The graph property owner. If this struct is defined within a node class and instanced
	 * to a state machine then the guid of class CDO is the owner. */
	UPROPERTY(meta = (IgnoreForMemberInitializationTest))
	FGuid OwnerGuid;
};

/**
 * Graph properties which represent a variable exposed on a node. Only for run-time use.
 */
USTRUCT()
struct SMSYSTEM_API FSMGraphProperty_Runtime : public FSMGraphProperty_Base_Runtime
{
public:
	GENERATED_USTRUCT_BODY()

	FSMGraphProperty_Runtime();
};


/**
 * EDITOR: The base struct for graph properties exposed on a node. Contains additional properties for configuration and compilation.
 * Any graph property types should inherit from this.
 */
USTRUCT()
struct SMSYSTEM_API FSMGraphProperty_Base : public FSMGraphProperty_Base_Runtime
{
public:
	GENERATED_USTRUCT_BODY()
	
	FSMGraphProperty_Base();
	virtual ~FSMGraphProperty_Base() = default;
	
	// FSMGraphProperty_Base_Runtime
	virtual const FGuid& SetGuid(const FGuid& NewGuid) override;
	// ~FSMGraphProperty_Base_Runtime
	const FGuid& SetGuid(const FGuid& NewGuid, int32 Index, bool bCountTemplate = true);
	const FGuid& GenerateNewGuid();
	const FGuid& GenerateNewGuidIfNotValid();

	void InvalidateGuid();
	
	const FGuid& SetTemplateGuid(const FGuid& NewGuid, bool bRefreshGuid = false);
	const FGuid& GetTemplateGuid() const { return TemplateGuid; }

	const FGuid& GetUnmodifiedGuid() const { return GuidUnmodified; }
	
	/** Used if this class should automatically assign itself to exposed variable properties. */
	virtual bool ShouldAutoAssignVariable() const { return VariableName != NAME_None; }
	/** Checked during duplication if the guid should be assigned from the variable. */
	virtual bool ShouldGenerateGuidFromVariable() const { return ShouldAutoAssignVariable(); }
	
	UPROPERTY(EditDefaultsOnly, Category = "Variable")
	FName VariableName;

	UPROPERTY()
	FEdGraphPinType VariableType;

	UPROPERTY()
	FMemberReference MemberReference;

	UPROPERTY()
	bool bIsInArray;
	
#if WITH_EDITORONLY_DATA
	/** Return the editor graph to use. The outer provided will be the editor module so the class can be retrieved. */
	virtual UClass* GetGraphClass(UObject* Outer) const;
	/** Return the editor graph schema to use. The outer provided will be the editor module so the class can be retrieved. */
	virtual UClass* GetGraphSchemaClass(UObject* Outer) const;
	/** Return the simple name of the editor module to use for looking up the graph classes. */
	virtual const FString& GetGraphModuleName() const;
	/** Load and return the editor module package. This is what will be passed to GetGraphClass and GetGraphSchemaClass. */
	virtual UPackage* GetEditorModule() const;
	/** The desired name of this property. This may be used at some display points in the editor. */
	virtual const FString& GetPropertyDisplayName() const;
	/** The desired vertical location on the graph node for this widget to be displayed. */
	virtual int32 GetVerticalDisplayOrder() const { return 0; }
	/** Should property be able to toggle between edit and view modes. */
	virtual bool AllowToggleGraphEdit() const { return false; }
	/** Should property default to view or edit mode. */
	virtual bool ShouldDefaultToEditMode() const { return true; }

	/** Returns either the property display name or the RealDisplayName. */
	FText GetDisplayName() const;

	/** Set when loaded in a graph node. */
	UPROPERTY()
	FText RealDisplayName;
	
	/** Set when loaded in a graph node in the event this is stored within an array. */
	UPROPERTY()
	int32 ArrayIndex;
protected:
	UPROPERTY()
	FName GraphClassName;
	UPROPERTY()
	FName GraphSchemaClassName;
	UPROPERTY()
	FString GraphModuleClassName;

	UPROPERTY(Transient)
	UClass* CachedGraphClass;
	UPROPERTY(Transient)
	UClass* CachedSchemaClass;
#endif
	
protected:
	/** The guid without the template. */
	UPROPERTY(meta = (IgnoreForMemberInitializationTest))
	FGuid GuidUnmodified;

	/** The guid of the template this belongs to. */
	UPROPERTY(meta = (IgnoreForMemberInitializationTest))
	FGuid TemplateGuid;

	UPROPERTY()
	int32 GuidIndex;
};

/**
 * EDITOR: Graph properties which represent a variable exposed on a node. Contains additional properties for configuration and compilation.
 */
USTRUCT()
struct SMSYSTEM_API FSMGraphProperty : public FSMGraphProperty_Base
{
public:
	GENERATED_USTRUCT_BODY()

	FSMGraphProperty();

#if WITH_EDITORONLY_DATA
	virtual int32 GetVerticalDisplayOrder() const override { return WidgetInfo.DisplayOrder; }
	
	UPROPERTY(EditDefaultsOnly, Category = "Widget")
	FSMTextDisplayWidgetInfo WidgetInfo;
#endif
};