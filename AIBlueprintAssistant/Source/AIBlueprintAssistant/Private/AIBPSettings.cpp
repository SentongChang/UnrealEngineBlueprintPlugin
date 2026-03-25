// Copyright (c) 2026 AIBlueprintAssistant. All Rights Reserved.

#include "AIBPSettings.h"

UAIBPSettings::UAIBPSettings()
{
	// API defaults — user should override ApiKey before generating.
	ApiUrl    = TEXT("https://api.openai.com/v1/chat/completions");
	ApiKey    = TEXT("");

	// Model defaults that work well for structured T3D generation.
	ModelName   = TEXT("gpt-4o");
	MaxTokens   = 2048;
	Temperature = 0.2f;

	// Prompt: empty means "use built-in default with few-shot examples".
	CustomSystemPrompt = TEXT("");

	// Behaviour defaults.
	bShowPreviewDialog        = true;   // Safer: user reviews T3D before import.
	bEnableConversationHistory = false;  // Off by default to keep requests simple.
	MaxRepairAttempts         = 3;      // Up to 3 auto-repair cycles on compile error.

	// Target engine version defaults to what this binary was compiled against.
	// Users can override in Project Settings to cross-target a different version.
	TargetEngineVersionMajor = ENGINE_MAJOR_VERSION;
	TargetEngineVersionMinor = ENGINE_MINOR_VERSION;
}

FName UAIBPSettings::GetCategoryName() const
{
	return FName(TEXT("Plugins"));
}

FText UAIBPSettings::GetSectionText() const
{
	return NSLOCTEXT("AIBPSettings", "SectionText", "AI Blueprint Assistant");
}
