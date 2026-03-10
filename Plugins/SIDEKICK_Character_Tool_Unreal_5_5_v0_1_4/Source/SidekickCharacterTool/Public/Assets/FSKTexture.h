#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"

class FSKTexture
{
public:
	
	/** Read the Pixel Colours from a Texture */
	static void ReadDataFromTexture(UTexture2D* Texture, TArray<FColor>* PixelColors);

	/** Write the Data back into the Texture */
	static void WriteDataToTexture(TArray<FColor>* PixelColors, UTexture2D* Texture);
	static void WriteDataToTexture(TArray<uint8> PixelColorsByteData, UTexture2D* Texture);
	
	static void UpdateTextureRegion(TArray<FColor>* PixelColors, UTexture2D* Texture);
	
	/** Checks to see if the texture is the correct size
	 * 
	 * @param Texture That will have its dimensions checked
	 * @return True if the Texture is the correct size, else false
	 */
	static bool ValidateTextureResolution(UTexture2D* Texture);

	/** Samples the provided texture, resulting in a Sidekick Texture that is standardised.
	 *
	 * Dimension: 32x32
	 */
	static void SampleTexture(UTexture2D* SourceTexture, UTexture2D* SampledTexture);

	/**
	 * Creates a .png file with the data from the Texture.
	 *
	 * Note: The Texture needs to be in sRGB, else you might end up with colourspace artifacts.
	 * 
	 * @param Texture Data that will be written to the .png
	 * @param FilePath path to where the file will be written. 
	 */
	static void SaveTexture(UTexture2D* Texture, FString FilePath);

	static UTexture2D* SerialiseAssetPackageTexture(FString TexturePath, UTexture2D* SourceTexture);
	
	/**
	 * Copies the Texture's BaseColor into a new Texture and assigns it to the Material.
	 * 
	 * @param Material that will have its base color texture copied
	 * @param Texture that will be updated
	 */
	static void CopyMaterialTextureToTexture(UMaterialInterface* Material, UTexture2D* Texture);

	/**
	 * Gets the Texture2D that is assigned the the BaseColor attribute on the specified material.
	 * 
	 * @param Material 
	 * @param Texture 
	 */
	static void GetBaseColorTexture(UMaterialInterface* Material, UTexture2D*& Texture);

	/**
	 * Updates the materials BaseColor texture
	 * 
	 * @param Material that will be updated
	 * @param Texture that will be assigned to the material
	 * @return true is assigning the Texture was successful.
	 */
	static bool SetBaseColorTexture(UMaterial* Material, UTexture2D* Texture);

	static FString ImportTextureAsset(FString TexturePath);

	/**
	 * Duplicates a material from one path to another path.
	 *
	 * Its main use it to copy the default material in the plugin into the Sidekicks content folder. 
	 * 
	 * @param SourceMaterialPath Path to the Material package being copied
	 * @param NewMaterialName Name of the new material file
	 * @param TargetPath Folder path to the new material
	 * @return 
	 */
	static UMaterialInterface* DuplicateMaterialFromPath(const FString& SourceMaterialPath, const FString& NewMaterialName, const FString& TargetPath);
	
private:
	static FString ConvertTexturePathToPackagePath(FString TexturePath);
};
