// Fill out your copyright notice in the Description page of Project Settings.

#include "SidekickDBSubsystem.h"
#include "Misc/Paths.h"

void USidekickDBSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	bConnected = false;
	OpenDatabase(bConnected);

	if (!bConnected)
	{
		UE_LOG(LogTemp, Error, TEXT("Database not connected"));
	}
}

void USidekickDBSubsystem::Deinitialize()
{
	CloseDatabase();
	Super::Deinitialize();
}

static FString SYNTY_DB_PATH = "Synty/SidekickCharacters/Database/Synty_Sidekick.db";


/**
 * Opens the Sidekick Database, so it can be queried
 * @return 
 */
void USidekickDBSubsystem::OpenDatabase(bool& bSuccess)
{
	FString dbPath = GetSyntyDbPath();

	if (SidekickConnDb == nullptr)
	{
		SidekickConnDb = new FSQLiteDatabaseConnection();
	}

	DatabaseExists(bSuccess);
	if (!bSuccess)
	{
		UE_LOG(LogTemp, Error, TEXT("File not found: %s"), *dbPath);
		return;
	}
	
	bConnected = SidekickConnDb->Open(*dbPath, nullptr, nullptr);

	bSuccess = bConnected;
}

/**
 * Closes the DB, and sets the global Database Connection to null
 * @return 
 */
void USidekickDBSubsystem::CloseDatabase()
{
	if (SidekickConnDb != nullptr)
	{
		SidekickConnDb->Close();

		FString Error = SidekickConnDb->GetLastError();

		if (Error.IsEmpty())
		{
			// delete SidekickConnDb;
			SidekickConnDb = nullptr;
			bConnected = false;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("%s"), *Error);
		}
	}
}

/**
 * Checks if the DB Connection object has an instance assigned to it
 * @param Exists is true if the connection is not a nullptr
 */
void USidekickDBSubsystem::ConnectionExists(bool& Exists)
{
	Exists = bConnected;
}

/**
 * Gets the Synty Sidekicks Database Path
 * @return Path to the sqlite db file in the project 
 */
FString USidekickDBSubsystem::GetSyntyDbPath()
{
	FString dbProjectPath = FPaths::ProjectContentDir();
	
	return FPaths::Combine(dbProjectPath, SYNTY_DB_PATH);
}

/**
 * Does the Synty Sidekick's SQLite database file exist in the project. 
 * @return True if file exists.
 */
void USidekickDBSubsystem::DatabaseExists(bool& bExists)
{
	bExists = FPaths::FileExists(GetSyntyDbPath());
}

// // Commented out as we will not allow for generic queries to be created and passed.. for now
//
// void USidekickDBSubsystem::ExecuteQuery(const FString& Query, FSQLiteResultSet*& result, bool& bSuccess)
// {
// 	
// 	bSuccess = SidekickConnDb->Execute(*Query, result);
// 	
// 	if (bSuccess)
// 	{
// 		auto columns = result->GetColumnNames();
// 		
// 		UE_LOGFMT(LogTemp, Log, "Column: {0}  {1}  {2}", columns[0].ColumnName, columns[1].ColumnName, columns[2].ColumnName);
//
// 		// Logs every row of the table
// 		for (FSQLiteResultSet::TIterator NameIterator(result); NameIterator; ++NameIterator)
// 		{
// 			UE_LOGFMT(LogTemp, Log, "Column: {0}  {1}  {2}", NameIterator->GetString(*columns[0].ColumnName), NameIterator->GetString(*columns[1].ColumnName), NameIterator->GetString(*columns[2].ColumnName));
// 		}
// 	}
// 	UE_LOG(LogTemp, Verbose, TEXT("This is the query > %s"), *Query);
// }

/**
 * Queries the Species Table in the database for all entries, that have a part in the project.
 * @param result Species data that was returned from the DB, that have a part registered to exist in the project.
 * @param bSuccess Status of the query, if false then the query failed
 */
void USidekickDBSubsystem::GetSpecies(TMap<FString, FSidekickSpecies>& result, bool& bSuccess)
{
	FSQLiteResultSet* QueryResult = nullptr;

	TMap<FString, FSidekickSpecies> SpeciesMap;
	
	FString Query = "SELECT DISTINCT sk_species.name, sk_species.id, sk_species.code FROM sk_part ";
	Query += "LEFT JOIN sk_species ON sk_part.ptr_species = sk_species.id ";
	Query += "WHERE sk_part.file_exists = 1";
	
	bSuccess = SidekickConnDb->Execute(*Query, QueryResult);

	if (!bSuccess) return;

	auto columns = QueryResult->GetColumnNames();
	
	// Logs every row of the table
	for (FSQLiteResultSet::TIterator NameIterator(QueryResult); NameIterator; ++NameIterator)
	{
		FSidekickSpecies* NewSpecies = new FSidekickSpecies();
		NewSpecies->Name = NameIterator->GetString(*columns[0].ColumnName);
		NewSpecies->Id = NameIterator->GetInt(*columns[1].ColumnName);
		NewSpecies->Code = NameIterator->GetString(*columns[2].ColumnName);
		SpeciesMap.Add(NewSpecies->Name, *NewSpecies);
	}

	result = SpeciesMap;
}

/**
 * Retrieves all Parts from the Part DB Table
 * @param result Map of Parts with the key being the part name, as the name should be unique.
 * @param bSuccess if the DB request was successful.
 */
void USidekickDBSubsystem::GetParts(TMap<FString, FSidekickPart>& result, bool& bSuccess)
{
	FSQLiteResultSet* QueryResult = nullptr;

	FString Query = "SELECT * from sk_part";
	
	bSuccess = SidekickConnDb->Execute(*Query, QueryResult);

	if (!bSuccess) return;

	auto columns = QueryResult->GetColumnNames();
	
	// Convert every row of the table
	for (FSQLiteResultSet::TIterator NameIterator(QueryResult); NameIterator; ++NameIterator)
	{
		FSidekickPart* Part = new FSidekickPart();
		Part->Id = NameIterator->GetInt(*columns[0].ColumnName);
		Part->SpeciesId = NameIterator->GetInt(*columns[1].ColumnName);
		Part->TypeID = NameIterator->GetInt(*columns[2].ColumnName);
		Part->PartGroup = NameIterator->GetInt(*columns[3].ColumnName);
		Part->Name = NameIterator->GetString(*columns[4].ColumnName);
		Part->FileName = NameIterator->GetString(*columns[5].ColumnName);
		Part->Path = NameIterator->GetString(*columns[6].ColumnName);
		Part->bUseWrap = NameIterator->GetInt(*columns[7].ColumnName) == 1;
		Part->bExists = NameIterator->GetInt(*columns[8].ColumnName) == 1;
		result.Add(Part->Name, *Part);
	}
}

/**
 * Gets all the Outfit Collections and the Part names that belongs to that collection.
 * 
 * Creates a Map, using the Collection Name as a key and the value is a list of Part names.
 *
*/ 
void USidekickDBSubsystem::GetOutfitCollections(TMap<FString, FSidekickCollection>& CollectionMap, bool& bSuccess)
{
	
	const FString Query = "SELECT sk_part_filter.filter_term, sk_part.name from sk_part_filter "
					      "LEFT JOIN sk_part_filter_row ON sk_part_filter_row.ptr_filter = sk_part_filter.id "
					      "LEFT JOIN sk_part ON sk_part_filter_row.ptr_part = sk_part.id "
						  "WHERE sk_part.file_exists = 1 ";

	FSQLiteResultSet* QueryResult = nullptr;
	bSuccess = SidekickConnDb->Execute(*Query, QueryResult);

	if (!bSuccess) return;

	auto columns = QueryResult->GetColumnNames();
	
	for (FSQLiteResultSet::TIterator NameIterator(QueryResult); NameIterator; ++NameIterator)
	{
		NameIterator->GetString(*columns[0].ColumnName); // Outfit Name
		NameIterator->GetString(*columns[1].ColumnName); // Part Name

		if (CollectionMap.Contains(NameIterator->GetString(*columns[0].ColumnName)))
		{
			CollectionMap[NameIterator->GetString(*columns[0].ColumnName)].Parts.Add(NameIterator->GetString(*columns[1].ColumnName));
		}
		else
		{
			CollectionMap.Add(NameIterator->GetString(*columns[0].ColumnName));
			CollectionMap[NameIterator->GetString(*columns[0].ColumnName)].Parts.Add(NameIterator->GetString(*columns[1].ColumnName));
		}
	}
}

/**
 * Get all Outfits that are available for the specified specie.
 * 
 * @param SpeciesID The Specie ID that you wish to get all the Outfits for
 * @param AvailableOutfits returned outfits for the specified Species
 * @param bSuccess Status of the DB query
 */
void USidekickDBSubsystem::GetAvailableOutfitsForSpecies(int32 SpeciesID, TMap<FString, int32>& AvailableOutfits, bool& bSuccess)
{
	// todo: The SQL statement might need updating to better filter only the outfits available for the species
	const FString Query = FString::Format(TEXT("SELECT DISTINCT filter_term, sk_part_filter.id  from sk_part_filter "
							"LEFT JOIN sk_part_filter_row ON sk_part_filter_row.ptr_filter = sk_part_filter.id "
							"LEFT JOIN sk_part ON sk_part_filter_row.ptr_part = sk_part.id "
							"LEFT JOIN sk_part_species_link ON sk_part.id = sk_part_species_link.ptr_part "
							"LEFT JOIN sk_species ON sk_part_species_link.ptr_species = sk_species.id "
							"WHERE sk_species.id = {0} "
							"AND sk_part.file_exists = 1 "
							"ORDER BY filter_term ASC "), {SpeciesID});

	TMap<FString, int32> SpeciesOutfits;
	
	FSQLiteResultSet* QueryResult = nullptr;
	bSuccess = SidekickConnDb->Execute(*Query, QueryResult);

	if (!bSuccess) return;

	auto columns = QueryResult->GetColumnNames();
	
	for (FSQLiteResultSet::TIterator NameIterator(QueryResult); NameIterator; ++NameIterator)
	{
		NameIterator->GetString(*columns[0].ColumnName); // Outfit Name
		NameIterator->GetInt(*columns[1].ColumnName); // Outfit ID

		if (!SpeciesOutfits.Contains(NameIterator->GetString(*columns[0].ColumnName)))
		{
			SpeciesOutfits.Add(NameIterator->GetString(*columns[0].ColumnName));
		}
		
		SpeciesOutfits[NameIterator->GetString(*columns[0].ColumnName)] = NameIterator->GetInt(*columns[1].ColumnName);
	}

	AvailableOutfits = SpeciesOutfits;
}

/** Sets all Part entries in the Parts table to `not exist` */
void USidekickDBSubsystem::SetPartsToNotExist(bool& bSuccess)
{
	const FString Query = "UPDATE sk_part SET file_exists = 0";

	bSuccess = SidekickConnDb->Execute(*Query);
}

/** Registers the Parts that exist in the current Unreal projects to the Database*/
void USidekickDBSubsystem::SetPartsToExist(TArray<int32>& PartIDs, bool& bSuccess)
{
	const FString QueryHeader = FString(TEXT("UPDATE sk_part "
												 "SET file_exists = 1 "));
	// FString WhenHeader = FString();
	FString WhereHeader = FString("WHERE id IN ( ");
	
	for (auto PartID : PartIDs)
	{
		// WhenHeader += FString::Format(TEXT("WHEN {0} THEN 1 "), {PartID});
		WhereHeader += FString::Format(TEXT("{0}, "), {PartID});
	}
	
	// WhenHeader += "END ";
	WhereHeader = WhereHeader.LeftChop(2) += ") ";
	
	// FString Query = QueryHeader + WhenHeader + WhereHeader;
	FString Query = QueryHeader + WhereHeader;

	bSuccess = SidekickConnDb->Execute(*Query);
}


/** Retrieves all Parts for the specified Specie ID */
void USidekickDBSubsystem::GetSpecieParts(int32 SpeciesID, TArray<FString>& SpecieParts, bool& bSuccess)
{
	FSQLiteResultSet* QueryResult = nullptr;
	
	const FString Query = FString::Format(TEXT(
		"SELECT name " 
		"FROM ( "
		"    SELECT *, MAX(filter_term) FROM sk_part"
		"    LEFT JOIN sk_part_filter_row ON sk_part.id = sk_part_filter_row.ptr_part"
		"    LEFT JOIN sk_part_filter ON sk_part_filter_row.ptr_filter = sk_part_filter.id"
		"    WHERE file_exists = 1 AND ptr_species = {0}"
		"    GROUP BY sk_part.id"
		")"
		"WHERE filter_term IS NULL"
		), {SpeciesID});

	bSuccess = SidekickConnDb->Execute(*Query, QueryResult);

	if (!bSuccess) return;
	
	auto columns = QueryResult->GetColumnNames();
	
	for (FSQLiteResultSet::TIterator NameIterator(QueryResult); NameIterator; ++NameIterator)
	{
		NameIterator->GetString(*columns[0].ColumnName); // Part Name
		
		SpecieParts.Add(NameIterator->GetString(*columns[0].ColumnName));
	}
}

// Retrieves all the Part Offsets for each morph target */
void USidekickDBSubsystem::GetMorphTargetOffsets(TMap<int32, FMorphTargetPartOffset>& MorphTargetOffsets, bool& bSuccess)
{
	FSQLiteResultSet* QueryResult = nullptr;

	// FString Query = "SELECT * from sk_blend_shape_rig_movement_ue";
	FString Query = "SELECT * from sk_blend_shape_rig_movement";
	
	bSuccess = SidekickConnDb->Execute(*Query, QueryResult);

	if (!bSuccess) return;

	auto columns = QueryResult->GetColumnNames();
	
	// Convert every row of the table
	for (FSQLiteResultSet::TIterator NameIterator(QueryResult); NameIterator; ++NameIterator)
	{
		int32 PartType = NameIterator->GetInt(*columns[1].ColumnName);
		int32 BlendType = NameIterator->GetInt(*columns[2].ColumnName);

		// Unity's world space is 100x smaller then Unreals, which is why we need to multiply the values
		float PosX = NameIterator->GetFloat(*columns[3].ColumnName) * 100.f;
		float Posy = NameIterator->GetFloat(*columns[4].ColumnName) * 100.f;
		float PosZ = NameIterator->GetFloat(*columns[5].ColumnName) * 100.f;

		float RotX = NameIterator->GetFloat(*columns[6].ColumnName);
		float Roty = NameIterator->GetFloat(*columns[7].ColumnName);
		float RotZ = NameIterator->GetFloat(*columns[8].ColumnName);

		// Commented out as Scale is not being used
		// float SclX = NameIterator->GetFloat(*columns[9].ColumnName);
		// float Scly = NameIterator->GetFloat(*columns[10].ColumnName);
		// float SclZ = NameIterator->GetFloat(*columns[11].ColumnName);

		FQuat QuatRotation = FQuat::MakeFromEuler({ RotX, Roty, RotZ});

		FTransform PartBlendOffset = FTransform();
		PartBlendOffset.SetTranslation({ PosX, Posy, PosZ });
		PartBlendOffset.SetRotation(QuatRotation);
		PartBlendOffset.SetScale3D({1, 1, 1});
		
		if (!MorphTargetOffsets.Contains(PartType))
		{
			FMorphTargetPartOffset* PartOffsetEntry = new FMorphTargetPartOffset();
			PartOffsetEntry->PartType = PartType;
			
			MorphTargetOffsets.Add(PartOffsetEntry->PartType, *PartOffsetEntry);
		}

		MorphTargetOffsets[PartType].PartBlendOffsets.Add(BlendType, PartBlendOffset);
	}
}


/**
 * Retrieves all Part Colour UDim lookup positions
 * @param Result Map lookup for the udim position in the 32x32 texture.
 * @param bSuccess if the DB request was successful.
 */
void USidekickDBSubsystem::GetColorProperties(TMap<FString, FColorProperty>& Result, bool& bSuccess)
{
	FSQLiteResultSet* QueryResult = nullptr;

	FString Query = "SELECT * from sk_color_property";
	
	bSuccess = SidekickConnDb->Execute(*Query, QueryResult);

	if (!bSuccess) return;

	auto columns = QueryResult->GetColumnNames();
	
	// Convert every row of the table
	for (FSQLiteResultSet::TIterator NameIterator(QueryResult); NameIterator; ++NameIterator)
	{
		FColorProperty* PartColor = new FColorProperty();
		PartColor->Id = NameIterator->GetInt(*columns[0].ColumnName);
		PartColor->ColorGroup = NameIterator->GetInt(*columns[1].ColumnName);
		PartColor->Name = NameIterator->GetString(*columns[2].ColumnName);
		PartColor->U = NameIterator->GetInt(*columns[3].ColumnName);
		PartColor->V = NameIterator->GetInt(*columns[4].ColumnName);
		Result.Add(PartColor->Name, *PartColor);
	}
}

/**
 * Retrives the Colour Preset entries from the DB and groups them by Species Name 
 * 
 * @param Result The resulting data from the query. With the name of the 
 * @param bSuccess if the DB query was successful
 * @param SpeciesID Optional filter input, returns only the specified entries for the specific Specie
 */
void USidekickDBSubsystem::GetColorPresets(TMap<int32, FColorPreset>& Result, bool& bSuccess, int32 SpeciesID)
{
	FSQLiteResultSet* QueryResult = nullptr;

	FString Query = "SELECT sk_color_preset.*, sk_species.name as SpeciesName FROM sk_color_preset "
					"LEFT JOIN sk_species ON sk_color_preset.ptr_species = sk_species.id ";

	if (SpeciesID >= 0)
	{
		Query += " WHERE sk_color_preset.ptr_species = " + FString::FromInt(SpeciesID);
	}
	
	bSuccess = SidekickConnDb->Execute(*Query, QueryResult);

	if (!bSuccess) return;

	auto columns = QueryResult->GetColumnNames();

	// Convert every row of the table
	for (FSQLiteResultSet::TIterator NameIterator(QueryResult); NameIterator; ++NameIterator)
	{
		FColorPreset* PartColor = new FColorPreset(); // todo: this needs to be handled better as it is heap alocated with `new` which means we need to handle its cleanup 
		PartColor->Id = NameIterator->GetInt(*columns[0].ColumnName);
		PartColor->SpeciesID = NameIterator->GetInt(*columns[1].ColumnName);
		PartColor->ColorGroup = NameIterator->GetInt(*columns[2].ColumnName);
		PartColor->Name = NameIterator->GetString(*columns[3].ColumnName);
		PartColor->SpeciesName = NameIterator->GetString(*columns[4].ColumnName);

		Result.Add(PartColor->Id, *PartColor);
	}
}

void USidekickDBSubsystem::GetColorPresetRows(TMap<int32, FSidekickColorPresetRow>& Result, bool& bSuccess, int32 ColorPresetID)
{
	FSQLiteResultSet* QueryResult = nullptr;

	FString Query = "SELECT sk_color_preset_row.*, sk_color_property.name FROM sk_color_preset_row "
					"LEFT JOIN sk_color_property ON sk_color_preset_row.ptr_color_property = sk_color_property.id";

	if (ColorPresetID >= 0)
	{
		Query += " WHERE sk_color_preset_row.ptr_color_preset = " + FString::FromInt(ColorPresetID);
	}
	
	bSuccess = SidekickConnDb->Execute(*Query, QueryResult);

	if (!bSuccess) return;

	auto columns = QueryResult->GetColumnNames();

	// Convert every row of the table
	for (FSQLiteResultSet::TIterator NameIterator(QueryResult); NameIterator; ++NameIterator)
	{
		FSidekickColorPresetRow* ColorPresetRow = new FSidekickColorPresetRow(); 
		ColorPresetRow->Id = NameIterator->GetInt(*columns[0].ColumnName);
		ColorPresetRow->ColorPresetID = NameIterator->GetInt(*columns[1].ColumnName);
		ColorPresetRow->ColorPropertyID = NameIterator->GetInt(*columns[2].ColumnName);
		ColorPresetRow->Color = NameIterator->GetString(*columns[3].ColumnName);
		ColorPresetRow->Metallic = NameIterator->GetString(*columns[4].ColumnName);
		ColorPresetRow->Smoothness = NameIterator->GetString(*columns[5].ColumnName);
		ColorPresetRow->Reflection = NameIterator->GetString(*columns[6].ColumnName);
		ColorPresetRow->Emission = NameIterator->GetString(*columns[7].ColumnName);
		ColorPresetRow->Opacity = NameIterator->GetString(*columns[8].ColumnName);
		ColorPresetRow->Name = NameIterator->GetString(*columns[9].ColumnName);

		Result.Add(ColorPresetRow->Id, *ColorPresetRow);
	}
}

void USidekickDBSubsystem::GetBodyShapePresets(TMap<FString, FSidekickBodyShapePreset>& Result, bool& bSuccess)
{
	FSQLiteResultSet* QueryResult = nullptr;
	
	FString Query = "SELECT * FROM sk_body_shape_preset";

	bSuccess = SidekickConnDb->Execute(*Query, QueryResult);

	if (!bSuccess) return;

	auto columns = QueryResult->GetColumnNames();
	
	// Convert every row of the table
	for (FSQLiteResultSet::TIterator NameIterator(QueryResult); NameIterator; ++NameIterator)
	{
		FSidekickBodyShapePreset* PartPreset = new FSidekickBodyShapePreset();
		PartPreset->Id = NameIterator->GetInt(*columns[0].ColumnName);
		PartPreset->Name = NameIterator->GetString(*columns[1].ColumnName);
		PartPreset->BodyType = NameIterator->GetInt(*columns[2].ColumnName);
		PartPreset->BodySize = NameIterator->GetInt(*columns[3].ColumnName);
		PartPreset->Musculature = NameIterator->GetInt(*columns[4].ColumnName);
		Result.Add(PartPreset->Name, *PartPreset);
	}
};

/**
 * Gets a list of available `Presets Collections` and the `Preset Groups` that contain all their parts.
 *
 *	Preset Collection = `Apocalypse - Outlaws`, `Fantasy Knight`
 *	Preset Group = `Apocalypse Outlaws 01`, `Apocalypse Outlaws 02`...
 *
 * @note If a Preset group requires a part, that is not available then the group will be filtered out.
 * @param bSuccess 
 */
void USidekickDBSubsystem::GetAvailablePartPresets(TArray<FSidekickPresetCollection>& Result, bool& bSuccess)
{
	FSQLiteResultSet* QueryResult = nullptr;
	
	FString Query = "SELECT sk_species.name as SpeciesName, filter_term as PresetCollection, sk_part_preset.part_group, sk_part_preset.name as PresetName "
					"FROM sk_part_preset "
					"LEFT JOIN sk_part_preset_row ON sk_part_preset_row.ptr_part_preset = sk_part_preset.id "
					"LEFT JOIN sk_part ON sk_part.name = part_name "
					"LEFT JOIN sk_preset_filter_row ON sk_part_preset.id = sk_preset_filter_row.ptr_preset " 
					"LEFT JOIN sk_preset_filter ON sk_preset_filter.id = sk_preset_filter_row.ptr_filter "
					"LEFT JOIN sk_species ON sk_species.id = sk_part_preset.ptr_species "
					"WHERE part_name IS NOT NULL "
					"GROUP BY sk_part_preset.id "
					"HAVING COUNT(*) = SUM(CASE WHEN file_exists = 1 THEN 1 ELSE 0 END)";

	bSuccess = SidekickConnDb->Execute(*Query, QueryResult);

	if (!bSuccess) return;

	auto columns = QueryResult->GetColumnNames();
	
	// Convert every row of the table
	for (FSQLiteResultSet::TIterator NameIterator(QueryResult); NameIterator; ++NameIterator)
	{
		FSidekickPresetCollection* PartPreset = new FSidekickPresetCollection();
		PartPreset->SpeciesName = NameIterator->GetString(*columns[0].ColumnName);
		PartPreset->PresetCollection = NameIterator->GetString(*columns[1].ColumnName);
		PartPreset->PartGroup = NameIterator->GetInt(*columns[2].ColumnName);
		PartPreset->PresetName = NameIterator->GetString(*columns[3].ColumnName);
		Result.Add(*PartPreset);
	}
}

/**
 * Gets a list of all Preset Collections, Presets and the Parts that make up those presets.
 * It filters out parts that do not exist.
 *
 * This results in Preset collections that might contain groups where not all parts are available,
 * and in worse case scenario only 1 part might be available.
 * 
 * @param Result 
 * @param bSuccess 
 */
void USidekickDBSubsystem::GetPartPresets(TArray<FSidekickPartPreset>& Result, bool& bSuccess)
{
	FSQLiteResultSet* QueryResult = nullptr;
	
	FString Query = "SELECT sk_species.name as SpeciesName, filter_term, sk_part_preset.part_group, sk_part_preset.name, part_name, sk_part.type "
					"FROM sk_part_preset "
					"LEFT JOIN sk_preset_filter_row ON sk_part_preset.id = sk_preset_filter_row.ptr_preset "
					"LEFT JOIN sk_preset_filter ON sk_preset_filter_row.ptr_filter = sk_preset_filter.id "
					"LEFT JOIN sk_part_preset_row ON sk_part_preset_row.ptr_part_preset = sk_part_preset.id "
					"LEFT JOIN sk_part ON sk_part.name = part_name "
					"LEFT JOIN sk_species ON sk_species.id = sk_part_preset.ptr_species "
					"WHERE part_name IS NOT NULL AND file_exists = 1";
	
	bSuccess = SidekickConnDb->Execute(*Query, QueryResult);

	if (!bSuccess) return;

	auto columns = QueryResult->GetColumnNames();
	
	// Convert every row of the table
	for (FSQLiteResultSet::TIterator NameIterator(QueryResult); NameIterator; ++NameIterator)
	{
		FSidekickPartPreset* PartPreset = new FSidekickPartPreset();
		PartPreset->SpeciesName = NameIterator->GetString(*columns[0].ColumnName);
		PartPreset->PresetCollection = NameIterator->GetString(*columns[1].ColumnName);
		PartPreset->PartGroup = NameIterator->GetInt(*columns[2].ColumnName);
		PartPreset->PresetName = NameIterator->GetString(*columns[3].ColumnName);
		PartPreset->PartName = NameIterator->GetString(*columns[4].ColumnName);
		PartPreset->PartType = NameIterator->GetInt(*columns[5].ColumnName);
		Result.Add(*PartPreset);
	}
};