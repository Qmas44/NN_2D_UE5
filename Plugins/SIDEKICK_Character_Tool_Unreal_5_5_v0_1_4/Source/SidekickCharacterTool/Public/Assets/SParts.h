#pragma once

#include "Enums/PartGroups.h"
#include "Enums/CharacterPartTypes.h"
#include "Engine/SkeletalMesh.h"

#include "DataTableUtils.h"
#include "AssetRegistry/AssetData.h"

class SParts
{
public:

	// static FString GetSidekickResourcesPath();
	
	void GetParts();

	/**
	 * Gets all the available swatch positions from the Skeletal Mesh Part provided.
	 *
	 * Note: This queries the UV data directly, so it comes with some minor overhead.
	 * 
	 * @param SKM_Part Will be queried for the indices it has available
	 * @return Unique list of colour swatch positions.
	 */
	static TSet<FIntPoint> GetAvailableColourSwatchIndices(USkeletalMesh* SKM_Part);
	
private:
	/**
	 * Converts the UV position into a Colour Swatch lookup position.
	 *
	 * This relates the the database table sk_color_property.
	 * 
	 * @param UV The UV data that will be converted into a color swatch lookup position.
	 */
	static FIntPoint ConvertUVToPartLookupIndices(const FVector2f* UV);
};

void ConvertUnityAssetsPathToUnrealPath(FString* ConvertPath);

FString GetSidekickResourcesPath();

TArray<FAssetData> GetAvailableParts();

/** Finds all SK Parts that exist in the project */
void GetAvailableParts(TMap<FString, FAssetData>* AvailableParts);

/** Lookup table for Part Grouping*/
const TMap<EPartGroup, TArray<ECharacterPartType>> PartsGrouping = {
	{EPartGroup::Head, {
			ECharacterPartType::Head,
			ECharacterPartType::Hair,
			ECharacterPartType::EyebrowLeft,
			ECharacterPartType::EyebrowRight,
			ECharacterPartType::EyeLeft,
			ECharacterPartType::EyeRight,
			ECharacterPartType::EarLeft,
			ECharacterPartType::EarRight,
			ECharacterPartType::FacialHair,
			ECharacterPartType::AttachmentHead,
			ECharacterPartType::AttachmentFace,
			ECharacterPartType::Nose,
			ECharacterPartType::Teeth,
			ECharacterPartType::Tongue
	}},
	{EPartGroup::UpperBody, {
			ECharacterPartType::Torso,
			ECharacterPartType::ArmUpperLeft,
			ECharacterPartType::ArmUpperRight,
			ECharacterPartType::ArmLowerLeft,
			ECharacterPartType::ArmLowerRight,
			ECharacterPartType::HandLeft,
			ECharacterPartType::HandRight,
			ECharacterPartType::AttachmentBack,
			ECharacterPartType::AttachmentShoulderLeft,
			ECharacterPartType::AttachmentShoulderRight,
			ECharacterPartType::AttachmentElbowLeft,
			ECharacterPartType::AttachmentElbowRight,
			ECharacterPartType::Wrap
	}},
	{EPartGroup::LowerBody, {
			ECharacterPartType::Hips,
			ECharacterPartType::LegLeft,
			ECharacterPartType::LegRight,
			ECharacterPartType::FootLeft,
			ECharacterPartType::FootRight,
			ECharacterPartType::AttachmentHipsFront,
			ECharacterPartType::AttachmentHipsBack,
			ECharacterPartType::AttachmentHipsLeft,
			ECharacterPartType::AttachmentHipsRight,
			ECharacterPartType::AttachmentKneeLeft,
			ECharacterPartType::AttachmentKneeRight
	}}
};

/** Gets the group that the part belongs to.*/
EPartGroup GetPartGroup(const ECharacterPartType PartType);