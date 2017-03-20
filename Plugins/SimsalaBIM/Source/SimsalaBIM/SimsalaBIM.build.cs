// Fill out your copyright notice in the Description page of Project Settings.

using System.IO;

namespace UnrealBuildTool.Rules
{
    public class SimsalaBIM : ModuleRules
    {
        public SimsalaBIM(TargetInfo Target)
        {
            PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "Json", "RawMesh", "RenderCore", "HTTP", "UMG", "Slate", "SlateCore" });
            PrivateDependencyModuleNames.AddRange(new string[] { });

            PrivateIncludePaths.AddRange(new string[] { "SimsalaBIM/Private" });
            PublicIncludePaths.AddRange(new string[] { "SimsalaBIM/Public" });

            // Uncomment if you are using Slate UI
            // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

            // Uncomment if you are using online features
            // PrivateDependencyModuleNames.Add("OnlineSubsystem");

            // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
        }
    }
}
