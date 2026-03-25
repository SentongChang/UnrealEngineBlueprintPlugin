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
 */
UCLASS()
class AIBLUEPRINTASSISTANT_API UAIBPHttpService : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Sends an asynchronous POST request to @p ApiUrl.
	 *
	 * @param UserRequirement  Natural-language description entered by the user.
	 * @param ContextJson      Blueprint context produced by UAIBPContextUtils.
	 *                         May be an empty (but valid) FJsonObject when no
	 *                         Blueprint editor is open.
	 * @param ApiKey           Bearer token for the API (e.g. "sk-...").
	 * @param ApiUrl           Full endpoint URL.
	 * @param OnComplete       Callback: (bSuccess, T3DCodeOrErrorMessage).
	 */
	static void SendRequest(
		const FString& UserRequirement,
		TSharedPtr<FJsonObject> ContextJson,
		const FString& ApiKey,
		const FString& ApiUrl,
		TFunction<void(bool bSuccess, const FString& Result)> OnComplete);

private:
	/** Builds the JSON request body following the OpenAI Chat Completions spec. */
	static FString BuildRequestBody(
		const FString& UserRequirement,
		TSharedPtr<FJsonObject> ContextJson);

	/**
	 * Parses the raw API response and extracts the first T3D code block
	 * (text between "Begin Object Class=" and the final "End Object").
	 *
	 * @return Extracted T3D text, or empty string on failure.
	 */
	static FString ExtractT3DFromResponse(const FString& ResponseBody);

	/** System prompt embedded in every request — instructs the model to output
	 *  only raw T3D-format Blueprint nodes. */
	static const TCHAR* SystemPrompt;
};
