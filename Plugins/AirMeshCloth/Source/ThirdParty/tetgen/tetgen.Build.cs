using System.IO;
using UnrealBuildTool;

public class tetgen : ModuleRules
{
	public tetgen(TargetInfo Target)
	{
		Type = ModuleType.External;

		string TetgenDir = Path.Combine(ModuleDirectory, "tetgen1.5.0");
		PublicSystemIncludePaths.AddRange(
			new string[] {
					Path.Combine(TetgenDir, "include"),
				}
			);

		if ((Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.Win32))
		{
			string PlatformString = (Target.Platform == UnrealTargetPlatform.Win64) ? "x64" : "x86";
			string LibraryFilename = "tetgen_wrapper.lib";

			PublicAdditionalLibraries.Add(Path.Combine(TetgenDir, "lib", PlatformString, LibraryFilename));
		}
	}
}

