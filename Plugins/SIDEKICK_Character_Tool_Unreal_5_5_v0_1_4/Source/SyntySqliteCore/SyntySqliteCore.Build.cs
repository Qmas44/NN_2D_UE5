using UnrealBuildTool;

public class SyntySqliteCore : ModuleRules
{
    public SyntySqliteCore(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "Blutility",
                "EditorSubsystem"
                
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "UnrealEd",
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "SQLiteCore",
                "SQLiteSupport"
            }
        );
    }
}