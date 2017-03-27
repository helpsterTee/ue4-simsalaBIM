// Fill out your copyright notice in the Description page of Project Settings.

using System.IO;

namespace UnrealBuildTool.Rules
{
    public class SimsalaBIM : ModuleRules
    {
        public SimsalaBIM(TargetInfo Target)
        {
            PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "Json", "HTTP", "RuntimeMeshComponent" });
            PrivateDependencyModuleNames.AddRange(new string[] { });

            PrivateIncludePaths.AddRange(new string[] { "SimsalaBIM/Private" });
            PublicIncludePaths.AddRange(new string[] { "SimsalaBIM/Public" });
        }
    }
}
