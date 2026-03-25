// Copyright (c) 2026 AIBlueprintAssistant. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SMultiLineEditableText.h"
#include "Widgets/Input/SEditableText.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"

/**
 * Main Slate UI panel for the AI Blueprint Assistant plugin.
 *
 * Layout (top to bottom):
 *   - API URL input field
 *   - API Key input field (password masked)
 *   - Multi-line requirement text editor
 *   - "Generate Blueprint Nodes" button
 *   - Scrollable status log
 */
class SAIBPAssistantWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAIBPAssistantWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	/** Fired when the user clicks "Generate Blueprint Nodes" */
	FReply OnGenerateClicked();

	/** Appends a line to the scrollable log and scrolls to the bottom */
	void AppendLog(const FString& Message);

	/** Sets the button enabled/disabled state and updates status text */
	void SetGenerating(bool bIsGenerating);

	// --- Slate widget references ---

	/** API endpoint URL editable field */
	TSharedPtr<SEditableText> ApiUrlInput;

	/** API key editable field (password masked) */
	TSharedPtr<SEditableText> ApiKeyInput;

	/** Multi-line user requirement text editor */
	TSharedPtr<SMultiLineEditableText> RequirementInput;

	/** The generate button — disabled while a request is in flight */
	TSharedPtr<SButton> GenerateButton;

	/** Short status label shown above the log ("Ready" / "Generating...") */
	TSharedPtr<STextBlock> StatusText;

	/** Scrollable container for log entries */
	TSharedPtr<SScrollBox> LogScrollBox;

	// --- State ---

	/** True while an HTTP request is in flight */
	bool bIsGenerating = false;
};
