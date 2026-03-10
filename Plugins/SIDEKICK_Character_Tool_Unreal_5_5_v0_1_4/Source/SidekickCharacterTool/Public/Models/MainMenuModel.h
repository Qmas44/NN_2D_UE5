#pragma once

#include "SKDBTables.h"
#include "Enums/CharacterPartTypes.h"
#include "SKCharacterPreset.h"

struct SKFileData;

/**
* Stores all options required for performing the Part combining.
*/
struct MergeOptions
{
	/** Combines all sections into one*/
	bool bMergeSection = false; // TODO: Implement
	bool bKeepFacialMorphTargets = false;
	bool bKeepBodyMorphTargets = false;
	bool bCustomMergeLocation = true;
};

class SIDEKICKCHARACTERTOOL_API MainMenuModel
{
public:

	MainMenuModel();
	~MainMenuModel();

	//todo: Add preflight check, that if a head is available, check for its type, and try to detect what species should default to.
	/** The selected species index in the combobox*/
	int SelectedComboBoxSpeciesOptions = 0;
	
	/** The selected(active) specie */
	TSharedPtr<FString> SelectedSpecie;
	
	/** Outfits that are currently selected/activated */
    TArray<TSharedPtr<FString>> SelectedOutfitKeys;
	
	/** List of available Species */
	TArray<TSharedPtr<FString>> AvailableSpecies;
	
	/** Outfits that are currently available for the selected Specie */
	TArray<TSharedPtr<FString>> AvailableOutfits;
	
	/** Lookup table for the available dropdown lists */
	TSharedPtr<TMap<ECharacterPartType, TArray<FString>>> AvailablePartsDropDowns;
	
	/** Stores the DB Table for Morph Target Offsets */
	TMap<FString, FMorphTargetPartOffset> MorphTargetOffsetLookup;

	/** lookup for the active part and its available colour swatches
	 *
	 * We store them against the key which is the name of the part the SK Part populates
	 * e.g. Head,Hair,Torso..
	 *
	 * The reason for this is to make it easier to update the swatches after a Part changes.
	 * Else the list of points would have to be regenerated after any part is modified.
	 */
	TMap<FString, TSet<FIntPoint>> AvailablePartColourSwatches;
	
	void Initialize();

	void RefreshActiveOutfits();
	
	/** Stores the list of available body preset names */
	TArray<TSharedPtr<FString>> AvailableBodyPresets;
	/** Lookup table for the body preset data */ 
	TMap<FString, FSidekickBodyShapePreset> BodyShapePresetLookup;

	/** Stores the merge part options */
	TSharedRef<MergeOptions> MergeSettings = MakeShared<MergeOptions>();
	
	/**
	 * @param Key row entry id.
	 * @param Value Row in the DB table `sk_color_preset`
	 */
	TMap<int32, FColorPreset> ColorPresetLookup;
	
	/* Helper variable that wraps the ColorPresetLookup in a more UI accesible data structure
	 * 
	 * Species > Colors [EColorGroup] > List of entries
	 *
	 * Species Name, Color Group Name, Available list of Color Presets
	 */
	TMap<FString, TMap<FString, TArray<TSharedPtr<FString>>>> AvailableColorPresets;
	
	/* Simple helper variable that when populated with the same data as above returns the DB table entry*/
	TMap<FString, TMap<FString, TArray<FColorPreset*>>> ColorPresetsToDBTable;

	/**
	 * Look up table that stores all the color preset rows from the db.
	 * 
	 * @param Key Color Preset ID
	 * @param Value Row in the DB table `sk_color_preset_row`
	 */
	TMap<int32, TArray<FSidekickColorPresetRow>> ColorPresetRowLookup;
	
	/** Stores the pixel/udim colour for each part.
	 * 
	 * @param Key The name of the Part. Example: "Head"
	 * @param Value The ColorProperty Entry in the DB that corresponds to the key
	 *
	 * @note Database Table: sk_color_property
	 */
	TMap<FString, FColorProperty> PartColourMapUdim;

	/** Stores the transient Colour Values for Texture. */
	TArray<FColor> PixelColors;
	FColor NoColor = FColor::White;
	
	/**
	 * Stores the list of species names to preset filter names, that
	 * are associated to the specie.
	 */
	TMap<FString, TArray<FString>> SpeciesToPresetFilter;
	
	/** List of available part presets. */
	TArray<FSidekickPartPreset> PartPresets; // todo: On the fence with removing this to a local variable during next refactor.
	/** List of Part Presets that contain all parts required */
	TArray<FSidekickPresetCollection> UniquePartPresets; // todo: On the fence with removing this to a local variable during next refactor.
	
	/** <Species Name><Preset Part Filter><Part Group>[Preset Name] */
	TMap<FString, TMap<FString, TMap<int32, TArray<FString>>>> PresetPartLookup;

	/** <Part Group> <Preset Name> <Part Type, Part Name> */
	TMap<int32, TMap<FString, TMap<int32, FString>>> PresetToParts;
	

	void AddPresetPartsFromFilter(const FString FilterName, TMap<FString, TArray<TSharedPtr<FString>>>& ModifiedAvailableParts);
	void RemovePresetPartsFromFilter(const FString FilterName, TMap<FString, TArray<TSharedPtr<FString>>>& ModifiedAvailableParts);
	
private:
	/** Stores the DB Table results for all Species
	 * Only returns species that contains parts that exist in the project.
	 */
	TMap<FString, FSidekickSpecies> SpeciesLookup;

	/** Stores the DB Table results for all Parts */
	TMap<FString, FSidekickPart> PartsLookup;
	
	/** All parts currently available in the project */
	TSharedPtr<TMap<FString, FAssetData>> AvailableParts;

	/** A Lookup table for all the parts, that uses the Species/Outfit as a key. */
	TSharedPtr<TMap<FString, FSidekickCollection>> CollectionToParts;

	
private:

	void GetMorphTargetPartOffsets();
	
	void GetAvailableSpecies();

	/** Get all available outfits for the active specie from the DB*/
	void GetOutfits();

	/** Gets all the outfits from the DB and the parts that make up each outfit. */
	void GetOutfitParts();

	/** Queries the DB for the preset tables */
	void GetPresetData();

	/** Gets all entries for the Body Shape preset from the DB */
	void GetBodyShapePresets();

	void GetColorPresets(int32 SpeciesID=-1);
	
	void AddPartsToDropdown(TArray<FString>& Parts);

	/** Gets all parts that relate to the active specie */
	void GetActiveSpecieParts();
	
	void UpdateOutfits();
	
public:

	/**
	 * Updates a Swatch (Specific Color Area) in the texture.
	 *
	 * Swatches are made up of 4x4 pixels 
	 * 
	 * @param SwatchName 
	 * @param Color 
	 */
	void SetSwatchColour(FString SwatchName, FColor Color);
	
	/** Creates a lookup table of part collections */
	void SetupGroupedPartDataStructure();

	/** Updates the internal part data relating to the active specie */ 
	void PostActiveSpecieUpdate();

	/** Updates base parts to the active specie */
	void UpdateSpecieParts();
	
	/** Updates the Parts DB Table, with parts that exist in the project. */ 
	void UpdateDBPartAvailability();

	/** Returns the Skeletal Mesh of the part named */
	USkeletalMesh* GetPartByName(FString PartName);

	/** The Character preset that is currently being modified */
	USKCharacterPreset* CharacterPreset = nullptr;

	FString CreateOutputTexturePath();

	/** 
	 * Gets all colour swatch indices for the active Parts
	 *
	 * Each entry has an [x,y] value that corelates to a specific pixel on texture,
	 * that is used to colour a section of a SKPart.
	 */
	void UpdateAvailableColourSwatches();

	/**
	 * Queries the DB for all preset colour rows. If ColorPresetID is populated then it gets only the relative entries.
	 * @param ColorPresetID 
	 */
	void GetColourPresetRows(int32 ColorPresetID=-1);

	/** Helper function that returns the Active Specie */
	TSharedPtr<FString> GetActiveSpecie();
	int GetActiveSpecieIndex();
	int GetActiveSpecieFromPreset();

	// Add and remove Outfits status from the datamodel
	void ActivateOutfit(TSharedPtr<FString> Outfit);
	void DeactivateOutfit(TSharedPtr<FString> Outfit);

	void PreflightChecksCharacterPreset();

	/** Loads the SK data into SKPreset */
	bool LoadSKData(SKFileData FileData);

private:

	/** Helper function that detects if a species exist in the project. */
	bool SpecieIDExists(int32 SpecieID);

	/** Helper function that returns the species name by id */
	FString GetSpecieName(int32 SpecieID);

	/** Checks if the Database file exists */
	bool DatabaseExists();

	/** Checks to see if the Database is connected, else it will try and connect. */
	bool DatabaseConnected();
	
	/** Updates the facial parts that relate to the active specie
	 * 
	 * Facial parts are handled a bit differently compared to other parts. 
	 * Due to this we need to search for all facial parts extept attachments and check there specie aligned.
	 */
	void UpdateSpecieFacialParts();
};

