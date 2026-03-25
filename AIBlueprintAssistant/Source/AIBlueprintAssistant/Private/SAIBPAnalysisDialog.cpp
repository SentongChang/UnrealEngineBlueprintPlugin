// Copyright (c) 2026 AIBlueprintAssistant. All Rights Reserved.

#include "SAIBPAnalysisDialog.h"

#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SMultiLineEditableText.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SAIBPAnalysisDialog"

void SAIBPAnalysisDialog::Construct(const FArguments& InArgs)
{
	OwnerWindow = InArgs._ParentWindow;

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(FMargin(12.f))
		[
			SNew(SVerticalBox)

			// ---- Header label ----
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 8.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("AnalysisHeader",
					"AI Blueprint Analysis  |  蓝图 AI 分析报告"))
				.Font(FAppStyle::GetFontStyle("LargeText"))
			]

			// ---- Scrollable analysis content ----
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			.Padding(0.f, 0.f, 0.f, 8.f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
				.Padding(FMargin(6.f))
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SNew(SMultiLineEditableText)
						.Text(FText::FromString(InArgs._AnalysisText))
						.IsReadOnly(true)
						.AutoWrapText(true)
						.Font(FCoreStyle::GetDefaultFontStyle("Mono", 9))
					]
				]
			]

			// ---- Close button ----
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(SBox)
				.MinDesiredWidth(120.f)
				[
					SNew(SButton)
					.Text(LOCTEXT("CloseBtn", "Close"))
					.HAlign(HAlign_Center)
					.OnClicked(this, &SAIBPAnalysisDialog::OnCloseClicked)
				]
			]
		]
	];
}

FReply SAIBPAnalysisDialog::OnCloseClicked()
{
	if (OwnerWindow.IsValid())
	{
		OwnerWindow->RequestDestroyWindow();
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
