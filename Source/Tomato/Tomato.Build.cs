// Fill out your copyright notice in the Description page of Project Settings.

using System.IO;
using UnrealBuildTool;

public class Tomato : ModuleRules
{
	public Tomato(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"UMG",
			"Slate",
			"SlateCore",
			"EnhancedInput",
			"UltraleapTracking"
		});

		PrivateIncludePaths.AddRange(new string[]
		{
			Path.GetFullPath(Path.Combine(ModuleDirectory, "../../Plugins/UltraleapTracking_ue5_4-5.0.1/Source/ThirdParty/LeapSDK/Include"))
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
