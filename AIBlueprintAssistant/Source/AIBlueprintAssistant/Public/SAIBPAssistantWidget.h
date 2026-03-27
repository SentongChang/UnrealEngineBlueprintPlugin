// Copyright (c) 2026 AIBlueprintAssistant. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SEditableText.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"

/**
 * Main Slate UI panel for the AI Blueprint Assistant plugin.
 *
 * Layout (top to bottom):
 *   - Model info bar (model name read from settings, "New Conversation" button)
 *   - Multi-line requirement text editor
 *   - "Generate Blueprint Nodes" button
 *   - Status text ("Ready" / "Generating...")
 *   - Scrollable status/conversation log
 *
 * API URL and Key are no longer shown inline — configure them once in
 * Edit > Project Settings > Plugins > AI Blueprint Assistant.
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

	/** Fired when the user clicks "Analyze Blueprint" */
	FReply OnAnalyzeClicked();

	/** Fired when the user clicks "New Conversation" */
	FReply OnNewConversationClicked();

	/** Shows a modal T3D preview window; returns true if the user confirmed import. */
	bool ShowPreviewDialog(const FString& T3DCode);

	/** Appends a line to the scrollable log and scrolls to the bottom. */
	void AppendLog(const FString& Message);

	/** Sets the button enabled/disabled state and updates status text. */
	void SetGenerating(bool bNewGenerating);

	/** Returns label text for the model info bar (reads UAIBPSettings). */
	FText GetModelInfoText() const;

	/**
	 * Handles an AI response for both initial generation and self-repair retries.
	 * On compile error, undoes the import and re-sends the errors to the AI
	 * (up to MaxRepairAttempts times).
	 *
	 * @param bSuccess         Whether the HTTP request succeeded.
	 * @param T3DResult        The T3D code (or error message on failure).
	 * @param OriginalUserReq  The user's original requirement text (for repair context).
	 * @param ContextJson      Blueprint context JSON forwarded to repair requests.
	 * @param UserContent      Full user+context string used to record conversation history.
	 * @param bUseHistory      Whether to record a successful turn in ConversationHistory.
	 * @param bIsFirstAttempt  Show the T3D preview dialog only on the first attempt.
	 * @param AttemptNum       Current attempt index (1-based).
	 * @param MaxAttempts      Total allowed attempts (0 = no repair, just import once).
	 */
	void HandleGenerationResponse(
		bool bSuccess,
		const FString& T3DResult,
		const FString& OriginalUserReq,
		TSharedPtr<FJsonObject> ContextJson,
		const FString& UserContent,
		bool bUseHistory,
		bool bIsFirstAttempt,
		int32 AttemptNum,
		int32 MaxAttempts);

	// --- Slate widget references ---

	/** Multi-line user requirement text editor */
	TSharedPtr<SMultiLineEditableTextBox> RequirementInput;

	/** The generate button — disabled while a request is in flight */
	TSharedPtr<SButton> GenerateButton;

	/** The analyze button — disabled while any request is in flight */
	TSharedPtr<SButton> AnalyzeButton;

	/** Short status label shown above the log */
	TSharedPtr<STextBlock> StatusText;

	/** Scrollable container for log entries */
	TSharedPtr<SScrollBox> LogScrollBox;

	// --- State ---

	/** True while an HTTP request is in flight */
	bool bIsGenerating = false;

	/**
	 * Multi-turn conversation history.
	 * Each entry is a JSON object: { "role": "user"|"assistant", "content": "..." }
	 * Populated only when UAIBPSettings::bEnableConversationHistory is true.
	 * Cleared by clicking "New Conversation".
	 */
	TArray<TSharedPtr<FJsonObject>> ConversationHistory;
};
