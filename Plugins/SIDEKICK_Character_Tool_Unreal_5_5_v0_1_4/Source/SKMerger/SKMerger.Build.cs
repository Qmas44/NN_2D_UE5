using UnrealBuildTool;

public class SKMerger : ModuleRules
{
    public SKMerger(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore", 
                "SkeletalMerging",
                "SkeletalMeshUtilitiesCommon",
                "AssetTools",
                "RenderCore",
                "RHI",
                "Renderer",
                "UnrealEd",
                "SKCharacter"
            }
        );
    }
}