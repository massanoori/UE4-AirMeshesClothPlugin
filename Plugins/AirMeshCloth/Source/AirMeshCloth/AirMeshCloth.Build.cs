// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

using System.IO;

public class AirMeshCloth : ModuleRules
{
	public AirMeshCloth(TargetInfo Target)
	{
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"RHI",
			"RenderCore",
			"ShaderCore",
		});

        // Avoid using AGPL software on UE4Game
        if (Target.Type == TargetRules.TargetType.Editor)
        {
            AddEngineThirdPartyPrivateStaticDependencies(Target, "tetgen");
        }
	}
}
