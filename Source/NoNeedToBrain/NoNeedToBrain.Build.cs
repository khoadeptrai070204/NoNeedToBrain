// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class NoNeedToBrain : ModuleRules
{
	public NoNeedToBrain(ReadOnlyTargetRules Target) : base(Target)
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
			"NavigationSystem",
			"GameplayTasks",
			"SlateCore",
            		"Niagara",
            		"NiagaraCore"

		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"NoNeedToBrain",
			"NoNeedToBrain/Variant_Platforming",
			"NoNeedToBrain/Variant_Platforming/Animation",
			"NoNeedToBrain/Variant_Combat",
			"NoNeedToBrain/Variant_Combat/AI",
			"NoNeedToBrain/Variant_Combat/Animation",
			"NoNeedToBrain/Variant_Combat/Gameplay",
			"NoNeedToBrain/Variant_Combat/Interfaces",
			"NoNeedToBrain/Variant_Combat/UI",
			"NoNeedToBrain/Variant_SideScrolling",
			"NoNeedToBrain/Variant_SideScrolling/AI",
			"NoNeedToBrain/Variant_SideScrolling/Gameplay",
			"NoNeedToBrain/Variant_SideScrolling/Interfaces",
			"NoNeedToBrain/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
