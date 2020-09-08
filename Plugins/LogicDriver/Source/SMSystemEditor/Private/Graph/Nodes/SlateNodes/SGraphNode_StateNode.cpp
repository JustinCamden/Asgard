// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#include "SGraphNode_StateNode.h"
#include "SLevelOfDetailBranchNode.h"
#include "SCommentBubble.h"
#include "SlateOptMacros.h"
#include "SGraphPin.h"
#include "Components/VerticalBox.h"
#include "SGraphPreviewer.h"
#include "GraphEditorSettings.h"
#include "Templates/SharedPointer.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SToolTip.h"
#include "Widgets/SWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "SMBlueprintEditorUtils.h"
#include "Configuration/SMEditorStyle.h"
#include "Graph/Nodes/SMGraphNode_StateNode.h"
#include "Graph/Nodes/SMGraphNode_ConduitNode.h"
#include "Graph/Nodes/PropertyNodes/SMGraphK2Node_GraphPropertyNode.h"
#include "Graph/Pins/SGraphPin_StatePin.h"
#include "Graph/Nodes/SMGraphNode_StateMachineStateNode.h"
#include "Graph/Nodes/SMGraphNode_StateMachineParentNode.h"
#include "Widgets/SSMGraphProperty.h"
#include "SMNodeInstanceUtils.h"

#define LOCTEXT_NAMESPACE "SGraphStateNode"


void SGraphNode_StateNode::Construct(const FArguments& InArgs, USMGraphNode_StateNodeBase* InNode)
{
	GraphNode = InNode;
	ContentPadding = InArgs._ContentPadding;
	UpdateGraphNode();
	SetCursor(EMouseCursor::CardinalCross);

	const USMEditorSettings* EditorSettings = FSMBlueprintEditorUtils::GetEditorSettings();
	{
		const FSlateBrush* ImageBrush = FSMEditorStyle::Get()->GetBrush(TEXT("SMGraph.StateModifier"));
		
		FLinearColor AnyStateColor = EditorSettings->AnyStateDefaultColor;
		AnyStateColor.A = 0.72f;
		
		AnyStateImpactWidget =
			SNew(SImage)
			.Image(ImageBrush)
			.ToolTipText(NSLOCTEXT("StateNode", "StateNodeAnyStateTooltip", "An `Any State` node is adding one or more transitions to this state."))
			.ColorAndOpacity(AnyStateColor)
			.Visibility(EVisibility::Visible);
	}
}

void SGraphNode_StateNode::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SGraphNode::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	USMGraphNode_StateNodeBase* StateNode = CastChecked<USMGraphNode_StateNodeBase>(GraphNode);
	StateNode->UpdateTime(InDeltaTime);
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SGraphNode_StateNode::UpdateGraphNode()
{
	InputPins.Empty();
	OutputPins.Empty();

	// Reset variables that are going to be exposed, in case we are refreshing an already setup node.
	RightNodeBox.Reset();
	LeftNodeBox.Reset();

	const FSlateBrush* NodeTypeIcon = GetNameIcon();
	const FLinearColor TitleShadowColor(0.6f, 0.6f, 0.6f);

	const float PinPadding = FSMBlueprintEditorUtils::GetEditorSettings()->StateConnectionSize;
	
	SetupErrorReporting();
	TSharedPtr<SErrorText> ErrorText;
	TSharedPtr<SVerticalBox> ContentBox = CreateContentBox();
	
	this->ContentScale.Bind(this, &SGraphNode::GetContentScale);
	this->GetOrAddSlot(ENodeZone::Center)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("Graph.StateNode.Body"))
			.Padding(0)
			.BorderBackgroundColor(this, &SGraphNode_StateNode::GetBorderBackgroundColor)
			[
				SNew(SOverlay)

				// PIN AREA
				+ SOverlay::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SAssignNew(RightNodeBox, SVerticalBox)
				]

				// STATE NAME AREA
				+ SOverlay::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(PinPadding)
				[
					SNew(SBorder)
					.BorderImage(FEditorStyle::GetBrush("Graph.StateNode.ColorSpill"))
					.BorderBackgroundColor(TitleShadowColor)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Visibility(EVisibility::SelfHitTestInvisible)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							// POPUP ERROR MESSAGE
							SAssignNew(ErrorText, SErrorText)
							.BackgroundColor(this, &SGraphNode_StateNode::GetErrorColor)
							.ToolTipText(this, &SGraphNode_StateNode::GetErrorMsgToolTip)
						]
						+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(SImage)
								.Image(NodeTypeIcon)
							]
						+ SHorizontalBox::Slot()
						.Padding(ContentPadding)
						[
							ContentBox.ToSharedRef()
						]
					]
				]
			]
		];

	// Finalize all property widgets now that parent - child relationship is complete.
	for (auto& PropertyWidget : PropertyWidgets)
	{
		PropertyWidget.Key->Finalize();
	}
	
	// Create comment bubble
	TSharedPtr<SCommentBubble> CommentBubble;
	const FSlateColor CommentColor = GetDefault<UGraphEditorSettings>()->DefaultCommentNodeTitleColor;

	SAssignNew(CommentBubble, SCommentBubble)
		.GraphNode(GraphNode)
		.Text(this, &SGraphNode::GetNodeComment)
		.OnTextCommitted(this, &SGraphNode::OnCommentTextCommitted)
		.ColorAndOpacity(CommentColor)
		.AllowPinning(true)
		.EnableTitleBarBubble(true)
		.EnableBubbleCtrls(true)
		.GraphLOD(this, &SGraphNode::GetCurrentLOD)
		.IsGraphNodeHovered(this, &SGraphNode::IsHovered);

	GetOrAddSlot(ENodeZone::TopCenter)
		.SlotOffset(TAttribute<FVector2D>(CommentBubble.Get(), &SCommentBubble::GetOffset))
		.SlotSize(TAttribute<FVector2D>(CommentBubble.Get(), &SCommentBubble::GetSize))
		.AllowScaling(TAttribute<bool>(CommentBubble.Get(), &SCommentBubble::IsScalingAllowed))
		.VAlign(VAlign_Top)
		[
			CommentBubble.ToSharedRef()
		];
	
	ErrorReporting = ErrorText;
	ErrorReporting->SetError(ErrorMsg);
	CreatePinWidgets();
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SGraphNode_StateNode::CreatePinWidgets()
{
	USMGraphNode_StateNodeBase* StateNode = CastChecked<USMGraphNode_StateNodeBase>(GraphNode);

	UEdGraphPin* CurPin = StateNode->GetOutputPin();
	if (!CurPin->bHidden)
	{
		TSharedPtr<SGraphPin> NewPin = SNew(SSMGraphPin_StatePin, CurPin);

		this->AddPin(NewPin.ToSharedRef());
	}
}

void SGraphNode_StateNode::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
{
	PinToAdd->SetOwner(SharedThis(this));
	RightNodeBox->AddSlot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.FillHeight(1.0f)
		[
			PinToAdd
		];
	OutputPins.Add(PinToAdd);
}

TSharedPtr<SToolTip> SGraphNode_StateNode::GetComplexTooltip()
{
	/* Display a pop-up on mouse hover with useful information. */
	TSharedRef<SVerticalBox> Widget = BuildComplexTooltip();

	return SNew(SToolTip)
		[
			Widget
		];
}

TArray<FOverlayWidgetInfo> SGraphNode_StateNode::GetOverlayWidgets(bool bSelected, const FVector2D& WidgetSize) const
{
	TArray<FOverlayWidgetInfo> Widgets;

	const USMEditorSettings* EditorSettings = FSMBlueprintEditorUtils::GetEditorSettings();
	if (!EditorSettings->bDisableVisualCues)
	{
		if (USMGraphNode_StateNodeBase* StateNode = Cast<USMGraphNode_StateNodeBase>(GraphNode))
		{
			if (FSMBlueprintEditorUtils::IsNodeImpactedFromAnyStateNode(StateNode))
			{
				const FSlateBrush* ImageBrush = FSMEditorStyle::Get()->GetBrush(TEXT("SMGraph.StateModifier"));

				FOverlayWidgetInfo Info;
				Info.OverlayOffset = FVector2D(WidgetSize.X - (ImageBrush->ImageSize.X * 0.5f) - (Widgets.Num() * OverlayWidgetPadding), -(ImageBrush->ImageSize.Y * 0.5f));
				Info.Widget = AnyStateImpactWidget;

				Widgets.Add(Info);
			}
		}
	}

	return Widgets;
}

FReply SGraphNode_StateNode::OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent)
{
	// Prevent double click from stealing interaction with widget.
	// TODO: See if property has any handling and call that, continue, or cancel.
	for(const auto& KeyVal : PropertyWidgets)
	{
		if(KeyVal.Key->GetCachedGeometry().IsUnderLocation(InMouseEvent.GetScreenSpacePosition()))
		{
			return FReply::Handled();
		}
	}
	
	return SGraphNode::OnMouseButtonDoubleClick(InMyGeometry, InMouseEvent);
}

void SGraphNode_StateNode::RequestRenameOnSpawn()
{
	if (USMGraphNode_Base* Node = Cast<USMGraphNode_Base>(GraphNode))
	{
		if (USMStateInstance_Base* NodeInstance = Cast<USMStateInstance_Base>(Node->GetNodeTemplate()))
		{
			if (!NodeInstance->ShouldDisplayNameWidget())
			{
				// No name widget to display -- see if there are other widgets to display.
				for(const auto& KeyVal : PropertyWidgets)
				{
					if(KeyVal.Value->IsConsideredForDefaultProperty())
					{
						KeyVal.Value->DefaultPropertyActionWhenPlaced(StaticCastSharedPtr<SWidget>(KeyVal.Key));
						break;
					}
				}
				
				return;
			}
		}
	}
	SGraphNode::RequestRenameOnSpawn();
}

FReply SGraphNode_StateNode::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	return FReply::Handled();
}

TSharedRef<SVerticalBox> SGraphNode_StateNode::BuildComplexTooltip()
{
	USMGraphNode_StateNodeBase* StateNode = CastChecked<USMGraphNode_StateNodeBase>(GraphNode);

	const bool bCanExecute = StateNode->HasInputConnections();
	const bool bIsEndState = StateNode->IsEndState(false);
	bool bIsAnyState = false;
	
	FString NodeType = "State";
	if (StateNode->IsA<USMGraphNode_StateMachineParentNode>())
	{
		NodeType = "Parent";
	}
	else if(USMGraphNode_StateMachineStateNode* StateMachineNode = Cast<USMGraphNode_StateMachineStateNode>(StateNode))
	{
		NodeType = StateMachineNode->IsStateMachineReference() ? "State Machine Reference" : "State Machine";
	}
	else if (USMGraphNode_AnyStateNode* AnyStateNode = Cast<USMGraphNode_AnyStateNode>(StateNode))
	{
		NodeType = "Any State";
		bIsAnyState = true;
	}

	const bool bAnyStateImpactsThisNode = !bIsAnyState && FSMBlueprintEditorUtils::IsNodeImpactedFromAnyStateNode(StateNode);
	
	TSharedRef<SVerticalBox> Widget = SNew(SVerticalBox);
	Widget->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.f, 0.f, 0.f, 4.f))
		[
			SNew(STextBlock)
			.TextStyle(FSMEditorStyle::Get(), "SMGraph.Tooltip.Title")
			.Text(FText::Format(LOCTEXT("StatePopupTitle", "{0} ({1})"), FText::FromString(StateNode->GetStateName()), FText::FromString(NodeType)))
		];

	if (USMNodeInstance* NodeTemplate = StateNode->GetNodeTemplate())
	{
		Widget->AddSlot()
			.AutoHeight()
			.Padding(FMargin(0.f, 0.f, 0.f, 4.f))
			[
				FSMNodeInstanceUtils::CreateNodeClassWidgetDisplay(NodeTemplate).ToSharedRef()
			];
	}
	if (UEdGraph* Graph = GetGraphToUseForTooltip())
	{
		Widget->AddSlot()
			.AutoHeight()
			[
				SAssignNew(GraphPreviewer, SGraphPreviewer, Graph)
				.ShowGraphStateOverlay(false)
			];
	}
	if (!bCanExecute && !bIsAnyState)
	{
		Widget->AddSlot()
			.AutoHeight()
			.Padding(FMargin(2.f, 4.f, 2.f, 2.f))
			[
				SNew(STextBlock)
				.TextStyle(FSMEditorStyle::Get(), "SMGraph.Tooltip.Warning")
				.Text(LOCTEXT("StateCantExecuteTooltip", "No Valid Input: State will never execute"))
			];
	}

	if (bIsEndState)
	{
		const FText EndStateTooltip = StateNode->IsEndState(true) ? LOCTEXT("EndStateTooltip", "End State: State will never exit") :
			LOCTEXT("NotEndStateTooltip", "Not an End State: An Any State node is adding transitions to this node");
		
		Widget->AddSlot()
			.AutoHeight()
			.Padding(FMargin(2.f, 4.f, 2.f, 2.f))
			[
				SNew(STextBlock)
				.TextStyle(FSMEditorStyle::Get(), "SMGraph.Tooltip.Info")
				.Text(EndStateTooltip)
			];
	}
	else if (bAnyStateImpactsThisNode)
	{
		Widget->AddSlot()
			.AutoHeight()
			.Padding(FMargin(2.f, 4.f, 2.f, 2.f))
			[
				SNew(STextBlock)
				.TextStyle(FSMEditorStyle::Get(), "SMGraph.Tooltip.Info")
				.Text(LOCTEXT("AnyStateImpactTooltip", "An Any State node is adding transitions to this node"))
			];
	}

	return Widget;
}

UEdGraph* SGraphNode_StateNode::GetGraphToUseForTooltip() const
{
	USMGraphNode_StateNodeBase* StateNode = CastChecked<USMGraphNode_StateNodeBase>(GraphNode);
	return StateNode->GetBoundGraph();
}

void SGraphNode_StateNode::GetNodeInfoPopups(FNodeInfoContext* Context,
	TArray<FGraphInformationPopupInfo>& Popups) const
{
	USMGraphNode_StateNodeBase* Node = CastChecked<USMGraphNode_StateNodeBase>(GraphNode);
	if (const FSMNode_Base* DebugNode = Node->GetDebugNode())
	{
		// Show active time or last active time over the node.

		if (Node->IsDebugNodeActive())
		{
			const FString StateText = FString::Printf(TEXT("Active for %.2f secs"), DebugNode->TimeInState);
			new (Popups) FGraphInformationPopupInfo(nullptr, Node->GetBackgroundColor(), StateText);
		}
		else if (Node->WasDebugNodeActive())
		{
			
			const USMEditorSettings* EditorSettings = FSMBlueprintEditorUtils::GetEditorSettings();

			const float StartFade = EditorSettings->TimeToDisplayLastActiveState;
			const float TimeToFade = EditorSettings->TimeToFadeLastActiveState;
			const float DebugTime = Node->GetDebugTime();

			if (DebugTime < StartFade + TimeToFade)
			{
				const FString StateText = FString::Printf(TEXT("Was Active for %.2f secs"), DebugNode->TimeInState);

				if (DebugTime > StartFade)
				{
					FLinearColor Color = Node->GetBackgroundColor();

					const float PercentComplete = TimeToFade <= 0.f ? 0.f : FMath::Clamp(Color.A * (1.f - (DebugTime - StartFade) / TimeToFade), 0.f, Color.A);
					Color.A *= PercentComplete;

					const FLinearColor ResultColor = Color;
					new (Popups) FGraphInformationPopupInfo(nullptr, ResultColor, StateText);
				}
				else
				{
					new (Popups) FGraphInformationPopupInfo(nullptr, Node->GetBackgroundColor(), StateText);
				}
			}
		}
	}
}

TSharedPtr<SVerticalBox> SGraphNode_StateNode::CreateContentBox()
{
	TSharedPtr<SVerticalBox> Content;
	TSharedPtr<SNodeTitle> NodeTitle = SNew(SNodeTitle, GraphNode);
	SAssignNew(Content, SVerticalBox);

	bool bDisplayTitle = true;
	if (USMGraphNode_Base* Node = Cast<USMGraphNode_Base>(GraphNode))
	{
		if (USMStateInstance_Base* NodeInstance = Cast<USMStateInstance_Base>(Node->GetNodeTemplate()))
		{
			if (!NodeInstance->ShouldDisplayNameWidget())
			{
				bDisplayTitle = false;
			}
		}
	}
	
	if(bDisplayTitle)
	{
		Content->AddSlot()
			.AutoHeight()
			[
				SAssignNew(InlineEditableText, SInlineEditableTextBlock)
				.Style(FEditorStyle::Get(), "Graph.StateNode.NodeTitleInlineEditableText")
				.Text(NodeTitle.Get(), &SNodeTitle::GetHeadTitle)
				.OnVerifyTextChanged(this, &SGraphNode_StateNode::OnVerifyNameTextChanged)
				.OnTextCommitted(this, &SGraphNode_StateNode::OnNameTextCommited)
				.IsReadOnly(this, &SGraphNode_StateNode::IsNameReadOnly)
				.IsSelected(this, &SGraphNode_StateNode::IsSelectedExclusively)
			];
		Content->AddSlot()
			.AutoHeight()
			[
				NodeTitle.ToSharedRef()
			];
	}
	
	PropertyWidgets.Reset();
	
	// Add custom property widgets sorted by user specification.
	USMGraphNode_Base* Node = CastChecked<USMGraphNode_Base>(GraphNode);
	TArray<USMGraphK2Node_PropertyNode_Base*> GraphPropertyNodes = Node->GetAllPropertyGraphNodesAsArray();

	TArray<USMGraphK2Node_PropertyNode_Base*> GraphPropertyBPVariablesOrdered;
	if(UBlueprint* NodeBlueprint = UBlueprint::GetBlueprintFromClass(Node->GetNodeClass()))
	{
		for(const FBPVariableDescription& Variable : NodeBlueprint->NewVariables)
		{
			FSMGraphProperty_Base Property;
			const FGuid& CalculatedGuid = Property.SetGuid(Variable.VarGuid, 0);

			// First lookup by var guid. This is the standard lookup.
			USMGraphK2Node_PropertyNode_Base* GraphProperty = Node->GetGraphPropertyNode(CalculatedGuid);
			
			// Attempt lookup by name -- Only can happen on extended graph properties.
			if(!GraphProperty)
			{
				GraphProperty = Node->GetGraphPropertyNode(Variable.VarName);
			}
			
			if(GraphProperty)
			{
				GraphPropertyNodes.Remove(GraphProperty);
				GraphPropertyBPVariablesOrdered.Add(GraphProperty);

				FSMGraphProperty_Base* RealPropertyNode = GraphProperty->GetPropertyNode();
				if(!RealPropertyNode)
				{
					continue;
				}

				// Look for array items that may belong to this property.
				TArray<USMGraphK2Node_PropertyNode_Base*> ArrayItems = GraphPropertyNodes.
					FilterByPredicate([&RealPropertyNode](USMGraphK2Node_PropertyNode_Base* PropertyNode)
					{
						FSMGraphProperty_Base* TestProperty = PropertyNode->GetPropertyNodeConst();
						if (!TestProperty)
						{
							return false;
						}
						return TestProperty->VariableName == RealPropertyNode->VariableName;
					});
				ArrayItems.Sort([&](USMGraphK2Node_PropertyNode_Base& lhs, USMGraphK2Node_PropertyNode_Base& rhs)
				{
					// Should never be null unless something was forcibly deleted or an underlying class removed.
					FSMGraphProperty_Base* PropertyLhs = lhs.GetPropertyNode();
					if (!PropertyLhs)
					{
						return false;
					}

					FSMGraphProperty_Base* PropertyRhs = rhs.GetPropertyNode();
					if (!PropertyRhs)
					{
						return false;
					}

					return PropertyLhs->ArrayIndex <= PropertyRhs->ArrayIndex;
				});

				// Add on array items in the correct order directly after the first element.
				for(USMGraphK2Node_PropertyNode_Base* ArrItem : ArrayItems)
				{
					GraphPropertyNodes.Remove(ArrItem);
				}
				GraphPropertyBPVariablesOrdered.Append(ArrayItems);
			}
		}
	}

	// GraphPropertyNodes are just native / non-variable properties. Add sorted BP on after.
	GraphPropertyNodes.Append(GraphPropertyBPVariablesOrdered);

	TMap<int32, TArray<USMGraphK2Node_PropertyNode_Base*>> CustomOrderMap;
	// Perform custom sorting.
	for (USMGraphK2Node_PropertyNode_Base* PropertyNode : GraphPropertyNodes)
	{
		if(FSMGraphProperty_Base* Property = PropertyNode->GetPropertyNode())
		{
			const int32 Order = Property->GetVerticalDisplayOrder();
			if(Order != 0)
			{
				// Look for all related elements since this could be an array that is being re-ordered.
				TArray<USMGraphK2Node_PropertyNode_Base*> PropertiesToMove = GraphPropertyNodes.
					FilterByPredicate([&Property](USMGraphK2Node_PropertyNode_Base* PropertyNode)
				{
					FSMGraphProperty_Base* TestProperty = PropertyNode->GetPropertyNodeConst();
					if (!TestProperty)
					{
						return false;
					}
					return TestProperty->VariableName == Property->VariableName;
				});
				
				
				CustomOrderMap.Add(Order, PropertiesToMove);
			}
		}
	}

	// Insert or add the elements to the array.
	for(auto& KeyVal : CustomOrderMap)
	{
		TArray<USMGraphK2Node_PropertyNode_Base*>& PropertyNodes = KeyVal.Value;
		for (USMGraphK2Node_PropertyNode_Base* PropertyNode : PropertyNodes)
		{
			GraphPropertyNodes.Remove(PropertyNode);
		}

		const int32 Index = FMath::Clamp(KeyVal.Key, 0, GraphPropertyNodes.Num());
		if(Index > GraphPropertyNodes.Num() - 1)
		{
			GraphPropertyNodes.Append(PropertyNodes);
		}
		else
		{
			GraphPropertyNodes.Insert(PropertyNodes, Index);
		}
	}
	
	for (USMGraphK2Node_PropertyNode_Base* PropertyNode : GraphPropertyNodes)
	{
		if (TSharedPtr<SSMGraphProperty_Base> PropertyWidget = PropertyNode->GetGraphNodeWidget())
		{
			// Must call finalize on these after the context box has been created and assigned.
			PropertyWidgets.Add(PropertyWidget, PropertyNode);
			
			Content->AddSlot()
				.Padding(0.f, 2.5f)
				.AutoHeight()
				[
					StaticCastSharedRef<SWidget>(PropertyWidget.ToSharedRef())
				];
		}
	}
	
	return Content;
}

FSlateColor SGraphNode_StateNode::GetBorderBackgroundColor() const
{
	USMGraphNode_StateNodeBase* StateNode = CastChecked<USMGraphNode_StateNodeBase>(GraphNode);
	return StateNode->GetBackgroundColor();
}

const FSlateBrush* SGraphNode_StateNode::GetNameIcon() const
{
	USMGraphNode_StateNodeBase* StateNode = CastChecked<USMGraphNode_StateNodeBase>(GraphNode);
	if(const FSlateBrush* Brush = StateNode->GetNodeIcon())
	{
		return Brush;
	}
	
	return FEditorStyle::GetBrush(TEXT("Graph.StateNode.Icon"));
}

#undef LOCTEXT_NAMESPACE

void SGraphNode_ConduitNode::Construct(const FArguments& InArgs, USMGraphNode_ConduitNode* InNode)
{
	const USMEditorSettings* EditorSettings = FSMBlueprintEditorUtils::GetEditorSettings();

	SGraphNode_StateNode::FArguments Args;
	Args.ContentPadding(EditorSettings->StateContentPadding);
	
	SGraphNode_StateNode::Construct(Args, InNode);
}

const FSlateBrush* SGraphNode_ConduitNode::GetNameIcon() const
{
	USMGraphNode_StateNodeBase* StateNode = CastChecked<USMGraphNode_StateNodeBase>(GraphNode);
	if (const FSlateBrush* Brush = StateNode->GetNodeIcon())
	{
		return Brush;
	}
	
	return FEditorStyle::GetBrush(TEXT("Graph.ConduitNode.Icon"));
}
