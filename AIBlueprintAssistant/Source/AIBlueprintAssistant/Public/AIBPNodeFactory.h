// Copyright (c) 2026 AIBlueprintAssistant. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

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
	 *
	 * @param T3DCode  Raw T3D text that begins with "Begin Object Class=" and
	 *                 ends with "End Object".
	 * @return true on success; false if no Blueprint graph is available or if
	 *         FEdGraphUtilities::ImportNodesFromText fails.
	 */
	static bool ExecuteT3DImport(const FString& T3DCode);

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
