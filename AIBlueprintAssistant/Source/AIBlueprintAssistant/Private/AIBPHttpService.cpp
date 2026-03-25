// Copyright (c) 2026 AIBlueprintAssistant. All Rights Reserved.

#include "AIBPHttpService.h"

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

#define LOCTEXT_NAMESPACE "UAIBPHttpService"

// ---------------------------------------------------------------------------
// System Prompt
// ---------------------------------------------------------------------------

const TCHAR* UAIBPHttpService::SystemPrompt =
TEXT(
"你现在是一个虚幻引擎 5.6 蓝图专家。\n"
"输入：用户需求 + 蓝图当前变量/组件上下文（JSON 格式）。\n"
"输出要求：\n"
"1. 仅输出以 Begin Object Class= 开头并以 End Object 结尾的 T3D 格式文本。\n"
"2. 严禁输出任何解释性文字、代码块标记（如 ```）或其他内容。\n"
"3. 必须使用上下文提供的变量名和组件名进行连线。\n"
"4. 优先使用 UE 5.6 的新 API（如 Enhanced Input）。\n"
"5. 确保节点之间的连接（Pin Links）逻辑严密，所有引脚均正确配对。\n"
"6. 直接从 Begin Object 开始输出，不要有任何前缀文本。"
);

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void UAIBPHttpService::SendRequest(
	const FString& UserRequirement,
	TSharedPtr<FJsonObject> ContextJson,
	const FString& ApiKey,
	const FString& ApiUrl,
	TFunction<void(bool bSuccess, const FString& Result)> OnComplete)
{
	if (ApiUrl.IsEmpty())
	{
		OnComplete(false, TEXT("API URL is empty. Please configure the endpoint URL."));
		return;
	}

	const FString RequestBody = BuildRequestBody(UserRequirement, ContextJson);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
		FHttpModule::Get().CreateRequest();

	Request->SetVerb(TEXT("POST"));
	Request->SetURL(ApiUrl);
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	if (!ApiKey.IsEmpty())
	{
		Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey));
	}

	Request->SetContentAsString(RequestBody);

	// Capture OnComplete by value so it outlives this stack frame
	Request->OnProcessRequestComplete().BindLambda(
		[OnComplete](FHttpRequestPtr /*Req*/, FHttpResponsePtr Response, bool bConnectedSuccessfully)
		{
			if (!bConnectedSuccessfully || !Response.IsValid())
			{
				OnComplete(false, TEXT("HTTP request failed: could not connect to the server."));
				return;
			}

			const int32 StatusCode = Response->GetResponseCode();
			if (StatusCode < 200 || StatusCode >= 300)
			{
				const FString ErrorMsg = FString::Printf(
					TEXT("HTTP request failed with status %d: %s"),
					StatusCode,
					*Response->GetContentAsString());
				OnComplete(false, ErrorMsg);
				return;
			}

			const FString T3DCode = ExtractT3DFromResponse(Response->GetContentAsString());
			if (T3DCode.IsEmpty())
			{
				OnComplete(false,
					TEXT("Could not extract T3D code from the AI response. "
						 "The model may have returned an explanation instead of raw T3D. "
						 "Check your system prompt or retry."));
				return;
			}

			OnComplete(true, T3DCode);
		});

	Request->ProcessRequest();
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

FString UAIBPHttpService::BuildRequestBody(
	const FString& UserRequirement,
	TSharedPtr<FJsonObject> ContextJson)
{
	// Serialise the context JSON to a compact string
	FString ContextStr;
	if (ContextJson.IsValid())
	{
		TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
			TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&ContextStr);
		FJsonSerializer::Serialize(ContextJson.ToSharedRef(), Writer);
	}

	// User message combines the natural-language requirement and the context blob
	const FString UserContent = FString::Printf(
		TEXT("用户需求：\n%s\n\n蓝图上下文（JSON）：\n%s"),
		*UserRequirement,
		ContextStr.IsEmpty() ? TEXT("{}") : *ContextStr);

	// Build the messages array
	TArray<TSharedPtr<FJsonValue>> Messages;

	// System message
	{
		TSharedPtr<FJsonObject> SysMsg = MakeShared<FJsonObject>();
		SysMsg->SetStringField(TEXT("role"),    TEXT("system"));
		SysMsg->SetStringField(TEXT("content"), FString(SystemPrompt));
		Messages.Add(MakeShared<FJsonValueObject>(SysMsg));
	}

	// User message
	{
		TSharedPtr<FJsonObject> UserMsg = MakeShared<FJsonObject>();
		UserMsg->SetStringField(TEXT("role"),    TEXT("user"));
		UserMsg->SetStringField(TEXT("content"), UserContent);
		Messages.Add(MakeShared<FJsonValueObject>(UserMsg));
	}

	// Root request object
	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("model"),       TEXT("gpt-4o"));
	Root->SetArrayField(TEXT("messages"),     Messages);
	Root->SetNumberField(TEXT("max_tokens"),  2048);
	Root->SetNumberField(TEXT("temperature"), 0.2);   // Lower = more deterministic T3D output

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
	return OutputString;
}

FString UAIBPHttpService::ExtractT3DFromResponse(const FString& ResponseBody)
{
	// Parse the OpenAI-style JSON response
	TSharedPtr<FJsonObject> ResponseJson;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);
	if (!FJsonSerializer::Deserialize(Reader, ResponseJson) || !ResponseJson.IsValid())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("AIBPHttpService: Failed to parse API response JSON. Body: %s"),
			*ResponseBody.Left(512));
		return FString();
	}

	// Navigate choices[0].message.content
	const TArray<TSharedPtr<FJsonValue>>* Choices = nullptr;
	if (!ResponseJson->TryGetArrayField(TEXT("choices"), Choices) || !Choices || Choices->IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("AIBPHttpService: 'choices' array missing or empty in response."));
		return FString();
	}

	const TSharedPtr<FJsonObject>* FirstChoice = (*Choices)[0]->AsObjectChecked() ? &(*Choices)[0]->AsObject() : nullptr;
	if (!FirstChoice || !(*FirstChoice).IsValid())
	{
		return FString();
	}

	const TSharedPtr<FJsonObject>* MessageObj = nullptr;
	if (!(*FirstChoice)->TryGetObjectField(TEXT("message"), MessageObj) || !MessageObj)
	{
		return FString();
	}

	FString Content;
	if (!(*MessageObj)->TryGetStringField(TEXT("content"), Content))
	{
		return FString();
	}

	// Strip any accidental markdown code fences the model may have added
	Content = Content.TrimStartAndEnd();
	if (Content.StartsWith(TEXT("```")))
	{
		// Remove the opening fence line
		int32 NewlineIdx = INDEX_NONE;
		Content.FindChar(TEXT('\n'), NewlineIdx);
		if (NewlineIdx != INDEX_NONE)
		{
			Content = Content.RightChop(NewlineIdx + 1);
		}
		// Remove the closing fence
		if (Content.EndsWith(TEXT("```")))
		{
			Content = Content.LeftChop(3).TrimEnd();
		}
	}

	// Validate that we actually have a T3D block
	if (!Content.Contains(TEXT("Begin Object Class=")) && !Content.Contains(TEXT("BEGIN OBJECT CLASS=")))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("AIBPHttpService: Response content does not contain a T3D Begin Object block."));
		return FString();
	}

	return Content;
}

#undef LOCTEXT_NAMESPACE
