// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "Graph/SMPropertyGraph.h"
#include "SMTextPropertyGraph.generated.h"


DECLARE_DELEGATE_OneParam(FSwitchTextEditAction, bool);

UCLASS()
class SMEXTENDEDEDITOR_API USMTextPropertyGraph : public USMPropertyGraph
{
	GENERATED_UCLASS_BODY()

public:
	/** The node used to format the text before the result node. */
	UPROPERTY()
	class UK2Node_FormatText* FormatTextNode;

	UEdGraphPin* GetFormatTextNodePin() const;

	void NotifyGraphChanged(const FEdGraphEditAction& Action) override;
	
	// USMPropertyGraph
	void RefreshProperty(bool bModify = true) override;
	void ResetGraph() override;
	void SetUsingGraphToEdit(bool bValue) override;
	// ~USMPropertyGraph

	/** Resets graph completely from updated text. */
	void SetNewText(const FText& PlainText);

	/** Reparse the rich text body from the current plain text body. */
	void RefreshTextBody(bool bModify = true);
	/** Set the plain text body and parse into rich text. */
	void SetTextBody(const FText& PlainText, bool bModify = true);
	/** Return a copy of the rich text body. */
	FText GetTextBody() const;

	FSwitchTextEditAction SwitchTextEditAction;
	/** Toggles the actual text input widget into or out of edit mode. */
	void SetTextEditMode(bool bValue);

protected:
	void SetFormatTextNodeText(const FText& NewText);
	
protected:
	/** Variable name to variable guid. */
	UPROPERTY()
	TMap<FName, FGuid> StoredProperties;

	UPROPERTY()
	TMap<FName, FGuid> StoredFunctions;
	
	UPROPERTY()
	FText RichTextBody;

	UPROPERTY()
	FText PlainTextBody;

	TSharedPtr<class FOurEditableTextGraphPin> EditableTextProperty;
};
