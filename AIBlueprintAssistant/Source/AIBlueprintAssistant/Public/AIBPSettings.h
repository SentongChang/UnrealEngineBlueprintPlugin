// Copyright (c) 2026 AIBlueprintAssistant. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "AIBPSettings.generated.h"

/**
 * Project-level settings for the AI Blueprint Assistant plugin.
 * Accessible at: Edit > Project Settings > Plugins > AI Blueprint Assistant
 *
 * Values are persisted to EditorPerProjectUserSettings (Config/UserEditorPerProjectIni).
 */
UCLASS(Config=EditorPerProjectUserSettings, meta=(DisplayName="AI Blueprint Assistant"))
class AIBLUEPRINTASSISTANT_API UAIBPSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UAIBPSettings();

	// ---- API ----------------------------------------------------------------

	/** Full OpenAI-compatible endpoint URL (e.g. https://api.openai.com/v1/chat/completions). */
	UPROPERTY(Config, EditAnywhere, Category="API", meta=(DisplayName="API URL"))
	FString ApiUrl;

	/** Bearer token for the API endpoint (e.g. "sk-..."). Stored per-user, not committed to source. */
	UPROPERTY(Config, EditAnywhere, Category="API", meta=(DisplayName="API Key"))
	FString ApiKey;

	// ---- Model --------------------------------------------------------------

	/** Model identifier sent in every request (e.g. gpt-4o, gpt-4-turbo, claude-3-5-sonnet-20241022). */
	UPROPERTY(Config, EditAnywhere, Category="Model", meta=(DisplayName="Model Name"))
	FString ModelName;

	/** Maximum number of tokens allowed in the model response. */
	UPROPERTY(Config, EditAnywhere, Category="Model",
		meta=(DisplayName="Max Tokens", ClampMin=256, ClampMax=8192))
	int32 MaxTokens;

	/** Sampling temperature: 0 = deterministic, 1 = creative. Keep low for T3D generation. */
	UPROPERTY(Config, EditAnywhere, Category="Model",
		meta=(DisplayName="Temperature", ClampMin=0.0f, ClampMax=1.0f))
	float Temperature;

	// ---- Prompt -------------------------------------------------------------

	/**
	 * Override the built-in system prompt. Leave empty to use the default prompt
	 * (which includes T3D format instructions and few-shot examples).
	 */
	UPROPERTY(Config, EditAnywhere, Category="Prompt",
		meta=(DisplayName="Custom System Prompt", MultiLine=true))
	FString CustomSystemPrompt;

	// ---- Target Engine Version ----------------------------------------------

	/**
	 * Major UE version the AI should target when generating Blueprint nodes.
	 * Defaults to ENGINE_MAJOR_VERSION (auto-detected at compile time).
	 * Override here to cross-target: e.g. run UE 5.6 editor but generate code
	 * compatible with a UE 5.3 project.
	 */
	UPROPERTY(Config, EditAnywhere, Category="Target Engine Version",
		meta=(DisplayName="Target Major Version", ClampMin=5, ClampMax=5))
	int32 TargetEngineVersionMajor;

	/**
	 * Minor UE version the AI should target (0 = 5.0, 3 = 5.3, 6 = 5.6, etc.).
	 * Defaults to ENGINE_MINOR_VERSION (auto-detected at compile time).
	 */
	UPROPERTY(Config, EditAnywhere, Category="Target Engine Version",
		meta=(DisplayName="Target Minor Version", ClampMin=0, ClampMax=9))
	int32 TargetEngineVersionMinor;

	// ---- Behavior -----------------------------------------------------------

	/**
	 * When enabled, a preview window shows the raw T3D code after each generation.
	 * You must explicitly click "Import Nodes" to apply the change.
	 * Disable for instant injection without confirmation.
	 */
	UPROPERTY(Config, EditAnywhere, Category="Behavior",
		meta=(DisplayName="Show T3D Preview Before Import"))
	bool bShowPreviewDialog;

	/**
	 * When enabled, previous request/response pairs are sent as conversation history
	 * so the model can refine or extend already-generated nodes.
	 * Use "New Conversation" in the panel to clear the history.
	 */
	UPROPERTY(Config, EditAnywhere, Category="Behavior",
		meta=(DisplayName="Enable Conversation History (Multi-Turn)"))
	bool bEnableConversationHistory;

	// ---- UDeveloperSettings interface ---------------------------------------

	virtual FName GetCategoryName() const override;
	virtual FText GetSectionText()  const override;
};
