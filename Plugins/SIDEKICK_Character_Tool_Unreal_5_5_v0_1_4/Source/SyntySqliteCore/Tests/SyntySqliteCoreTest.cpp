
#include "SidekickDBSubsystem.h"
#include "CoreMinimal.h"
#include "Editor.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDatabaseExistsTest, "Synty.Sidekick.Database.Exists", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDatabaseExistsTest::RunTest(const FString& Parameters)
{

	USidekickDBSubsystem* MySubsystem = GEditor->GetEditorSubsystem<USidekickDBSubsystem>();
	if (!MySubsystem->IsValidLowLevel())
		return false;
	
	bool Exists = false;
	
	MySubsystem->DatabaseExists(Exists);
	
	if (!Exists)
	{
		AddError(FString::Printf(TEXT("Database file not found.")));
	}
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGetPartsTest, "Synty.Sidekick.Database.GetParts", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGetPartsTest::RunTest(const FString& Parameters)
{

	USidekickDBSubsystem* MySubsystem = GEditor->GetEditorSubsystem<USidekickDBSubsystem>();
	if (!MySubsystem->IsValidLowLevel())
		return false;

	TMap<FString, FSidekickPart> PartsMap;
	bool QuerySuccess = false;
	
	MySubsystem->GetParts(PartsMap, QuerySuccess);
	
	if (!QuerySuccess)
	{
		AddError(FString::Printf(TEXT("Failed to retrieve Parts.")));
	}

	AddInfo(FString::Printf(TEXT("Total Parts Count: %i"), PartsMap.Num()));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGetOutfitsTest, "Synty.Sidekick.Database.GetOutfits", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGetOutfitsTest::RunTest(const FString& Parameters)
{

	USidekickDBSubsystem* MySubsystem = GEditor->GetEditorSubsystem<USidekickDBSubsystem>();
	if (!MySubsystem->IsValidLowLevel())
		return false;

	TMap<FString, FSidekickCollection> CollectionMap;
	bool QuerySuccess = false;
	
	MySubsystem->GetOutfitCollections(CollectionMap, QuerySuccess);
	
	if (!QuerySuccess)
	{
		AddError(FString::Printf(TEXT("Failed to retrieve Outfits.")));
	}

	AddInfo(FString::Printf(TEXT("Total Outfits Count: %i"), CollectionMap.Num()));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGetMorphTargetOffsets, "Synty.Sidekick.Database.GetMorphTargetOffsets", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGetMorphTargetOffsets::RunTest(const FString& Parameters)
{

	USidekickDBSubsystem* MySubsystem = GEditor->GetEditorSubsystem<USidekickDBSubsystem>();
	if (!MySubsystem->IsValidLowLevel())
		return false;

	TMap<int32, FMorphTargetPartOffset> CollectionMap;
	bool QuerySuccess = false;
	
	MySubsystem->GetMorphTargetOffsets(CollectionMap, QuerySuccess);
	
	if (!QuerySuccess)
	{
		AddError(FString::Printf(TEXT("Failed to retrieve Morph Target Part Offsets.")));
	}

	AddInfo(FString::Printf(TEXT("Total Part Offsets retrieved Count: %i"), CollectionMap.Num()));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGetPartPresets, "Synty.Sidekick.Database.GetPartPresets", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGetPartPresets::RunTest(const FString& Parameters)
{

	USidekickDBSubsystem* MySubsystem = GEditor->GetEditorSubsystem<USidekickDBSubsystem>();
	if (!MySubsystem->IsValidLowLevel())
		return false;

	TArray<FSidekickPartPreset> PartPresets;
	bool QuerySuccess = false;
	
	MySubsystem->GetPartPresets(PartPresets, QuerySuccess);
	
	if (!QuerySuccess)
	{
		AddError(FString::Printf(TEXT("Failed to retrieve Part Presets.")));
	}

	AddInfo(FString::Printf(TEXT("Total Part Presets retrieved Count: %i"), PartPresets.Num()));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGetColorPresets, "Synty.Sidekick.Database.GetColorPresets", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGetColorPresets::RunTest(const FString& Parameters)
{

	USidekickDBSubsystem* MySubsystem = GEditor->GetEditorSubsystem<USidekickDBSubsystem>();
	if (!MySubsystem->IsValidLowLevel())
		return false;

	TMap<int32, FColorPreset> ColorPresets;
	bool QuerySuccess = false;
	
	MySubsystem->GetColorPresets(ColorPresets, QuerySuccess);
	
	if (!QuerySuccess)
	{
		AddError(FString::Printf(TEXT("Failed to retrieve Part Presets.")));
	}

	AddInfo(FString::Printf(TEXT("Total Part Presets retrieved Count: %i"), ColorPresets.Num()));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGetColorPresetsBySpecie, "Synty.Sidekick.Database.GetColorPresetsBySpecie", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGetColorPresetsBySpecie::RunTest(const FString& Parameters)
{

	USidekickDBSubsystem* MySubsystem = GEditor->GetEditorSubsystem<USidekickDBSubsystem>();
	if (!MySubsystem->IsValidLowLevel())
		return false;

	TMap<int32, FColorPreset> ColorPresets;
	bool QuerySuccess = false;
	
	MySubsystem->GetColorPresets(ColorPresets, QuerySuccess, 1);
	
	if (!QuerySuccess)
	{
		AddError(FString::Printf(TEXT("Failed to retrieve Part Presets.")));
	}

	AddInfo(FString::Printf(TEXT("Total Part Presets retrieved Count: %i"), ColorPresets.Num()));

	return true;
}

// IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOpenDatabaseTest, "Synty.Sidekick.Database.ConnectsDisconnect", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
//
// bool FOpenDatabaseTest::RunTest(const FString& Parameters)
// {
//
// 	bool Open = false;
// 	FSQLiteDatabaseConnection* conn;
// 	
// 	USidekickSqliteManager::OpenDatabase(conn, Open);
//
// 	USidekickSqliteManager::CloseDatabase(conn);
// 	
// 	if (!Open)
// 	{
// 		AddError(FString::Printf(TEXT("Database could not be opened.")));
// 	}
//
// 	if (conn != nullptr)
// 	{
// 		AddError(FString::Printf(TEXT("Database could not be closed.")));
// 	}
// 	
// 	return true;
// }

// IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExecuteQueryTest, "Synty.Sidekick.Database.Query", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
//
// bool FExecuteQueryTest::RunTest(const FString& Parameters)
// {
//
// 	USidekickDBSubsystem* MySubsystem = GEditor->GetEditorSubsystem<USidekickDBSubsystem>();
//
// 	
// 	// FSQLiteResultSet* result;
// 	
// 	bool QueryStatus = false;
// 	MySubsystem->ExecuteQuery("SELECT * from sk_species", QueryStatus);
//
// 	if (QueryStatus)
// 	{
// 		AddInfo(FString::Printf(TEXT("  Query: Species Records found:")));
// 	}
// 	else if (!QueryStatus)
// 	{
// 		AddError(FString::Printf(TEXT("Failed to execute query.")));
// 	}
// 	
// 	
// 	return true;
// }