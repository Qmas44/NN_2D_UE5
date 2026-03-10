// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SidekickCharacterTool : ModuleRules
{
	public SidekickCharacterTool(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"Blutility",
				"SKCharacter"
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
				"InputCore",
				"EditorFramework",
				"UnrealEd",
				"ToolMenus",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"SyntySqliteCore",
				"SQLiteCore",
				"SQLiteSupport",
				"ToolWidgets", 
				"AppFramework", 
				"ImageWrapper",
				// Needed for UV Skeletal Mesh query
				"RenderCore",
				"RHI", 
				"ImageWriteQueue", 
				"SkeletalMerging", 
				"ModelingComponentsEditorOnly", 
				"SkeletalMeshModelingTools",
				"SKMerger",
				"DesktopPlatform",
				"HTTP",
				"Json",
				"JsonUtilities"
			}
		);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
