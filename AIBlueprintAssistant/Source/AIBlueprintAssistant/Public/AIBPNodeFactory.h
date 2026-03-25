// Copyright (c) 2026 AIBlueprintAssistant. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Result of a T3D import operation, including post-import Blueprint compilation status.
 */
struct FAIBPImportResult
{
	/** T3D was parsed and at least one node was imported into the graph. */
	bool bImportSuccess = false;

	/** Blueprint compiled without errors after import (only meaningful when bImportSuccess is true). */
	bool bCompileSuccess = false;

	/**
	 * Human-readable error messages collected from the Blueprint compiler.
	 * Contains one line per erroneous node: "[NodeTitle] ErrorMsg".
	 * Empty when bCompileSuccess is true.
	 */
	FString CompileErrors;
};

/**
 * Core generation class that imports a T3D-encoded graph fragment into the
 * currently active Blueprint graph.
 *
 * All public methods MUST be called on the Game Thread.
 *
 * Undo/Redo support:
 *   The operation is wrapped in an FScopedTransaction; on failure the
 *   transaction is rolled back automatically when the scoped object is
 *   destroyed.  Errors are routed to FMessageLog("AIBlueprintAssistant").
 */
class AIBLUEPRINTASSISTANT_API FAIBPNodeFactory
{
public:
	/**
	 * Parses @p T3DCode and inserts the encoded nodes into the active
	 * Blueprint's EventGraph (or whichever graph is currently focused).
	 * After import the Blueprint is compiled; compilation errors are reported
	 * in the returned FAIBPImportResult so the caller can attempt self-repair.
	 *
	 * @param T3DCode  Raw T3D text that begins with "Begin Object Class=" and
	 *                 ends with "End Object".
	 * @return Result struct describing import and compilation outcome.
	 */
	static FAIBPImportResult ExecuteT3DImport(const FString& T3DCode);

private:
	/** Returns the UEdGraph that should receive the new nodes. */
	static class UEdGraph* GetActiveEdGraph(class UBlueprint*& OutBlueprint);

	/**
	 * Offsets the positions of @p ImportedNodes so they do not overlap with
	 * nodes that existed before the import.
	 */
	static void OffsetImportedNodes(
		const TSet<class UEdGraphNode*>& ImportedNodes,
		int32 ExistingNodeCount);
};
