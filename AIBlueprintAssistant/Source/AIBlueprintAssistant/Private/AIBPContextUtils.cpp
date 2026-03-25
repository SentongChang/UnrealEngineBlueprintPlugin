// Copyright (c) 2026 AIBlueprintAssistant. All Rights Reserved.

#include "AIBPContextUtils.h"

#include "Engine/Blueprint.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "EdGraph/EdGraphPin.h"
#include "Kismet2/BlueprintEditorUtils.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

// Editor subsystem / asset editor helpers
#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Toolkits/IToolkit.h"

#define LOCTEXT_NAMESPACE "UAIBPContextUtils"

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> UAIBPContextUtils::GetActiveBlueprintData()
{
	if (!GEditor)
	{
		UE_LOG(LogTemp, Warning, TEXT("AIBPContextUtils: GEditor is null — cannot retrieve blueprint context."));
		return nullptr;
	}

	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	if (!AssetEditorSubsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("AIBPContextUtils: UAssetEditorSubsystem not available."));
		return nullptr;
	}

	// Iterate all currently open assets to find the first UBlueprint
	UBlueprint* ActiveBlueprint = nullptr;
	TArray<UObject*> EditedAssets = AssetEditorSubsystem->GetAllEditedAssets();
	for (UObject* Asset : EditedAssets)
	{
		if (UBlueprint* BP = Cast<UBlueprint>(Asset))
		{
			ActiveBlueprint = BP;
			break;
		}
	}

	if (!ActiveBlueprint)
	{
		UE_LOG(LogTemp, Warning, TEXT("AIBPContextUtils: No Blueprint asset is currently open in an editor."));
		return nullptr;
	}

	return BuildJsonFromBlueprint(ActiveBlueprint);
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> UAIBPContextUtils::BuildJsonFromBlueprint(UBlueprint* Blueprint)
{
	// This is an internal helper — declared inline here since the header only
	// exposes VariableToJson and CollectComponents.
	check(Blueprint);

	TSharedPtr<FJsonObject> RootJson = MakeShared<FJsonObject>();

	// --- Base class ---
	const FString BaseClassName = Blueprint->ParentClass
		? Blueprint->ParentClass->GetName()
		: TEXT("Unknown");
	RootJson->SetStringField(TEXT("baseClass"), BaseClassName);

	// --- Variables ---
	TArray<TSharedPtr<FJsonValue>> VariablesArray;
	for (const FBPVariableDescription& VarDesc : Blueprint->NewVariables)
	{
		TSharedPtr<FJsonObject> VarJson = VariableToJson(VarDesc);
		if (VarJson.IsValid())
		{
			VariablesArray.Add(MakeShared<FJsonValueObject>(VarJson));
		}
	}
	RootJson->SetArrayField(TEXT("variables"), VariablesArray);

	// --- Components (from SimpleConstructionScript) ---
	TArray<TSharedPtr<FJsonValue>> ComponentsArray;
	if (Blueprint->SimpleConstructionScript)
	{
		for (USCS_Node* RootNode : Blueprint->SimpleConstructionScript->GetRootNodes())
		{
			CollectComponents(RootNode, ComponentsArray);
		}
	}
	RootJson->SetArrayField(TEXT("components"), ComponentsArray);

	return RootJson;
}

TSharedPtr<FJsonObject> UAIBPContextUtils::VariableToJson(const FBPVariableDescription& Var)
{
	TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();

	Json->SetStringField(TEXT("name"), Var.VarName.ToString());

	// Derive a human-readable type string from the pin type
	FString TypeStr = Var.VarType.PinCategory.ToString();
	if (Var.VarType.PinSubCategory != NAME_None)
	{
		TypeStr += FString::Printf(TEXT("/%s"), *Var.VarType.PinSubCategory.ToString());
	}
	if (Var.VarType.PinSubCategoryObject.IsValid())
	{
		TypeStr += FString::Printf(TEXT("/%s"), *Var.VarType.PinSubCategoryObject->GetName());
	}
	Json->SetStringField(TEXT("type"), TypeStr);

	return Json;
}

void UAIBPContextUtils::CollectComponents(USCS_Node* Node, TArray<TSharedPtr<FJsonValue>>& OutArray)
{
	if (!Node)
	{
		return;
	}

	TSharedPtr<FJsonObject> CompJson = MakeShared<FJsonObject>();
	CompJson->SetStringField(TEXT("name"), Node->GetVariableName().ToString());

	const FString ClassName = Node->ComponentClass
		? Node->ComponentClass->GetName()
		: TEXT("Unknown");
	CompJson->SetStringField(TEXT("class"), ClassName);

	OutArray.Add(MakeShared<FJsonValueObject>(CompJson));

	// Recurse into child nodes
	for (USCS_Node* Child : Node->GetChildNodes())
	{
		CollectComponents(Child, OutArray);
	}
}

// Provide the missing declaration that was forward-declared in the header
// as a private static but not listed as a member (BuildJsonFromBlueprint).
// Actually we declared it inline above. The header only needs the two listed privates.

#undef LOCTEXT_NAMESPACE
