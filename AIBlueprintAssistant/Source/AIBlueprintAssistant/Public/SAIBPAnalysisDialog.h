// Copyright (c) 2026 AIBlueprintAssistant. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SWindow.h"

/**
 * Slate widget shown inside a modal SWindow to display the AI-generated
 * Blueprint analysis. The report is bilingual (Chinese + English) and covers
 * an overall summary and a per-module logical breakdown.
 *
 * Usage:
 *   TSharedRef<SWindow> Window = SNew(SWindow)...;
 *   TSharedRef<SAIBPAnalysisDialog> Dialog = SNew(SAIBPAnalysisDialog)
 *       .AnalysisText(MyText).ParentWindow(Window);
 *   Window->SetContent(Dialog);
 *   FSlateApplication::Get().AddModalWindow(Window, ParentWidget);
 */
class SAIBPAnalysisDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAIBPAnalysisDialog)
		: _AnalysisText()
		, _ParentWindow()
	{}
		/** The bilingual analysis text returned by the AI. */
		SLATE_ARGUMENT(FString, AnalysisText)

		/** The SWindow that hosts this widget — closed when Close is clicked. */
		SLATE_ARGUMENT(TSharedPtr<SWindow>, ParentWindow)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FReply OnCloseClicked();

	TSharedPtr<SWindow> OwnerWindow;
};
