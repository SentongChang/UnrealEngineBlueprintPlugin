// Copyright (c) 2026 AIBlueprintAssistant. All Rights Reserved.

#include "AIBPNodeFactory.h"

#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraphUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"

#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "BlueprintEditor.h"             // FBlueprintEditor

#include "ScopedTransaction.h"
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"

#define LOCTEXT_NAMESPACE "FAIBPNodeFactory"

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

FAIBPImportResult FAIBPNodeFactory::ExecuteT3DImport(const FString& T3DCode)
{
	FAIBPImportResult Result;

	if (T3DCode.IsEmpty())
	{
		FMessageLog("AIBlueprintAssistant").Error(
			LOCTEXT("EmptyT3D", "ExecuteT3DImport: T3D code string is empty."));
		return Result;
	}

	// --- Locate the active graph ---
	UBlueprint* Blueprint = nullptr;
	UEdGraph* Graph = GetActiveEdGraph(Blueprint);

	if (!Graph || !Blueprint)
	{
		FMessageLog("AIBlueprintAssistant").Error(
			LOCTEXT("NoActiveGraph", "ExecuteT3DImport: No active Blueprint graph found. "
									"Please open a Blueprint in the editor before generating nodes."));
		FMessageLog("AIBlueprintAssistant").Open(EMessageSeverity::Error);
		return Result;
	}

	const int32 ExistingNodeCount = Graph->Nodes.Num();

	// --- Begin undoable transaction ---
	// FScopedTransaction auto-rolls back if we call Cancel() or if an exception
	// unwinds the stack before the destructor commits.
	FScopedTransaction Transaction(
		LOCTEXT("TransactionName", "AI Generate Blueprint Nodes"));

	Graph->Modify();
	Blueprint->Modify();

	// --- Import nodes from T3D text ---
	// ImportNodesFromText API changed in UE 5.4:
	//   UE 5.0–5.3: returns TSet<UEdGraphNode*> directly (no out-param overload)
	//   UE 5.4+:    returns void, nodes collected via TSet* out-param
	TSet<UEdGraphNode*> ImportedNodes;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
	FEdGraphUtilities::ImportNodesFromText(Graph, T3DCode, &ImportedNodes);
#else
	ImportedNodes = FEdGraphUtilities::ImportNodesFromText(Graph, T3DCode);
#endif

	if (ImportedNodes.IsEmpty())
	{
		// Roll back the transaction by cancelling it
		Transaction.Cancel();

		FMessageLog("AIBlueprintAssistant").Error(
			LOCTEXT("ImportFailed",
					"ExecuteT3DImport: FEdGraphUtilities::ImportNodesFromText produced no nodes. "
					"The T3D code may be malformed or incompatible with this engine version."));
		FMessageLog("AIBlueprintAssistant").Open(EMessageSeverity::Error);
		return Result;
	}

	// --- Offset nodes to avoid overlap ---
	OffsetImportedNodes(ImportedNodes, ExistingNodeCount);

	// --- Mark the Blueprint dirty and notify the graph ---
	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	Graph->NotifyGraphChanged();

	Result.bImportSuccess = true;

	UE_LOG(LogTemp, Log,
		TEXT("AIBPNodeFactory: Successfully imported %d node(s) into graph '%s'."),
		ImportedNodes.Num(),
		*Graph->GetName());

	// --- Compile the Blueprint and collect any errors ---
	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	const bool bHasError = (Blueprint->Status == EBlueprintStatus::BS_Error ||
	                        Blueprint->Status == EBlueprintStatus::BS_Unknown);

	if (bHasError)
	{
		// Walk all event graph pages and collect per-node error messages
		TArray<FString> ErrorLines;
		for (UEdGraph* Page : Blueprint->UbergraphPages)
		{
			if (!Page) { continue; }
			for (UEdGraphNode* Node : Page->Nodes)
			{
				if (Node && Node->bHasCompilerMessage && !Node->ErrorMsg.IsEmpty())
				{
					ErrorLines.Add(FString::Printf(
						TEXT("[%s] %s"),
						*Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString(),
						*Node->ErrorMsg));
				}
			}
		}

		Result.CompileErrors = ErrorLines.Num() > 0
			? FString::Join(ErrorLines, TEXT("\n"))
			: TEXT("Blueprint compilation failed (no specific node-level errors reported).");

		UE_LOG(LogTemp, Warning,
			TEXT("AIBPNodeFactory: Blueprint '%s' has compile errors after import:\n%s"),
			*Blueprint->GetName(), *Result.CompileErrors);
	}
	else
	{
		Result.bCompileSuccess = true;

		UE_LOG(LogTemp, Log,
			TEXT("AIBPNodeFactory: Blueprint '%s' compiled successfully after import."),
			*Blueprint->GetName());
	}

	return Result;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

UEdGraph* FAIBPNodeFactory::GetActiveEdGraph(UBlueprint*& OutBlueprint)
{
	OutBlueprint = nullptr;

	if (!GEditor)
	{
		return nullptr;
	}

	UAssetEditorSubsystem* AssetEditorSub =
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	if (!AssetEditorSub)
	{
		return nullptr;
	}

	// Find the first open Blueprint asset
	for (UObject* Asset : AssetEditorSub->GetAllEditedAssets())
	{
		UBlueprint* BP = Cast<UBlueprint>(Asset);
		if (!BP)
		{
			continue;
		}

		IAssetEditorInstance* EditorInstance =
			AssetEditorSub->FindEditorForAsset(BP, /*bFocusIfOpen*/ false);
		if (!EditorInstance)
		{
			continue;
		}

		FBlueprintEditor* BPEditor =
			static_cast<FBlueprintEditor*>(EditorInstance);

		// Prefer the currently focused graph; fall back to the event graph.
		UEdGraph* FocusedGraph = BPEditor->GetFocusedGraph();
		if (!FocusedGraph && BP->UbergraphPages.Num() > 0)
		{
			FocusedGraph = BP->UbergraphPages[0];
		}

		if (FocusedGraph)
		{
			OutBlueprint = BP;
			return FocusedGraph;
		}
	}

	return nullptr;
}

void FAIBPNodeFactory::OffsetImportedNodes(
	const TSet<UEdGraphNode*>& ImportedNodes,
	int32 ExistingNodeCount)
{
	// Simple grid-based offset: shift each imported node by a multiple of 300
	// units in X so they appear to the right of existing nodes.
	constexpr float OffsetStep = 300.f;
	const float BaseOffset = static_cast<float>(ExistingNodeCount) * OffsetStep;

	int32 Index = 0;
	for (UEdGraphNode* Node : ImportedNodes)
	{
		if (Node)
		{
			Node->NodePosX += static_cast<int32>(BaseOffset);
			Node->NodePosY += Index * static_cast<int32>(OffsetStep);
			++Index;
		}
	}
}

#undef LOCTEXT_NAMESPACE
