#include "Assets/SParts.h"

#include "SystemTextures.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/StaticMeshVertexDataInterface.h"
#include "AssetRegistry/AssetData.h"

/**
 * Get all available Parts in the current project
 */
void SParts::GetParts()
{
}

/**
 * 
 * @return the full system path name for the meshes folder
 */
FString GetSidekickResourcesPath()
{
	FString ProjectRootPath = FPaths::ProjectContentDir();
	
	return ProjectRootPath + "Synty/SidekickCharacters/Resources/Meshes";
}

/**
 * Converts the Stored DB Path that comes from Unity into a Unreal Path
 * @param ConvertPath The Unity path starting with `Assets\` that will be converted to `Game\`
 */
void ConvertUnityAssetsPathToUnrealPath(FString* ConvertPath)
{
	FString FromPath = "Assets";
	FString ReplaceWith = "Game";
	
	if (ConvertPath->Left(6).Equals(FromPath, ESearchCase::IgnoreCase) )
	{
		*ConvertPath = ReplaceWith + ConvertPath->Mid(6);
	}
}

/** Gets all the Parts that are currently available in the project's SidekickCharacters Meshes folder. */
TArray<FAssetData> GetAvailableParts()
{
	TArray<FAssetData> AssetData;
	
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	
	// Filter to retrieve only SkeletalMeshes that exist under the Sidekick path
	FARFilter Filter;
	Filter.ClassPaths.Add(USkeletalMesh::StaticClass()->GetClassPathName());
	Filter.PackagePaths.Add("/Game/Synty/SidekickCharacters/Resources/Meshes");
	Filter.bRecursivePaths = true;

	AssetRegistryModule.Get().GetAssets(Filter, AssetData);

	return AssetData;
}

/**
 * Creates a Map of all the available parts
 * - Key = AssetName
 * 
 * @param AvailableParts 
 */
void GetAvailableParts(TMap<FString, FAssetData>* AvailableParts)
{
	auto LocalParts = GetAvailableParts();

	for (auto LocalPart : LocalParts)
	{
		AvailableParts->Add(LocalPart.AssetName.ToString(), LocalPart);
	}
}

/** Gets the Group that the Part belongs to*/
EPartGroup GetPartGroup(const ECharacterPartType PartType)
{
	switch(PartType)
	{
		case ECharacterPartType::Head:
        case ECharacterPartType::Hair:
        case ECharacterPartType::EyebrowLeft:
        case ECharacterPartType::EyebrowRight:
        case ECharacterPartType::EyeLeft:
        case ECharacterPartType::EyeRight:
        case ECharacterPartType::EarLeft:
        case ECharacterPartType::EarRight:
        case ECharacterPartType::FacialHair:
        case ECharacterPartType::AttachmentHead:
        case ECharacterPartType::AttachmentFace:
        case ECharacterPartType::Nose:
        case ECharacterPartType::Teeth:
        case ECharacterPartType::Tongue:
            return EPartGroup::Head;

        case ECharacterPartType::Torso:
        case ECharacterPartType::ArmUpperLeft:
        case ECharacterPartType::ArmUpperRight:
        case ECharacterPartType::ArmLowerLeft:
        case ECharacterPartType::ArmLowerRight:
        case ECharacterPartType::HandLeft:
        case ECharacterPartType::HandRight:
        case ECharacterPartType::AttachmentBack:
        case ECharacterPartType::AttachmentShoulderLeft:
        case ECharacterPartType::AttachmentShoulderRight:
        case ECharacterPartType::AttachmentElbowLeft:
        case ECharacterPartType::AttachmentElbowRight:
        case ECharacterPartType::Wrap:
            return EPartGroup::UpperBody;

        case ECharacterPartType::Hips:
        case ECharacterPartType::LegLeft:
        case ECharacterPartType::LegRight:
        case ECharacterPartType::FootLeft:
        case ECharacterPartType::FootRight:
        case ECharacterPartType::AttachmentHipsFront:
        case ECharacterPartType::AttachmentHipsBack:
        case ECharacterPartType::AttachmentHipsLeft:
        case ECharacterPartType::AttachmentHipsRight:
        case ECharacterPartType::AttachmentKneeLeft:
        case ECharacterPartType::AttachmentKneeRight:
            return EPartGroup::LowerBody;

        default:
            return EPartGroup::Max;
	}
}

// note: In the future maybe we serialise the Swatch Indices into the DB, that way reducing the performance hit when changing a part.
// todo: Thought serialise the GetUdims into the DB, so each part on DB update gets there UV's checked and updated.
TSet<FIntPoint> SParts::GetAvailableColourSwatchIndices(USkeletalMesh* SKM_Part)
{
	// Unique list of indices found. These represent all the available Colour Swatches for the part provided
	TSet<FIntPoint> AvailableLookupIndices;
	
	if (!SKM_Part) return AvailableLookupIndices;
	
	FSkeletalMeshRenderData* RenderData = SKM_Part->GetResourceForRendering();
	if (!RenderData || RenderData->LODRenderData.Num() == 0) return AvailableLookupIndices;

	// Retreives Mesh data
	const FSkeletalMeshLODRenderData& LODData = RenderData->LODRenderData[0];
	const FStaticMeshVertexBuffer& StaticVertBuffer = LODData.StaticVertexBuffers.StaticMeshVertexBuffer;
	
	const int32 NumVerts = LODData.StaticVertexBuffers.PositionVertexBuffer.GetNumVertices();
	
	// Get each vertex uv's part index for the lookup colour swatch 
	for (int32 i = 0; i < NumVerts; i++)
	{
		FVector2f UV = StaticVertBuffer.GetVertexUV(i, 0);
		
		FIntPoint UVPartIndices = SParts::ConvertUVToPartLookupIndices(&UV);

		AvailableLookupIndices.Add(UVPartIndices);
	}
	
	return AvailableLookupIndices;
}

FIntPoint SParts::ConvertUVToPartLookupIndices(const FVector2f* UV)
{
	// Converts the UV value from udim space into integers that can be correlated to the database UV position
	int32 ScaledU = FMath::FloorToInt(UV->X * 16.f);
	int32 ScaledV = FMath::FloorToInt(UV->Y * 16.f);
	if (ScaledU == 16)
	{
		ScaledU = 15;
	}
	if (ScaledV == 16)
	{
		ScaledV = 15;
	}

	// inverts the V(Y) axis due to how Unreal computes its data compared to Unity.
	ScaledV = 15 - ScaledV;
	
	return {ScaledU, ScaledV};
};