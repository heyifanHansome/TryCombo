// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TryComboWepon : ModuleRules
{
	public TryComboWepon(ReadOnlyTargetRules Target) : base(Target)
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
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"TryComboWepon",
			"TryComboWepon/Variant_Platforming",
			"TryComboWepon/Variant_Platforming/Animation",
			"TryComboWepon/Variant_Combat",
			"TryComboWepon/Variant_Combat/AI",
			"TryComboWepon/Variant_Combat/Animation",
			"TryComboWepon/Variant_Combat/Gameplay",
			"TryComboWepon/Variant_Combat/Interfaces",
			"TryComboWepon/Variant_Combat/UI",
			"TryComboWepon/Variant_SideScrolling",
			"TryComboWepon/Variant_SideScrolling/AI",
			"TryComboWepon/Variant_SideScrolling/Gameplay",
			"TryComboWepon/Variant_SideScrolling/Interfaces",
			"TryComboWepon/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
