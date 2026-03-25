// Copyright (c) 2026 AIBlueprintAssistant. All Rights Reserved.

#include "AIBlueprintAssistant.h"
#include "SAIBPAssistantWidget.h"

#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "ToolMenus.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Logging/MessageLog.h"
#include "MessageLogModule.h"

#define LOCTEXT_NAMESPACE "FAIBlueprintAssistantModule"

const FName FAIBlueprintAssistantModule::TabName = TEXT("AIBlueprintAssistant");

void FAIBlueprintAssistantModule::StartupModule()
{
	// Register the FMessageLog category so log entries appear in the Message Log panel
	FMessageLogModule& MessageLogModule =
		FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
	MessageLogModule.RegisterLogListing(
		FName("AIBlueprintAssistant"),
		LOCTEXT("MessageLogLabel", "AI Blueprint Assistant"));

	// Register the nomad tab spawner so the window can be opened from the Window menu
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		TabName,
		FOnSpawnTab::CreateRaw(this, &FAIBlueprintAssistantModule::SpawnAssistantTab))
		.SetDisplayName(LOCTEXT("TabTitle", "AI Blueprint Assistant"))
		.SetTooltipText(LOCTEXT("TabTooltip", "Generate Blueprint nodes from natural language using AI."))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory());

	RegisterMenuExtension();
}

void FAIBlueprintAssistantModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabName);

	// Unregister message log category
	if (FModuleManager::Get().IsModuleLoaded("MessageLog"))
	{
		FMessageLogModule& MessageLogModule =
			FModuleManager::GetModuleChecked<FMessageLogModule>("MessageLog");
		MessageLogModule.UnregisterLogListing(FName("AIBlueprintAssistant"));
	}
}

TSharedRef<SDockTab> FAIBlueprintAssistantModule::SpawnAssistantTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SAIBPAssistantWidget)
		];
}

void FAIBlueprintAssistantModule::RegisterMenuExtension()
{
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateLambda([this]()
		{
			FToolMenuOwnerScoped OwnerScoped(this);

			// Add entry under Window menu
			UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("MainFrame.MainMenu.Window");
			if (Menu)
			{
				FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
				Section.AddMenuEntry(
					"AIBlueprintAssistant",
					LOCTEXT("MenuItemLabel", "AI Blueprint Assistant"),
					LOCTEXT("MenuItemTooltip", "Open the AI Blueprint Assistant panel"),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda([]()
					{
						FGlobalTabmanager::Get()->TryInvokeTab(FAIBlueprintAssistantModule::TabName);
					}))
				);
			}
		}));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAIBlueprintAssistantModule, AIBlueprintAssistant)
