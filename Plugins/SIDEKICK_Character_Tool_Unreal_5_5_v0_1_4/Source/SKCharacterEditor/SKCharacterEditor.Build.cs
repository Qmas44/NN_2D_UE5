using UnrealBuildTool;

public class SKCharacterEditor : ModuleRules
{
    public SKCharacterEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core", 
                "UnrealEd",
                "SidekickCharacterTool"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "AssetDefinition", 
                "AdvancedPreviewScene",
                "SKCharacter",
                "SyntySqliteCore",
                "Projects"
            }
        );
    }
}