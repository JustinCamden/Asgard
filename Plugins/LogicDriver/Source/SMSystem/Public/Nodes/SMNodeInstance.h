// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "ISMStateMachineInterface.h"
#include "SMNodeRules.h"
#include "Engine/Texture2D.h"
#include "SMLogging.h"
#include "SMNodeInstance.generated.h"

DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("SMNodeInstances"), STAT_NodeInstances, STATGROUP_LogicDriver, SMSYSTEM_API);

/**
 * This information will be viewable when selecting new nodes or hovering over nodes.
 */
USTRUCT()
struct SMSYSTEM_API FSMNodeDescription
{
	GENERATED_USTRUCT_BODY()

	/** The name of this node type. */
	UPROPERTY(EditDefaultsOnly, Category = "General", meta = (InstancedTemplate))
	FName Name;

	/** Which category this should fall under. */
	UPROPERTY(EditDefaultsOnly, Category = "General", meta = (InstancedTemplate))
	FText Category;

	/** The tooltip when selecting the action. */
	UPROPERTY(EditDefaultsOnly, Category = "General", meta = (InstancedTemplate, MultiLine = true))
	FText Description;
};

/**
 * The abstract base node instance class all state machine nodes derive from.
 * 
 * To expose native member properties on the node they must be marked BlueprintReadWrite and not contain the meta keyword HideOnNode.
 */
UCLASS(Abstract, NotBlueprintable, BlueprintType, classGroup = "State Machine", hideCategories = (SMNodeInstance), meta = (DisplayName = "State Machine Node Class Base"))
class SMSYSTEM_API USMNodeInstance : public UObject, public ISMInstanceInterface
{
	GENERATED_BODY()

public:
	USMNodeInstance();
	
	// UObject
	virtual UWorld* GetWorld() const override;
	virtual void BeginDestroy() override;
	// ~UObject

	// ISMInstanceInterface
	/** The object which this node is running for. Determined by the owning state machine. */
	UFUNCTION(BlueprintCallable, Category = "Logic Driver|Node Instance")
	virtual UObject* GetContext() const override;
	// ~ISMInstanceInterface

	/** A construction script that runs when this node is placed in a graph or instantiated during run-time. */
	UFUNCTION(BlueprintNativeEvent, Category = "Node Instance")
	void ConstructionScript();
	
	/**
	 * Retrieve an owning blueprint state machine.
	 * @param bTopMostInstance If the state machine is a reference return the top most instance.
	 * @return The state machine instance this node is running for.
	 */
	UFUNCTION(BlueprintCallable, Category = "Logic Driver|Node Instance")
	class USMInstance* GetStateMachineInstance(bool bTopMostInstance = false) const;

	/** Set during initialization of the state machine. */
	void SetOwningNode(FSMNode_Base* Node);
	
	/** Reference to the owning node within a state machine. */
	const FSMNode_Base* GetOwningNode() const { return OwningNode; }

	/** Some nodes such as references may have special handling for returning a container node. */
	virtual const FSMNode_Base* GetOwningNodeContainer() const { return GetOwningNode(); }
	
	/** The instance of the direct state machine node this node is part of. Every node except the root state machine has an owning state machine node. */
	UFUNCTION(BlueprintCallable, Category = "Logic Driver|Node Instance")
	class USMStateMachineInstance* GetOwningStateMachineNodeInstance() const;

	/** The current time spent in the state. */
	UFUNCTION(BlueprintCallable, Category = "Logic Driver|Node Instance")
	virtual float GetTimeInState() const;

	/** State Machine is in an end state or the state is an end state. */
	UFUNCTION(BlueprintCallable, Category = "Logic Driver|Node Instance")
	virtual bool IsInEndState() const;

	/** State has updated at least once. */
	UFUNCTION(BlueprintCallable, Category = "Logic Driver|Node Instance")
	virtual bool HasUpdated() const;
	
	/** If this node is active. */
	UFUNCTION(BlueprintCallable, Category = "Logic Driver|Node Instance")
	bool IsActive() const;

	/** Retrieve the node name. */
	UFUNCTION(BlueprintCallable, Category = "Logic Driver|Node Instance")
	const FString& GetNodeName() const;
	
	/** Unique identifier taking into account qualified path. Unique across blueprints if called after Instance initialization. */
	UFUNCTION(BlueprintCallable, Category = "Logic Driver|Node Instance")
	const FGuid& GetGuid() const;
	
	/** Retrieve the icon representing this node. Null by default. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Node Display")
	UTexture2D* GetNodeIcon() const;

	/** Retrieve the size to use when displaying the icon. Leave 0,0 to auto size. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Node Display")
	FVector2D GetNodeIconSize() const;

	/** Retrieve the tint to use when displaying the icon. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Node Display")
	FLinearColor GetNodeIconTintColor() const;
	
	/** Evaluate graphs of properties exposed directly on this node. */
	UFUNCTION(BlueprintCallable, Category = "Logic Driver|Node Instance")
	void EvaluateGraphProperties();

	/** Retrieve the template guid. The template guid cannot be modified at runtime. */
	const FGuid& GetTemplateGuid() const { return TemplateGuid; }
	
	/**
	 * Properties marked as public will be exposed on this node as a graph.
	 * 
	 * When this is true that graph will automatically evaluate on state entry.
	 * When this is false you should manually call EvaluateGraphProperties().
	 * 
	 * Graph properties are only valid for nodes deriving from USMStateInstance_Base.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Graph Properties", meta = (InstancedTemplate, HideOnNode))
	bool bAutoEvalExposedProperties;
	
	/** Override graph property values. Match the variable name with the variable you want to override. Property must be instance editable. */
	UPROPERTY(EditDefaultsOnly, AdvancedDisplay, Category = "Graph Properties", meta = (InstancedTemplate, HideOnNode))
	TArray<FSMGraphProperty> ExposedPropertyOverrides;
	
	/*
	 * Hack: Helpers for determining if an array property was changed in the editor. Ideally this would be under
	 * the editor module in SMGraphNode_Base's PostEditChangeChainProperty. That method doesn't appear to have
	 * a good way of finding the specific changes that are available here.
	 */
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	bool WasArrayPropertyModified(const FName& PropertyName) const;
	void ResetArrayCheck();
	FName ArrayPropertyChanged;
	int32 ArrayIndexChanged;
	uint32 ArrayChangeType;
#endif

protected:
	virtual void ConstructionScript_Implementation() {}
	virtual UTexture2D* GetNodeIcon_Implementation() const;
	virtual FVector2D GetNodeIconSize_Implementation() const;
	virtual FLinearColor GetNodeIconTintColor_Implementation() const;
protected:
	/**
	 * The icon to use when displaying this node.
	 * This exists in run-time as well in case this image is needed for purposes outside of editor use. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Display", meta = (DisplayPriority = 1, NodeBaseOnly))
	UTexture2D* NodeIcon;

	/** The size of the node icon. Leave 0,0 to auto size. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Display", meta = (DisplayPriority = 2, NodeBaseOnly))
	FVector2D NodeIconSize;

	/** The tint color to apply to the node icon. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Display", meta = (DisplayPriority = 3, NodeBaseOnly))
	FLinearColor NodeIconTintColor;
	
#if WITH_EDITORONLY_DATA
public:
	bool HasCustomColor() const { return bUseCustomColors; }
	const FLinearColor& GetNodeColor() const { return NodeColor; }
	const FSMNodeDescription& GetNodeDescription() const { return NodeDescription; }
	bool HasCustomIcon() const { return bDisplayCustomIcon; }

	/** The default name which should be used. */
	FString GetNodeDisplayName() const;

	/** Sets the template guid. Editor use only. */
	void SetTemplateGuid(const FGuid& NewTemplateGuid) { TemplateGuid = NewTemplateGuid; }
protected:
	/** Describe the node. This provides information to the context menu and to tooltips. */
	UPROPERTY(EditDefaultsOnly, Category = "General", meta = (InstancedTemplate, ShowOnlyInnerProperties))
	FSMNodeDescription NodeDescription;

	/** Override editor default icon with the custom icon chosen. */
	UPROPERTY(EditDefaultsOnly, Category = "Display", meta = (DisplayPriority = 0, NodeBaseOnly))
	bool bDisplayCustomIcon;

	// NodeIcon
	
	/** Override editor preference colors. */
	UPROPERTY(EditDefaultsOnly, Category = "Color", meta = (DisplayPriority = 0))
	bool bUseCustomColors;

	/** The standard color for this node. */
	UPROPERTY(EditDefaultsOnly, Category = "Color", meta = (EditCondition = "bUseCustomColors", DisplayPriority = 1))
	FLinearColor NodeColor;

#endif
	
private:
	/** The owning node in the state machine instance. */
	FSMNode_Base* OwningNode;

	/** Assigned from the editor and used in tracking specific templates. */
	UPROPERTY()
	FGuid TemplateGuid;
};