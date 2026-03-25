// Copyright (c) 2026 AIBlueprintAssistant. All Rights Reserved.

#include "SAIBPAssistantWidget.h"
#include "AIBPContextUtils.h"
#include "AIBPHttpService.h"
#include "AIBPNodeFactory.h"
#include "AIBPSettings.h"
#include "SAIBPPreviewDialog.h"
#include "SAIBPAnalysisDialog.h"

#include "Async/Async.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/AppStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"

#include "Editor.h"   // GEditor->UndoTransaction()

#define LOCTEXT_NAMESPACE "SAIBPAssistantWidget"

void SAIBPAssistantWidget::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(FMargin(8.f))
		[
			SNew(SVerticalBox)

			// ---- Model Info Bar ----
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 6.f)
			[
				SNew(SHorizontalBox)

				// Model name (live from settings)
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.FillWidth(1.f)
				[
					SNew(STextBlock)
					.Text(this, &SAIBPAssistantWidget::GetModelInfoText)
					.Font(FAppStyle::GetFontStyle("SmallFont"))
					.ColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f)))
				]

				// "New Conversation" button (clears history)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.Text(LOCTEXT("NewConversationBtn", "New Conversation"))
					.ToolTipText(LOCTEXT("NewConversationTooltip",
						"Clear conversation history and start a fresh session."))
					.OnClicked(this, &SAIBPAssistantWidget::OnNewConversationClicked)
				]
			]

			// ---- Settings hint ----
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 6.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("SettingsHint",
					"Configure API URL, key, model, and prompt in:\n"
					"Edit \u2192 Project Settings \u2192 Plugins \u2192 AI Blueprint Assistant"))
				.Font(FAppStyle::GetFontStyle("SmallFont"))
				.ColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.55f, 0.55f)))
				.AutoWrapText(true)
			]

			// ---- Requirement Input ----
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 4.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("RequirementLabel", "Describe what Blueprint nodes to generate:"))
					.Font(FAppStyle::GetFontStyle("SmallFont"))
				]
				+ SVerticalBox::Slot()
				.MaxHeight(120.f)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
					.Padding(FMargin(4.f))
					[
						SAssignNew(RequirementInput, SMultiLineEditableText)
						.HintText(LOCTEXT("RequirementHint",
							"e.g. On BeginPlay, set Health to 100 and print it to screen."))
						.AutoWrapText(true)
					]
				]
			]

			// ---- Generate Button ----
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 8.f, 0.f, 2.f)
			[
				SAssignNew(GenerateButton, SButton)
				.Text(LOCTEXT("GenerateButtonLabel", "Generate Blueprint Nodes"))
				.HAlign(HAlign_Center)
				.OnClicked(this, &SAIBPAssistantWidget::OnGenerateClicked)
			]

			// ---- Analyze Button ----
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 2.f, 0.f, 4.f)
			[
				SAssignNew(AnalyzeButton, SButton)
				.Text(LOCTEXT("AnalyzeButtonLabel", "Analyze Blueprint"))
				.HAlign(HAlign_Center)
				.ToolTipText(LOCTEXT("AnalyzeButtonTooltip",
					"Ask AI to summarise what this Blueprint does and explain it module by module."))
				.OnClicked(this, &SAIBPAssistantWidget::OnAnalyzeClicked)
			]

			// ---- Status Text ----
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 2.f)
			[
				SAssignNew(StatusText, STextBlock)
				.Text(LOCTEXT("StatusReady", "Ready"))
				.Font(FAppStyle::GetFontStyle("SmallFont"))
				.ColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.9f, 0.5f)))
			]

			// ---- Log ScrollBox ----
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			.Padding(0.f, 4.f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
				.Padding(FMargin(4.f))
				[
					SAssignNew(LogScrollBox, SScrollBox)
				]
			]
		]
	];
}

FReply SAIBPAssistantWidget::OnGenerateClicked()
{
	if (bIsGenerating)
	{
		return FReply::Handled();
	}

	const FString UserReq = RequirementInput
		? RequirementInput->GetText().ToString()
		: FString();

	if (UserReq.IsEmpty())
	{
		AppendLog(TEXT("[Error] Please enter a requirement before generating."));
		return FReply::Handled();
	}

	// Read settings for API credentials
	const UAIBPSettings* Settings = GetDefault<UAIBPSettings>();
	const FString ApiUrl = Settings ? Settings->ApiUrl : FString();
	const FString ApiKey = Settings ? Settings->ApiKey : FString();

	if (ApiKey.IsEmpty())
	{
		AppendLog(TEXT("[Warning] No API key configured — request may be rejected. "
					   "Set it in Project Settings > Plugins > AI Blueprint Assistant."));
	}

	SetGenerating(true);
	AppendLog(TEXT("[Info] Fetching Blueprint context..."));

	TSharedPtr<FJsonObject> ContextJson = UAIBPContextUtils::GetActiveBlueprintData();
	if (!ContextJson.IsValid())
	{
		AppendLog(TEXT("[Warning] No active Blueprint editor found — sending request without context."));
		ContextJson = MakeShared<FJsonObject>();
	}

	AppendLog(TEXT("[Info] Sending request to AI..."));

	// Build conversation history to pass (empty array if history is disabled)
	const bool bUseHistory = Settings ? Settings->bEnableConversationHistory : false;
	TArray<TSharedPtr<FJsonObject>> HistoryToSend = bUseHistory ? ConversationHistory : TArray<TSharedPtr<FJsonObject>>();

	// Snapshot user content for history recording after the response
	FString ContextStr;
	if (ContextJson.IsValid())
	{
		TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
			TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&ContextStr);
		FJsonSerializer::Serialize(ContextJson.ToSharedRef(), Writer);
	}
	const FString UserContent = FString::Printf(
		TEXT("User requirement:\n%s\n\nBlueprint context (JSON):\n%s"),
		*UserReq,
		ContextStr.IsEmpty() ? TEXT("{}") : *ContextStr);

	// Prevent use-after-free if the tab is closed while the request is in flight
	TWeakPtr<SAIBPAssistantWidget> WeakThis = SharedThis(this);

	const UAIBPSettings* SettingsForRepair = GetDefault<UAIBPSettings>();
	const int32 MaxAttempts = (SettingsForRepair && SettingsForRepair->MaxRepairAttempts > 0)
		? SettingsForRepair->MaxRepairAttempts
		: 1;  // 0 means "no repair" — still do one import attempt

	UAIBPHttpService::SendRequest(
		UserReq,
		ContextJson,
		ApiKey,
		ApiUrl,
		HistoryToSend,
		[WeakThis, bUseHistory, UserContent, UserReq, ContextJson, MaxAttempts]
		(bool bSuccess, const FString& Result)
		{
			// Marshal back to GameThread for all UObject / Slate access
			AsyncTask(ENamedThreads::GameThread,
			[WeakThis, bSuccess, Result, bUseHistory, UserContent, UserReq, ContextJson, MaxAttempts]()
			{
				TSharedPtr<SAIBPAssistantWidget> PinnedThis = WeakThis.Pin();
				if (!PinnedThis.IsValid()) { return; }

				PinnedThis->HandleGenerationResponse(
					bSuccess, Result,
					UserReq, ContextJson, UserContent,
					bUseHistory,
					/*bIsFirstAttempt=*/true,
					/*AttemptNum=*/1,
					MaxAttempts);
			});
		}
	);

	return FReply::Handled();
}

void SAIBPAssistantWidget::HandleGenerationResponse(
	bool bSuccess,
	const FString& T3DResult,
	const FString& OriginalUserReq,
	TSharedPtr<FJsonObject> ContextJson,
	const FString& UserContent,
	bool bUseHistory,
	bool bIsFirstAttempt,
	int32 AttemptNum,
	int32 MaxAttempts)
{
	if (!bSuccess)
	{
		AppendLog(FString::Printf(TEXT("[Error] Request failed: %s"), *T3DResult));
		SetGenerating(false);
		return;
	}

	AppendLog(TEXT("[Info] AI response received."));

	// ---- Optional preview dialog (first attempt only) ----
	if (bIsFirstAttempt)
	{
		const UAIBPSettings* Cfg = GetDefault<UAIBPSettings>();
		if (Cfg && Cfg->bShowPreviewDialog)
		{
			if (!ShowPreviewDialog(T3DResult))
			{
				AppendLog(TEXT("[Info] Import cancelled by user."));
				SetGenerating(false);
				return;
			}
		}
	}

	// ---- Import nodes ----
	AppendLog(AttemptNum == 1
		? TEXT("[Info] Importing nodes into Blueprint graph...")
		: FString::Printf(TEXT("[Info] Re-importing corrected nodes (attempt %d/%d)..."),
		                  AttemptNum, MaxAttempts));

	const FAIBPImportResult ImportResult = FAIBPNodeFactory::ExecuteT3DImport(T3DResult);

	if (!ImportResult.bImportSuccess)
	{
		AppendLog(TEXT("[Error] Failed to import T3D nodes. See Message Log for details."));
		SetGenerating(false);
		return;
	}

	// ---- Compilation succeeded ----
	if (ImportResult.bCompileSuccess)
	{
		AppendLog(TEXT("[Success] Blueprint nodes generated and compiled successfully."));

		if (bUseHistory)
		{
			TSharedPtr<FJsonObject> UserMsg = MakeShared<FJsonObject>();
			UserMsg->SetStringField(TEXT("role"),    TEXT("user"));
			UserMsg->SetStringField(TEXT("content"), UserContent);

			TSharedPtr<FJsonObject> AssistantMsg = MakeShared<FJsonObject>();
			AssistantMsg->SetStringField(TEXT("role"),    TEXT("assistant"));
			AssistantMsg->SetStringField(TEXT("content"), T3DResult);

			ConversationHistory.Add(UserMsg);
			ConversationHistory.Add(AssistantMsg);

			AppendLog(FString::Printf(
				TEXT("[Info] Conversation history: %d turn(s) recorded."),
				ConversationHistory.Num() / 2));
		}

		SetGenerating(false);
		return;
	}

	// ---- Compilation failed ----
	AppendLog(FString::Printf(
		TEXT("[Warning] Blueprint has compile errors (attempt %d/%d):\n%s"),
		AttemptNum, MaxAttempts, *ImportResult.CompileErrors));

	// If self-repair is disabled or we've exhausted attempts, keep the nodes as-is
	const UAIBPSettings* Cfg = GetDefault<UAIBPSettings>();
	const bool bRepairEnabled = Cfg && Cfg->MaxRepairAttempts > 0;

	if (!bRepairEnabled || AttemptNum >= MaxAttempts)
	{
		AppendLog(bRepairEnabled
			? TEXT("[Warning] Max self-repair attempts reached. Blueprint may still have errors.")
			: TEXT("[Warning] Self-repair is disabled. Blueprint may have errors."));
		SetGenerating(false);
		return;
	}

	// ---- Undo failed import and request repair from AI ----
	AppendLog(FString::Printf(
		TEXT("[Info] Undoing import and requesting self-repair from AI (attempt %d/%d)..."),
		AttemptNum + 1, MaxAttempts));

	if (GEditor)
	{
		GEditor->UndoTransaction();
	}

	// Build repair message: include original requirement + compiler errors
	const FString RepairReq = FString::Printf(
		TEXT("The Blueprint nodes you generated had compilation errors. Please fix them.\n\n"
		     "Original requirement:\n%s\n\n"
		     "Compilation errors:\n%s\n\n"
		     "Generate corrected T3D that resolves all errors above."),
		*OriginalUserReq,
		*ImportResult.CompileErrors);

	// Send repair request — do NOT include conversation history to keep context clean
	TWeakPtr<SAIBPAssistantWidget> WeakThis = SharedThis(this);
	const int32 NextAttempt = AttemptNum + 1;

	UAIBPHttpService::SendRequest(
		RepairReq,
		ContextJson,
		Cfg ? Cfg->ApiKey : FString(),
		Cfg ? Cfg->ApiUrl : FString(),
		TArray<TSharedPtr<FJsonObject>>(),   // no history for repair requests
		[WeakThis, OriginalUserReq, ContextJson, UserContent, bUseHistory, NextAttempt, MaxAttempts]
		(bool bSuc, const FString& RepairResult)
		{
			AsyncTask(ENamedThreads::GameThread,
			[WeakThis, bSuc, RepairResult, OriginalUserReq, ContextJson,
			 UserContent, bUseHistory, NextAttempt, MaxAttempts]()
			{
				TSharedPtr<SAIBPAssistantWidget> PinnedThis = WeakThis.Pin();
				if (!PinnedThis.IsValid()) { return; }

				PinnedThis->HandleGenerationResponse(
					bSuc, RepairResult,
					OriginalUserReq, ContextJson, UserContent,
					bUseHistory,
					/*bIsFirstAttempt=*/false,
					NextAttempt,
					MaxAttempts);
			});
		}
	);
}

FReply SAIBPAssistantWidget::OnAnalyzeClicked()
{
	if (bIsGenerating)
	{
		return FReply::Handled();
	}

	const UAIBPSettings* Settings = GetDefault<UAIBPSettings>();
	const FString ApiUrl = Settings ? Settings->ApiUrl : FString();
	const FString ApiKey = Settings ? Settings->ApiKey : FString();

	if (ApiKey.IsEmpty())
	{
		AppendLog(TEXT("[Warning] No API key configured — request may be rejected. "
					   "Set it in Project Settings > Plugins > AI Blueprint Assistant."));
	}

	SetGenerating(true);
	AppendLog(TEXT("[Analyze] Fetching Blueprint context and graph data..."));

	TSharedPtr<FJsonObject> ContextJson = UAIBPContextUtils::GetActiveBlueprintData();
	TSharedPtr<FJsonObject> GraphJson   = UAIBPContextUtils::GetActiveBlueprintGraphData();

	if (!ContextJson.IsValid() && !GraphJson.IsValid())
	{
		AppendLog(TEXT("[Warning] No active Blueprint editor found. Please open a Blueprint first."));
		SetGenerating(false);
		return FReply::Handled();
	}

	if (!ContextJson.IsValid())
	{
		ContextJson = MakeShared<FJsonObject>();
	}
	if (!GraphJson.IsValid())
	{
		GraphJson = MakeShared<FJsonObject>();
	}

	AppendLog(TEXT("[Analyze] Sending to AI..."));

	TWeakPtr<SAIBPAssistantWidget> WeakThis = SharedThis(this);

	UAIBPHttpService::SendAnalysisRequest(
		ContextJson,
		GraphJson,
		ApiKey,
		ApiUrl,
		[WeakThis](bool bSuccess, const FString& Result)
		{
			AsyncTask(ENamedThreads::GameThread, [WeakThis, bSuccess, Result]()
			{
				TSharedPtr<SAIBPAssistantWidget> PinnedThis = WeakThis.Pin();
				if (!PinnedThis.IsValid()) { return; }

				if (!bSuccess)
				{
					PinnedThis->AppendLog(FString::Printf(
						TEXT("[Error] Analysis request failed: %s"), *Result));
					PinnedThis->SetGenerating(false);
					return;
				}

				PinnedThis->AppendLog(TEXT("[Analyze] Analysis complete. Opening report window..."));

				// Open the analysis result in a modal dialog
				TSharedRef<SWindow> AnalysisWindow = SNew(SWindow)
					.Title(LOCTEXT("AnalysisWindowTitle",
						"AI Blueprint Assistant — Blueprint Analysis Report"))
					.ClientSize(FVector2D(720.f, 600.f))
					.SupportsMinimize(false)
					.SupportsMaximize(true)
					.SizingRule(ESizingRule::UserSized);

				TSharedRef<SAIBPAnalysisDialog> DialogContent =
					SNew(SAIBPAnalysisDialog)
					.AnalysisText(Result)
					.ParentWindow(AnalysisWindow);

				AnalysisWindow->SetContent(DialogContent);
				FSlateApplication::Get().AddModalWindow(AnalysisWindow, PinnedThis);

				PinnedThis->SetGenerating(false);
			});
		}
	);

	return FReply::Handled();
}

FReply SAIBPAssistantWidget::OnNewConversationClicked()
{
	const int32 PreviousTurns = ConversationHistory.Num() / 2;
	ConversationHistory.Empty();
	AppendLog(FString::Printf(
		TEXT("[Info] Conversation history cleared (%d turn(s) removed). Ready for a new session."),
		PreviousTurns));
	return FReply::Handled();
}

bool SAIBPAssistantWidget::ShowPreviewDialog(const FString& T3DCode)
{
	TSharedRef<SWindow> PreviewWindow = SNew(SWindow)
		.Title(LOCTEXT("PreviewWindowTitle", "AI Blueprint Assistant — Review Generated Nodes"))
		.ClientSize(FVector2D(700.f, 500.f))
		.SupportsMinimize(false)
		.SupportsMaximize(true)
		.SizingRule(ESizingRule::UserSized);

	TSharedRef<SAIBPPreviewDialog> DialogContent =
		SNew(SAIBPPreviewDialog)
		.T3DCode(T3DCode)
		.ParentWindow(PreviewWindow);

	PreviewWindow->SetContent(DialogContent);

	// AddModalWindow blocks until the window is closed
	FSlateApplication::Get().AddModalWindow(PreviewWindow, SharedThis(this));

	return DialogContent->WasConfirmed();
}

void SAIBPAssistantWidget::AppendLog(const FString& Message)
{
	if (!LogScrollBox.IsValid())
	{
		return;
	}

	LogScrollBox->AddSlot()
	[
		SNew(STextBlock)
		.Text(FText::FromString(Message))
		.Font(FAppStyle::GetFontStyle("SmallFont"))
		.AutoWrapText(true)
	];

	LogScrollBox->ScrollToEnd();
}

void SAIBPAssistantWidget::SetGenerating(bool bNewGenerating)
{
	bIsGenerating = bNewGenerating;

	if (GenerateButton.IsValid())
	{
		GenerateButton->SetEnabled(!bIsGenerating);
	}

	if (AnalyzeButton.IsValid())
	{
		AnalyzeButton->SetEnabled(!bIsGenerating);
	}

	if (StatusText.IsValid())
	{
		if (bIsGenerating)
		{
			StatusText->SetText(LOCTEXT("StatusGenerating", "Generating..."));
			StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.85f, 0.3f)));
		}
		else
		{
			StatusText->SetText(LOCTEXT("StatusReady", "Ready"));
			StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.9f, 0.5f)));
		}
	}
}

FText SAIBPAssistantWidget::GetModelInfoText() const
{
	const UAIBPSettings* Settings = GetDefault<UAIBPSettings>();
	if (!Settings)
	{
		return LOCTEXT("ModelInfoUnknown", "Model: (unknown)");
	}

	const bool bHasKey = !Settings->ApiKey.IsEmpty();
	const bool bHasHistory = Settings->bEnableConversationHistory;
	const int32 HistoryTurns = ConversationHistory.Num() / 2;

	FString Info = FString::Printf(TEXT("Model: %s"), *Settings->ModelName);

	// Show which UE version the AI is targeting — updates instantly when changed in Project Settings.
	Info += FString::Printf(TEXT("  |  Target UE: %d.%d"),
		Settings->TargetEngineVersionMajor,
		Settings->TargetEngineVersionMinor);

	if (!bHasKey)
	{
		Info += TEXT("  |  [No API Key]");
	}
	if (bHasHistory)
	{
		Info += FString::Printf(TEXT("  |  History: %d turn(s)"), HistoryTurns);
	}
	return FText::FromString(Info);
}

#undef LOCTEXT_NAMESPACE
