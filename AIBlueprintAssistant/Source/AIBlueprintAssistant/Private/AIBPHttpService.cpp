// Copyright (c) 2026 AIBlueprintAssistant. All Rights Reserved.

#include "AIBPHttpService.h"
#include "AIBPSettings.h"

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

#define LOCTEXT_NAMESPACE "UAIBPHttpService"

// ---------------------------------------------------------------------------
// Default System Prompt (with Few-Shot T3D examples)
// ---------------------------------------------------------------------------

FString UAIBPHttpService::GetDefaultSystemPrompt()
{
	// Read target version from project settings; fall back to compile-time engine
	// version if settings are unavailable (e.g. during automated tests).
	const UAIBPSettings* Settings = GetDefault<UAIBPSettings>();
	const int32 MajorVer = Settings ? Settings->TargetEngineVersionMajor : ENGINE_MAJOR_VERSION;
	const int32 MinorVer = Settings ? Settings->TargetEngineVersionMinor : ENGINE_MINOR_VERSION;
	const FString Ver = FString::Printf(TEXT("%d.%d"), MajorVer, MinorVer);

	const FString Line1 = FString::Printf(
		TEXT("You are an Unreal Engine %s Blueprint expert.\n"), *Ver);
	const FString Line4 = FString::Printf(
		TEXT("4. Prefer UE %s APIs (Enhanced Input, etc.) over deprecated ones.\n"), *Ver);

	return Line1
		+ TEXT("Input: a natural-language requirement from the user, plus a JSON snapshot of the\n")
		TEXT("currently open Blueprint (baseClass, variables, components, functions, interfaces).\n")
		TEXT("\n")
		TEXT("Output rules:\n")
		TEXT("1. Output ONLY raw T3D text. Start directly with 'Begin Object Class=' — no preamble.\n")
		TEXT("2. Do NOT output markdown fences (```), explanations, or any other text.\n")
		TEXT("3. Use the variable/component/function names exactly as they appear in the JSON context.\n")
		+ Line4
		+ TEXT("5. Ensure every Pin link is logically complete and correctly paired.\n")
		TEXT("6. You may output multiple Begin Object / End Object blocks for multi-node graphs.\n")
		TEXT("\n")
		TEXT("--- FEW-SHOT EXAMPLE 1 ---\n")
		TEXT("User: Print 'Hello' on BeginPlay.\n")
		TEXT("Assistant:\n")
		TEXT("Begin Object Class=/Script/BlueprintGraph.K2Node_Event Name=\"K2Node_Event_0\"\n")
		TEXT("   EventReference=(MemberParent=Class'/Script/Engine.Actor',MemberName=\"ReceiveBeginPlay\")\n")
		TEXT("   bOverrideFunction=True\n")
		TEXT("   NodePosX=0\n")
		TEXT("   NodePosY=0\n")
		TEXT("   NodeGuid=A1000000000000000000000000000001\n")
		TEXT("   CustomProperties Pin (PinId=A1000000000000000000000000000010,PinName=\"OutputDelegate\",")
		TEXT("PinType.PinCategory=\"delegate\",Direction=\"EGPD_Output\")\n")
		TEXT("   CustomProperties Pin (PinId=A1000000000000000000000000000011,PinName=\"then\",")
		TEXT("PinType.PinCategory=\"exec\",Direction=\"EGPD_Output\",")
		TEXT("LinkedTo=(K2Node_CallFunction_0 B1000000000000000000000000000010,))\n")
		TEXT("End Object\n")
		TEXT("Begin Object Class=/Script/BlueprintGraph.K2Node_CallFunction Name=\"K2Node_CallFunction_0\"\n")
		TEXT("   FunctionReference=(MemberParent=Class'/Script/Engine.KismetSystemLibrary',MemberName=\"PrintString\")\n")
		TEXT("   NodePosX=300\n")
		TEXT("   NodePosY=0\n")
		TEXT("   NodeGuid=B1000000000000000000000000000001\n")
		TEXT("   CustomProperties Pin (PinId=B1000000000000000000000000000010,PinName=\"execute\",")
		TEXT("PinType.PinCategory=\"exec\",")
		TEXT("LinkedTo=(K2Node_Event_0 A1000000000000000000000000000011,))\n")
		TEXT("   CustomProperties Pin (PinId=B1000000000000000000000000000011,PinName=\"then\",")
		TEXT("PinType.PinCategory=\"exec\",Direction=\"EGPD_Output\")\n")
		TEXT("   CustomProperties Pin (PinId=B1000000000000000000000000000012,PinName=\"InString\",")
		TEXT("PinType.PinCategory=\"string\",DefaultValue=\"Hello\")\n")
		TEXT("End Object\n")
		TEXT("\n")
		TEXT("--- FEW-SHOT EXAMPLE 2 ---\n")
		TEXT("User: On BeginPlay, set variable Health to 100.\n")
		TEXT("Context: { \"variables\": [{\"name\":\"Health\",\"type\":\"float\"}] }\n")
		TEXT("Assistant:\n")
		TEXT("Begin Object Class=/Script/BlueprintGraph.K2Node_Event Name=\"K2Node_Event_0\"\n")
		TEXT("   EventReference=(MemberParent=Class'/Script/Engine.Actor',MemberName=\"ReceiveBeginPlay\")\n")
		TEXT("   bOverrideFunction=True\n")
		TEXT("   NodePosX=0\n")
		TEXT("   NodePosY=0\n")
		TEXT("   NodeGuid=A2000000000000000000000000000001\n")
		TEXT("   CustomProperties Pin (PinId=A2000000000000000000000000000010,PinName=\"OutputDelegate\",")
		TEXT("PinType.PinCategory=\"delegate\",Direction=\"EGPD_Output\")\n")
		TEXT("   CustomProperties Pin (PinId=A2000000000000000000000000000011,PinName=\"then\",")
		TEXT("PinType.PinCategory=\"exec\",Direction=\"EGPD_Output\",")
		TEXT("LinkedTo=(K2Node_VariableSet_0 B2000000000000000000000000000010,))\n")
		TEXT("End Object\n")
		TEXT("Begin Object Class=/Script/BlueprintGraph.K2Node_VariableSet Name=\"K2Node_VariableSet_0\"\n")
		TEXT("   VariableReference=(MemberName=\"Health\",bSelfContext=True)\n")
		TEXT("   NodePosX=300\n")
		TEXT("   NodePosY=0\n")
		TEXT("   NodeGuid=B2000000000000000000000000000001\n")
		TEXT("   CustomProperties Pin (PinId=B2000000000000000000000000000010,PinName=\"execute\",")
		TEXT("PinType.PinCategory=\"exec\",")
		TEXT("LinkedTo=(K2Node_Event_0 A2000000000000000000000000000011,))\n")
		TEXT("   CustomProperties Pin (PinId=B2000000000000000000000000000011,PinName=\"then\",")
		TEXT("PinType.PinCategory=\"exec\",Direction=\"EGPD_Output\")\n")
		TEXT("   CustomProperties Pin (PinId=B2000000000000000000000000000012,PinName=\"Health\",")
		TEXT("PinType.PinCategory=\"real\",PinType.PinSubCategory=\"float\",DefaultValue=\"100.000000\")\n")
		TEXT("End Object\n")
		TEXT("\n")
		+ TEXT("Now generate T3D for the user's requirement. Output ONLY the T3D blocks, nothing else.");
}

// ---------------------------------------------------------------------------
// Analysis System Prompt
// ---------------------------------------------------------------------------

FString UAIBPHttpService::GetAnalysisSystemPrompt()
{
	return
		TEXT("You are an expert Unreal Engine Blueprint analyst.\n")
		TEXT("The user will provide:\n")
		TEXT("  1. A Blueprint context JSON (baseClass, variables, components, functions, interfaces)\n")
		TEXT("  2. A graph data JSON listing every node in every graph of the Blueprint\n")
		TEXT("\n")
		TEXT("Your task: analyse the Blueprint and produce a structured bilingual report.\n")
		TEXT("\n")
		TEXT("Output format (follow EXACTLY, no extra text outside this structure):\n")
		TEXT("\n")
		TEXT("## 总结\n")
		TEXT("[2-3 sentences in Chinese describing the overall purpose of this Blueprint]\n")
		TEXT("\n")
		TEXT("## Summary\n")
		TEXT("[2-3 sentences in English describing the overall purpose of this Blueprint]\n")
		TEXT("\n")
		TEXT("## 模块分析\n")
		TEXT("\n")
		TEXT("### [模块名 — use a descriptive Chinese name, e.g. 初始化 / 战斗逻辑 / UI 更新]\n")
		TEXT("**说明：** [Chinese explanation of what this group of nodes collectively achieves]\n")
		TEXT("**Description:** [English explanation]\n")
		TEXT("**涉及节点：** [comma-separated key node titles involved]\n")
		TEXT("\n")
		TEXT("[Repeat the ### block for each logical module found in the Blueprint]\n")
		TEXT("\n")
		TEXT("Rules:\n")
		TEXT("- Group nodes by logical purpose, not by graph page\n")
		TEXT("- Skip boilerplate/default nodes that add no meaningful logic\n")
		TEXT("- Do NOT output any T3D, code snippets, or JSON\n")
		TEXT("- Keep each explanation concise (1-3 sentences per module)");
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void UAIBPHttpService::SendAnalysisRequest(
	TSharedPtr<FJsonObject> ContextJson,
	TSharedPtr<FJsonObject> GraphJson,
	const FString& ApiKey,
	const FString& ApiUrl,
	TFunction<void(bool bSuccess, const FString& Result)> OnComplete)
{
	const UAIBPSettings* Settings = GetDefault<UAIBPSettings>();

	const FString EffectiveUrl = ApiUrl.IsEmpty()
		? (Settings ? Settings->ApiUrl : FString())
		: ApiUrl;
	const FString EffectiveKey = ApiKey.IsEmpty()
		? (Settings ? Settings->ApiKey : FString())
		: ApiKey;
	const FString EffectiveModel  = Settings ? Settings->ModelName  : TEXT("gpt-4o");
	const int32   EffectiveTokens = Settings ? Settings->MaxTokens  : 2048;
	const float   EffectiveTemp   = Settings ? Settings->Temperature : 0.2f;

	if (EffectiveUrl.IsEmpty())
	{
		OnComplete(false, TEXT("API URL is empty. Configure it in Project Settings > Plugins > AI Blueprint Assistant."));
		return;
	}

	// Serialise ContextJson and GraphJson into strings for the user message
	auto JsonToString = [](TSharedPtr<FJsonObject> Json) -> FString
	{
		if (!Json.IsValid()) { return TEXT("{}"); }
		FString Out;
		TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> W =
			TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Out);
		FJsonSerializer::Serialize(Json.ToSharedRef(), W);
		return Out;
	};

	const FString UserContent = FString::Printf(
		TEXT("Blueprint context (JSON):\n%s\n\nBlueprint graph nodes (JSON):\n%s"),
		*JsonToString(ContextJson),
		*JsonToString(GraphJson));

	const FString RequestBody = BuildRequestBody(
		GetAnalysisSystemPrompt(),
		TArray<TSharedPtr<FJsonObject>>(),  // no conversation history for analysis
		UserContent,
		EffectiveModel, EffectiveTokens, EffectiveTemp);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
		FHttpModule::Get().CreateRequest();

	Request->SetVerb(TEXT("POST"));
	Request->SetURL(EffectiveUrl);
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	if (!EffectiveKey.IsEmpty())
	{
		Request->SetHeader(TEXT("Authorization"),
			FString::Printf(TEXT("Bearer %s"), *EffectiveKey));
	}

	Request->SetContentAsString(RequestBody);

	Request->OnProcessRequestComplete().BindLambda(
		[OnComplete](FHttpRequestPtr /*Req*/, FHttpResponsePtr Response, bool bConnected)
		{
			if (!bConnected || !Response.IsValid())
			{
				OnComplete(false, TEXT("HTTP request failed: could not connect to the server."));
				return;
			}

			const int32 StatusCode = Response->GetResponseCode();
			if (StatusCode < 200 || StatusCode >= 300)
			{
				OnComplete(false, FString::Printf(
					TEXT("HTTP request failed with status %d: %s"),
					StatusCode, *Response->GetContentAsString()));
				return;
			}

			// Extract the text content from choices[0].message.content
			TSharedPtr<FJsonObject> ResponseJson;
			TSharedRef<TJsonReader<>> Reader =
				TJsonReaderFactory<>::Create(Response->GetContentAsString());
			if (!FJsonSerializer::Deserialize(Reader, ResponseJson) || !ResponseJson.IsValid())
			{
				OnComplete(false, TEXT("Failed to parse API response JSON."));
				return;
			}

			const TArray<TSharedPtr<FJsonValue>>* Choices = nullptr;
			if (!ResponseJson->TryGetArrayField(TEXT("choices"), Choices)
				|| !Choices || Choices->IsEmpty())
			{
				OnComplete(false, TEXT("'choices' array missing or empty in API response."));
				return;
			}

			const TSharedPtr<FJsonObject> FirstChoice = (*Choices)[0]->AsObject();
			if (!FirstChoice.IsValid())
			{
				OnComplete(false, TEXT("Could not read first choice from API response."));
				return;
			}

			const TSharedPtr<FJsonObject>* MessageObj = nullptr;
			if (!FirstChoice->TryGetObjectField(TEXT("message"), MessageObj) || !MessageObj)
			{
				OnComplete(false, TEXT("'message' field missing in API response choice."));
				return;
			}

			FString Content;
			if (!(*MessageObj)->TryGetStringField(TEXT("content"), Content))
			{
				OnComplete(false, TEXT("'content' field missing in API response message."));
				return;
			}

			OnComplete(true, Content.TrimStartAndEnd());
		});

	Request->ProcessRequest();
}

void UAIBPHttpService::SendRequest(
	const FString& UserRequirement,
	TSharedPtr<FJsonObject> ContextJson,
	const FString& ApiKey,
	const FString& ApiUrl,
	const TArray<TSharedPtr<FJsonObject>>& ConversationHistory,
	TFunction<void(bool bSuccess, const FString& Result)> OnComplete)
{
	// Resolve effective settings: prefer explicit args, fall back to UAIBPSettings.
	const UAIBPSettings* Settings = GetDefault<UAIBPSettings>();

	const FString EffectiveUrl = ApiUrl.IsEmpty()
		? (Settings ? Settings->ApiUrl : FString())
		: ApiUrl;
	const FString EffectiveKey = ApiKey.IsEmpty()
		? (Settings ? Settings->ApiKey : FString())
		: ApiKey;
	const FString EffectiveModel = Settings ? Settings->ModelName : TEXT("gpt-4o");
	const int32   EffectiveTokens = Settings ? Settings->MaxTokens : 2048;
	const float   EffectiveTemp   = Settings ? Settings->Temperature : 0.2f;
	const FString SystemPrompt    = (Settings && !Settings->CustomSystemPrompt.IsEmpty())
		? Settings->CustomSystemPrompt
		: GetDefaultSystemPrompt();

	if (EffectiveUrl.IsEmpty())
	{
		OnComplete(false, TEXT("API URL is empty. Configure it in Project Settings > Plugins > AI Blueprint Assistant."));
		return;
	}

	// Build user message content: requirement + context JSON
	FString ContextStr;
	if (ContextJson.IsValid())
	{
		TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
			TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&ContextStr);
		FJsonSerializer::Serialize(ContextJson.ToSharedRef(), Writer);
	}

	const FString UserContent = FString::Printf(
		TEXT("User requirement:\n%s\n\nBlueprint context (JSON):\n%s"),
		*UserRequirement,
		ContextStr.IsEmpty() ? TEXT("{}") : *ContextStr);

	const FString RequestBody = BuildRequestBody(
		SystemPrompt, ConversationHistory, UserContent,
		EffectiveModel, EffectiveTokens, EffectiveTemp);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
		FHttpModule::Get().CreateRequest();

	Request->SetVerb(TEXT("POST"));
	Request->SetURL(EffectiveUrl);
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	if (!EffectiveKey.IsEmpty())
	{
		Request->SetHeader(TEXT("Authorization"),
			FString::Printf(TEXT("Bearer %s"), *EffectiveKey));
	}

	Request->SetContentAsString(RequestBody);

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
					StatusCode, *Response->GetContentAsString());
				OnComplete(false, ErrorMsg);
				return;
			}

			const FString T3DCode = ExtractT3DFromResponse(Response->GetContentAsString());
			if (T3DCode.IsEmpty())
			{
				OnComplete(false,
					TEXT("Could not extract T3D code from the AI response. "
						 "The model may have returned an explanation instead of raw T3D. "
						 "Try adjusting the system prompt or retry."));
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
	const FString& SystemPrompt,
	const TArray<TSharedPtr<FJsonObject>>& ConversationHistory,
	const FString& UserContent,
	const FString& ModelName,
	int32 MaxTokens,
	float Temperature)
{
	TArray<TSharedPtr<FJsonValue>> Messages;

	// System message (always first)
	{
		TSharedPtr<FJsonObject> SysMsg = MakeShared<FJsonObject>();
		SysMsg->SetStringField(TEXT("role"),    TEXT("system"));
		SysMsg->SetStringField(TEXT("content"), SystemPrompt);
		Messages.Add(MakeShared<FJsonValueObject>(SysMsg));
	}

	// Conversation history (multi-turn): inject prior user/assistant turns
	for (const TSharedPtr<FJsonObject>& HistoryMsg : ConversationHistory)
	{
		if (HistoryMsg.IsValid())
		{
			Messages.Add(MakeShared<FJsonValueObject>(HistoryMsg));
		}
	}

	// Current user message
	{
		TSharedPtr<FJsonObject> UserMsg = MakeShared<FJsonObject>();
		UserMsg->SetStringField(TEXT("role"),    TEXT("user"));
		UserMsg->SetStringField(TEXT("content"), UserContent);
		Messages.Add(MakeShared<FJsonValueObject>(UserMsg));
	}

	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("model"),       ModelName);
	Root->SetArrayField(TEXT("messages"),     Messages);
	Root->SetNumberField(TEXT("max_tokens"),  MaxTokens);
	Root->SetNumberField(TEXT("temperature"), Temperature);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
	return OutputString;
}

FString UAIBPHttpService::ExtractT3DFromResponse(const FString& ResponseBody)
{
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

	const TSharedPtr<FJsonObject>* FirstChoice =
		(*Choices)[0]->AsObjectChecked() ? &(*Choices)[0]->AsObject() : nullptr;
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

	// Strip accidental markdown fences the model may have added
	Content = Content.TrimStartAndEnd();
	if (Content.StartsWith(TEXT("```")))
	{
		int32 NewlineIdx = INDEX_NONE;
		Content.FindChar(TEXT('\n'), NewlineIdx);
		if (NewlineIdx != INDEX_NONE)
		{
			Content = Content.RightChop(NewlineIdx + 1);
		}
		if (Content.EndsWith(TEXT("```")))
		{
			Content = Content.LeftChop(3).TrimEnd();
		}
	}

	// Validate T3D block presence
	if (!Content.Contains(TEXT("Begin Object Class=")) && !Content.Contains(TEXT("BEGIN OBJECT CLASS=")))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("AIBPHttpService: Response does not contain a T3D Begin Object block."));
		return FString();
	}

	return Content;
}

#undef LOCTEXT_NAMESPACE
