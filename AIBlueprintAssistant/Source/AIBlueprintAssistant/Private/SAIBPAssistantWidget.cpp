// Copyright (c) 2026 AIBlueprintAssistant. All Rights Reserved.

#include "SAIBPAssistantWidget.h"
#include "AIBPContextUtils.h"
#include "AIBPHttpService.h"
#include "AIBPNodeFactory.h"
#include "AIBPSettings.h"
#include "SAIBPPreviewDialog.h"

#include "Async/Async.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/AppStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"

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
			.Padding(0.f, 8.f, 0.f, 4.f)
			[
				SAssignNew(GenerateButton, SButton)
				.Text(LOCTEXT("GenerateButtonLabel", "Generate Blueprint Nodes"))
				.HAlign(HAlign_Center)
				.OnClicked(this, &SAIBPAssistantWidget::OnGenerateClicked)
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

	UAIBPHttpService::SendRequest(
		UserReq,
		ContextJson,
		ApiKey,
		ApiUrl,
		HistoryToSend,
		[WeakThis, bUseHistory, UserContent](bool bSuccess, const FString& Result)
		{
			// Marshal back to GameThread for all UObject / Slate access
			AsyncTask(ENamedThreads::GameThread, [WeakThis, bSuccess, Result, bUseHistory, UserContent]()
			{
				TSharedPtr<SAIBPAssistantWidget> PinnedThis = WeakThis.Pin();
				if (!PinnedThis.IsValid())
				{
					return;
				}

				if (!bSuccess)
				{
					PinnedThis->AppendLog(FString::Printf(
						TEXT("[Error] Request failed: %s"), *Result));
					PinnedThis->SetGenerating(false);
					return;
				}

				PinnedThis->AppendLog(TEXT("[Info] AI response received."));

				// ---- Optional preview dialog ----
				const UAIBPSettings* Cfg = GetDefault<UAIBPSettings>();
				const bool bPreview = Cfg ? Cfg->bShowPreviewDialog : false;
				bool bShouldImport = true;

				if (bPreview)
				{
					bShouldImport = PinnedThis->ShowPreviewDialog(Result);
					if (!bShouldImport)
					{
						PinnedThis->AppendLog(TEXT("[Info] Import cancelled by user."));
						PinnedThis->SetGenerating(false);
						return;
					}
				}

				// ---- Import nodes ----
				PinnedThis->AppendLog(TEXT("[Info] Importing nodes into Blueprint graph..."));
				const bool bImported = FAIBPNodeFactory::ExecuteT3DImport(Result);
				if (bImported)
				{
					PinnedThis->AppendLog(TEXT("[Success] Blueprint nodes generated successfully."));

					// Record this turn in conversation history (if enabled)
					if (bUseHistory)
					{
						TSharedPtr<FJsonObject> UserMsg = MakeShared<FJsonObject>();
						UserMsg->SetStringField(TEXT("role"),    TEXT("user"));
						UserMsg->SetStringField(TEXT("content"), UserContent);

						TSharedPtr<FJsonObject> AssistantMsg = MakeShared<FJsonObject>();
						AssistantMsg->SetStringField(TEXT("role"),    TEXT("assistant"));
						AssistantMsg->SetStringField(TEXT("content"), Result);

						PinnedThis->ConversationHistory.Add(UserMsg);
						PinnedThis->ConversationHistory.Add(AssistantMsg);

						PinnedThis->AppendLog(FString::Printf(
							TEXT("[Info] Conversation history: %d turn(s) recorded."),
							PinnedThis->ConversationHistory.Num() / 2));
					}
				}
				else
				{
					PinnedThis->AppendLog(
						TEXT("[Error] Failed to import T3D nodes. See Message Log for details."));
				}

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
