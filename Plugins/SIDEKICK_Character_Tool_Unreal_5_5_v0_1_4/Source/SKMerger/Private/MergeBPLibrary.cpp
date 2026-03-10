// Fill out your copyright notice in the Description page of Project Settings.


#include "MergeBPLibrary.h"

#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Animation/Skeleton.h"
#include "Engine/SkinnedAssetCommon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Engine/SkeletalMesh.h"
#include "PackageTools.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "LODUtilities.h"
#include "SkeletalMeshAttributes.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Materials/MaterialInterface.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

#define LOCTEXT_NAMESPACE "FSKMergerModule"

/** keeps track of an existing section that need to be merged with another */
struct FMergeSectionInfo
{
	/** ptr to source skeletal mesh for this section */
	const USkeletalMesh* SkelMesh;
	/** ptr to source section for merging */
	const FSkelMeshRenderSection* Section;
	/** mapping from the original BoneMap for this sections chunk to the new MergedBoneMap */
	TArray<FBoneIndexType> BoneMapToMergedBoneMap;
	/** transform from the original UVs */
	TArray<FTransform> UVTransforms; // todo: Remove this as we will not be transforming any UVs

	FMergeSectionInfo( const USkeletalMesh* InSkelMesh,const FSkelMeshRenderSection* InSection, TArray<FTransform> & InUVTransforms )
		:	SkelMesh(InSkelMesh)
		,	Section(InSection)
		,	UVTransforms(InUVTransforms)
	{}
};


/** info needed to create a new merged section */
struct FNewSectionInfo
{
	/** array of existing sections to merge */
	TArray<FMergeSectionInfo> MergeSections;
	/** merged bonemap */
	TArray<FBoneIndexType> MergedBoneMap;
	/** material for use by this section */
	UMaterialInterface* Material;
	
	
	/** 
	* if -1 then we use the Material* to match new section entries
	* otherwise the MaterialId is used to find new section entries
	*/
	int32 MaterialId;

	/** material slot name, if multiple section use the same material we use the first slot name found */
	FName SlotName;

	/** Default UVChannelData for new sections. Will be recomputed if necessary */
	FMeshUVChannelInfo UVChannelData;

	/** The source that the seciton originated from */
	const USkeletalMesh* SkelMesh;
	
	FNewSectionInfo( UMaterialInterface* InMaterial, int32 InMaterialId, FName InSlotName, const FMeshUVChannelInfo& InUVChannelData, const USkeletalMesh* InSkelMesh )
		: Material(InMaterial)
		, MaterialId(InMaterialId)
		, SlotName(InSlotName)
		, UVChannelData(InUVChannelData)
		, SkelMesh(InSkelMesh)
	{}
};


/** Info about source mesh used in merge. */
struct FMergeMeshInfo
{
	/** Mapping from RefSkeleton bone index in source mesh to output bone index. */
	TArray<int32> SrcToDestRefSkeletonMap;
};

#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION < 6)	
/**
 * Retrieves the World Transform, by walking up the hierarchy to the root.
 * 
 * @param RefSkeleton 
 * @param BoneIndex 
 * @return 
 */
FTransform GetWorldTransform(const FReferenceSkeleton& RefSkeleton, const int32 BoneIndex)
{
	const TArray<FTransform>& RefBonePoses = RefSkeleton.GetRefBonePose();
    
	FTransform BoneWorldTM = FTransform::Identity;
	
	int32 CurrentBoneIdx = BoneIndex;
	while (CurrentBoneIdx != INDEX_NONE)
	{
		BoneWorldTM = RefBonePoses[CurrentBoneIdx] * BoneWorldTM;
        
		// Get parent index for next iteration
		CurrentBoneIdx = RefSkeleton.GetParentIndex(CurrentBoneIdx);
	}

	return BoneWorldTM;
}
#endif

// Helper function taken from the FBXSkeletalMeshImporter
/**
 * 
 * @param PackageName 
 * @param AssetPackageName 
 * @param bIncrement Increment the package if the specified package if found to exist, else overwrite it.
 * @return 
 */
UPackage* CreatePackageForNode(FString PackageName, /**out*/ FString &AssetPackageName, bool bIncrement=false)
{
	FString PackageNameOfficial = UPackageTools::SanitizePackageName(PackageName);
	
	// We can not create assets that share the name of a map file in the same location
	bool IsPkgExist = FPackageName::DoesPackageExist(PackageNameOfficial);
	if (!IsPkgExist)
	{
		IsPkgExist = FindObject<UPackage>(nullptr, *PackageNameOfficial) != nullptr;
	}

	if (bIncrement) // if false, then we do not incrment the asset if it is found to exist, we use it
	{
		int32 tryCount = 1;
		while (IsPkgExist)
		{
			PackageNameOfficial = PackageName;
			PackageNameOfficial += TEXT("_");
			PackageNameOfficial += FString::FromInt(tryCount++);
			PackageNameOfficial = UPackageTools::SanitizePackageName(PackageNameOfficial);
			IsPkgExist = FPackageName::DoesPackageExist(PackageNameOfficial);
			if (!IsPkgExist)
			{
				IsPkgExist = FindObject<UPackage>(nullptr, *PackageNameOfficial) != nullptr;
			}
		}
	}
	
	UPackage* Pkg = CreatePackage( *PackageNameOfficial);
	if (!ensure(Pkg))
	{
		return nullptr;
	}
	Pkg->FullyLoad();

	AssetPackageName = FPackageName::GetLongPackageAssetName(Pkg->GetOutermost()->GetName());
	return Pkg;
}

/**
 * Copies Sockets from the provided list of sockets onto the specified Mesh.
 * 
 * @param MergeMesh will have sockets added to it
 * @param SocketsToCopy list of sockets that will be copied onto the SkeletalMesh's Skeleton
 * @param bAreSkeletonSockets The type of sockets to Add.
 */
void AddSockets(USkeletalMesh* MergeMesh, const TArray<USkeletalMeshSocket*>& SocketsToCopy, bool bAreSkeletonSockets)
{
	if(bAreSkeletonSockets)
	{
		for (const TObjectPtr<USkeletalMeshSocket>& MergeSocket : SocketsToCopy)
		{
			USkeletalMeshSocket* NewSocket = NewObject<USkeletalMeshSocket>(MergeMesh->Skeleton);
			if (NewSocket != nullptr)
			{
				// Copy over all socket information
				NewSocket->SocketName = MergeSocket->SocketName;
				NewSocket->BoneName = MergeSocket->BoneName;
				NewSocket->RelativeLocation = MergeSocket->RelativeLocation;
				NewSocket->RelativeRotation = MergeSocket->RelativeRotation;
				NewSocket->RelativeScale = MergeSocket->RelativeScale;
				NewSocket->bForceAlwaysAnimated = MergeSocket->bForceAlwaysAnimated;

				// only add the socket if a socket with a different name exists
				if (!MergeMesh->Skeleton->FindSocket(MergeSocket->SocketName))
				{
					MergeMesh->Skeleton->Sockets.Add(NewSocket);	
				}
				// else
				// {
				// 	UE_LOG(LogTemp, Warning, TEXT("Socket already exists `%s`, it will not be added"), *MergeSocket->SocketName.ToString());
				// }
			}
		}
	}
	else
	{
		for (const TObjectPtr<USkeletalMeshSocket>& MergeSocket : SocketsToCopy)
		{
			USkeletalMeshSocket* NewSocket = NewObject<USkeletalMeshSocket>(MergeMesh);
			if (NewSocket != nullptr)
			{				
				// Copy over all socket information
				NewSocket->SocketName = MergeSocket->SocketName;
				NewSocket->BoneName = MergeSocket->BoneName;
				NewSocket->RelativeLocation = MergeSocket->RelativeLocation;
				NewSocket->RelativeRotation = MergeSocket->RelativeRotation;
				NewSocket->RelativeScale = MergeSocket->RelativeScale;
				NewSocket->bForceAlwaysAnimated = MergeSocket->bForceAlwaysAnimated;
				
				MergeMesh->AddSocket(NewSocket, false);
			}
		}
	}
}

void BuildSockets(USkeletalMesh* MergeMesh, const TArray<USkeletalMesh*>& SkeletalMeshes)
{
	TArray<TObjectPtr<USkeletalMeshSocket>>& MeshSocketList = MergeMesh->GetMeshOnlySocketList();
	MeshSocketList.Empty();

	// Iterate through the all the source MESH sockets, only adding the new sockets.

	for (USkeletalMesh const * const SourceMesh : SkeletalMeshes)
	{
		if (SourceMesh)
		{
			const TArray<USkeletalMeshSocket*>& NewMeshSocketList = SourceMesh->GetMeshOnlySocketList();
			AddSockets(MergeMesh, NewMeshSocketList, false);
		}
	}

	// Iterate through the all the source SKELETON sockets, only adding the new sockets.

	for (USkeletalMesh const * const SourceMesh : SkeletalMeshes)
	{
		if (SourceMesh && SourceMesh->GetSkeleton())
		{
			const TArray<USkeletalMeshSocket*>& NewSkeletonSocketList = SourceMesh->GetSkeleton()->Sockets;
			AddSockets(MergeMesh, NewSkeletonSocketList, true);
		}
	}

	MergeMesh->RebuildSocketMap();
}

void BuildReferenceSkeleton(const TArray<USkeletalMesh*>& SourceMeshList, FReferenceSkeleton& RefSkeleton, const USkeleton* Skeleton)
{
	RefSkeleton.Empty(); // Clears all data

	// Iterate through all source mesh reference skeletons and compose the merged reference skeleton

	FReferenceSkeletonModifier RefSkelModifier(RefSkeleton, Skeleton);

	for (int32 SKMIndex = 0; SKMIndex < SourceMeshList.Num(); SKMIndex++)
	{
		USkeletalMesh* SourceMesh = SourceMeshList[SKMIndex];

		if (!SourceMesh)
		{
			// SourceMesh entries can be empty
			continue;
		}

		// If the reference skeleton is the same skeleton as the SourceMesh then we skip any building
		if (&RefSkeleton == &SourceMesh->GetRefSkeleton())
		{
			continue;
		}
		
		// Initialise new RefSkeleton with first mesh.
		
		if (RefSkeleton.GetRawBoneNum() == 0)
		{
			RefSkeleton = SourceMesh->GetRefSkeleton();	// copies the returned reference data, not a reference binding.
			continue;
		}

		// For subsequent meshes, add any missing bones

		for (int32 i=1; i < SourceMesh->GetRefSkeleton().GetRawBoneNum(); ++i)
		{
			FName SourceBoneName = SourceMesh->GetRefSkeleton().GetBoneName(i);
			int32 TargetBoneIndex = RefSkeleton.FindBoneIndex(SourceBoneName);

			// If the source bone is present in the new RefSkeleton, we skip it.

			if (TargetBoneIndex != INDEX_NONE)
			{
				continue;
			}

			// Add the source bone to the RefSkeleton
			
			int32 SourceParentIndex = SourceMesh->GetRefSkeleton().GetParentIndex(i);
			FName SourceParentName = SourceMesh->GetRefSkeleton().GetBoneName(SourceParentIndex);
			int32 TargetParentIndex = RefSkeleton.FindBoneIndex(SourceParentName);

			if (TargetParentIndex == INDEX_NONE)
			{
				continue;
			}

			FMeshBoneInfo MeshBoneInfo = SourceMesh->GetRefSkeleton().GetRefBoneInfo()[i];
			MeshBoneInfo.ParentIndex = TargetParentIndex;

			// If the bone and parent already exist in refskel, dont add it.
			if (RefSkelModifier.FindBoneIndex(SourceBoneName) != INDEX_NONE)
			{
				UE_LOG(LogTemp, Display, TEXT("Skipping: Joint Name: %s  Parent Index: %i"), *MeshBoneInfo.Name.ToString(), MeshBoneInfo.ParentIndex);
				continue;
			}
			
			RefSkelModifier.Add(MeshBoneInfo, SourceMesh->GetRefSkeleton().GetRefBonePose()[i]);
			UE_LOG(LogTemp, Display, TEXT("Adding: Joint Name: %s  Parent Index: %i"), *MeshBoneInfo.Name.ToString(), MeshBoneInfo.ParentIndex);
		}
	}
}

/**
 * Copies the render data into the Mesh Model (import data), so it can be loaded up at the start of the next session.
 *
 * Without running this the data will not be persistant, and will disappear next session.
 */
void PopulateSKMImportModel(USkeletalMesh* SkeletalMesh, TArray<USkeletalMesh*> SrcMeshes)
{
	FSkeletalMeshModel* skMeshModel = SkeletalMesh->GetImportedModel();
	{
		FSkeletalMeshRenderData* renderData = SkeletalMesh->GetResourceForRendering();
		for (int32 lodIdx = 0; lodIdx < renderData->LODRenderData.Num(); lodIdx++)
		{
			FSkeletalMeshLODModel* skMeshLODModel = new FSkeletalMeshLODModel();
			{
				FSkeletalMeshLODRenderData* mainLODRenderData = &renderData->LODRenderData[lodIdx];

				TArray<uint32> indexBuffer;
				indexBuffer.Reserve(mainLODRenderData->MultiSizeIndexContainer.GetIndexBuffer()->Num());
				for (int32 i = 0; i < mainLODRenderData->MultiSizeIndexContainer.GetIndexBuffer()->Num(); i++)
				{
					indexBuffer.Add(mainLODRenderData->MultiSizeIndexContainer.GetIndexBuffer()->Get(i));
				}

				TArray<FSkelMeshSection> sections;
				sections.Reserve(mainLODRenderData->RenderSections.Num());
				for (int32 i = 0; i < mainLODRenderData->RenderSections.Num(); i++)
				{
					FSkelMeshRenderSection& renderSection = mainLODRenderData->RenderSections[i];
					FSkelMeshSection section;
					section.BaseIndex = renderSection.BaseIndex;
					section.BaseVertexIndex = renderSection.BaseVertexIndex;
					section.bCastShadow = renderSection.bCastShadow;
					section.bDisabled = renderSection.bDisabled;
					section.BoneMap = renderSection.BoneMap;
					section.bRecomputeTangent = renderSection.bRecomputeTangent;
					section.bSelected = false;
					section.ClothingData = renderSection.ClothingData;
					section.ClothMappingDataLODs = renderSection.ClothMappingDataLODs;
					section.CorrespondClothAssetIndex = renderSection.CorrespondClothAssetIndex;
					section.GenerateUpToLodIndex = -1;
					section.MaterialIndex = renderSection.MaterialIndex;

					//FSkeletalMeshMerge doesn't set the MaxBoneInfluences field correctly.
					//section.MaxBoneInfluences = renderSection.MaxBoneInfluences;
					section.MaxBoneInfluences = mainLODRenderData->GetSkinWeightVertexBuffer()->GetMaxBoneInfluences();

					section.NumTriangles = renderSection.NumTriangles;
					section.NumVertices = renderSection.NumVertices;

					TArray<FSoftSkinVertex>& dstVertices{ section.SoftVertices };
					for (auto vertexIndex = renderSection.BaseVertexIndex; vertexIndex < renderSection.BaseVertexIndex + renderSection.GetNumVertices(); ++vertexIndex)
					{
						dstVertices.Emplace();
						auto& dstVertex = dstVertices.Last();

						dstVertex.Position = mainLODRenderData->StaticVertexBuffers.PositionVertexBuffer.VertexPosition(vertexIndex);
						dstVertex.TangentX = mainLODRenderData->StaticVertexBuffers.StaticMeshVertexBuffer.VertexTangentX(vertexIndex);
						dstVertex.TangentY = mainLODRenderData->StaticVertexBuffers.StaticMeshVertexBuffer.VertexTangentY(vertexIndex);
						dstVertex.TangentZ = mainLODRenderData->StaticVertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(vertexIndex);
						FMemory::Memzero(dstVertex.UVs);
						for (uint32 j = 0; j < mainLODRenderData->StaticVertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords(); j++)
						{
							dstVertex.UVs[j] = mainLODRenderData->StaticVertexBuffers.StaticMeshVertexBuffer.GetVertexUV(vertexIndex, j);
						}
						// dstVertex.Color = mainLODRenderData->StaticVertexBuffers.ColorVertexBuffer.VertexColor(vertexIndex);
						dstVertex.Color = FColor::White;
						
						FMemory::Memzero(dstVertex.InfluenceBones);
						FMemory::Memzero(dstVertex.InfluenceWeights);


						const auto& SrcSkinWeights{ mainLODRenderData->GetSkinWeightVertexBuffer()->GetVertexSkinWeights(vertexIndex) };

						for (uint32 l = 0; l < MAX_TOTAL_INFLUENCES; ++l)
						{
							dstVertex.InfluenceBones[l] = SrcSkinWeights.InfluenceBones[l];
							dstVertex.InfluenceWeights[l] = SrcSkinWeights.InfluenceWeights[l];
						}

						//overlapping vertices. This code IS working, but FSkeletalMeshMerge doesn't copy this data currently, because 
						//FDuplicatedVerticesBuffer::bHasOverlappingVertices is not serialized and therefore _always_ false.
						//In addition, FSkeletalMeshMerge has TWO additional bugs in that regard: It reads uninitialized data, and does not set the Length field in FIndexLengthPair.
						//
						//I decided against modifying the engine here, because we _currently_ do not use the OverlappingVertices data anyhow.
						//
						//If you feel inclined to fix FSkeletalMeshMerge's behaviour, here are some tips:
						//
						//instead of       *((uint32*)(VertData + i * sizeof(uint32))) += CurrentBaseVertexIndex - MergeSectionInfo.Section->BaseVertexIndex;
						//rather write     *((uint32*)(VertData + i * sizeof(uint32))) = *((uint32*)(SrcVertData + (i-StartIndex) * sizeof(uint32))) + CurrentBaseVertexIndex - MergeSectionInfo.Section->BaseVertexIndex;
						//where            const uint8* const SrcVertData = const_cast<FSkelMeshRenderSection *>(MergeSectionInfo.Section)->DuplicatedVerticesBuffer.DupVertData.GetDataPointer();
						//and instead of   ((FIndexLengthPair*)(IndexData + i * sizeof(FIndexLengthPair)))->Index += StartIndex;
						//better use       ((FIndexLengthPair*)(IndexData + i * sizeof(FIndexLengthPair)))->Index = ((FIndexLengthPair*)(SrcIndexData + (i - StartVertex) * sizeof(FIndexLengthPair)))->Index + StartIndex;
						//and              ((FIndexLengthPair*)(IndexData + i * sizeof(FIndexLengthPair)))->Length = ((FIndexLengthPair*)(SrcIndexData + (i - StartVertex) * sizeof(FIndexLengthPair)))->Length;
						//where            const uint8* const SrcIndexData = const_cast<FSkelMeshRenderSection *>(MergeSectionInfo.Section)->DuplicatedVerticesBuffer.DupVertIndexData.GetDataPointer();
						if (renderSection.DuplicatedVerticesBuffer.bHasOverlappingVertices)
						{
							const auto vertexInSection{ dstVertices.Num() - 1 };
							const auto dupVertIndexStride{ renderSection.DuplicatedVerticesBuffer.DupVertIndexData.GetStride() };
							const auto * const dupVertIndexPtr{ renderSection.DuplicatedVerticesBuffer.DupVertIndexData.GetDataPointer() };

							auto indexAndCount{ reinterpret_cast<const FIndexLengthPair * const>(dupVertIndexPtr + vertexInSection * dupVertIndexStride) };
							for (uint32 m = 0; m < indexAndCount->Length; ++m)
							{
								const auto dupVertStride{ renderSection.DuplicatedVerticesBuffer.DupVertData.GetStride() };
								const auto dupVertPtr{ renderSection.DuplicatedVerticesBuffer.DupVertData.GetDataPointer() };
								section.OverlappingVertices.FindOrAdd(vertexInSection).Add(*reinterpret_cast<const uint32 * const>(dupVertPtr + (indexAndCount->Index + m) *dupVertStride));
							}
						}
					}
					sections.Add(section);
				}

				//------------------------------------
				skMeshLODModel->ActiveBoneIndices = mainLODRenderData->ActiveBoneIndices;
				skMeshLODModel->IndexBuffer = indexBuffer;
				skMeshLODModel->MaxImportVertex = mainLODRenderData->GetNumVertices();
				//skMeshLODModel->MeshToImportVertexMap = ;
				skMeshLODModel->NumTexCoords = mainLODRenderData->GetNumTexCoords();
				skMeshLODModel->NumVertices = mainLODRenderData->GetNumVertices();
				//skMeshLODModel->RawPointIndices = ;
				//skMeshLODModel->RawSkeletalMeshBulkData = ;
				skMeshLODModel->RequiredBones = mainLODRenderData->RequiredBones;
				skMeshLODModel->Sections = sections;

				TMap<FName, FImportedSkinWeightProfileData> mergedImportedSkinWeightProfile;
				for (auto& meshToMerge : SrcMeshes)
				{
					mergedImportedSkinWeightProfile.Append(meshToMerge->GetImportedModel()->LODModels[lodIdx].SkinWeightProfiles);
				}
				for (const FSkinWeightProfileInfo& profile : SkeletalMesh->GetSkinWeightProfiles())
				{
					skMeshLODModel->SkinWeightProfiles.Add(profile.Name, *mergedImportedSkinWeightProfile.Find(profile.Name));
				}
			}
			skMeshModel->LODModels.Add(skMeshLODModel);
		}
	}
}


void SetupMorphTargets(TArray<USkeletalMesh*> SKMSources)
{
	// To add a Morph Target to a USkeletonMesh
	// RegisterMorphTarget(UMorphTarget* MorphTarget, bool bInvalidateRenderData)

	for (int32 i = 0; i < SKMSources.Num(); i++)
	{
		USkeletalMesh* SourceSKM{ SKMSources[i] };

		TArray<TObjectPtr<UMorphTarget>> MorphTargets = SourceSKM->GetMorphTargets();

		for (int32 MTIndex = 0; MTIndex < MorphTargets.Num(); MTIndex++)
		{
			auto MTName = MorphTargets[MTIndex]->GetName();
			auto MorphLODs = MorphTargets[MTIndex]->GetMorphLODModels();
			
			UE_LOG(LogTemp, Warning, TEXT("Morph Target Name: %s -- Vertex Count: %d"), *MTName, MorphLODs[0].NumVertices);

			if (MorphLODs.Num() == 0)
			{
				continue;
			}
			else if (MorphLODs.Num() > 1)
			{
				UE_LOG(LogTemp, Error, TEXT("Current SK design only takes into account LOD 0, other lods are ignored."));
			}

			FMorphTargetLODModel MTLOD = MorphLODs[0];
		}
		
	}
}

using VertexDataType = TGPUSkinVertexFloat16Uvs<1>;


/**
 * Updates the Skeleton joint of an attachment, by offsetting it.
 *
 * Note: The reason most data is converted to Matrices, is they are easier to work with as transforms
 * Add location data after the rotation, which can result in very perculiar results.
 * 
 * @param InSourceMesh Attachment Part
 
 * @param MorphTargetOffsets Offset transforms for each attachment type.
 */
void ApplySkeletonAttachmentOffset(USkeletalMesh* InSourceMesh, TMap<FString, FTransform> MorphTargetOffsets)
{
	TMap<FString, FString> AttachmentCodes = {
		{"AttachmentBack","Attachment Back"},
		{"AttachmentShoulderLeft", "Attachment Shoulder Left"},
		{"AttachmentShoulderRight","Attachment Shoulder Right"},		
		{"AttachmentElbowLeft", "Attachment Elbow Left"},
		{"AttachmentElbowRight","Attachment Elbow Right"},
		{"AttachmentHipsFront","Attachment Hips Front"},
		{"AttachmentHipsBack","Attachment Hips Back"},
		{"AttachmentHipsLeft","Attachment Hips Left"},
		{"AttachmentHipsRight","Attachment Hips Right"},
		{"AttachmentKneeLeft","Attachment Knee Left"},
		{"AttachmentKneeRight","Attachment Knee Right"}
	};

	TMap<FString, FVector> ConversionMultiplier = {
		{"AttachmentBack",FVector(1,1,-1)},
		{"AttachmentShoulderLeft",FVector(1,1,1)},
		{"AttachmentShoulderRight",FVector(-1,-1,1)},		
		{"AttachmentHipsFront",FVector(1,1,1)},
		{"AttachmentElbowLeft",FVector(1.0,1.0,-1.0)},
		{"AttachmentElbowRight",FVector(1,-1,1)},
		{"AttachmentHipsBack",FVector(1,1,1)},
		{"AttachmentHipsLeft",FVector(1,1,1)},
		{"AttachmentHipsRight",FVector(1,1,1)},
		{"AttachmentKneeLeft",FVector(-1,1,-1)},
		{"AttachmentKneeRight",FVector(1,1,-1)}
	};
	
	const TArray<USkeletalMeshSocket*>& SocketList = InSourceMesh->GetSkeleton()->Sockets;

	FReferenceSkeletonModifier RefSkelModifier(InSourceMesh->GetRefSkeleton(), InSourceMesh->Skeleton);
	
	for (const TObjectPtr<USkeletalMeshSocket>& AttachSocket : SocketList)
	{
		FString AttachName = AttachSocket->SocketName.ToString();

		if (!AttachmentCodes.Contains(AttachName))
		{
			continue;
		}
		
		FName BoneName = AttachSocket->BoneName;
		int32 BoneIndex = InSourceMesh->GetRefSkeleton().FindBoneIndex(BoneName);
		// Gets the Offset transform for the part for the morph target.
		FTransform OffsetTrans = MorphTargetOffsets[AttachmentCodes[AttachName]];
		// The transform conversion, this is required due to the space going from
		// Local Unity bone rotation to Unreal.
		FVector ConversionVector = ConversionMultiplier[AttachName];
		
		FVector BoneOffset = OffsetTrans.GetLocation() * ConversionVector;
		FTransform BoneOffsetTrans;
		BoneOffsetTrans.SetLocation(BoneOffset);
		FMatrix BoneOffsetMtx = BoneOffsetTrans.ToMatrixNoScale();
		
		FTransform LocalBoneTransform = InSourceMesh->GetRefSkeleton().GetRefBonePose()[BoneIndex];
		FMatrix LocalMtx = LocalBoneTransform.ToMatrixNoScale();

		// Offsets the position of the Bone by the offset amount from the morph target offset dictionary.
		BoneOffsetMtx = BoneOffsetMtx * LocalMtx;
		
		// Converts the Matrix into a transform.
		LocalBoneTransform = FTransform::Identity;
		LocalBoneTransform.SetFromMatrix(BoneOffsetMtx);

		RefSkelModifier.UpdateRefPoseTransform(BoneIndex, LocalBoneTransform);
	}
}


/**
 * Bakes all blendshape data onto the vertex provided 
 * 
 * @param InSourceMesh 
 * @param VertexIndex Index position of the blendshape delta that will be read 
 * @param OutVertex Vertex Data that will have the offset applied to it
 * @param Weight Blendshape names and there associated weight's that will be applied. 
 */
void ApplyMorphTarget(const USkeletalMesh* InSourceMesh, uint32 VertexIndex, VertexDataType& OutVertex, TMap<FString,float> MorphWeights)
{
	if (MorphWeights.IsEmpty())
	{
		return;
	}

	TMap<FName, int32> MorphIndexMap = InSourceMesh->GetMorphTargetIndexMap();
	
	for (auto WeightEntry : MorphWeights)
	{
		if (WeightEntry.Value == 0.f)
		{
			continue;
		}
		
		FName MorphTargetName = FName(WeightEntry.Key);
	
		if (MorphIndexMap.IsEmpty() || !MorphIndexMap.Find(MorphTargetName))
		{
			UE_LOG(LogTemp,Warning, TEXT("[%s] Morph Target not found: %s"), *InSourceMesh->GetName(), *MorphTargetName.ToString());
			continue;
		}

		int32 MTIndex = MorphIndexMap[MorphTargetName];
		
		TArray<TObjectPtr<UMorphTarget>> MorphTargets = InSourceMesh->GetMorphTargets();
		
		auto MTName = MorphTargets[MTIndex]->GetName();
		auto MorphLODs = MorphTargets[MTIndex]->GetMorphLODModels();
			
		UE_LOG(LogTemp, Warning, TEXT("Morph Target Name: %s -- Vertex Count: %d"), *MTName, MorphLODs[0].NumVertices);

		if (MorphLODs.Num() == 0)
		{
			UE_LOG(LogTemp, Error, TEXT("No Morph LODs found."));
			continue;
		}
		else if (MorphLODs.Num() > 1)
		{
			UE_LOG(LogTemp, Error, TEXT("Current SK design only takes into account LOD 0, other lods are ignored."));
		}

		FMorphTargetLODModel MTLOD = MorphLODs[0];

		for (auto VertMorph : MTLOD.Vertices)
		{
			if (VertMorph.SourceIdx == VertexIndex)
			{
				OutVertex.Position += VertMorph.PositionDelta * WeightEntry.Value;
				break;		
			}
		}		
	}
}

bool IsPartAttachment(FString PartName)
{
	if (PartName.IsEmpty())
	{
		return false;
	}

	TArray<FString> AttachmentNames = {
		"22AHED",
		"23AFAC",
		"24ABAC",
		"29ASHL", "30ASHR",		
		"31AEBL", "32AEBR",
		"25AHPF",
		"26AHPB",
		"27AHPL", "28AHPR",
		"33AKNL", "34AKNR"
	};

	for (auto AttachCode : AttachmentNames)
	{
		if (PartName.Contains(AttachCode))
		{
			return true;
		}
	}
	return false;
}


/**
 * Copies Morph Targets from one SKM to another.
 *
 * VertexOffset is required to make sure that the merged SKM's have the vertex index aligned correctly.
 * 
 * @param SourceMesh The Mesh that will have the MorphTargets that will be copied.
 * @param OutMesh Will have new Morph Targets added to it
 * @param VertexOffset The amount of vertices between the last mesh being added
 * @param IgnoreNames Names of Morph Targets that will not get added
 */
void GetMorphTargets(const USkeletalMesh* SourceMesh, USkeletalMesh* OutMesh, int32 VertexOffset, const TArray<FString> IgnoreNames)
{
	if (SourceMesh->MorphTargets.IsEmpty())
	{
		return;
	}

	int32 LODIndex = 0;
	
	auto SourceMorphTargets = SourceMesh->GetMorphTargets();
	for (int MorphIndex = 0; MorphIndex < SourceMorphTargets.Num(); ++MorphIndex)
	{
		UMorphTarget* SourceMorphTarget = SourceMorphTargets[MorphIndex];
		UMorphTarget* ToUpdateMorphTarget = nullptr;

		// if morph target name appears in the ignore list skip it.
		if (!IgnoreNames.IsEmpty() && IgnoreNames.Contains(SourceMorphTarget->GetName()))
		{
			continue;
		}
		
		// Check if Morph Target exist already in Target Mesh
		for (UMorphTarget* MorphTarget : OutMesh->GetMorphTargets())
		{
			if (MorphTarget->GetName() == SourceMorphTarget->GetName())
			{
				ToUpdateMorphTarget = MorphTarget;
				break;
			}
		}

		// Create Morph Target entry as it does not exist
		if (!ToUpdateMorphTarget)
		{
			ToUpdateMorphTarget = NewObject<UMorphTarget>(OutMesh->GetOuter(), FName(SourceMorphTarget->GetName()));
			check(ToUpdateMorphTarget);
			
			ToUpdateMorphTarget->BaseSkelMesh = OutMesh;
			OutMesh->GetMorphTargets().Add(ToUpdateMorphTarget);
		}

		// Populate the Morph Target with the delta data
		
		FSkeletalMeshLODModel& LODModel = OutMesh->GetImportedModel()->LODModels[0];

		// Number of Deltas
		int32 DeltaNum = -1;
		TArray<FMorphTargetDelta> MorphDeltas;
		const FMorphTargetDelta* SrcDeltas = SourceMorphTarget->GetMorphTargetDelta(LODIndex,DeltaNum);
		
		MorphDeltas.AddUninitialized(DeltaNum);
		FMemory::Memcpy(MorphDeltas.GetData(), SrcDeltas, sizeof(FMorphTargetDelta) * DeltaNum);

		// Loop over each delta and adds the offset vertex amount.
		for (int i = 0; i < MorphDeltas.Num(); ++i)
		{
			MorphDeltas[i].SourceIdx += VertexOffset;
		}
		
		// Check if MorphTarget already exists, if so add the new deltas to the existing deltas
		const FMorphTargetDelta* ExistingDeltasPntr = ToUpdateMorphTarget->GetMorphTargetDelta(LODIndex,DeltaNum);
		if (DeltaNum > 0)
		{
			TArray<FMorphTargetDelta> ExistingMorphTargets;
			
			ExistingMorphTargets.AddUninitialized(DeltaNum);
			FMemory::Memcpy(ExistingMorphTargets.GetData(), ExistingDeltasPntr, sizeof(FMorphTargetDelta) * DeltaNum);

			ExistingMorphTargets.Append(MorphDeltas);
			ToUpdateMorphTarget->PopulateDeltas(ExistingMorphTargets, LODIndex, LODModel.Sections, true);
		}
		else
		{
			ToUpdateMorphTarget->PopulateDeltas(MorphDeltas, LODIndex, LODModel.Sections, true);
		}
		
		ToUpdateMorphTarget->MarkPackageDirty();
	}
}


void GenerateMergeRenderData(USkeletalMesh* MergeMesh, TArray<USkeletalMesh*> SKMParts, UMaterialInterface* BaseMaterial, bool bKeepFacialMorphTargets, bool bKeepBodyMorphTargets, const TMap<FString, float>& MorphWeights, USKCharacterPreset* SKPreset)
{
	/** Array of source mesh info structs. */
	TArray<FMergeMeshInfo> SrcMeshInfo;

	/** Store all the new sections that will need to be added*/
	TArray<FNewSectionInfo> NewSectionArray;
	NewSectionArray.Empty();
	
	FReferenceSkeleton NewReferenceSkeleton;
	USkeleton* MergeSkeleton = MergeMesh->GetSkeleton();
	
	// Release the rendering resources.
	MergeMesh->ReleaseResources();
	MergeMesh->ReleaseResourcesFence.Wait();
	
	// Build the reference skeleton and sockets
	BuildReferenceSkeleton(SKMParts, NewReferenceSkeleton, MergeSkeleton);
	MergeMesh->SetRefSkeleton(NewReferenceSkeleton);
	
	BuildSockets(MergeMesh, SKMParts);
	
	// Rebuild inverse ref pose matrices here as some access patterns 
	// may need to access these matrices before FinalizeMesh is called
	// (which would *normally* rebuild the inv ref matrices).
	MergeMesh->GetRefBasesInvMatrix().Empty();
	MergeMesh->CalculateInvRefMatrices();

	// Release any resources the merge mesh might be holding.
	
	FSkeletalMeshRenderData* Resource = MergeMesh->GetResourceForRendering();
	if (Resource)
	{
		Resource->LODRenderData.Empty(1); // keep one as we only need the top most LOD
	}

	MergeMesh->ResetLODInfo();
	MergeMesh->GetMaterials().Empty();


	// -- Create bone mapping

	SrcMeshInfo.Empty();
	SrcMeshInfo.AddZeroed(SKMParts.Num());
	
	for (int32 MeshIdx=0; MeshIdx < SKMParts.Num(); MeshIdx++)
	{
		USkeletalMesh* SrcMesh = SKMParts[MeshIdx];

		if (SrcMesh->GetHasVertexColors())
		{
			MergeMesh->SetHasVertexColors(true);
			MergeMesh->SetVertexColorGuid(FGuid::NewGuid());
		}

		FMergeMeshInfo& MeshInfo = SrcMeshInfo[MeshIdx];
		MeshInfo.SrcToDestRefSkeletonMap.AddUninitialized(SrcMesh->GetRefSkeleton().GetRawBoneNum());

		for (int32 i = 0; i < SrcMesh->GetRefSkeleton().GetRawBoneNum(); i++)
		{
			FName SrcBoneName = SrcMesh->GetRefSkeleton().GetBoneName(i);
			int32 DestBoneIndex = NewReferenceSkeleton.FindBoneIndex(SrcBoneName);

			if (DestBoneIndex == INDEX_NONE)
			{
				// Missing bones shouldn't be possible, but can happen with invalid meshes;
				// map any bone we are missing to the 'root'.

				DestBoneIndex = 0;
			}

			MeshInfo.SrcToDestRefSkeletonMap[i] = DestBoneIndex;
		}
	}


	// -- UV data

	uint32 LODIndex = 0;
	uint32 NumUVSets = 0;
	uint32 MaxBoneInfluences = 0;
	bool bUse16BitBoneIndex = false;

	for (int32 MeshIdx=0; MeshIdx < SKMParts.Num(); MeshIdx++)
	{
		USkeletalMesh* SrcMesh = SKMParts[MeshIdx];

		FSkeletalMeshRenderData* SrcResource = SrcMesh->GetResourceForRendering();

		if (SrcResource->LODRenderData.IsValidIndex(LODIndex))
		{
			NumUVSets = FMath::Max(NumUVSets, SrcResource->LODRenderData[LODIndex].GetNumTexCoords());

			MaxBoneInfluences = FMath::Max(MaxBoneInfluences, SrcResource->LODRenderData[LODIndex].GetVertexBufferMaxBoneInfluences());
			bUse16BitBoneIndex |= SrcResource->LODRenderData[LODIndex].DoesVertexBufferUse16BitBoneIndex();
		}
	}


	// -- process each LOD for the new merged mesh
	
	MergeMesh->AllocateResourceForRendering();

	const FSkeletalMeshLODInfo* LODInfoPtr = MergeMesh->GetLODInfo(LODIndex);
	bool bUseFullPrecisionUVs = LODInfoPtr ? LODInfoPtr->BuildSettings.bUseFullPrecisionUVs : false;
	
	// -- Start of GenerateLODModel
	
	// add the new LOD model entry
	FSkeletalMeshRenderData* MergeResource = MergeMesh->GetResourceForRendering();
	check(MergeResource);

	FSkeletalMeshLODRenderData& MergeLODData = *new FSkeletalMeshLODRenderData;
	MergeResource->LODRenderData.Add(&MergeLODData);
	// add the new LOD info entry
	FSkeletalMeshLODInfo& MergeLODInfo = MergeMesh->AddLODInfo();
	MergeLODInfo.ScreenSize = MergeLODInfo.LODHysteresis = UE_MAX_FLT;

	MergeLODInfo.MorphTargetPositionErrorTolerance = 0.1f; // required for blendshapes to appear on reload
	MergeLODInfo.bAllowCPUAccess = true;// required for blendshapes to appear on reload
	MergeLODInfo.BuildSettings.bRecomputeNormals = false;
	MergeLODInfo.BuildSettings.bRecomputeTangents = false;
	
	// generate an array with info about new sections that need to be created
	const int32 MaxGPUSkinBones = FGPUBaseSkinVertexFactory::GetMaxGPUSkinBones();

	for (int32 MeshIdx=0; MeshIdx < SKMParts.Num(); MeshIdx++)
	{
		USkeletalMesh* SrcMesh = SKMParts[MeshIdx];
		
		FSkeletalMeshRenderData* SrcResource = SrcMesh->GetResourceForRendering();
		FSkeletalMeshLODRenderData& SrcLODData = SrcResource->LODRenderData[LODIndex];
		FSkeletalMeshLODInfo& SrcLODInfo = *(SrcMesh->GetLODInfo(LODIndex));

		// iterate over each section of this LOD
		for ( int32 SectionIdx=0; SectionIdx < SrcLODData.RenderSections.Num(); SectionIdx++)
		{
			int32 MaterialId = -1;

			FSkelMeshRenderSection& Section = SrcLODData.RenderSections[SectionIdx];

			// Convert Chunk.BoneMap from src to dest bone indices
			TArray<FBoneIndexType> DestChunkBoneMap;
			DestChunkBoneMap.Empty();
			DestChunkBoneMap.AddUninitialized(Section.BoneMap.Num());
			for(int32 i=0; i<Section.BoneMap.Num(); i++)
			{
				check(Section.BoneMap[i] < SrcMeshInfo[MeshIdx].SrcToDestRefSkeletonMap.Num());
				DestChunkBoneMap[i] = SrcMeshInfo[MeshIdx].SrcToDestRefSkeletonMap[Section.BoneMap[i]];
			}


			// get the material for this section
			int32 MaterialIndex = Section.MaterialIndex;
			// use the remapping of material indices if there is a valid value
			if(SrcLODInfo.LODMaterialMap.IsValidIndex(SectionIdx) && SrcLODInfo.LODMaterialMap[SectionIdx] != INDEX_NONE && SrcMesh->GetMaterials().Num() > 0)
			{
				MaterialIndex = FMath::Clamp<int32>( SrcLODInfo.LODMaterialMap[SectionIdx],0, SrcMesh->GetMaterials().Num() - 1);
			}

			const FSkeletalMaterial& SkeletalMaterial = SrcMesh->GetMaterials()[MaterialIndex];
			UMaterialInterface* Material = SkeletalMaterial.MaterialInterface;

			// see if there is an existing entry in the array of new sections that matches its material
			// if there is a match then the source section can be added to its list of sections to merge
			int32 FoundIdx = INDEX_NONE;
			// NOTE: This looks like a bug from the source code, that will never evaluate. Actually it will if called with more then 2 skms, using the macro
			// for (int32 Idx=0; Idx < NewSectionArray.Num(); Idx++)
			// {
			// 	
			// }

			// new section entries will be created if the material for the source section was not found
			// or merging it with an existing entry would go over the bone limit for the GPU Skinning
			if ( FoundIdx == INDEX_NONE )
			{
				// create a new section entry
				const FName& MaterialSlotName = FName(SrcMesh->GetName()); // sets the Material Slot's name to be Part name.
				const FMeshUVChannelInfo& UVChannelData = SkeletalMaterial.UVChannelData;
				FNewSectionInfo& NewSectionInfo = *new(NewSectionArray) FNewSectionInfo(Material, MaterialId, MaterialSlotName, UVChannelData, SrcMesh);
				// initialize the merged bonemap to simply use the original chunk bonemap
				NewSectionInfo.MergedBoneMap = DestChunkBoneMap;

				TArray<FTransform> SrcUVTransform;
				// add a new merge section entry
				FMergeSectionInfo& MergeSectionInfo = *new(NewSectionInfo.MergeSections) FMergeSectionInfo(
					SrcMesh,
					&SrcLODData.RenderSections[SectionIdx],
					SrcUVTransform);
				// since merged bonemap == chunk.bonemap then remapping is just a pass-through
				MergeSectionInfo.BoneMapToMergedBoneMap.Empty(DestChunkBoneMap.Num());
				for (int32 i=0; i<DestChunkBoneMap.Num(); i++)
				{
					MergeSectionInfo.BoneMapToMergedBoneMap.Add((FBoneIndexType)i);
				}
			}
		}
		
	}

	// -- back in GenerateLODModel


	// using VertexDataType = TGPUSkinVertexFloat16Uvs<1>;
	
	uint32 MaxIndex = 0;
	// merged vertex buffer
	TArray< VertexDataType > MergedVertexBuffer; // 1 = 1 UV set
	// merged skin weight buffer
	TArray< FSkinWeightInfo > MergedSkinWeightBuffer;
	// merged vertex color buffer
	TArray< FColor > MergedColorBuffer;
	// merged index buffer
	TArray< uint32 > MergedIndexBuffer;

	// the total number of UV sets for this LOD model
	uint32 TotalNumUVs = 0;

	uint32 SourceMaxBoneInfluences = 0;
	bool bSourceUse16BitBoneIndex = false;

	for ( int32 CreateIdx=0; CreateIdx < NewSectionArray.Num(); CreateIdx++)
	{
		FNewSectionInfo& NewSectionInfo = NewSectionArray[CreateIdx];

		// ActiveBoneIndices contains all the bones used by the verts from all the sections of this LOD model
		// Add the bones used by this new section
		for ( int32 Idx=0; Idx < NewSectionInfo.MergedBoneMap.Num(); Idx++)
		{
			MergeLODData.ActiveBoneIndices.AddUnique( NewSectionInfo.MergedBoneMap[Idx] );
		}
		
		// add the new section entry
		FSkelMeshRenderSection& Section = *new(MergeLODData.RenderSections) FSkelMeshRenderSection;

		// set the new bonemap from the merged sections
		// these are the bones that will be used by this new section
		Section.BoneMap = NewSectionInfo.MergedBoneMap;

		// init vert totals
		Section.NumVertices = 0;

		// keep track of the current base vertex for this section in the merged vertex buffer
		Section.BaseVertexIndex = MergedVertexBuffer.Num();

		// find existing material index
		int32 MatIndex = INDEX_NONE;

		if (MatIndex == INDEX_NONE)
		{
			// Set the Section to use the BaseMaterial
			FSkeletalMaterial SkeletalMaterial(BaseMaterial, true,false, NewSectionInfo.SlotName);
			SkeletalMaterial.UVChannelData = NewSectionInfo.UVChannelData;
			MergeMesh->GetMaterials().Add(SkeletalMaterial);

			Section.MaterialIndex = MergeMesh->GetMaterials().Num()-1;
		}

		// init tri totals
		Section.NumTriangles = 0;
		// keep track of the current base index for section in the merged index buffer
		Section.BaseIndex = MergedIndexBuffer.Num();

		FMeshUVChannelInfo& MergedUVData = MergeMesh->GetMaterials()[Section.MaterialIndex].UVChannelData;

		bool bIsAttachment = IsPartAttachment(NewSectionInfo.SkelMesh->GetName());
		
		// iterate over all sections that need to be merged together
		for ( int32 MergeIdx=0; MergeIdx < NewSectionInfo.MergeSections.Num(); MergeIdx++)
		{
			FMergeSectionInfo& MergeSectionInfo = NewSectionInfo.MergeSections[MergeIdx]; // array of new section to add, as each mesh sould be made up of multiple sections 
			int32 SourceLODIdx = LODIndex;

			// Take the max UV density for each UVChannel between all sections that are being merged.
			const int32 NewSectionMatId = MergeSectionInfo.Section->MaterialIndex;
			if(MergeSectionInfo.SkelMesh->GetMaterials().IsValidIndex(NewSectionMatId))
			{
				const FMeshUVChannelInfo& NewSectionUVData = MergeSectionInfo.SkelMesh->GetMaterials()[NewSectionMatId].UVChannelData;
				for (int32 i = 0; i < MAX_TEXCOORDS; i++)
				{
					const float NewSectionUVDensity = NewSectionUVData.LocalUVDensities[i];
					float& UVDensity = MergedUVData.LocalUVDensities[i];

					UVDensity = FMath::Max(UVDensity, NewSectionUVDensity);
				}
			}

			// get the source skeleton LOD info from this merge entry
			const FSkeletalMeshLODInfo& SrcLODInfo = *(MergeSectionInfo.SkelMesh->GetLODInfo(LODIndex));

			// keep track of the lowest LOD displayfactor and hysteresis
			MergeLODInfo.ScreenSize.Default = FMath::Min<float>(MergeLODInfo.ScreenSize.Default, SrcLODInfo.ScreenSize.Default);
#if WITH_EDITORONLY_DATA
			for(const TPair<FName, float>& PerPlatform : SrcLODInfo.ScreenSize.PerPlatform)
			{
				float* Value = MergeLODInfo.ScreenSize.PerPlatform.Find(PerPlatform.Key);
				if(Value)
				{
					*Value = FMath::Min<float>(PerPlatform.Value, *Value);	
				}
				else
				{
					MergeLODInfo.ScreenSize.PerPlatform.Add(PerPlatform.Key, PerPlatform.Value);
				}
			}
#endif
			MergeLODInfo.BuildSettings.bUseFullPrecisionUVs |= SrcLODInfo.BuildSettings.bUseFullPrecisionUVs;
			MergeLODInfo.BuildSettings.bUseBackwardsCompatibleF16TruncUVs |= SrcLODInfo.BuildSettings.bUseBackwardsCompatibleF16TruncUVs;
			MergeLODInfo.BuildSettings.bUseHighPrecisionTangentBasis |= SrcLODInfo.BuildSettings.bUseHighPrecisionTangentBasis;

			MergeLODInfo.LODHysteresis = FMath::Min<float>(MergeLODInfo.LODHysteresis,SrcLODInfo.LODHysteresis);

			// get the source skeleton LOD model from this merge entry
			const FSkeletalMeshLODRenderData& SrcLODData = MergeSectionInfo.SkelMesh->GetResourceForRendering()->LODRenderData[SourceLODIdx];

			// add required bones from this source model entry to the merge model entry
			for (int32 Idx=0; Idx < SrcLODData.RequiredBones.Num(); Idx++)
			{
				FName SrcLODBoneName = MergeSectionInfo.SkelMesh->GetRefSkeleton().GetBoneName(SrcLODData.RequiredBones[Idx]);
				int32 MergeBoneIndex = NewReferenceSkeleton.FindBoneIndex(SrcLODBoneName);

				if (MergeBoneIndex != INDEX_NONE)
				{
					MergeLODData.RequiredBones.AddUnique(MergeBoneIndex);
				}
			}

			// update vert total
			Section.NumVertices += MergeSectionInfo.Section->NumVertices;

			// update total number of vertices
			const int32 NumTotalVertices = MergeSectionInfo.Section->NumVertices;

			// add the vertices from the original source mesh to the merged vertex buffer
			const int32 MaxVertIdx = FMath::Min<int32>(
				MergeSectionInfo.Section->BaseVertexIndex + NumTotalVertices,
				SrcLODData.StaticVertexBuffers.PositionVertexBuffer.GetNumVertices());

			// The amount of colour vertices that currently exist on the 'source skm lod's section'
			const int32 MaxColorIdx = SrcLODData.StaticVertexBuffers.ColorVertexBuffer.GetNumVertices();

			// update max number of influences
			MaxBoneInfluences = SrcLODData.GetSkinWeightVertexBuffer()->GetMaxBoneInfluences();
			bUse16BitBoneIndex = SrcLODData.GetSkinWeightVertexBuffer()->Use16BitBoneIndex();
			
			SourceMaxBoneInfluences = FMath::Max(SourceMaxBoneInfluences, MaxBoneInfluences);
			bSourceUse16BitBoneIndex |= bUse16BitBoneIndex;

			// update RenderSection number of max influences
			Section.MaxBoneInfluences = MaxBoneInfluences;

			// update total number of TexCoords
			const uint32 LODNumTexCoords = SrcLODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords();
			if (TotalNumUVs < LODNumTexCoords)
			{
				TotalNumUVs = LODNumTexCoords;
			}

			// keep track of the current base vertex index before adding any new vertices
			// this will be needed to remap the index buffer values to the new range
			const int32 CurrentBaseVertexIndex = MergedVertexBuffer.Num();

			for ( int32 VertexIdx=MergeSectionInfo.Section->BaseVertexIndex; VertexIdx<MaxVertIdx; VertexIdx++)
			{
				// add the new vertex
				VertexDataType& DestVert = MergedVertexBuffer[MergedVertexBuffer.AddUninitialized()];
				FSkinWeightInfo& DestWeight = MergedSkinWeightBuffer[MergedSkinWeightBuffer.AddUninitialized()];

				// -- CopyVertexFromSource<VertexDataType>(DestVert, SrcLODData, VertIdx, MergeSectionInfo);
				{
					DestVert.Position = SrcLODData.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(VertexIdx);
					DestVert.TangentX = SrcLODData.StaticVertexBuffers.StaticMeshVertexBuffer.VertexTangentX(VertexIdx);
					DestVert.TangentZ = SrcLODData.StaticVertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(VertexIdx);
					
					ApplyMorphTarget(NewSectionInfo.SkelMesh, VertexIdx, DestVert, MorphWeights);

					// NOTE: This has been commented out, as we should be updating the bone position for the attachment locations
					// // todo: working on implementing vertex position offset
					// if (bIsAttachment)
					// {
					// 	ApplyAttachmentOffset(NewSectionInfo.SkelMesh, DestVert, SKPreset->MorphTargetOffsets);
					// }
					
					// Copy all UVs that are available
					uint32 NumTexCoords = SrcLODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords();
					const uint32 ValidLoopCount = FMath::Min(VertexDataType::NumTexCoords, NumTexCoords);
					for (uint32 UVIndex=0; UVIndex<ValidLoopCount; UVIndex++)
					{
						FVector2D UVs = FVector2D(SrcLODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetVertexUV_Typed<VertexDataType::StaticMeshVertexUVType>(VertexIdx, UVIndex));
						DestVert.UVs[UVIndex] = FVector2f(UVs);
					}

					// now just fill up zero value if we didn't reach the end
					for (uint32 UVIndex=ValidLoopCount; UVIndex < VertexDataType::NumTexCoords; UVIndex++)
					{
						DestVert.UVs[UVIndex] = FVector2f::ZeroVector;
					}
					// -- end
				}


				DestWeight = SrcLODData.GetSkinWeightVertexBuffer()->GetVertexSkinWeights(VertexIdx);

				// if the mesh uses vertex colors, copy the source color if possible or default to white
				if( MergeMesh->GetHasVertexColors() )
				{
					if( VertexIdx < MaxColorIdx )
					{
						const FColor& SrcColor = SrcLODData.StaticVertexBuffers.ColorVertexBuffer.VertexColor(VertexIdx);
						MergedColorBuffer.Add(SrcColor);
					}
					else
					{
						const FColor ColorWhite(255,255,255);
						MergedColorBuffer.Add(ColorWhite);
					}
				}

				// remap the bone index used by this vertex to match the mergedbonemap
				for ( uint32 Idx=0; Idx < MAX_TOTAL_INFLUENCES; Idx++)
				{
					if (DestWeight.InfluenceWeights[Idx] > 0)
					{
						checkSlow(MergeSectionInfo.BoneMapToMergedBoneMap.IsValidIndex(DestWeight.InfluenceBones[Idx]));
						DestWeight.InfluenceBones[Idx] = (FBoneIndexType)MergeSectionInfo.BoneMapToMergedBoneMap[DestWeight.InfluenceBones[Idx]];
					}
				}
			}

			// update total number of triangles
			Section.NumTriangles += MergeSectionInfo.Section->NumTriangles;

			// add the indices from the original source mesh to the merged index buffer
			const int32 MaxIndexIdx = FMath::Min<int32>(
				MergeSectionInfo.Section->BaseIndex + MergeSectionInfo.Section->NumTriangles * 3,
				SrcLODData.MultiSizeIndexContainer.GetIndexBuffer()->Num()
				);
			for (int32 IndexIdx = MergeSectionInfo.Section->BaseIndex; IndexIdx < MaxIndexIdx; IndexIdx++)
			{
				uint32 SrcIndex = SrcLODData.MultiSizeIndexContainer.GetIndexBuffer()->Get(IndexIdx);

				// add offset to each index to match the new entries in the merged vertex buffer
				checkSlow(SrcIndex >= MergeSectionInfo.Section->BaseVertexIndex);
				uint32 DstIndex = SrcIndex - MergeSectionInfo.Section->BaseVertexIndex + CurrentBaseVertexIndex;
				checkSlow(DstIndex < (uint32)MergedVertexBuffer.Num());

				// add the new index to the merged vertex buffer
				MergedIndexBuffer.Add(DstIndex);
				if (MaxIndex < DstIndex)
				{
					MaxIndex = DstIndex;
				}
			}


			{
                if (MergeSectionInfo.Section->DuplicatedVerticesBuffer.bHasOverlappingVertices)
                {
                    if (Section.DuplicatedVerticesBuffer.bHasOverlappingVertices)
                    {
                        // Merge
                        int32 StartIndex = Section.DuplicatedVerticesBuffer.DupVertData.Num();
                        int32 StartVertex = Section.DuplicatedVerticesBuffer.DupVertIndexData.Num();
                        Section.DuplicatedVerticesBuffer.DupVertData.ResizeBuffer(StartIndex + MergeSectionInfo.Section->DuplicatedVerticesBuffer.DupVertData.Num());
                        Section.DuplicatedVerticesBuffer.DupVertIndexData.ResizeBuffer(Section.NumVertices);
                    
                        uint8* VertData = Section.DuplicatedVerticesBuffer.DupVertData.GetDataPointer();
                        uint8* IndexData = Section.DuplicatedVerticesBuffer.DupVertIndexData.GetDataPointer();
                        for (int32 i = StartIndex; i < Section.DuplicatedVerticesBuffer.DupVertData.Num(); ++i)
                        {
                            *((uint32*)(VertData + i * sizeof(uint32))) += CurrentBaseVertexIndex - MergeSectionInfo.Section->BaseVertexIndex;
                        }
                        for (uint32 i = StartVertex; i < Section.NumVertices; ++i)
                        {
                            ((FIndexLengthPair*)(IndexData + i * sizeof(FIndexLengthPair)))->Index += StartIndex;
                        }
                    }
                    else
                    {
                        Section.DuplicatedVerticesBuffer.DupVertData = MergeSectionInfo.Section->DuplicatedVerticesBuffer.DupVertData;
                        Section.DuplicatedVerticesBuffer.DupVertIndexData = MergeSectionInfo.Section->DuplicatedVerticesBuffer.DupVertIndexData;
                        uint8* VertData = Section.DuplicatedVerticesBuffer.DupVertData.GetDataPointer();
                        for (int32 i = 0; i < Section.DuplicatedVerticesBuffer.DupVertData.Num(); ++i)
                        {
                            *((uint32*)(VertData + i * sizeof(uint32))) += CurrentBaseVertexIndex - MergeSectionInfo.Section->BaseVertexIndex;
                        }
                    }
                    Section.DuplicatedVerticesBuffer.bHasOverlappingVertices = true;
                }
                else
                {
                    Section.DuplicatedVerticesBuffer.DupVertData.ResizeBuffer(1);
                    Section.DuplicatedVerticesBuffer.DupVertIndexData.ResizeBuffer(Section.NumVertices);

                    uint8* VertData = Section.DuplicatedVerticesBuffer.DupVertData.GetDataPointer();
                    uint8* IndexData = Section.DuplicatedVerticesBuffer.DupVertIndexData.GetDataPointer();
                    
                    FMemory::Memzero(IndexData, Section.NumVertices * sizeof(FIndexLengthPair));
                    FMemory::Memzero(VertData, sizeof(uint32));
                }
            }
		}
	}
	
	// The asset will need to be accessed by the CPU
	const bool bNeedsCPUAccess = true;

	// sort required bone array in strictly increasing order
	MergeLODData.RequiredBones.Sort();
	MergeMesh->GetRefSkeleton().EnsureParentsExistAndSort(MergeLODData.ActiveBoneIndices);

	// copy the new vertices and indices to the vertex buffer for the new model
	MergeLODData.StaticVertexBuffers.StaticMeshVertexBuffer.SetUseFullPrecisionUVs(MergeLODInfo.BuildSettings.bUseFullPrecisionUVs);

	MergeLODData.StaticVertexBuffers.PositionVertexBuffer.Init(MergedVertexBuffer.Num(), bNeedsCPUAccess);
	MergeLODData.StaticVertexBuffers.StaticMeshVertexBuffer.Init(MergedVertexBuffer.Num(), TotalNumUVs, bNeedsCPUAccess);

	bool bUseBackwardsCompatibleF16TruncUVS = MergeLODInfo.BuildSettings.bUseBackwardsCompatibleF16TruncUVs;

	for (int i=0; i < MergedVertexBuffer.Num(); i++)
	{
		MergeLODData.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(i) = MergedVertexBuffer[i].Position;
		MergeLODData.StaticVertexBuffers.StaticMeshVertexBuffer.SetVertexTangents(i, MergedVertexBuffer[i].TangentX.ToFVector3f(), MergedVertexBuffer[i].GetTangentY(), MergedVertexBuffer[i].TangentZ.ToFVector3f());
		for (uint32 j=0; j < TotalNumUVs; j++)
		{
			MergeLODData.StaticVertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i,j,MergedVertexBuffer[i].UVs[j], bUseBackwardsCompatibleF16TruncUVS);
		}
	}

	MergeLODData.SkinWeightVertexBuffer.SetMaxBoneInfluences(SourceMaxBoneInfluences);
	MergeLODData.SkinWeightVertexBuffer.SetUse16BitBoneIndex(bSourceUse16BitBoneIndex);
	MergeLODData.SkinWeightVertexBuffer.SetNeedsCPUAccess(bNeedsCPUAccess);

	// copy vertex resource arrays
	MergeLODData.SkinWeightVertexBuffer = MergedSkinWeightBuffer;

	if ( MergeMesh->GetHasVertexColors() )
	{
		MergeLODData.StaticVertexBuffers.ColorVertexBuffer.InitFromColorArray(MergedColorBuffer);
	}

	const uint8 DataTypeSize = (MaxIndex < MAX_uint16) ? sizeof(uint16) : sizeof(uint32);
	MergeLODData.MultiSizeIndexContainer.RebuildIndexBuffer(DataTypeSize, MergedIndexBuffer);
	
	// -- end of GerenateLODModel
	
	// copy settings and bone info from src meshes
	bool bNeedsInit=true;

	for( int32 MeshIdx=0; MeshIdx < SKMParts.Num(); MeshIdx++ )
	{
		USkeletalMesh* SrcMesh = SKMParts[MeshIdx];
		if( SrcMesh )
		{
			if( bNeedsInit )
			{
				// initialize the merged mesh with the first src mesh entry used
				MergeMesh->SetImportedBounds(SrcMesh->GetImportedBounds());
				// only initialize once
				bNeedsInit = false;
			}
			else
			{
				// add bounds
				MergeMesh->SetImportedBounds(MergeMesh->GetImportedBounds() + SrcMesh->GetImportedBounds());
			}
		}
	}
	// Rebuild inverse ref pose matrices.
	MergeMesh->GetRefBasesInvMatrix().Empty();
	MergeMesh->CalculateInvRefMatrices();
	
	// Reinitialize the mesh's render resources.
	MergeMesh->InitResources();
	
	// //We need to have a valid render data to create physic asset
	// MergeMesh->Build();
	
	// Newly generated mesh, that will have the two SKMs merged
	if (!MergeMesh->GetSkeleton())
	{
		MergeMesh->SetSkeleton(MergeSkeleton);
	}
	
	MergeSkeleton->MergeAllBonesToBoneTree(MergeMesh);
	MergeSkeleton->SetPreviewMesh(MergeMesh);
	MergeSkeleton->PostEditChange();
	
	PopulateSKMImportModel(MergeMesh, SKMParts);

	// -- Morph Target Merge
	
	MergeMesh->PreEditChange(nullptr);
	
	int32 VertexOffset = 0;
	for ( int32 CreateIdx=0; CreateIdx < NewSectionArray.Num(); CreateIdx++)
	{
		TArray<FString> IgnoreNames;
		FNewSectionInfo& NewSectionInfo = NewSectionArray[CreateIdx];
		for (int i = 0; i < NewSectionInfo.MergeSections.Num(); ++i)
		{
			if (!bKeepFacialMorphTargets && !bKeepBodyMorphTargets)
			{
				// Remove all Morph Targets
				continue;
			}
			if (!bKeepFacialMorphTargets)
			{
				IgnoreNames = {
					"browFrownLeft",
					"eyeBlinkLowerLeft",
					"eyeWideLowerRight",
					"eyeSquintRight",
					"eyeWideUpperRight",
					"eyeWideLowerLeft",
					"eyeBlinkLowerRight",
					"eyeBlinkUpperLeft",
					"noseSneerLeft",
					"eyeBlinkUpperRight",
					"noseSneerRight",
					"browInnerUpRight",
					"browInnerUpLeft",
					"eyeSquintLeft",
					"mouthLeft",
					"jawRight",
					"jawOpen",
					"jawLeft",
					"jawForward",
					"browInnerDownLeft",
					"jawBackward",
					"mouthRight",
					"mouthClose",
					"browRaiseLeft",
					"browOuterUpLeft",
					"browOuterDownLeft",
					"browInnerDownRight",
					"eyeWideUpperLeft",
					"browFrownRight",
					"jawBackward",
					"mouthRight",
					"mouthClose",
					"mouthLeft",
					"jawForward",
					"jawLeft",
					"jawOpen",
					"jawRight",
					"cheekPuffLeft",
					"cheekPuffRight",
					"cheekSquintLeft",
					"eyeLookOutLeft",
					"cheekSquintRight",
					"mouthDimpleLeft",
					"mouthDimpleRight",
					"mouthFrownLeft",
					"eyeLookOutRight",
					"mouthFrownRight",
					"mouthFunnel",
					"eyeLookInRight",
					"mouthLowerDownLeft",
					"mouthLowerDownRight",
					"mouthPressLeft",
					"mouthPressRight",
					"mouthPucker",
					"eyeLookUpLeft",
					"mouthRollLower",
					"browOuterDownRight",
					"mouthRollOutLower",
					"mouthRollOutUpper",
					"mouthRollUpper",
					"mouthShrugLower",
					"mouthShrugUpper",
					"mouthSmileLeft",
					"mouthSmileRight",
					"mouthStretchLeft",
					"eyeLookUpRight",
					"mouthStretchRight",
					"mouthUpperUpLeft",
					"mouthUpperUpRight",
					"browOuterUpRight",
					"eyeLookDownLeft",
					"eyeLookDownRight",
					"eyeLookInLeft",
					"browRaiseRight",
					"cheekHollowLeft",
					"cheekHollowRight",
					"browOuterDownRight",
					"browOuterUpRight",
					"browRaiseRight",
					"irisSmallLeft",
					"irisSmallRight",
					"mouthRollOutUpper",
					"mouthShrugUpper",
					"mouthUpperUpLeft",
					"mouthUpperUpRight",
					"cheekHollowLeft",
					"cheekHollowRight",
					"cheekPuffLeft",
					"cheekPuffRight",
					"cheekSquintLeft",
					"cheekSquintRight",
					"mouthDimpleLeft",
					"mouthDimpleRight",
					"mouthFrownLeft",
					"mouthFrownRight",
					"mouthFunnel",
					"mouthLowerDownLeft",
					"mouthLowerDownRight",
					"mouthPressLeft",
					"mouthPressRight",
					"mouthPucker",
					"mouthRollLower",
					"mouthRollOutLower",
					"mouthRollUpper",
					"mouthShrugLower",
					"mouthSmileLeft",
					"mouthSmileRight",
					"mouthStretchLeft",
					"mouthStretchRight",
					"tongueCurlUp",
					"tongueDown",
					"tongueIn",
					"tongueLower",
					"tongueOut",
					"tongueRaise",
					"tongueSideCurlDown",
					"tongueSideCurlUp",
					"tongueTwistLeft",
					"tongueTwistRight",
					"tongueUp",
					"tongueCurlDown",
					"tongueCurlLeft",
					"tongueCurlRight",
				};
			}
			if (!bKeepBodyMorphTargets)
			{
				IgnoreNames = {"defaultBuff", "defaultSkinny", "defaultHeavy", "masculineFeminine"};
			}
			
			auto MergeSectionInfo = NewSectionInfo.MergeSections[i];
			GetMorphTargets(MergeSectionInfo.SkelMesh, MergeMesh, VertexOffset, IgnoreNames);
			VertexOffset += MergeSectionInfo.Section->NumVertices;
		}
	}
	
	MergeMesh->InitMorphTargetsAndRebuildRenderData();

	// Update Socket Bone positions
	ApplySkeletonAttachmentOffset(MergeMesh, SKPreset->MorphTargetOffsets);
	
	MergeSkeleton->MarkPackageDirty();
	MergeMesh->MarkPackageDirty();
	
	/**
	 * Sets the Remap Morph Target to true. This is required as Unreal will auto regenerate the Morph Targets as there
	 * is no import data. In doing so Unreal classifies the regenerated Morph Targets as being Remapped.
	 */
	MergeMesh->GetLODInfo(LODIndex)->ReductionSettings.bRemapMorphTargets = true;
	MergeMesh->GetLODInfo(LODIndex)->ReductionSettings.NumOfTrianglesPercentage = 1.0f;
	MergeMesh->GetLODInfo(LODIndex)->bHasBeenSimplified = true;
	/***/
	
	FAssetRegistryModule::AssetCreated(MergeSkeleton);
	FAssetRegistryModule::AssetCreated(MergeMesh);


}



/**
 * Popus up a dialog entry UI, allowing the user to specify where the generated SKM and SK files will go and there name.
 * 
 * @param SKPreset that will be used for its name and source location.
 * @param OutSaveObjectPath custom populated full name.
 */
void UMergeBPLibrary::SelectSkeletalPackagePath(USKCharacterPreset* SKPreset, FString& OutSaveObjectPath)
{
	if (!SKPreset)
	{
		return;
	}
	
	FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");

	// Create an appropriate and unique name 
	FString SKMName;
	FString SKMPackageName;

	FString Suffix = "_SKM";
	AssetToolsModule.Get().CreateUniqueAssetName(SKPreset->GetOutermost()->GetName(), Suffix, /*out*/ SKMPackageName, /*out*/ SKMName);
	
	// const FString DefaultPath = FPackageName::GetLongPackagePath(DefaultPackageName);
	// const FString DefaultName = FPackageName::GetShortName(DefaultPackageName);
	
	FSaveAssetDialogConfig SaveAssetDialogConfig;
	SaveAssetDialogConfig.DialogTitleOverride = LOCTEXT("CreateMergedSidekickCharacter", "Create Merged Sidekick Character");
	SaveAssetDialogConfig.DefaultPath = SKMPackageName;
	SaveAssetDialogConfig.DefaultAssetName = SKMName;
	SaveAssetDialogConfig.ExistingAssetPolicy = ESaveAssetDialogExistingAssetPolicy::AllowButWarn;
	SaveAssetDialogConfig.AssetClassNames.Add(USkeletalMesh::StaticClass()->GetClassPathName());

	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	FString SaveObjectPath = ContentBrowserModule.Get().CreateModalSaveAssetDialog(SaveAssetDialogConfig);
	OutSaveObjectPath = SaveObjectPath;
}

/**
 * Generates an SkeletalMesh and a Skeleton from the Sidekick character data
 * 
 * @param SKPreset 
 * @param bMergeSection 
 * @param bKeepFacialMorphTargets 
 * @param bKeepBodyMorphTargets 
 * @param bCustomMergeLocation 
 */
void UMergeBPLibrary::GenerateSkeletalMesh(USKCharacterPreset* SKPreset, bool bMergeSection, bool bKeepFacialMorphTargets, bool bKeepBodyMorphTargets, bool bCustomMergeLocation)
{
	UE_LOG(LogTemp, Display, TEXT("GenerateSkeletalMesh"));

	FString SKName;
	FString SKMName;
	FString SKPackagePath;
	FString SKMPackagePath;
	
	// formatting asset paths for output
	
	if (bCustomMergeLocation)
	{
		// Asks User for filename input.
		FString ObjectPath = "";
		SelectSkeletalPackagePath(SKPreset, ObjectPath);

		if (ObjectPath.IsEmpty())
		{
			return;
		}
		
		SKName = FPaths::GetBaseFilename(ObjectPath);
		SKMName = FPaths::GetBaseFilename(ObjectPath);
		SKPackagePath = FPaths::GetPath(ObjectPath);
		SKMPackagePath = FPaths::GetPath(ObjectPath);

		if (SKMName.Contains("_SKM"))
		{
			// SKMName += "." + SKMName;
			SKName = SKMName.Replace(TEXT("_SKM"), TEXT("_SK"));
		}
		else
		{
			SKName += "_SK";
			SKMName += "_SKM";
		}
	}
	else
	{
		// Generates skm and sk details from the SKPreset.
		SKName = SKPreset->GetName() + "_SK";
		SKMName = SKPreset->GetName() + "_SKM";
		SKPackagePath = FPaths::GetPath(SKPreset->GetPathName());
		SKMPackagePath = FPaths::GetPath(SKPreset->GetPathName());
	}
	
	// Check if assets already exists, exits early if it does.
	bool SKPkgExist = FPackageName::DoesPackageExist(SKPackagePath + "/" + SKName);
	bool SKMPkgExist = FPackageName::DoesPackageExist(SKMPackagePath + "/" + SKMName);

	if (SKPkgExist || SKMPkgExist)
	{
		UE_LOG(LogTemp, Error, TEXT("Skeletal Mesh and or Skeleton already exist at %s, %s"), *SKName, *SKMName);
		return;
	}

	// Generate New Package for SK and SKM
	FString AssetPackageName;
	UPackage* NewPackage_SK = CreatePackageForNode(SKPackagePath + "/" + SKName, AssetPackageName);
	UPackage* NewPackage_SKM = CreatePackageForNode(SKMPackagePath + "/" + SKMName, AssetPackageName);
	
	// Generate New Assets for the new packages
	USkeleton* MergeSkeleton;
	USkeletalMesh* MergeMesh;
	MergeSkeleton = NewObject<USkeleton>(NewPackage_SK->GetOutermost(), *SKName, RF_Public | RF_Standalone );
	MergeMesh = NewObject<USkeletalMesh>(NewPackage_SKM->GetOutermost(), *SKMName, RF_Public | RF_Standalone );
	
	MergeMesh->SetSkeleton(MergeSkeleton);

	TArray<USkeletalMesh*> Parts;
	SKPreset->SKMParts.GenerateValueArray(Parts);

	// Removes all empty pointers in the Parts list. These are entries that have not been populated with parts.
	Parts.RemoveAll([](const UObject* Item)
	{
		return Item == nullptr;
	});

	// Morph weights that will be baked down
	TMap<FString, float> MorphWeights = {
		{"defaultBuff", SKPreset->MTBodyMusculature}, 
		{"defaultSkinny", 0.f},
		{"defaultHeavy", 0.f},
		{"masculineFeminine", SKPreset->MTBodyType}
	};

	if (SKPreset->MTBodySize < 0.f)
	{
		MorphWeights["defaultSkinny"] = -1.0f * SKPreset->MTBodySize;
		MorphWeights["defaultHeavy"] = 0.f;
	}
	else
	{
		MorphWeights["defaultHeavy"] = SKPreset->MTBodySize;
		MorphWeights["defaultSkinny"] = 0.f;
	}
	
	
	// TODO: Duplicate Material and apply texture, then use it as the default material 
	// TODO: Assing Texture to newly created default material
	GenerateMergeRenderData(MergeMesh, Parts, SKPreset->BaseMaterial, bKeepFacialMorphTargets, bKeepBodyMorphTargets, MorphWeights, SKPreset);
	
	NewPackage_SK->MarkPackageDirty();
	NewPackage_SKM->MarkPackageDirty();
	
	if (!MergeMesh->GetSkeleton()->IsCompatibleMesh(MergeMesh))
	{
		UE_LOG(LogTemp, Error, TEXT("SKM and Skeleton Failed to be compatible"));
	}
}

/**
 * Merges two SkeletalMeshes together, generating a unique SkeletalMesh and Skeleton
 * 
 * @param MeshA 
 * @param MeshB 
 */
void UMergeBPLibrary::MergeSkeletalMesh(USkeletalMesh* MeshA, USkeletalMesh* MeshB)
{
	/** Array of source mesh info structs. */
	TArray<FMergeMeshInfo> SrcMeshInfo;

	/** Store all the new sections that will need to be added*/
	TArray<FNewSectionInfo> NewSectionArray;
	NewSectionArray.Empty();
	
	FString PackagePath = FString("/Game/MergedTEST/");
	FString PackageName = FString("MergedAsset");
	FString AssetPackageName;
	
	UPackage* NewPackage_SK = CreatePackageForNode(PackagePath + "/" + PackageName + "_SK", AssetPackageName);
	UPackage* NewPackage_SKM = CreatePackageForNode(PackagePath + "/" + PackageName + "_SKM", AssetPackageName);
	
	// Newly generated skeleton, that will have the two other hierarchies merged
	USkeleton* MergeSkeleton;
	
	// Newly generated mesh, that will have the two SKMs merged
	USkeletalMesh* MergeMesh;
	
	MergeSkeleton = NewObject<USkeleton>(NewPackage_SK->GetOutermost(), *(PackageName + TEXT("_SK")), RF_Public | RF_Standalone );
	MergeMesh = NewObject<USkeletalMesh>(NewPackage_SKM->GetOutermost(), *(PackageName + TEXT("_SKM")), RF_Public | RF_Standalone );
	
	// List of SKMs to Merge
	TArray<USkeletalMesh*> SKMSources = { MeshA, MeshB };

	// SetupMorphTargets(SKMSources);
	
	MergeMesh->SetSkeleton(MergeSkeleton);

	// Release the rendering resources.

	MergeMesh->ReleaseResources();
	MergeMesh->ReleaseResourcesFence.Wait();

	FReferenceSkeleton NewReferenceSkeleton;

	// Build the reference skeleton and sockets
	
	BuildReferenceSkeleton(SKMSources, NewReferenceSkeleton, MergeSkeleton);

	// Assigns the new reference skeleton
	
	MergeMesh->SetRefSkeleton(NewReferenceSkeleton);

	BuildSockets(MergeMesh, SKMSources);

	// Rebuild inverse ref pose matrices here as some access patterns 
	// may need to access these matrices before FinalizeMesh is called
	// (which would *normally* rebuild the inv ref matrices).
	MergeMesh->GetRefBasesInvMatrix().Empty();
	MergeMesh->CalculateInvRefMatrices();

	
	// Release any resources the merge mesh might be holding.
	
	FSkeletalMeshRenderData* Resource = MergeMesh->GetResourceForRendering();
	if (Resource)
	{
		Resource->LODRenderData.Empty(1); // keep one as we only need the top most LOD
	}

	MergeMesh->ResetLODInfo();
	MergeMesh->GetMaterials().Empty();


	// -- Create bone mapping

	SrcMeshInfo.Empty();
	SrcMeshInfo.AddZeroed(SKMSources.Num());
	
	for (int32 MeshIdx=0; MeshIdx < SKMSources.Num(); MeshIdx++)
	{
		USkeletalMesh* SrcMesh = SKMSources[MeshIdx];

		if (!SrcMesh)
		{
			continue;
		}

		if (SrcMesh->GetHasVertexColors())
		{
			MergeMesh->SetHasVertexColors(true);
			MergeMesh->SetVertexColorGuid(FGuid::NewGuid());
		}

		FMergeMeshInfo& MeshInfo = SrcMeshInfo[MeshIdx];
		MeshInfo.SrcToDestRefSkeletonMap.AddUninitialized(SrcMesh->GetRefSkeleton().GetRawBoneNum());

		for (int32 i = 0; i < SrcMesh->GetRefSkeleton().GetRawBoneNum(); i++)
		{
			FName SrcBoneName = SrcMesh->GetRefSkeleton().GetBoneName(i);
			int32 DestBoneIndex = NewReferenceSkeleton.FindBoneIndex(SrcBoneName);

			if (DestBoneIndex == INDEX_NONE)
			{
				// Missing bones shouldn't be possible, but can happen with invalid meshes;
				// map any bone we are missing to the 'root'.

				DestBoneIndex = 0;
			}

			MeshInfo.SrcToDestRefSkeletonMap[i] = DestBoneIndex;
		}
	}


	// -- UV data

	uint32 LODIndex = 0;
	uint32 NumUVSets = 0;
	uint32 MaxBoneInfluences = 0;
	bool bUse16BitBoneIndex = false;

	for (int32 MeshIdx=0; MeshIdx < SKMSources.Num(); MeshIdx++)
	{
		USkeletalMesh* SrcMesh = SKMSources[MeshIdx];
		FSkeletalMeshRenderData* SrcResource = SrcMesh->GetResourceForRendering();

		if (SrcResource->LODRenderData.IsValidIndex(LODIndex))
		{
			NumUVSets = FMath::Max(NumUVSets, SrcResource->LODRenderData[LODIndex].GetNumTexCoords());

			MaxBoneInfluences = FMath::Max(MaxBoneInfluences, SrcResource->LODRenderData[LODIndex].GetVertexBufferMaxBoneInfluences());
			bUse16BitBoneIndex |= SrcResource->LODRenderData[LODIndex].DoesVertexBufferUse16BitBoneIndex();
		}
	}


	// -- process each LOD for the new merged mesh
	
	MergeMesh->AllocateResourceForRendering();

	const FSkeletalMeshLODInfo* LODInfoPtr = MergeMesh->GetLODInfo(LODIndex);
	bool bUseFullPrecisionUVs = LODInfoPtr ? LODInfoPtr->BuildSettings.bUseFullPrecisionUVs : false;
	
	// -- Start of GenerateLODModel
	
	// add the new LOD model entry
	FSkeletalMeshRenderData* MergeResource = MergeMesh->GetResourceForRendering();
	check(MergeResource);

	FSkeletalMeshLODRenderData& MergeLODData = *new FSkeletalMeshLODRenderData;
	MergeResource->LODRenderData.Add(&MergeLODData);
	// add the new LOD info entry
	FSkeletalMeshLODInfo& MergeLODInfo = MergeMesh->AddLODInfo();
	MergeLODInfo.ScreenSize = MergeLODInfo.LODHysteresis = UE_MAX_FLT;

	MergeLODInfo.MorphTargetPositionErrorTolerance = 0.1f; // required for blendshapes to appear on reload
	MergeLODInfo.bAllowCPUAccess = true;// required for blendshapes to appear on reload
	MergeLODInfo.BuildSettings.bRecomputeNormals = false;
	MergeLODInfo.BuildSettings.bRecomputeTangents = false;
	
	// generate an array with info about new sections that need to be created
	const int32 MaxGPUSkinBones = FGPUBaseSkinVertexFactory::GetMaxGPUSkinBones();

	for (int32 MeshIdx=0; MeshIdx < SKMSources.Num(); MeshIdx++)
	{
		USkeletalMesh* SrcMesh = SKMSources[MeshIdx];

		if (!SrcMesh)
		{
			continue;
		}

		FSkeletalMeshRenderData* SrcResource = SrcMesh->GetResourceForRendering();
		FSkeletalMeshLODRenderData& SrcLODData = SrcResource->LODRenderData[LODIndex];
		FSkeletalMeshLODInfo& SrcLODInfo = *(SrcMesh->GetLODInfo(LODIndex));

		// iterate over each section of this LOD
		for ( int32 SectionIdx=0; SectionIdx < SrcLODData.RenderSections.Num(); SectionIdx++)
		{
			int32 MaterialId = -1;

			FSkelMeshRenderSection& Section = SrcLODData.RenderSections[SectionIdx];

			// Convert Chunk.BoneMap from src to dest bone indices
			TArray<FBoneIndexType> DestChunkBoneMap;
			DestChunkBoneMap.Empty();
			DestChunkBoneMap.AddUninitialized(Section.BoneMap.Num());
			for(int32 i=0; i<Section.BoneMap.Num(); i++)
			{
				check(Section.BoneMap[i] < SrcMeshInfo[MeshIdx].SrcToDestRefSkeletonMap.Num());
				DestChunkBoneMap[i] = SrcMeshInfo[MeshIdx].SrcToDestRefSkeletonMap[Section.BoneMap[i]];
			}


			// get the material for this section
			int32 MaterialIndex = Section.MaterialIndex;
			// use the remapping of material indices if there is a valid value
			if(SrcLODInfo.LODMaterialMap.IsValidIndex(SectionIdx) && SrcLODInfo.LODMaterialMap[SectionIdx] != INDEX_NONE && SrcMesh->GetMaterials().Num() > 0)
			{
				MaterialIndex = FMath::Clamp<int32>( SrcLODInfo.LODMaterialMap[SectionIdx],0, SrcMesh->GetMaterials().Num() - 1);
			}

			const FSkeletalMaterial& SkeletalMaterial = SrcMesh->GetMaterials()[MaterialIndex];
			UMaterialInterface* Material = SkeletalMaterial.MaterialInterface;

			// see if there is an existing entry in the array of new sections that matches its material
			// if there is a match then the source section can be added to its list of sections to merge
			int32 FoundIdx = INDEX_NONE;
			// NOTE: This looks like a bug from the source code, that will never evaluate. Actually it will if called with more then 2 skms, using the macro
			// for (int32 Idx=0; Idx < NewSectionArray.Num(); Idx++)
			// {
			// 	
			// }

			// new section entries will be created if the material for the source section was not found
			// or merging it with an existing entry would go over the bone limit for the GPU Skinning
			if ( FoundIdx == INDEX_NONE )
			{
				// create a new section entry
				const FName& MaterialSlotName = SkeletalMaterial.MaterialSlotName;
				const FMeshUVChannelInfo& UVChannelData = SkeletalMaterial.UVChannelData;
				FNewSectionInfo& NewSectionInfo = *new(NewSectionArray) FNewSectionInfo(Material, MaterialId, MaterialSlotName, UVChannelData, SrcMesh);
				// initialize the merged bonemap to simply use the original chunk bonemap
				NewSectionInfo.MergedBoneMap = DestChunkBoneMap;

				TArray<FTransform> SrcUVTransform;
				// add a new merge section entry
				FMergeSectionInfo& MergeSectionInfo = *new(NewSectionInfo.MergeSections) FMergeSectionInfo(
					SrcMesh,
					&SrcLODData.RenderSections[SectionIdx],
					SrcUVTransform);
				// since merged bonemap == chunk.bonemap then remapping is just a pass-through
				MergeSectionInfo.BoneMapToMergedBoneMap.Empty(DestChunkBoneMap.Num());
				for (int32 i=0; i<DestChunkBoneMap.Num(); i++)
				{
					MergeSectionInfo.BoneMapToMergedBoneMap.Add((FBoneIndexType)i);
				}
			}
		}
		
	}

	// -- back in GenerateLODModel


	// using VertexDataType = TGPUSkinVertexFloat16Uvs<1>;
	
	uint32 MaxIndex = 0;
	// merged vertex buffer
	TArray< VertexDataType > MergedVertexBuffer; // 1 = 1 UV set
	// merged skin weight buffer
	TArray< FSkinWeightInfo > MergedSkinWeightBuffer;
	// merged vertex color buffer
	TArray< FColor > MergedColorBuffer;
	// merged index buffer
	TArray< uint32 > MergedIndexBuffer;

	// the total number of UV sets for this LOD model
	uint32 TotalNumUVs = 0;

	uint32 SourceMaxBoneInfluences = 0;
	bool bSourceUse16BitBoneIndex = false;

	for ( int32 CreateIdx=0; CreateIdx < NewSectionArray.Num(); CreateIdx++)
	{
		FNewSectionInfo& NewSectionInfo = NewSectionArray[CreateIdx];

		// ActiveBoneIndices contains all the bones used by the verts from all the sections of this LOD model
		// Add the bones used by this new section
		for ( int32 Idx=0; Idx < NewSectionInfo.MergedBoneMap.Num(); Idx++)
		{
			MergeLODData.ActiveBoneIndices.AddUnique( NewSectionInfo.MergedBoneMap[Idx] );
		}
		
		// add the new section entry
		FSkelMeshRenderSection& Section = *new(MergeLODData.RenderSections) FSkelMeshRenderSection;

		// set the new bonemap from the merged sections
		// these are the bones that will be used by this new section
		Section.BoneMap = NewSectionInfo.MergedBoneMap;

		// init vert totals
		Section.NumVertices = 0;

		// keep track of the current base vertex for this section in the merged vertex buffer
		Section.BaseVertexIndex = MergedVertexBuffer.Num();

		// find existing material index
		int32 MatIndex = INDEX_NONE;

		if (MatIndex == INDEX_NONE)
		{
			FSkeletalMaterial SkeletalMaterial(NewSectionInfo.Material, true,false, NewSectionInfo.SlotName);
			SkeletalMaterial.UVChannelData = NewSectionInfo.UVChannelData;
			MergeMesh->GetMaterials().Add(SkeletalMaterial);

			Section.MaterialIndex = MergeMesh->GetMaterials().Num()-1;
		}

		// init tri totals
		Section.NumTriangles = 0;
		// keep track of the current base index for section in the merged index buffer
		Section.BaseIndex = MergedIndexBuffer.Num();

		FMeshUVChannelInfo& MergedUVData = MergeMesh->GetMaterials()[Section.MaterialIndex].UVChannelData;

		// iterate over all of the sections that need to be merged together
		for ( int32 MergeIdx=0; MergeIdx < NewSectionInfo.MergeSections.Num(); MergeIdx++)
		{
			FMergeSectionInfo& MergeSectionInfo = NewSectionInfo.MergeSections[MergeIdx]; // array of new section to add, as each mesh sould be made up of multiple sections 
			int32 SourceLODIdx = LODIndex;

			// Take the max UV density for each UVChannel between all sections that are being merged.
			const int32 NewSectionMatId = MergeSectionInfo.Section->MaterialIndex;
			if(MergeSectionInfo.SkelMesh->GetMaterials().IsValidIndex(NewSectionMatId))
			{
				const FMeshUVChannelInfo& NewSectionUVData = MergeSectionInfo.SkelMesh->GetMaterials()[NewSectionMatId].UVChannelData;
				for (int32 i = 0; i < MAX_TEXCOORDS; i++)
				{
					const float NewSectionUVDensity = NewSectionUVData.LocalUVDensities[i];
					float& UVDensity = MergedUVData.LocalUVDensities[i];

					UVDensity = FMath::Max(UVDensity, NewSectionUVDensity);
				}
			}

			// get the source skeleton LOD info from this merge entry
			const FSkeletalMeshLODInfo& SrcLODInfo = *(MergeSectionInfo.SkelMesh->GetLODInfo(LODIndex));

			// keep track of the lowest LOD displayfactor and hysteresis
			MergeLODInfo.ScreenSize.Default = FMath::Min<float>(MergeLODInfo.ScreenSize.Default, SrcLODInfo.ScreenSize.Default);
#if WITH_EDITORONLY_DATA
			for(const TPair<FName, float>& PerPlatform : SrcLODInfo.ScreenSize.PerPlatform)
			{
				float* Value = MergeLODInfo.ScreenSize.PerPlatform.Find(PerPlatform.Key);
				if(Value)
				{
					*Value = FMath::Min<float>(PerPlatform.Value, *Value);	
				}
				else
				{
					MergeLODInfo.ScreenSize.PerPlatform.Add(PerPlatform.Key, PerPlatform.Value);
				}
			}
#endif
			MergeLODInfo.BuildSettings.bUseFullPrecisionUVs |= SrcLODInfo.BuildSettings.bUseFullPrecisionUVs;
			MergeLODInfo.BuildSettings.bUseBackwardsCompatibleF16TruncUVs |= SrcLODInfo.BuildSettings.bUseBackwardsCompatibleF16TruncUVs;
			MergeLODInfo.BuildSettings.bUseHighPrecisionTangentBasis |= SrcLODInfo.BuildSettings.bUseHighPrecisionTangentBasis;

			MergeLODInfo.LODHysteresis = FMath::Min<float>(MergeLODInfo.LODHysteresis,SrcLODInfo.LODHysteresis);

			// get the source skeleton LOD model from this merge entry
			const FSkeletalMeshLODRenderData& SrcLODData = MergeSectionInfo.SkelMesh->GetResourceForRendering()->LODRenderData[SourceLODIdx];

			// add required bones from this source model entry to the merge model entry
			for (int32 Idx=0; Idx < SrcLODData.RequiredBones.Num(); Idx++)
			{
				FName SrcLODBoneName = MergeSectionInfo.SkelMesh->GetRefSkeleton().GetBoneName(SrcLODData.RequiredBones[Idx]);
				int32 MergeBoneIndex = NewReferenceSkeleton.FindBoneIndex(SrcLODBoneName);

				if (MergeBoneIndex != INDEX_NONE)
				{
					MergeLODData.RequiredBones.AddUnique(MergeBoneIndex);
				}
			}

			// update vert total
			Section.NumVertices += MergeSectionInfo.Section->NumVertices;

			// update total number of vertices
			const int32 NumTotalVertices = MergeSectionInfo.Section->NumVertices;

			// add the vertices from the original source mesh to the merged vertex buffer
			const int32 MaxVertIdx = FMath::Min<int32>(
				MergeSectionInfo.Section->BaseVertexIndex + NumTotalVertices,
				SrcLODData.StaticVertexBuffers.PositionVertexBuffer.GetNumVertices());

			// The amount of colour vertices that currently exist on the 'source skm lod's section'
			const int32 MaxColorIdx = SrcLODData.StaticVertexBuffers.ColorVertexBuffer.GetNumVertices();

			// update max number of influences
			MaxBoneInfluences = SrcLODData.GetSkinWeightVertexBuffer()->GetMaxBoneInfluences();
			bUse16BitBoneIndex = SrcLODData.GetSkinWeightVertexBuffer()->Use16BitBoneIndex();
			
			SourceMaxBoneInfluences = FMath::Max(SourceMaxBoneInfluences, MaxBoneInfluences);
			bSourceUse16BitBoneIndex |= bUse16BitBoneIndex;

			// update RenderSection number of max influences
			Section.MaxBoneInfluences = MaxBoneInfluences;

			// update total number of TexCoords
			const uint32 LODNumTexCoords = SrcLODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords();
			if (TotalNumUVs < LODNumTexCoords)
			{
				TotalNumUVs = LODNumTexCoords;
			}

			// keep track of the current base vertex index before adding any new vertices
			// this will be needed to remap the index buffer values to the new range
			const int32 CurrentBaseVertexIndex = MergedVertexBuffer.Num();

			for ( int32 VertexIdx=MergeSectionInfo.Section->BaseVertexIndex; VertexIdx<MaxVertIdx; VertexIdx++)
			{
				// add the new vertex
				VertexDataType& DestVert = MergedVertexBuffer[MergedVertexBuffer.AddUninitialized()];
				FSkinWeightInfo& DestWeight = MergedSkinWeightBuffer[MergedSkinWeightBuffer.AddUninitialized()];

				// -- CopyVertexFromSource<VertexDataType>(DestVert, SrcLODData, VertIdx, MergeSectionInfo);
				{
					DestVert.Position = SrcLODData.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(VertexIdx);
					DestVert.TangentX = SrcLODData.StaticVertexBuffers.StaticMeshVertexBuffer.VertexTangentX(VertexIdx);
					DestVert.TangentZ = SrcLODData.StaticVertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(VertexIdx);

					// TODO: Investigation. Applies all blendshapes with a weight of 0.5
					// ApplyMorphTarget(NewSectionInfo.SkelMesh, VertexIdx, DestVert, 0.5);
					
					// Copy all UVs that are available
					uint32 NumTexCoords = SrcLODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords();
					const uint32 ValidLoopCount = FMath::Min(VertexDataType::NumTexCoords, NumTexCoords);
					for (uint32 UVIndex=0; UVIndex<ValidLoopCount; UVIndex++)
					{
						FVector2D UVs = FVector2D(SrcLODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetVertexUV_Typed<VertexDataType::StaticMeshVertexUVType>(VertexIdx, UVIndex));
						DestVert.UVs[UVIndex] = FVector2f(UVs);
					}

					// now just fill up zero value if we didn't reach the end
					for (uint32 UVIndex=ValidLoopCount; UVIndex < VertexDataType::NumTexCoords; UVIndex++)
					{
						DestVert.UVs[UVIndex] = FVector2f::ZeroVector;
					}
					// -- end
				}


				DestWeight = SrcLODData.GetSkinWeightVertexBuffer()->GetVertexSkinWeights(VertexIdx);

				// if the mesh uses vertex colors, copy the source color if possible or default to white
				if( MergeMesh->GetHasVertexColors() )
				{
					if( VertexIdx < MaxColorIdx )
					{
						const FColor& SrcColor = SrcLODData.StaticVertexBuffers.ColorVertexBuffer.VertexColor(VertexIdx);
						MergedColorBuffer.Add(SrcColor);
					}
					else
					{
						const FColor ColorWhite(255,255,255);
						MergedColorBuffer.Add(ColorWhite);
					}
				}

				// remap the bone index used by this vertex to match the mergedbonemap
				for ( uint32 Idx=0; Idx < MAX_TOTAL_INFLUENCES; Idx++)
				{
					if (DestWeight.InfluenceWeights[Idx] > 0)
					{
						checkSlow(MergeSectionInfo.BoneMapToMergedBoneMap.IsValidIndex(DestWeight.InfluenceBones[Idx]));
						DestWeight.InfluenceBones[Idx] = (FBoneIndexType)MergeSectionInfo.BoneMapToMergedBoneMap[DestWeight.InfluenceBones[Idx]];
					}
				}
			}

			// update total number of triangles
			Section.NumTriangles += MergeSectionInfo.Section->NumTriangles;

			// add the indices from the original source mesh to the merged index buffer
			const int32 MaxIndexIdx = FMath::Min<int32>(
				MergeSectionInfo.Section->BaseIndex + MergeSectionInfo.Section->NumTriangles * 3,
				SrcLODData.MultiSizeIndexContainer.GetIndexBuffer()->Num()
				);
			for (int32 IndexIdx = MergeSectionInfo.Section->BaseIndex; IndexIdx < MaxIndexIdx; IndexIdx++)
			{
				uint32 SrcIndex = SrcLODData.MultiSizeIndexContainer.GetIndexBuffer()->Get(IndexIdx);

				// add offset to each index to match the new entries in the merged vertex buffer
				checkSlow(SrcIndex >= MergeSectionInfo.Section->BaseVertexIndex);
				uint32 DstIndex = SrcIndex - MergeSectionInfo.Section->BaseVertexIndex + CurrentBaseVertexIndex;
				checkSlow(DstIndex < (uint32)MergedVertexBuffer.Num());

				// add the new index to the merged vertex buffer
				MergedIndexBuffer.Add(DstIndex);
				if (MaxIndex < DstIndex)
				{
					MaxIndex = DstIndex;
				}
			}


			{
                if (MergeSectionInfo.Section->DuplicatedVerticesBuffer.bHasOverlappingVertices)
                {
                    if (Section.DuplicatedVerticesBuffer.bHasOverlappingVertices)
                    {
                        // Merge
                        int32 StartIndex = Section.DuplicatedVerticesBuffer.DupVertData.Num();
                        int32 StartVertex = Section.DuplicatedVerticesBuffer.DupVertIndexData.Num();
                        Section.DuplicatedVerticesBuffer.DupVertData.ResizeBuffer(StartIndex + MergeSectionInfo.Section->DuplicatedVerticesBuffer.DupVertData.Num());
                        Section.DuplicatedVerticesBuffer.DupVertIndexData.ResizeBuffer(Section.NumVertices);
                    
                        uint8* VertData = Section.DuplicatedVerticesBuffer.DupVertData.GetDataPointer();
                        uint8* IndexData = Section.DuplicatedVerticesBuffer.DupVertIndexData.GetDataPointer();
                        for (int32 i = StartIndex; i < Section.DuplicatedVerticesBuffer.DupVertData.Num(); ++i)
                        {
                            *((uint32*)(VertData + i * sizeof(uint32))) += CurrentBaseVertexIndex - MergeSectionInfo.Section->BaseVertexIndex;
                        }
                        for (uint32 i = StartVertex; i < Section.NumVertices; ++i)
                        {
                            ((FIndexLengthPair*)(IndexData + i * sizeof(FIndexLengthPair)))->Index += StartIndex;
                        }
                    }
                    else
                    {
                        Section.DuplicatedVerticesBuffer.DupVertData = MergeSectionInfo.Section->DuplicatedVerticesBuffer.DupVertData;
                        Section.DuplicatedVerticesBuffer.DupVertIndexData = MergeSectionInfo.Section->DuplicatedVerticesBuffer.DupVertIndexData;
                        uint8* VertData = Section.DuplicatedVerticesBuffer.DupVertData.GetDataPointer();
                        for (int32 i = 0; i < Section.DuplicatedVerticesBuffer.DupVertData.Num(); ++i)
                        {
                            *((uint32*)(VertData + i * sizeof(uint32))) += CurrentBaseVertexIndex - MergeSectionInfo.Section->BaseVertexIndex;
                        }
                    }
                    Section.DuplicatedVerticesBuffer.bHasOverlappingVertices = true;
                }
                else
                {
                    Section.DuplicatedVerticesBuffer.DupVertData.ResizeBuffer(1);
                    Section.DuplicatedVerticesBuffer.DupVertIndexData.ResizeBuffer(Section.NumVertices);

                    uint8* VertData = Section.DuplicatedVerticesBuffer.DupVertData.GetDataPointer();
                    uint8* IndexData = Section.DuplicatedVerticesBuffer.DupVertIndexData.GetDataPointer();
                    
                    FMemory::Memzero(IndexData, Section.NumVertices * sizeof(FIndexLengthPair));
                    FMemory::Memzero(VertData, sizeof(uint32));
                }
            }
		}
	}
	
	// The asset will need to be accessed by the CPU
	const bool bNeedsCPUAccess = true;

	// sort required bone array in strictly increasing order
	MergeLODData.RequiredBones.Sort();
	MergeMesh->GetRefSkeleton().EnsureParentsExistAndSort(MergeLODData.ActiveBoneIndices);

	// copy the new vertices and indices to the vertex buffer for the new model
	MergeLODData.StaticVertexBuffers.StaticMeshVertexBuffer.SetUseFullPrecisionUVs(MergeLODInfo.BuildSettings.bUseFullPrecisionUVs);

	MergeLODData.StaticVertexBuffers.PositionVertexBuffer.Init(MergedVertexBuffer.Num(), bNeedsCPUAccess);
	MergeLODData.StaticVertexBuffers.StaticMeshVertexBuffer.Init(MergedVertexBuffer.Num(), TotalNumUVs, bNeedsCPUAccess);

	bool bUseBackwardsCompatibleF16TruncUVS = MergeLODInfo.BuildSettings.bUseBackwardsCompatibleF16TruncUVs;

	for (int i=0; i < MergedVertexBuffer.Num(); i++)
	{
		MergeLODData.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(i) = MergedVertexBuffer[i].Position;
		MergeLODData.StaticVertexBuffers.StaticMeshVertexBuffer.SetVertexTangents(i, MergedVertexBuffer[i].TangentX.ToFVector3f(), MergedVertexBuffer[i].GetTangentY(), MergedVertexBuffer[i].TangentZ.ToFVector3f());
		for (uint32 j=0; j < TotalNumUVs; j++)
		{
			MergeLODData.StaticVertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i,j,MergedVertexBuffer[i].UVs[j], bUseBackwardsCompatibleF16TruncUVS);
		}
	}

	MergeLODData.SkinWeightVertexBuffer.SetMaxBoneInfluences(SourceMaxBoneInfluences);
	MergeLODData.SkinWeightVertexBuffer.SetUse16BitBoneIndex(bSourceUse16BitBoneIndex);
	MergeLODData.SkinWeightVertexBuffer.SetNeedsCPUAccess(bNeedsCPUAccess);

	// copy vertex resource arrays
	MergeLODData.SkinWeightVertexBuffer = MergedSkinWeightBuffer;

	if ( MergeMesh->GetHasVertexColors() )
	{
		MergeLODData.StaticVertexBuffers.ColorVertexBuffer.InitFromColorArray(MergedColorBuffer);
	}

	const uint8 DataTypeSize = (MaxIndex < MAX_uint16) ? sizeof(uint16) : sizeof(uint32);
	MergeLODData.MultiSizeIndexContainer.RebuildIndexBuffer(DataTypeSize, MergedIndexBuffer);
	
	// -- end of GerenateLODModel
	
	// copy settings and bone info from src meshes
	bool bNeedsInit=true;

	for( int32 MeshIdx=0; MeshIdx < SKMSources.Num(); MeshIdx++ )
	{
		USkeletalMesh* SrcMesh = SKMSources[MeshIdx];
		if( SrcMesh )
		{
			if( bNeedsInit )
			{
				// initialize the merged mesh with the first src mesh entry used
				MergeMesh->SetImportedBounds(SrcMesh->GetImportedBounds());
				// only initialize once
				bNeedsInit = false;
			}
			else
			{
				// add bounds
				MergeMesh->SetImportedBounds(MergeMesh->GetImportedBounds() + SrcMesh->GetImportedBounds());
			}
		}
	}
	// Rebuild inverse ref pose matrices.
	MergeMesh->GetRefBasesInvMatrix().Empty();
	MergeMesh->CalculateInvRefMatrices();
	
	// Reinitialize the mesh's render resources.
	MergeMesh->InitResources();
	
	// //We need to have a valid render data to create physic asset
	// MergeMesh->Build();
	
	// Newly generated mesh, that will have the two SKMs merged
	if (!MergeMesh->GetSkeleton())
	{
		MergeMesh->SetSkeleton(MergeSkeleton);
	}
	
	MergeSkeleton->MergeAllBonesToBoneTree(MergeMesh);
	MergeSkeleton->SetPreviewMesh(MergeMesh);
	MergeSkeleton->PostEditChange();
	
	PopulateSKMImportModel(MergeMesh, SKMSources);

	// -- Morph Target Merge
	
	MergeMesh->PreEditChange(nullptr);
	
	int32 VertexOffset = 0;
	TArray<FString> IgnoreNames;
	for( int32 MeshIdx=0; MeshIdx < SKMSources.Num(); MeshIdx++ )
	{
		USkeletalMesh* SrcMesh = SKMSources[MeshIdx];
		GetMorphTargets(SrcMesh, MergeMesh, VertexOffset, IgnoreNames);
	}

	MergeMesh->InitMorphTargetsAndRebuildRenderData();
		
	MergeSkeleton->MarkPackageDirty();
	MergeMesh->MarkPackageDirty();
	
	/**
	 * Sets the Remap Morph Target to true. This is required as Unreal will auto regenerate the Morph Targets as there
	 * is no import data. In doing so Unreal classifies the regenerated Morph Targets as being Remapped.
	 */
	MergeMesh->GetLODInfo(LODIndex)->ReductionSettings.bRemapMorphTargets = true;
	MergeMesh->GetLODInfo(LODIndex)->ReductionSettings.NumOfTrianglesPercentage = 1.0f;
	MergeMesh->GetLODInfo(LODIndex)->bHasBeenSimplified = true;
	/***/
	
	FAssetRegistryModule::AssetCreated(MergeSkeleton);
	FAssetRegistryModule::AssetCreated(MergeMesh);

	NewPackage_SK->MarkPackageDirty();
	NewPackage_SKM->MarkPackageDirty();
	
	
	if (!MergeMesh->GetSkeleton()->IsCompatibleMesh(MergeMesh))
	{
		UE_LOG(LogTemp, Error, TEXT("SKM and Skeleton Failed to be compatible"));
	}
	
}
