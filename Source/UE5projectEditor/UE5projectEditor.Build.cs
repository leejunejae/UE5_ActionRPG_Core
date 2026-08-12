using UnrealBuildTool;

public class UE5projectEditor : ModuleRules
{
	public UE5projectEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"CurveEditor"
		});
	}
}
