// Copyright (c) 2026 AIBlueprintAssistant. All Rights Reserved.

#include "SAIBPAssistantWidget.h"
#include "AIBPContextUtils.h"
#include "AIBPHttpService.h"
#include "AIBPNodeFactory.h"

#include "Async/Async.h"
#include "Styling/AppStyle.h"          // FAppStyle (replaces deprecated FEditorStyle)
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

			// ---- API URL ----
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 4.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ApiUrlLabel", "API URL"))
					.Font(FAppStyle::GetFontStyle("SmallFont"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(ApiUrlInput, SEditableText)
					.HintText(LOCTEXT("ApiUrlHint", "https://api.openai.com/v1/chat/completions"))
					.Text(FText::FromString(TEXT("https://api.openai.com/v1/chat/completions")))
				]
			]

			// ---- API Key ----
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 4.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ApiKeyLabel", "API Key"))
					.Font(FAppStyle::GetFontStyle("SmallFont"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(ApiKeyInput, SEditableText)
					.HintText(LOCTEXT("ApiKeyHint", "sk-..."))
					.IsPassword(true)
				]
			]

			// ---- Requirement Input ----
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 4.f)
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
						.HintText(LOCTEXT("RequirementHint", "e.g. When the player overlaps with TriggerBox, print the player's health variable to screen."))
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

	const FString ApiUrl  = ApiUrlInput  ? ApiUrlInput->GetText().ToString()  : FString();
	const FString ApiKey  = ApiKeyInput  ? ApiKeyInput->GetText().ToString()  : FString();
	const FString UserReq = RequirementInput ? RequirementInput->GetText().ToString() : FString();

	if (UserReq.IsEmpty())
	{
		AppendLog(TEXT("[Error] Please enter a requirement before generating."));
		return FReply::Handled();
	}

	if (ApiKey.IsEmpty())
	{
		AppendLog(TEXT("[Warning] No API key provided — request may be rejected by the server."));
	}

	SetGenerating(true);
	AppendLog(TEXT("[Info] Fetching blueprint context..."));

	// Extract current blueprint context
	TSharedPtr<FJsonObject> ContextJson = UAIBPContextUtils::GetActiveBlueprintData();

	if (!ContextJson.IsValid())
	{
		AppendLog(TEXT("[Warning] No active Blueprint editor found. Sending request without context."));
		ContextJson = MakeShared<FJsonObject>();
	}

	AppendLog(TEXT("[Info] Sending request to AI..."));

	// Fire async HTTP request
	UAIBPHttpService::SendRequest(
		UserReq,
		ContextJson,
		ApiKey,
		ApiUrl,
		[this](bool bSuccess, const FString& Result)
		{
			// This callback may arrive on a background thread; marshal to GameThread.
			AsyncTask(ENamedThreads::GameThread, [this, bSuccess, Result]()
			{
				if (bSuccess)
				{
					AppendLog(TEXT("[Info] AI response received. Importing nodes..."));
					const bool bImported = FAIBPNodeFactory::ExecuteT3DImport(Result);
					if (bImported)
					{
						AppendLog(TEXT("[Success] Blueprint nodes generated successfully."));
					}
					else
					{
						AppendLog(TEXT("[Error] Failed to import T3D nodes. See Message Log for details."));
					}
				}
				else
				{
					AppendLog(FString::Printf(TEXT("[Error] Request failed: %s"), *Result));
				}
				SetGenerating(false);
			});
		}
	);

	return FReply::Handled();
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

	// Scroll to bottom
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

#undef LOCTEXT_NAMESPACE
