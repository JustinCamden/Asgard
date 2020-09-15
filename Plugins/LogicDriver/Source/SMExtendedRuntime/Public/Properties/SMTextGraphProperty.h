// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "SMGraphProperty_Base.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SMTextNodeWidgetInfo.h"
#include "SMTextGraphProperty.generated.h"

/**
 * Helper for converting object data to text in FSMTextGraphProperty.
 */
USTRUCT()
struct SMEXTENDEDRUNTIME_API FSMTextSerializer
{
	GENERATED_BODY()
	
	/**
	 * When an object is placed in the text graph this function will be looked up from the object and executed.
	 * The function should be pure and return only text.
	 *
	 * If empty no function is looked up.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Text Conversion")
	TArray<FName> ToTextFunctionNames;
};

/**
	A property which should only be placed in state machine node classes!
	The state machine graph node which owns the class will display this property
	as an editable text box directly on the node and create a blueprint graph
	to parse the text.
 */
USTRUCT(BlueprintType, meta = (DisplayName = "SM Text Graph Property", HasNativeBreak = "SMExtendedRuntime.SMExtendedGraphPropertyHelpers.BreakTextGraphProperty"))
struct SMEXTENDEDRUNTIME_API FSMTextGraphProperty : public FSMGraphProperty_Base
{
public:
	GENERATED_BODY()

	FSMTextGraphProperty();

	UPROPERTY(BlueprintReadWrite, Category = Result, meta = (AlwaysAsPin))
	FText Result;
	
	// FSMGraphProperty
	virtual uint8* GetResult() const override { return (uint8*)&Result; }
	virtual void SetResult(uint8* Value) override;
	virtual bool ShouldAutoAssignVariable() const override { return false; }
	// ~FSMGraphProperty

#if WITH_EDITORONLY_DATA
public:
	virtual int32 GetVerticalDisplayOrder() const override { return WidgetInfo.DisplayOrder; }
	virtual bool ShouldDefaultToEditMode() const override { return false; }
	virtual bool AllowToggleGraphEdit() const override { return true; }

#endif

	// WidgetInfo should be EDITORONLY_DATA but bp nativization will throw warnings unless it's always included.
	/** Configure the widget display properties. */
	UPROPERTY(EditDefaultsOnly, Category = "Node Widget")
	FSMTextNodeWidgetInfo WidgetInfo;

	UPROPERTY(EditDefaultsOnly, Category = "Text Serializer")
	FSMTextSerializer TextSerializer;
};