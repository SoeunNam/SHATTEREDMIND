// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ShatteredMind : ModuleRules
{
	public ShatteredMind(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",  "GameplayTasks", "AIModule" , "EnhancedInput",// (AI MoveTo/BT 쓰면 권장) , "NavigationSystem"  <-(네비 사용 시 권장) 
    "NavigationSystem" , "UMG" , "Slate", "SlateCore"  });
 

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
