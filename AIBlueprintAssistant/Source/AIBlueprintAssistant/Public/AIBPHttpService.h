// Copyright (c) 2026 AIBlueprintAssistant. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "AIBPHttpService.generated.h"

/**
 * Asynchronous HTTP service that sends a user requirement + Blueprint context
 * to an OpenAI-compatible API endpoint and returns the AI-generated T3D code.
 *
 * The callback is invoked on a background thread; callers are responsible for
 * marshalling back to the Game Thread before touching UObjects or Slate widgets.
 *
 * Supported API format: OpenAI Chat Completions (POST /v1/chat/completions)
 *
 * Settings (model name, max_tokens, temperature, custom system prompt) are read
 * from UAIBPSettings at call time, so they always reflect the latest Project Settings.
 */
UCLASS()
class AIBLUEPRINTASSISTANT_API UAIBPHttpService : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Sends an asynchronous POST request.
	 *
	 * @param UserRequirement     Natural-language description entered by the user.
	 * @param ContextJson         Blueprint context produced by UAIBPContextUtils.
	 * @param ApiKey              Bearer token. Reads from UAIBPSettings when empty.
	 * @param ApiUrl              Full endpoint URL. Reads from UAIBPSettings when empty.
	 * @param ConversationHistory Previous messages for multi-turn conversation support.
	 *                            Each entry is a JSON object with "role" and "content" fields.
	 *                            Pass an empty array for a fresh (single-turn) request.
	 * @param OnComplete          Callback: (bSuccess, T3DCodeOrErrorMessage).
	 */
	static void SendRequest(
		const FString& UserRequirement,
		TSharedPtr<FJsonObject> ContextJson,
		const FString& ApiKey,
		const FString& ApiUrl,
		const TArray<TSharedPtr<FJsonObject>>& ConversationHistory,
		TFunction<void(bool bSuccess, const FString& Result)> OnComplete);

	/** Returns the built-in default system prompt (with few-shot T3D examples). */
	static FString GetDefaultSystemPrompt();

private:
	/**
	 * Builds the JSON request body following the OpenAI Chat Completions spec.
	 *
	 * @param SystemPrompt        System message content (built-in default or custom).
	 * @param ConversationHistory Prior assistant/user turns to include (multi-turn).
	 * @param UserContent         The new user message content for this turn.
	 * @param ModelName           Model identifier from UAIBPSettings.
	 * @param MaxTokens           Token limit from UAIBPSettings.
	 * @param Temperature         Sampling temperature from UAIBPSettings.
	 */
	static FString BuildRequestBody(
		const FString& SystemPrompt,
		const TArray<TSharedPtr<FJsonObject>>& ConversationHistory,
		const FString& UserContent,
		const FString& ModelName,
		int32 MaxTokens,
		float Temperature);

	/**
	 * Parses the raw API response and extracts the first T3D code block.
	 * @return Extracted T3D text, or empty string on failure.
	 */
	static FString ExtractT3DFromResponse(const FString& ResponseBody);
};
