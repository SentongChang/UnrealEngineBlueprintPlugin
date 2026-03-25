// Copyright (c) 2026 AIBlueprintAssistant. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class FToolBarBuilder;
class FMenuBuilder;
class SDockTab;
class FSpawnTabArgs;

/**
 * Main module class for the AIBlueprintAssistant editor plugin.
 * Registers a nomad dockable tab that hosts the Slate UI panel.
 */
class FAIBlueprintAssistantModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** Accessor for the module singleton */
	static FAIBlueprintAssistantModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FAIBlueprintAssistantModule>("AIBlueprintAssistant");
	}

	static bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("AIBlueprintAssistant");
	}

	/** The tab identifier used by FGlobalTabmanager */
	static const FName TabName;

private:
	/** Spawns the dockable tab window containing the Slate UI */
	TSharedRef<SDockTab> SpawnAssistantTab(const FSpawnTabArgs& Args);

	/** Registers the "Window > AI Blueprint Assistant" menu entry */
	void RegisterMenuExtension();
};
