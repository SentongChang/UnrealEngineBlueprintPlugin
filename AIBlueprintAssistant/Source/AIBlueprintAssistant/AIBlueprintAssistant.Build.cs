// Copyright (c) 2026 AIBlueprintAssistant. All Rights Reserved.

using UnrealBuildTool;

public class AIBlueprintAssistant : ModuleRules
{
	public AIBlueprintAssistant(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"Slate",
			"SlateCore",
			"UnrealEd",
			"BlueprintGraph",
			"Kismet",
			"KismetCompiler",
			"GraphEditor",
			"HTTP",
			"Json",
			"JsonUtilities",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"EditorWidgets",      // Replaces deprecated EditorStyle (UE 5.1+)
			"ToolMenus",
			"ApplicationCore",
			"MessageLog",         // FMessageLog category registration
			"DeveloperSettings",  // UAIBPSettings (UDeveloperSettings subclass)
		});
	}
}
