#include "Models/MainMenuModel.h"

#include "CoreMinimal.h"
#include "AsyncImageExport.h"
#include "ParticleHelper.h"
#include "SidekickDBSubsystem.h"
#include "Algo/ForEach.h"
#include "Assets/SParts.h"
#include "Enums/ColorGroup.h"
#include "Assets/FSKCharacterFile.h"
#include "Assets/FSKTexture.h"
#include "AssetRegistry/AssetData.h"
#include "Editor.h"
#include "DataTableUtils.h"

MainMenuModel::MainMenuModel()
{
	AvailableSpecies = TArray<TSharedPtr<FString>>();
	AvailableOutfits = TArray<TSharedPtr<FString>>();
	AvailableParts = MakeShared<TMap<FString, FAssetData>>();
	CollectionToParts = MakeShared<TMap<FString, FSidekickCollection>>();
	AvailablePartColourSwatches = TMap<FString, TSet<FIntPoint>>();
	AvailableBodyPresets = TArray<TSharedPtr<FString>>();

	// Check DB exists, test connection is working
	if (DatabaseExists())
	{
		DatabaseConnected();
	}
}

MainMenuModel::~MainMenuModel()
{
	CharacterPreset = nullptr;

	AvailableSpecies.Empty();
	AvailableOutfits.Empty();
	AvailableParts->Empty();
	CollectionToParts->Empty();
	AvailablePartColourSwatches.Empty();
	AvailableBodyPresets.Empty();

	// Close the DB after use.
	USidekickDBSubsystem* Subsystem = GEditor->GetEditorSubsystem<USidekickDBSubsystem>();
	if (Subsystem != nullptr)
	{
		Subsystem->CloseDatabase();
	}
	
}


void MainMenuModel::Initialize()
{
	// Gets all parts that exist in the project
	GetAvailableParts(AvailableParts.Get());

	// Refresh DB Parts Table, with parts that are available in current project.	
	UpdateDBPartAvailability();

	GetAvailableSpecies();

	GetMorphTargetPartOffsets();

	// Sets the activated selection to be the first available specie
	if (AvailableSpecies.IsEmpty())
	{
		return;
	}

	SelectedSpecie = AvailableSpecies[0];
	
	GetOutfitParts();
	GetPresetData();
	GetBodyShapePresets();
	GetColorPresets();
	GetColourPresetRows();
	
	PostActiveSpecieUpdate();
	// Creates Collections of Parts for the dropdown UI
	SetupGroupedPartDataStructure();
}

void MainMenuModel:: GetAvailableSpecies()
{
	AvailableSpecies.Empty();

	if (DatabaseExists())
	{
		USidekickDBSubsystem* Subsystem = GEditor->GetEditorSubsystem<USidekickDBSubsystem>();
		
		bool QueryStatus = false;
		
		TArray<FString> LocalSpeciesKeys;
		Subsystem->GetSpecies(SpeciesLookup, QueryStatus);

		if (!QueryStatus)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed querying database for Species."));
			return;
		}
		
		SpeciesLookup.GetKeys(LocalSpeciesKeys);
		
		for (const FString& Key : LocalSpeciesKeys)
		{
			AvailableSpecies.Add(MakeShared<FString>(Key));
		}
	}
}

void MainMenuModel::GetOutfits()
{
	AvailableOutfits.Empty();
	
	if (SpeciesLookup.Num() <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Species lookup is empty"));
		return;
	}
	
	int32 SpeciesID = SpeciesLookup[*SelectedSpecie].Id;

	TMap<FString, int32> ReturnedOutfits;
	
	if (GEditor)
	{
		USidekickDBSubsystem* Subsystem = GEditor->GetEditorSubsystem<USidekickDBSubsystem>();
		if (Subsystem)
		{
			bool QuerySuccess = false;
			Subsystem->DatabaseExists(QuerySuccess);
			if (!QuerySuccess) return;
			
			Subsystem->GetAvailableOutfitsForSpecies(SpeciesID, ReturnedOutfits,QuerySuccess);

			if (!QuerySuccess)
			{
				UE_LOG(LogDataTable, Error, TEXT("Failed to get all available Outfits for SpecieID: %i"), SpeciesID);
				return;
			}
			
			TArray<FString> LocalOutfitKeys;
			ReturnedOutfits.GetKeys(LocalOutfitKeys);
			
			for (const FString& OutfitName : LocalOutfitKeys)
			{
				AvailableOutfits.Add(MakeShared<FString>(OutfitName));
			}
			
		}
	}
}

void MainMenuModel::GetPresetData()
{

	TMap<int32, FString> TempPresetFilterByID;
	TMap<int32, FString> TempSpeciesByID;
	TMap<FString, TSet<FString>> SpeciesToFilterNameSet;
	
	if (GEditor)
	{
		USidekickDBSubsystem* Subsystem = GEditor->GetEditorSubsystem<USidekickDBSubsystem>();
		if (Subsystem)
		{
			bool QuerySuccess = false;
			Subsystem->DatabaseExists(QuerySuccess);
			if (!QuerySuccess) return;
			
			Subsystem->GetPartPresets(PartPresets, QuerySuccess);
			Subsystem->GetAvailablePartPresets(UniquePartPresets, QuerySuccess);

			for (auto CompletePresetEntry : UniquePartPresets)
			{
				// Create a unique list of Species to filter group
				auto& CollectionNameSet = SpeciesToFilterNameSet.FindOrAdd(CompletePresetEntry.SpeciesName);

				FString PresetCollection = CompletePresetEntry.PresetCollection;

				if (PresetCollection.IsEmpty())
				{
					PresetCollection = "Generic Specie";
				}
				
				CollectionNameSet.Add(PresetCollection);

				PresetPartLookup.FindOrAdd(CompletePresetEntry.SpeciesName);
				PresetPartLookup[CompletePresetEntry.SpeciesName].FindOrAdd(PresetCollection);
				PresetPartLookup[CompletePresetEntry.SpeciesName][PresetCollection].FindOrAdd(CompletePresetEntry.PartGroup);

				auto PartData = PresetPartLookup[CompletePresetEntry.SpeciesName][PresetCollection].Find(CompletePresetEntry.PartGroup);
				if (PartData == nullptr) continue;
				if (!PartData->Contains(CompletePresetEntry.PresetName))
				{
					PartData->Add(CompletePresetEntry.PresetName);
				}
			}
			
			for (auto PresetFilterEntry : PartPresets)
			{
				// Construct the Parts for a Preset Part collection
				PresetToParts.FindOrAdd(PresetFilterEntry.PartGroup);
				PresetToParts[PresetFilterEntry.PartGroup].FindOrAdd(PresetFilterEntry.PresetName);
				PresetToParts[PresetFilterEntry.PartGroup][PresetFilterEntry.PresetName].FindOrAdd(PresetFilterEntry.PartType, PresetFilterEntry.PartName);
			}
		}
	}
	
	// Converts the Set of species to Filter Names into a list for the UI
	for (auto PresetFilterByID : SpeciesToFilterNameSet)
	{
		SpeciesToPresetFilter.FindOrAdd(PresetFilterByID.Key);
		SpeciesToPresetFilter[PresetFilterByID.Key] = PresetFilterByID.Value.Array();
	}
}


void MainMenuModel::GetBodyShapePresets()
{
	if (GEditor)
	{
		USidekickDBSubsystem* Subsystem = GEditor->GetEditorSubsystem<USidekickDBSubsystem>();
		if (Subsystem)
		{
			bool QuerySuccess = false;
			
			Subsystem->DatabaseExists(QuerySuccess);
			if (!QuerySuccess) return;
			
			Subsystem->GetBodyShapePresets(BodyShapePresetLookup, QuerySuccess);

			if (!QuerySuccess)
			{
				UE_LOG(LogDataTable, Error, TEXT("Failed to get body shape presets"));
				return;
			}

			// Generates the list of available body preset names
			
			TArray<FString> ShapePresetKeys;
			BodyShapePresetLookup.GetKeys(ShapePresetKeys);
			ShapePresetKeys.Sort();
			
			for (const FString& PresetName : ShapePresetKeys)
			{
				AvailableBodyPresets.Add(MakeShared<FString>(PresetName));
			}
			
		}
	}
}

void MainMenuModel::GetColorPresets(int32 SpeciesID)
{
	if (GEditor)
	{
		USidekickDBSubsystem* Subsystem = GEditor->GetEditorSubsystem<USidekickDBSubsystem>();
		if (Subsystem)
		{
			bool QuerySuccess = false;
			
			Subsystem->DatabaseExists(QuerySuccess);
			if (!QuerySuccess) return;
			
			Subsystem->GetColorPresets(ColorPresetLookup,QuerySuccess, SpeciesID);
			
			if (!QuerySuccess)
			{
				UE_LOG(LogDataTable, Error, TEXT("Failed to get body shape presets"));
				return;
			}

			for (auto ColorPresetRow : ColorPresetLookup)
			{
				FColorPreset& PresetEntry = ColorPresetRow.Value;
				TMap<FString, TArray<TSharedPtr<FString>>>& ColourGroup = AvailableColorPresets.FindOrAdd(PresetEntry.SpeciesName);
				TMap<FString, TArray<FColorPreset*>>& LookupDbRow = ColorPresetsToDBTable.FindOrAdd(PresetEntry.SpeciesName);
				
				FString ColourGroupName = ColorGroupFromIntToString(PresetEntry.ColorGroup);
				
				// Add empty colour groups for each specie. This is required for UI dropdown simplification.
				for (int i = 1; i < static_cast<int>(EColorGroup::Max); i++)
				{
					EColorGroup ColourEnum = static_cast<EColorGroup>(i);
					ColourGroup.FindOrAdd(ColorGroupToString(ColourEnum));
				}
				
				if (ColourGroupName.IsEmpty()) continue;
				TArray<TSharedPtr<FString>>& ColourPresetNames = ColourGroup.FindOrAdd(ColourGroupName);
				TArray<FColorPreset*>& ColourPresetNamesDBEntries = LookupDbRow.FindOrAdd(ColourGroupName);

				ColourPresetNames.Add(MakeShared<FString>(PresetEntry.Name));
				ColourPresetNamesDBEntries.Add(&ColorPresetLookup[ColorPresetRow.Key]);
			}

			
		}
	}
}

void MainMenuModel::GetColourPresetRows(int32 ColorPresetID)
{
	TMap<int32, FSidekickColorPresetRow> DbResults;
	
	bool QuerySuccess = false;
	
	if (DatabaseExists())
	{
		USidekickDBSubsystem* Subsystem = GEditor->GetEditorSubsystem<USidekickDBSubsystem>();
		
		Subsystem->GetColorPresetRows(DbResults,QuerySuccess, ColorPresetID);
		
		if (!QuerySuccess)
		{
			ColorPresetRowLookup.Empty();
			UE_LOG(LogDataTable, Error, TEXT("Failed to get Color Presets"));
			return;
		}

		// Formats the result into a more user-friendly data structure

		for (auto ColorPresetRow : DbResults)
		{
			int32 PresetID = ColorPresetRow.Value.ColorPresetID;

			TArray<FSidekickColorPresetRow>& PresetList = ColorPresetRowLookup.FindOrAdd(PresetID);
			PresetList.Add(DbResults[ColorPresetRow.Key]);
		}
	}
}


void MainMenuModel::UpdateOutfits()
{
	GetOutfits();
	RefreshActiveOutfits();
}

/**
 * Clears and repopulates the active outfit list
 */
void MainMenuModel::RefreshActiveOutfits()
{
	SelectedOutfitKeys.Empty();

	for (auto OutfitName : AvailableOutfits)
	{
		SelectedOutfitKeys.Add(OutfitName);
	}
}

/**
 * Updates the Parts Table in the DB with parts that exist in the project.
 *
 * That way queries to the DB know exactly which Parts or collections to return.
 * @param AvailableParts 
 */
void MainMenuModel::UpdateDBPartAvailability()
{
	bool DBSuccess = false;

	if (!GEditor) return;
	if (!DatabaseExists())
	{
		return;
	}
	USidekickDBSubsystem* Subsystem = GEditor->GetEditorSubsystem<USidekickDBSubsystem>();
	
	Subsystem->SetPartsToNotExist(DBSuccess);
	if (!DBSuccess)
	{
		UE_LOG(LogDataTable, Error, TEXT("Could not Update Parts Table, to clear all exist entries."));
		return;
	}
	
	// Gets all the Part data from the DB
	Subsystem->GetParts(PartsLookup, DBSuccess);

	// Creates a list of all Part IDs that have been found in the project.
	TArray<int32> FoundPartIDs;
	TArray<FString> PartNames;
	AvailableParts->GetKeys(PartNames);
	
	for (FString PartName : PartNames)
	{
		if (!PartsLookup.Contains(PartName)) continue;

		int32 PartID = PartsLookup[PartName].Id;

		// Updates the already retrieved Parts DB table
		PartsLookup[PartName].bExists = true;
		
		FoundPartIDs.Add(PartID);
	}

	Subsystem->SetPartsToExist(FoundPartIDs, DBSuccess);

	UE_LOG(LogTemp, Display, TEXT("MainMenuModel::UpdateDBPartAvailability : %s"), DBSuccess ? TEXT("True") : TEXT("False"));
}

/**
 * Creates collections of parts that will be used by the dropdown menus
 */
void MainMenuModel::SetupGroupedPartDataStructure()
{
	if (!CollectionToParts.IsValid() || CollectionToParts->IsEmpty())
		return;
	
	if (!AvailablePartsDropDowns.IsValid())
	{
		AvailablePartsDropDowns = MakeShared<TMap<ECharacterPartType, TArray<FString>>>();
	}

	AvailablePartsDropDowns->Empty();

	// Add Generic Specie parts if they exist
	if (CollectionToParts->Contains(*SelectedSpecie))
	{
		auto SpecieParts = (*CollectionToParts)[*SelectedSpecie].Parts;
		AddPartsToDropdown(SpecieParts);
	}

	for (auto OutfitKey : SelectedOutfitKeys)
	{
		FString Outfit = *OutfitKey;
		if (CollectionToParts->Contains(Outfit))
		{
			auto OutfitParts = (*CollectionToParts)[Outfit].Parts;
			AddPartsToDropdown(OutfitParts);
		}
	}
	
}

void MainMenuModel::AddPartsToDropdown(TArray<FString>& Parts)
{
	for (auto PartName : Parts)
	{
		if (!PartsLookup.Contains(PartName))
			continue;
		
		FSidekickPart PartDBEntry = PartsLookup[PartName];

		ECharacterPartType PartGroup = static_cast<ECharacterPartType>(PartDBEntry.TypeID);
		
		if (!AvailablePartsDropDowns->Contains(PartGroup))
		{
			// Creates empty group if it doesn't contain one
			AvailablePartsDropDowns->Add(PartGroup, TArray<FString>());
		}
		(*AvailablePartsDropDowns)[PartGroup].Add(PartName);
	}
}

/**
 * Get all the parts for each outfit from the DB.
 * 
 * Stores them in a TMap for easy lookup.
 */
void MainMenuModel::GetOutfitParts()
{
	bool DBSuccess = false;

	CollectionToParts->Empty();
	
	if (!DatabaseExists())
	{
		return;
	}
	
	USidekickDBSubsystem* DBSubsystem = GEditor->GetEditorSubsystem<USidekickDBSubsystem>();
	
	// Get Parts for the active Specie and its Active Outfits
	DBSubsystem->GetOutfitCollections(*CollectionToParts.ToSharedRef(), DBSuccess);
}


void MainMenuModel::GetActiveSpecieParts()
{
	bool DBSuccess = false;

	if (!DatabaseExists())
	{
		return;
	}
	
	USidekickDBSubsystem* DBSubsystem = GEditor->GetEditorSubsystem<USidekickDBSubsystem>();

	if (!SelectedSpecie.IsValid())
	{
		if (CollectionToParts.IsValid())
		{
			CollectionToParts->Empty();
		}
		return;
	}
	
	int32 SpeciesID = SpeciesLookup[*SelectedSpecie].Id;

	TArray<FString> SpecieParts = TArray<FString>();
	
	DBSubsystem->GetSpecieParts(SpeciesID, SpecieParts, DBSuccess);

	if (!DBSuccess)
	{
		return;
	}

	/* Adds the Species Parts to the Part Collection group. This allows for the Specie's name to be passed into the
	 * Collection lookup and return all the part names that relate to the species. Keeping all lookups in one location.
	*/ 
	if (!CollectionToParts.IsValid())
	{
		return;
	}
	
	FSidekickCollection& SidekickArray = (*CollectionToParts).FindOrAdd(*SelectedSpecie);

	SidekickArray.Parts.Empty();
	SidekickArray.Parts.Append(SpecieParts);
}

/**
 * Retrieves the available part using the part name
 */
USkeletalMesh* MainMenuModel::GetPartByName(FString PartName)
{
	// As dropdowns can be Empty or None, we need to filter these out.
	if (PartName == "None" || PartName == "Empty"|| PartName.IsEmpty()) return nullptr;

	if (!(*AvailableParts).Contains(PartName))
	{
		UE_LOG(LogTemp, Error, TEXT("PartName %s not found"), *PartName);
		return nullptr;
	}
	
	FAssetData PartData = (*AvailableParts)[PartName];
	USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(PartData.GetAsset());

	if (SkeletalMesh)
	{
		return SkeletalMesh;	
	}
	
	return nullptr;
}

/**
 * Gets All Morph Target Part Offset data from the DB and stores it in an easy to look up TMap.
 */
void MainMenuModel::GetMorphTargetPartOffsets()
{
	MorphTargetOffsetLookup.Empty();
	
	TMap<int32, FMorphTargetPartOffset> DbMtPartOffsets;

	if (DatabaseExists())
	{
		USidekickDBSubsystem* Subsystem = GEditor->GetEditorSubsystem<USidekickDBSubsystem>();
		
		bool DBExists = false;
		
		Subsystem->GetMorphTargetOffsets(DbMtPartOffsets, DBExists);

		// todo: Convert Maya/Unity WorldSpace to Unreal WorldSpace
		// todo: On Blendshape change, trigger ReCalculate Offset position

		// Converts the Key from an int(ID) to a String
		for (auto OffsetEntry : DbMtPartOffsets)
		{
			ECharacterPartType PartGroup = static_cast<ECharacterPartType>(OffsetEntry.Key);
			FString PartGroupName = CharacterPartTypeToString(PartGroup);
			MorphTargetOffsetLookup.Add(PartGroupName, OffsetEntry.Value);
		}
	}
}

/**
 * Generates the Path(.png) to the specific SK Model in the project.
 *
 * Filename will be named the same as the model.
 * 
 * @return 
 */
FString MainMenuModel::CreateOutputTexturePath()
{
	auto Path = FPaths::ConvertRelativePathToFull(CharacterPreset->GetPackage()->GetPathName());

	FString OutputTexturePath;
	if (FPackageName::TryConvertLongPackageNameToFilename(Path, OutputTexturePath, TEXT(".png")))
	{
		auto FileName = FPaths::GetCleanFilename(OutputTexturePath);
		auto FolderPath = FPaths::GetPath(OutputTexturePath);
		FileName.InsertAt(0, TEXT("T_"));
		return FPaths::Combine(FolderPath, FileName);
	}

	return "";
}

void MainMenuModel::UpdateAvailableColourSwatches()
{
	if (CharacterPreset == nullptr) return;
	
	for (auto PartEntry : CharacterPreset->SKMParts)
	{
		if (PartEntry.Value == nullptr) continue;
		
		TSet<FIntPoint> ColourIndices = SParts::GetAvailableColourSwatchIndices(PartEntry.Value);

		AvailablePartColourSwatches.FindOrAdd(PartEntry.Key);
		AvailablePartColourSwatches[PartEntry.Key] = ColourIndices;
	}
}

void MainMenuModel::SetSwatchColour(FString SwatchName, FColor Color)
{
	if (PixelColors.Num() < 32*32) return;
	
	auto ColorSwatchData = PartColourMapUdim[SwatchName];

	int32 Index_0 = (ColorSwatchData.U * 2)       + (((32-1) - ColorSwatchData.V * 2) * 32);
	int32 Index_1 = ((ColorSwatchData.U * 2) + 1) + (((32-1) - ColorSwatchData.V * 2) * 32);
	int32 Index_2 = (ColorSwatchData.U * 2)       + ((((32-1) - ColorSwatchData.V * 2) - 1 ) * 32);
	int32 Index_3 = ((ColorSwatchData.U * 2) + 1) + ((((32-1) - ColorSwatchData.V * 2) - 1 ) * 32);
	
	PixelColors[Index_0] = Color;
	PixelColors[Index_1] = Color;
	PixelColors[Index_2] = Color;
	PixelColors[Index_3] = Color;
}

/**
 * 
 * @param FilterName Name of the Preset Filter, 'head', 'upper body'... 
 * @param ModifiedAvailableParts 
 */
void MainMenuModel::AddPresetPartsFromFilter(const FString FilterName, TMap<FString, TArray<TSharedPtr<FString>>>& ModifiedAvailableParts)
{
	FString ActiveSpecie = *GetActiveSpecie();
	auto PartFilterPartGroups = PresetPartLookup[ActiveSpecie].Find(*FilterName);

	if (PartFilterPartGroups)
	{
		for (int32 EIndex = 1; EIndex < static_cast<int32>(EPartGroup::Max); ++EIndex)
		{
			EPartGroup PartGroup = static_cast<EPartGroup>(EIndex);
			FString PartGroupString = PartGroupToString(PartGroup);

			if ((*PartFilterPartGroups).Find(EIndex) == nullptr) continue;
			
			TArray<FString> AvailableCollectionNames = (*PartFilterPartGroups)[EIndex];

			ModifiedAvailableParts.FindOrAdd(PartGroupString);

			// Convert names to shared pointers and add it
			for (auto CollectionName : AvailableCollectionNames)
			{
				ModifiedAvailableParts[PartGroupString].Add(MakeShared<FString>(CollectionName));
			}
			
			
			// Sorts the list alphabetically, and maintain `None` at element 0
			auto NoneEntry = ModifiedAvailableParts[PartGroupString][0];
			
			Algo::Sort(ModifiedAvailableParts[PartGroupString], [](const TSharedPtr<FString>& A, const TSharedPtr<FString>& B)
			{
				return *A < *B;
			});
			
			auto NoneIndex = ModifiedAvailableParts[PartGroupString].Find(NoneEntry);
			ModifiedAvailableParts[PartGroupString].RemoveAt(NoneIndex);
			ModifiedAvailableParts[PartGroupString].Insert(NoneEntry, 0);
			
		}
	}
	else
	{
		for (int32 EIndex = 1; EIndex < static_cast<int32>(EPartGroup::Max); ++EIndex)
		{
			EPartGroup PartGroup = static_cast<EPartGroup>(EIndex);
			FString PartGroupString = PartGroupToString(PartGroup);
			ModifiedAvailableParts.FindOrAdd(PartGroupString);
			ModifiedAvailableParts[PartGroupString].Empty();

			ModifiedAvailableParts[PartGroupString].Insert(MakeShared<FString>("None"),0);
		}
	}
}

void MainMenuModel::RemovePresetPartsFromFilter(const FString FilterName, TMap<FString, TArray<TSharedPtr<FString>>>& ModifiedAvailableParts)
{
	FString ActiveSpecie = *GetActiveSpecie();
	auto PartFilterPartGroups = PresetPartLookup[ActiveSpecie].Find(*FilterName);

	if (PartFilterPartGroups)
	{
		for (int32 EIndex = 1; EIndex < static_cast<int32>(EPartGroup::Max); ++EIndex)
		{
			EPartGroup PartGroup = static_cast<EPartGroup>(EIndex);
			FString PartGroupString = PartGroupToString(PartGroup);

			if ((*PartFilterPartGroups).Find(EIndex) == nullptr) continue;
			
			TArray<FString> AvailableCollectionNames = (*PartFilterPartGroups)[EIndex];
			
			// removes parts from the list that exist in the preset
			for (auto CollectionName : AvailableCollectionNames)
			{
				auto& PartGroupIter = ModifiedAvailableParts[PartGroupString];
				
				PartGroupIter.RemoveAll([CollectionName](const TSharedPtr<FString>& Item)
				{
					return Item.IsValid() && *Item == CollectionName;
				});
			}
			
			// Sorts the list alphabetically, and maintain `None` at element 0
			auto NoneEntry = ModifiedAvailableParts[PartGroupString][0];
			
			Algo::Sort(ModifiedAvailableParts[PartGroupString], [](const TSharedPtr<FString>& A, const TSharedPtr<FString>& B)
			{
				return *A < *B;
			});
			
			auto NoneIndex = ModifiedAvailableParts[PartGroupString].Find(NoneEntry);
			ModifiedAvailableParts[PartGroupString].RemoveAt(NoneIndex);
			ModifiedAvailableParts[PartGroupString].Insert(NoneEntry, 0);
		}
	}
}

TSharedPtr<FString> MainMenuModel::GetActiveSpecie()
{
	return AvailableSpecies[SelectedComboBoxSpeciesOptions];
}

int MainMenuModel::GetActiveSpecieIndex()
{
	return SelectedComboBoxSpeciesOptions;
}

/**
 * Gets the active specie from the parts in the species preset, and returns its index.
 *
 * @return index of the species in the availble species list
 */
int MainMenuModel::GetActiveSpecieFromPreset()
{
	int SpeciesIndex = -1;
	
	if (!CharacterPreset || CharacterPreset->SKMParts.IsEmpty())
	{
		return SpeciesIndex;
	}

	for (auto PartEntry : CharacterPreset->SKMParts)
	{
		if (!PartEntry.Value) {
			continue;
		}
		
		FString PartName = PartEntry.Value->GetName(); 
		if (!PartName.Contains("_BASE_"))
		{
			continue;
		}

		TArray<FString> PartToken;
        PartName.ParseIntoArray(PartToken, TEXT("_"), true);
		const FString& Token = PartToken[1];

		// TODO: The database should have a look up for the part token species code, as they differ to the two character code.
		
		if ( Token == "HUMN")
		{
			SpeciesIndex = AvailableSpecies.IndexOfByPredicate([](const TSharedPtr<FString>& Item)
			{
				return Item.IsValid() && *Item == "Human";
			});
		}
			
		else if ( Token == "GOBL")
		{
			SpeciesIndex = AvailableSpecies.IndexOfByPredicate([](const TSharedPtr<FString>& Item)
			{
				return Item.IsValid() && *Item == "Goblin";
			});
		}
		
		else if ( Token == "SKTN")
		{
			SpeciesIndex = AvailableSpecies.IndexOfByPredicate([](const TSharedPtr<FString>& Item)
			{
				return Item.IsValid() && *Item == "Skeleton";
			});
		}
	}

	return SpeciesIndex;
}

void MainMenuModel::PostActiveSpecieUpdate()
{
	UpdateOutfits();
	GetActiveSpecieParts();
}

/**
* Updates all parts that are base parts with the available specy part.
*
* Note: If head_03 is active and the newly selected specie only has a head_02,
* then the code will fall back to the closes integer for the specifiec part.
*/
void MainMenuModel::UpdateSpecieParts()
{
	// No Sidekick Character Preset, exit early.
	if (!CharacterPreset || CharacterPreset->SKMParts.IsEmpty())
	{
		return;
	}

	// cannot update parts if there are no available parts to read from
	if (!AvailablePartsDropDowns.IsValid())
	{
		return;
	}
	else if (AvailablePartsDropDowns->Num() == 0)
	{
		return;
	}
	else if ((*AvailablePartsDropDowns).Num() == 0) // was having issues on my mac and adding this seemed to help..
	{
		return;
	}
	
	for (auto ActivePartEntry: CharacterPreset->SKMParts)
	{
		if (!ActivePartEntry.Value)
		{
			continue;
		}
		
		FString PartName = ActivePartEntry.Value->GetName();

		if (!PartName.Contains("_BASE_"))
		{
			continue;
		}
		
		// Gets variant number from part
		TArray<FString> PartTokens;
		PartName.ParseIntoArray(PartTokens, TEXT("_"), true);
		const int PartVariant = FCString::Atoi(*PartTokens[3]);

		CharacterPreset->Modify();

		FString KeyNoSpaces = ActivePartEntry.Key;
		KeyNoSpaces.RemoveSpacesInline();
		
		ECharacterPartType PartType = CharacterPartTypeFromString(KeyNoSpaces);	
		if (AvailablePartsDropDowns->Contains(PartType))
		{
			TArray<FString>* Parts = AvailablePartsDropDowns->Find(PartType);

			if (!Parts || Parts->IsEmpty())
			{
				// if there are no available parts in the drop down then we must assume
				// that the species does not have that part.
				// Example: Skeleton specie's does not contain a Nose
				CharacterPreset->SKMParts[ActivePartEntry.Key] = nullptr;
				continue;
			}
			
			if (Parts && !Parts->IsEmpty())
			{
				FString ClosestPartMatch = "";
				int ClosestPartVariant = -1;
				
				for (FString PartEntry : *Parts)
				{
					if (PartEntry.Contains("_BASE_"))
					{
						TArray<FString> PartEntryTokens;
						PartEntry.ParseIntoArray(PartTokens, TEXT("_"), true);
						const int EntryVariant = FCString::Atoi(*PartTokens[3]);
						
						if (PartVariant==EntryVariant)
						{
							// found a base species part that has matching index
							ClosestPartMatch = PartEntry;
							ClosestPartVariant = EntryVariant;

							CharacterPreset->SKMParts[ActivePartEntry.Key] = GetPartByName(PartEntry);
							break;
						}
						else if (ClosestPartVariant < EntryVariant)
						{
							ClosestPartMatch = PartEntry;
							ClosestPartVariant = EntryVariant;
							CharacterPreset->SKMParts[ActivePartEntry.Key] = GetPartByName(PartEntry);
						}
					}
				}

				if ( ClosestPartVariant == -1)
				{
					// no base parts exist in the drop down for the active specie's part
					// Example: skeleton dont have hair.
					CharacterPreset->SKMParts[ActivePartEntry.Key] = nullptr;
				}
			}
		}
	}
	
	UpdateSpecieFacialParts();
	
	CharacterPreset->OnObjectChanged.Broadcast();
}

void MainMenuModel::UpdateSpecieFacialParts()
{
	for (ECharacterPartType PartType : PartsGrouping[EPartGroup::Head])
	{
		// Skip updating attachments parts, as they do not relate to a species.
		if (PartType == ECharacterPartType::AttachmentHead ||
			PartType == ECharacterPartType::AttachmentFace)
		{
			continue;
		}
		
		FString PartTypeName = CharacterPartTypeToString(PartType);
		
		auto ActiveSpecie = GetActiveSpecie();
		
		
		if (CharacterPreset->SKMParts.Contains(PartTypeName))
		{
			
		}
	}
}

void MainMenuModel::ActivateOutfit(TSharedPtr<FString> Outfit)
{
	if (SelectedOutfitKeys.FindByKey(Outfit) == nullptr)
	{
		SelectedOutfitKeys.Add(Outfit);
	}
}
void MainMenuModel::DeactivateOutfit(TSharedPtr<FString> Outfit)
{
	SelectedOutfitKeys.Remove(Outfit);
}

/**
 * Performs data checks the Character Preset
 *
 * - Adding all the Part Names to the Part TMap.
 * - Assigning the default Material if one is found.
 */
void MainMenuModel::PreflightChecksCharacterPreset()
{
	if (CharacterPreset == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("No Character Preset Found..."));
		return;
	}

	// Set active specie to that of the base in preset assets
	int FoundSpecieIndex = GetActiveSpecieFromPreset();
	if (FoundSpecieIndex > -1)
	{
		SelectedComboBoxSpeciesOptions = FoundSpecieIndex;
		SelectedSpecie = GetActiveSpecie();
		PostActiveSpecieUpdate();
		SetupGroupedPartDataStructure();
	}
	
	// Initialise Part Keys in TMap
	
	for (int32 i = 1; i < static_cast<int32>(EPartGroup::Max); ++i)
	{
		EPartGroup PartGroup = static_cast<EPartGroup>(i);
		for (auto PartType : PartsGrouping[PartGroup])
		{
			FString PartName = CharacterPartTypeToString(PartType);
			if (!CharacterPreset->SKMParts.Contains(PartName))
			{
				CharacterPreset->SKMParts.Add(PartName);
			}
		}
	}

	// Assigns the default Sidekicks Material
	
	if (CharacterPreset->BaseMaterial == nullptr)
	{
		FString MaterialAssetPath = "/Game/Synty/SidekickCharacters/Resources/Materials/M_Default_Sidekick.M_Default_Sidekick";
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *MaterialAssetPath);
		if (Material)
		{
			CharacterPreset->BaseMaterial = Material;
		}
	}
}

/**
 * Inserts a space in front of any capital letter, except the first character.
 *
 * @example "ThisIsADog" = "This Is A Dog"
 * 
 * @param TextToModify String that will be read for modification.
 */
FString SpaceInserterInline(const FString& TextToModify)
{
	FString OutText = "";
	
	for (int i = 0; i < TextToModify.Len(); ++i)
	{
		if (i > 0 && FChar::IsUpper(TextToModify[i]))
		{
			OutText += " ";	
		}
		OutText += TextToModify[i];
	}
	
	return OutText;
} 

bool MainMenuModel::SpecieIDExists(int32 SpecieID)
{
	for (auto SpecieEntry : SpeciesLookup)
	{
		if (SpecieEntry.Value.Id == SpecieID) {
			return true;
		}
	}
	return false;
}

FString MainMenuModel::GetSpecieName(int32 SpecieID)
{
	for (auto SpecieEntry : SpeciesLookup)
	{
		if (SpecieEntry.Value.Id == SpecieID) {
			return SpecieEntry.Value.Name;
		}
	}
	return "";
}

bool MainMenuModel::LoadSKData(SKFileData FileData)
{	
	CharacterPreset->Modify();
	
	// Activate Species
	if(SpecieIDExists(FileData.SpecieID))
	{
		FString SpecieName = GetSpecieName(FileData.SpecieID);

		int SpeciesIndex = AvailableSpecies.IndexOfByPredicate([SpecieName](const TSharedPtr<FString>& Item)
		{
			return Item.IsValid() && *Item == SpecieName;
		});

		// Forces the species selection update of the UI
		
		SelectedComboBoxSpeciesOptions = SpeciesIndex;
        SelectedSpecie = GetActiveSpecie();
        PostActiveSpecieUpdate();
        SetupGroupedPartDataStructure();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Project missing Specie that exists in SK file.."));
		return false;
	}

	// Removes all Parts.
	for ( auto ExistingPart : CharacterPreset->SKMParts)
	{
		CharacterPreset->SKMParts[ExistingPart.Key] = nullptr;
	} 

	// Adds Parts that are available on device.
	for (auto PartEntry : FileData.Parts)
	{
		FString ConditionedPartName = SpaceInserterInline(PartEntry.Key);
		if (CharacterPreset->SKMParts.Contains(ConditionedPartName))
		{
			USkeletalMesh* Part = GetPartByName(PartEntry.Value);
			
			if (!Part)
			{
				UE_LOG(LogTemp, Warning, TEXT("SK Part Not Found: %s"), *PartEntry.Value);
				CharacterPreset->SKMParts[ConditionedPartName] = nullptr;
				continue;
			}
			
			CharacterPreset->SKMParts[ConditionedPartName] = Part;
		}	
	}
	
	// Set Blendshape / Morph targets
	
	// NOTE: could recode this and utilise the FindByPropertyName, but to keep it simple for now
	//		 just use a string comparison.
	for (auto MorphEntry : FileData.MorphTargets)
	{
		if (MorphEntry.Key == "MTBodyMusculature")
		{
			CharacterPreset->MTBodyMusculature = MorphEntry.Value;
		}
		else if (MorphEntry.Key == "MTBodyType")
		{
			CharacterPreset->MTBodyType = MorphEntry.Value;
		}
		else if (MorphEntry.Key == "MTBodySize")
		{
			CharacterPreset->MTBodySize = MorphEntry.Value;
		}
	}

	// Texture update

	TMap<int32, FString> TemporaryLookup;
	for (auto DbRow : PartColourMapUdim)
	{
		FString& ValueEntry = TemporaryLookup.FindOrAdd(DbRow.Value.Id);
		ValueEntry = DbRow.Key;
	}
	
	for (auto Entry : FileData.MainColor)
	{
		FString ColourSlotName = TemporaryLookup[Entry.Key];
		FString ColourHex = Entry.Value;
		// UE_LOG(LogTemp, Warning, TEXT("  %s"), ColourSlotName);

		// Updates the flat texture array in memory.
		SetSwatchColour(ColourSlotName, FColor::FromHex(ColourHex));
		FSKTexture::WriteDataToTexture(&PixelColors, CharacterPreset->TemporaryTexture);	
	}
	
	CharacterPreset->OnObjectChanged.Broadcast();
	
	return true;
}

/**
 * Checks if the Database file exists
 * 
 * @return True if the Database could be reached
 */
bool MainMenuModel::DatabaseExists()
{
	bool Exists = false;
	if (GEditor)
	{
		USidekickDBSubsystem* Subsystem = GEditor->GetEditorSubsystem<USidekickDBSubsystem>();
		if (Subsystem)
		{
			Subsystem->DatabaseExists(Exists);
		}
		else
		{
			UE_LOG(LogDataTable, Error, TEXT("Unable to access SidekickDB Subsystem"));
			return Exists;
		}
	}

	// if (!Exists)
	// {
	// 	UE_LOG(LogTemp, Error, TEXT("Failed to find and query the database."));
	// }
	
	return Exists;
}

/**
 * Checks to see if the Database is connected, else it will try and connect.
 * @return 
 */
bool MainMenuModel::DatabaseConnected()
{
	bool bStatus = false;
	
	USidekickDBSubsystem* Subsystem = GEditor->GetEditorSubsystem<USidekickDBSubsystem>();
	Subsystem->ConnectionExists(bStatus);
	if (!bStatus)
	{
		Subsystem->OpenDatabase(bStatus);
		UE_LOG(LogTemp, Warning, TEXT("Database file exists, not connected, trying to connect..."));
	}
	return bStatus;
}