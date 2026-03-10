#include "Misc/AutomationTest.h"
#include "Assets/SParts.h"
#include "Misc/Paths.h"
#include "DataTableUtils.h"
#include "AssetRegistry/AssetData.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(SPartsTest, "Synty.Sidekick.CharacterTool.PartsTest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool SPartsTest::RunTest(const FString& Parameters)
{
	TArray<FAssetData> LocalParts = GetAvailableParts();

	if (LocalParts.Num() == 0)
	{
		AddError(FString::Printf(TEXT("No parts found")));
	}
	AddInfo(FString::Printf(TEXT("Parts found: %d"), LocalParts.Num()));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(SConvertUnityPathTest, "Synty.Sidekick.CharacterTool.UnityPathConversionTest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool SConvertUnityPathTest::RunTest(const FString& Parameters)
{
	FString TestPath = FString("Assets\\Synty\\SidekickCharacters\\Resources\\Meshes\\Outfits\\ApocalypseOutlaws\\SK_APOC_OUTL_01_02HAIR_HU01.fbx");
	ConvertUnityAssetsPathToUnrealPath(&TestPath);

	if (TestPath.Left(4).Equals("Game", ESearchCase::IgnoreCase) )
	{
		return true;
	}

	AddError(FString::Printf(TEXT("Game Not found at the start of the Path: %s"), *TestPath));
	return false;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(SGetSidekickResourcesPathTest, "Synty.Sidekick.CharacterTool.GetSidekickResourcePathTest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool SGetSidekickResourcesPathTest::RunTest(const FString& Parameters)
{
	FString path = GetSidekickResourcesPath();
	
	if (!FPaths::DirectoryExists(path))
	{
		AddError(FString::Printf(TEXT("Path does not exist: %s"), *path));
	}
	else
	{
		AddInfo(FString::Printf(TEXT("Path exists: %s"), *path));
	}
	
	return true;
}