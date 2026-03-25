// Copyright (c) 2026 AIBlueprintAssistant. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "AIBPContextUtils.generated.h"

/**
 * Static utility class that extracts structured context data from the currently
 * active Blueprint editor. The returned JSON is forwarded to the AI API so that
 * it can generate nodes referencing existing variables, components, functions,
 * and interfaces.
 *
 * Returned JSON structure:
 * {
 *   "baseClass":   "Character",
 *   "variables":   [ { "name": "Health", "type": "float" }, ... ],
 *   "components":  [ { "name": "SkeletalMeshComponent0", "class": "SkeletalMeshComponent" }, ... ],
 *   "functions":   [ "Sprint", "TakeDamage_Custom", ... ],
 *   "interfaces":  [ "BPInterface_Damageable", ... ]
 * }
 */
UCLASS()
class AIBLUEPRINTASSISTANT_API UAIBPContextUtils : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Gathers variable, component, function, interface, and parent-class information
	 * from whichever UBlueprint asset is currently open in an asset editor.
	 *
	 * @return A populated JSON object on success, or nullptr when no Blueprint
	 *         editor is open or the active asset is not a UBlueprint.
	 */
	static TSharedPtr<FJsonObject> GetActiveBlueprintData();

	/**
	 * Serialises every node in every graph of the currently active Blueprint.
	 * Covers the EventGraph (UbergraphPages) and all custom function graphs.
	 * Returns nullptr when no Blueprint is open.
	 *
	 * JSON shape:
	 * {
	 *   "blueprintName": "BP_Player",
	 *   "graphs": [
	 *     { "name": "EventGraph",
	 *       "nodes": [ {"title": "Event BeginPlay", "class": "K2Node_Event"}, ... ] },
	 *     { "name": "Sprint",
	 *       "nodes": [ ... ] }
	 *   ]
	 * }
	 */
	static TSharedPtr<FJsonObject> GetActiveBlueprintGraphData();

private:
	/** Builds a complete JSON snapshot from a UBlueprint asset. */
	static TSharedPtr<FJsonObject> BuildJsonFromBlueprint(class UBlueprint* Blueprint);

	/** Serialises a single FBPVariableDescription into a JSON object. */
	static TSharedPtr<FJsonObject> VariableToJson(const struct FBPVariableDescription& Var);

	/** Recursively walks the SimpleConstructionScript and collects component entries. */
	static void CollectComponents(
		class USCS_Node* Node,
		TArray<TSharedPtr<FJsonValue>>& OutArray);
};
