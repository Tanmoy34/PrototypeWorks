// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
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
			"OnlineSubsystemSteam",
			"AudioCaptureCore", // mic capture, for VoiceCommandComponent
			"Json", "JsonUtilities" // parsing Vosk's recognition result
		});

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

		// ---- Vosk (free, offline speech recognition for VoiceCommandComponent) ----
		// No linking here at all anymore - libvosk.dll is loaded and its
		// functions resolved entirely at runtime (see VoiceCommandComponent.cpp
		// LoadVoskDll()), so this module has zero link-time dependency on it.
		// This block only makes sure it ships correctly in packaged builds.
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			// ModuleDirectory = .../Source/CoopAdventure, so ".." x2 gets to the
			// project root, where ThirdParty lives.
			string VoskPath = Path.Combine(ModuleDirectory, "..", "..", "ThirdParty", "Vosk", "vosk-win64-0.3.45");

			string[] RuntimeDlls = {
				"libvosk.dll",
				"libgcc_s_seh-1.dll",
				"libstdc++-6.dll",
				"libwinpthread-1.dll"
			};

			foreach (string Dll in RuntimeDlls)
			{
				string DllPath = Path.Combine(VoskPath, Dll);
				if (File.Exists(DllPath))
				{
					RuntimeDependencies.Add(DllPath);
				}
			}
		}

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
