#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "SKDBTables.generated.h"

USTRUCT(BlueprintType)
struct SYNTYSQLITECORE_API FSidekickSpecies
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	int32 Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	FString Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	FString Code;
};

/**
 * Sidekick Part Struct that is used to wrap a row from the database table.
 */
USTRUCT(BlueprintType)
struct SYNTYSQLITECORE_API FSidekickPart
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	int32 Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	int32 SpeciesId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	int32 TypeID;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	int32 PartGroup;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	FString Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	FString FileName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	FString Path;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	bool bUseWrap;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	bool bExists;
};

USTRUCT(BlueprintType)
struct SYNTYSQLITECORE_API FSidekickCollection
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	TArray<FString> Parts;
};

USTRUCT(BlueprintType)
struct SYNTYSQLITECORE_API FMorphTargetPartOffset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	int32 PartType;

	/** The offsets for each blendshape for the specified blend type,
	 * 
	 * In otherwords the offset value when the blendshape is set to one.
	 * Each blend_type correlates to a blendshape "BlendShapeTypes".
	 * This is to make sure parts do not get engulfed by shape changes.
	 * 
	 * In the table, this information is stored in a flat structure(row)
	 * 
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	TMap<int32, FTransform> PartBlendOffsets;
};

/**
 * SK_Color_property Database table.
 */
USTRUCT(BlueprintType)
struct SYNTYSQLITECORE_API FColorProperty
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	int32 Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	int32 ColorGroup;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	FString Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	int32 U;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	int32 V;
};

/**
 * SK_Color_preset Database table.
 */
USTRUCT(BlueprintType)
struct SYNTYSQLITECORE_API FColorPreset
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	int32 Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	int32 SpeciesID;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	int32 ColorGroup;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	FString Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	FString SpeciesName;

};

/**
 * Contains a streamlined combination of
 * - sk_preset
 * - sk_preset_filter_row
 * - sk_part_preset
 * - sk_part_preset_row
 * - sk_part
 */
USTRUCT(BlueprintType)
struct SYNTYSQLITECORE_API FSidekickPartPreset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	FString SpeciesName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	FString PresetCollection;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	int32 PartGroup;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	FString PresetName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	FString PartName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	int32 PartType;
};

/**
 * Used to get a unique list of Preset Collection that are fully populated with parts.
 * - Preset Groups that are missing a part get filtered out.
 */
USTRUCT(BlueprintType)
struct SYNTYSQLITECORE_API FSidekickPresetCollection
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	FString SpeciesName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	FString PresetCollection;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	int32 PartGroup;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	FString PresetName;
};

USTRUCT(BlueprintType)
struct SYNTYSQLITECORE_API FSidekickBodyShapePreset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	int32 Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	int32 BodyType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	int32 BodySize;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	int32 Musculature;
};

USTRUCT(BlueprintType)
struct SYNTYSQLITECORE_API FSidekickColorPresetRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	int32 Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	int32 ColorPresetID;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	int32 ColorPropertyID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	FString Color;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	FString Metallic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	FString Smoothness;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	FString Reflection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	FString Emission;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	FString Opacity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyntySidekickDB")
	FString Name;
};