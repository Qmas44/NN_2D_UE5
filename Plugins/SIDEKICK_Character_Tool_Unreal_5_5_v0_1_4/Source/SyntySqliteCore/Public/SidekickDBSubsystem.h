// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SQLiteDatabaseConnection.h"
#include "SKDBTables.h"
#include "EditorSubsystem.h"
#include "SidekickDBSubsystem.generated.h"


/**
 * 
 */
UCLASS()
class SYNTYSQLITECORE_API USidekickDBSubsystem : public UEditorSubsystem // UEditorUtilitySubsystem
{
	GENERATED_BODY()

private:
	FSQLiteDatabaseConnection* SidekickConnDb;
	bool bConnected;
	
public:
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	static FString GetSyntyDbPath();

	UFUNCTION(BlueprintCallable, Category = "Sidekick DB")
	void DatabaseExists(bool& bExists);

	UFUNCTION(BlueprintCallable, Category = "Sidekick DB")
	void OpenDatabase(bool& bSuccess);

	UFUNCTION(BlueprintCallable, Category = "Sidekick DB")
	void CloseDatabase();

	UFUNCTION(BlueprintCallable, Category = "Sidekick DB")
	void ConnectionExists(bool& Exists);

	// UFUNCTION(BlueprintCallable, Category = "Sidekick DB")
	// void ExecuteQuery(const FString& Query, FSQLiteResultSet*& result, bool& bSuccess);

	UFUNCTION(BlueprintCallable, Category = "Sidekick DB")
	void GetSpecies(TMap<FString, FSidekickSpecies>& result, bool& bSuccess);

	UFUNCTION(BlueprintCallable, Category = "Sidekick DB")
	void GetParts(TMap<FString, FSidekickPart>& result, bool& bSuccess);

	UFUNCTION(BlueprintCallable, Category = "Sidekick DB")
	void GetOutfitCollections(TMap<FString, FSidekickCollection>& CollectionMap, bool& bSuccess);

	UFUNCTION(BlueprintCallable, Category = "Sidekick DB")
	void GetAvailableOutfitsForSpecies(int32 SpeciesID, TMap<FString, int32>& AvailableOutfits, bool& bSuccess);

	UFUNCTION(BlueprintCallable, Category = "Sidekick DB")
	void SetPartsToNotExist(bool& bSuccess);

	UFUNCTION(BlueprintCallable, Category = "Sidekick DB")
	void SetPartsToExist(TArray<int32>& PartIDs, bool& bSuccess);

	UFUNCTION(BlueprintCallable, Category = "Sidekick DB")
	void GetSpecieParts(int32 SpeciesID, TArray<FString>& SpecieParts, bool& bSuccess);

	UFUNCTION(BlueprintCallable, Category = "Sidekick DB")
	void GetMorphTargetOffsets(TMap<int32, FMorphTargetPartOffset>& MorphTargetOffsets, bool& bSuccess);
	
	UFUNCTION(BlueprintCallable, Category = "Sidekick DB")
	void GetPartPresets(TArray<FSidekickPartPreset>& Result, bool& bSuccess);

	UFUNCTION(BlueprintCallable, Category = "Sidekick DB")
	void GetAvailablePartPresets(TArray<FSidekickPresetCollection>& Result, bool& bSuccess);
	
	UFUNCTION(BlueprintCallable, Category = "Sidekick DB")
	void GetBodyShapePresets(TMap<FString, FSidekickBodyShapePreset>& Result, bool& bSuccess);
	
	UFUNCTION(BlueprintCallable, Category = "Sidekick DB")
	void GetColorProperties(TMap<FString, FColorProperty>& Result, bool& bSuccess);

	UFUNCTION(BlueprintCallable, Category = "Sidekick DB")
	void GetColorPresets(TMap<int32, FColorPreset>& Result, bool& bSuccess, int32 SpeciesID=-1);

	UFUNCTION(BlueprintCallable, Category = "Sidekick DB")
	void GetColorPresetRows(TMap<int32, FSidekickColorPresetRow>& Result, bool& bSuccess, int32 ColorPresetID=-1);
};
