// Copyright (c) 2026 AIBlueprintAssistant. All Rights Reserved.

#include "AIBPContextUtils.h"

#include "Engine/Blueprint.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "EdGraph/EdGraph.h"
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
	UBlueprint* ActiveBlueprint = FindActiveBlueprint();
	if (!ActiveBlueprint)
	{
		return nullptr;
	}

	return BuildJsonFromBlueprint(ActiveBlueprint);
}

TSharedPtr<FJsonObject> UAIBPContextUtils::GetActiveBlueprintGraphData()
{
	UBlueprint* ActiveBlueprint = FindActiveBlueprint();
	if (!ActiveBlueprint)
	{
		return nullptr;
	}

	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("blueprintName"), ActiveBlueprint->GetName());

	TArray<TSharedPtr<FJsonValue>> GraphsArray;

	// Helper lambda: serialise one graph page into a JSON object
	auto SerialiseGraph = [](UEdGraph* Graph) -> TSharedPtr<FJsonObject>
	{
		TSharedPtr<FJsonObject> GraphJson = MakeShared<FJsonObject>();
		GraphJson->SetStringField(TEXT("name"), Graph->GetName());

		TArray<TSharedPtr<FJsonValue>> NodesArray;
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (!Node) { continue; }

			TSharedPtr<FJsonObject> NodeJson = MakeShared<FJsonObject>();
			NodeJson->SetStringField(TEXT("title"),
				Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
			NodeJson->SetStringField(TEXT("class"),
				Node->GetClass()->GetName());
			NodesArray.Add(MakeShared<FJsonValueObject>(NodeJson));
		}
		GraphJson->SetArrayField(TEXT("nodes"), NodesArray);
		return GraphJson;
	};

	// Event graphs (UbergraphPages)
	for (UEdGraph* Graph : ActiveBlueprint->UbergraphPages)
	{
		if (Graph)
		{
			GraphsArray.Add(MakeShared<FJsonValueObject>(SerialiseGraph(Graph)));
		}
	}

	// Custom function graphs
	for (UEdGraph* Graph : ActiveBlueprint->FunctionGraphs)
	{
		if (Graph)
		{
			GraphsArray.Add(MakeShared<FJsonValueObject>(SerialiseGraph(Graph)));
		}
	}

	Root->SetArrayField(TEXT("graphs"), GraphsArray);
	return Root;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

UBlueprint* UAIBPContextUtils::FindActiveBlueprint()
{
	if (!GEditor)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("AIBPContextUtils: GEditor is null — cannot retrieve blueprint context."));
		return nullptr;
	}

	UAssetEditorSubsystem* AssetEditorSubsystem =
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	if (!AssetEditorSubsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("AIBPContextUtils: UAssetEditorSubsystem not available."));
		return nullptr;
	}

	for (UObject* Asset : AssetEditorSubsystem->GetAllEditedAssets())
	{
		if (UBlueprint* BP = Cast<UBlueprint>(Asset))
		{
			return BP;
		}
	}

	UE_LOG(LogTemp, Warning,
		TEXT("AIBPContextUtils: No Blueprint asset is currently open in an editor."));
	return nullptr;
}

TSharedPtr<FJsonObject> UAIBPContextUtils::BuildJsonFromBlueprint(UBlueprint* Blueprint)
{
	check(Blueprint);

	TSharedPtr<FJsonObject> RootJson = MakeShared<FJsonObject>();

	// ---- Base class ----
	const FString BaseClassName = Blueprint->ParentClass
		? Blueprint->ParentClass->GetName()
		: TEXT("Unknown");
	RootJson->SetStringField(TEXT("baseClass"), BaseClassName);

	// ---- Variables ----
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

	// ---- Components (from SimpleConstructionScript) ----
	TArray<TSharedPtr<FJsonValue>> ComponentsArray;
	if (Blueprint->SimpleConstructionScript)
	{
		for (USCS_Node* RootNode : Blueprint->SimpleConstructionScript->GetRootNodes())
		{
			CollectComponents(RootNode, ComponentsArray);
		}
	}
	RootJson->SetArrayField(TEXT("components"), ComponentsArray);

	// ---- Functions (user-created function graphs, excluding EventGraph/Ubergraph) ----
	TArray<TSharedPtr<FJsonValue>> FunctionsArray;
	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		if (Graph)
		{
			FunctionsArray.Add(MakeShared<FJsonValueString>(Graph->GetName()));
		}
	}
	RootJson->SetArrayField(TEXT("functions"), FunctionsArray);

	// ---- Implemented Interfaces ----
	TArray<TSharedPtr<FJsonValue>> InterfacesArray;
	for (const FBPInterfaceDescription& InterfaceDesc : Blueprint->ImplementedInterfaces)
	{
		if (InterfaceDesc.Interface)
		{
			InterfacesArray.Add(
				MakeShared<FJsonValueString>(InterfaceDesc.Interface->GetName()));
		}
	}
	RootJson->SetArrayField(TEXT("interfaces"), InterfacesArray);

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

#undef LOCTEXT_NAMESPACE
