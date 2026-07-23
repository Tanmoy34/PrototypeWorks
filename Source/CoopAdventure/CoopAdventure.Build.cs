// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CoopAdventure : ModuleRules
{
	public CoopAdventure(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"OnlineSubsystem",
			"NavigationSystem",
			"OnlineSubsystemSteam"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"CoopAdventure",
			"CoopAdventure/Variant_Platforming",
			"CoopAdventure/Variant_Platforming/Animation",
			"CoopAdventure/Variant_Combat",
			"CoopAdventure/Variant_Combat/AI",
			"CoopAdventure/Variant_Combat/Animation",
			"CoopAdventure/Variant_Combat/Gameplay",
			"CoopAdventure/Variant_Combat/Interfaces",
			"CoopAdventure/Variant_Combat/UI",
			"CoopAdventure/Variant_SideScrolling",
			"CoopAdventure/Variant_SideScrolling/AI",
			"CoopAdventure/Variant_SideScrolling/Gameplay",
			"CoopAdventure/Variant_SideScrolling/Interfaces",
			"CoopAdventure/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
