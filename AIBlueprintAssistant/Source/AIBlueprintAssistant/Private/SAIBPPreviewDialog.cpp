// Copyright (c) 2026 AIBlueprintAssistant. All Rights Reserved.

#include "SAIBPPreviewDialog.h"

#include "Framework/Application/SlateApplication.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SAIBPPreviewDialog"

void SAIBPPreviewDialog::Construct(const FArguments& InArgs)
{
	OwnerWindow = InArgs._ParentWindow;

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(FMargin(12.f))
		[
			SNew(SVerticalBox)

			// ---- Title ----
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 8.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Title", "Review AI-Generated T3D Nodes"))
				.Font(FAppStyle::GetFontStyle("NormalFont"))
			]

			// ---- Subtitle ----
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 6.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Subtitle",
					"Inspect the generated T3D code below before importing it into your Blueprint graph."))
				.Font(FAppStyle::GetFontStyle("SmallFont"))
				.AutoWrapText(true)
			]

			// ---- T3D Code View ----
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			.Padding(0.f, 0.f, 0.f, 10.f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
				.Padding(FMargin(4.f))
				[
					SNew(SScrollBox)
					.Orientation(Orient_Vertical)

					+ SScrollBox::Slot()
					[
						SNew(SMultiLineEditableTextBox)
						.Text(FText::FromString(InArgs._T3DCode))
						.IsReadOnly(true)
						.AutoWrapText(false)
						.Font(FCoreStyle::GetDefaultFontStyle("Mono", 9))
					]
				]
			]

			// ---- Action Buttons ----
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)

				// Push buttons to the right
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.f, 0.f, 8.f, 0.f)
				[
					SNew(SButton)
					.Text(LOCTEXT("ImportBtn", "Import Nodes"))
					.ToolTipText(LOCTEXT("ImportTooltip",
						"Insert the generated nodes into the active Blueprint graph."))
					.OnClicked(this, &SAIBPPreviewDialog::OnImportClicked)
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.Text(LOCTEXT("CancelBtn", "Cancel"))
					.ToolTipText(LOCTEXT("CancelTooltip",
						"Discard the generated T3D code and do nothing."))
					.OnClicked(this, &SAIBPPreviewDialog::OnCancelClicked)
				]
			]
		]
	];
}

FReply SAIBPPreviewDialog::OnImportClicked()
{
	bConfirmed = true;
	if (OwnerWindow.IsValid())
	{
		OwnerWindow->RequestDestroyWindow();
	}
	return FReply::Handled();
}

FReply SAIBPPreviewDialog::OnCancelClicked()
{
	bConfirmed = false;
	if (OwnerWindow.IsValid())
	{
		OwnerWindow->RequestDestroyWindow();
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
