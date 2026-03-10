#include "Assets/FSKTexture.h"

#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "NavigationSystemTypes.h"
#include "ObjectTools.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorFramework/AssetImportData.h"
#include "Factories/TextureFactory.h"
#include "UObject/SavePackage.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "TextureResource.h"
#include "Engine/Texture2d.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "AssetToolsModule.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Modules/ModuleManager.h"
#include "Materials/MaterialInterface.h"

/**
 * Converts the Texture Data into an array of colour.
 * 
 * @param Texture 
 * @param PixelColors 
 */
void FSKTexture::ReadDataFromTexture(UTexture2D* Texture, TArray<FColor>* PixelColors)
{
	if (!Texture) return;

	if (Texture->GetPlatformData()->Mips.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("No mips found. %s"), *Texture->GetFullName());
		return;
	}
	
	FTexture2DMipMap& MipMap = Texture->GetPlatformData()->Mips[0];
	void* Data = MipMap.BulkData.Lock(LOCK_READ_WRITE);

	if (Data == nullptr) { return; }

	// Create a chunk in memory the size of the texture
	int32 Width = MipMap.SizeX;
	int32 Height = MipMap.SizeY;
	int32 PixelCount = Width * Height;
	PixelColors->AddUninitialized(PixelCount);

	// Copies the data from the MipMap to the Color array.
	FMemory::Memcpy(PixelColors->GetData(), Data, PixelCount * sizeof(FColor));

	MipMap.BulkData.Unlock();
}

void FSKTexture::WriteDataToTexture(TArray<FColor>* PixelColors, UTexture2D* Texture)
{
	if (Texture == nullptr || PixelColors->Num() != 32 * 32) { return; }
	
	FTexturePlatformData* PlatformData = Texture->GetPlatformData();
	if (PlatformData == nullptr) { 
		PlatformData = new FTexturePlatformData();
		Texture->SetPlatformData(PlatformData);
	}	

	PlatformData->SizeX = 32;
	PlatformData->SizeY = 32;
	PlatformData->SetNumSlices(1);
	PlatformData->PixelFormat = PF_B8G8R8A8;

	if (PlatformData->Mips.Num() == 0)
	{
		PlatformData->Mips.Add(new FTexture2DMipMap(32, 32, 1));
	}

	FTexture2DMipMap& TempMipMap = PlatformData->Mips[0];	
	
	TempMipMap.BulkData.Lock(LOCK_READ_WRITE);
	
	void* TempData = TempMipMap.BulkData.Realloc(32 * 32 * sizeof(FColor));
	const uint8* PixelsData = (uint8*)PixelColors->GetData();
	FMemory::Memcpy(TempData, PixelsData, 32 * 32 * sizeof(FColor));

	TempMipMap.BulkData.Unlock();
								
	Texture->MarkPackageDirty();
	Texture->PostEditChange();
	Texture->UpdateResource();
}

void FSKTexture::WriteDataToTexture(TArray<uint8> PixelColorsByteData, UTexture2D* Texture)
{
	
	FTexturePlatformData* PlatformData = Texture->GetPlatformData();
	if (PlatformData == nullptr) { 
		PlatformData = new FTexturePlatformData();
		Texture->SetPlatformData(PlatformData);
	}	

	PlatformData->SizeX = 32;
	PlatformData->SizeY = 32;
	PlatformData->SetNumSlices(1);
	PlatformData->PixelFormat = PF_B8G8R8A8;

	if (PlatformData->Mips.Num() == 0)
	{
		PlatformData->Mips.Add(new FTexture2DMipMap(32, 32, 1));
	}

	FTexture2DMipMap& TempMipMap = PlatformData->Mips[0];	
	
	TempMipMap.BulkData.Lock(LOCK_READ_WRITE);
	
	void* TempData = TempMipMap.BulkData.Realloc(32 * 32 * sizeof(FColor));
	// const uint8* PixelsData = (uint8*)PixelColors->GetData();
	const uint8* PixelsData = (uint8*)PixelColorsByteData.GetData();
	FMemory::Memcpy(TempData, PixelsData, 32 * 32 * sizeof(FColor));

	TempMipMap.BulkData.Unlock();
								
	Texture->UpdateResource();
	Texture->MarkPackageDirty();
}

void FSKTexture::UpdateTextureRegion(TArray<FColor>* PixelColors, UTexture2D* Texture)
{
	if (Texture->GetSizeX() != 32 || Texture->GetSizeY() != 32) { return; }
	if (Texture == nullptr || PixelColors->Num() != 32 * 32) { return; }
	
	FTexturePlatformData* PlatformData = Texture->GetPlatformData();
	
	if (PlatformData->Mips.Num() == 0)
	{
		PlatformData->Mips.Add(new FTexture2DMipMap(32, 32, 1));
		Texture->UpdateResource();
	}
	
	FUpdateTextureRegion2D Region(0, 0, 0, 0, 32, 32);

	Texture->UpdateTextureRegions(
		0, // mip index
		1, // num regions
		&Region,
		32 * sizeof(FColor), // source row pitch
		sizeof(FColor),      // source pixel stride
		(uint8*)PixelColors->GetData()
	);
}

bool FSKTexture::ValidateTextureResolution(UTexture2D* Texture)
{
	if (Texture->GetPlatformData()->Mips.Num() == 0) return false;
	
	FTexture2DMipMap& MipMap = Texture->GetPlatformData()->Mips[0];
	int32 Width = MipMap.SizeX;
	int32 Height = MipMap.SizeY;

	if (Width > 32 || Height > 32)
	{
		return false;
	}

	return true;
}

// todo: Fix the Sample Texture as it has not been updated to take into account the 4x4 pixels per a swatch
void FSKTexture::SampleTexture(UTexture2D* SourceTexture, UTexture2D* SampledTexture)
{
	TArray<FColor> PixelColors;
	TArray<FColor> SampledColors;

	FTexture2DMipMap& MipMap = SourceTexture->GetPlatformData()->Mips[0];
	int32 WidthSampleRate = MipMap.SizeX / 32;
	int32 HeightSampleRate = MipMap.SizeY / 32;

	SampledColors.AddUninitialized(32 * 32);
	
	FSKTexture::ReadDataFromTexture(SourceTexture, &PixelColors);

	// Samples the Data from the Colour Array(1D) 
	for (int SampleX = 0; SampleX < 32; ++SampleX)
	{
		for (int SampleY = 0; SampleY < 32; ++SampleY)
		{
			int SampleIndex = (SampleY * HeightSampleRate) + (SampleX * WidthSampleRate);
			int index = SampleY * 32 + SampleX;

			SampledColors[index] = PixelColors[SampleIndex];
		}
	}

	FSKTexture::WriteDataToTexture(&SampledColors, SampledTexture);
}

void FSKTexture::SaveTexture(UTexture2D* Texture, const FString FilePath)
{
	if (!Texture || !Texture->GetPlatformData() || Texture->GetPlatformData()->Mips.Num() == 0)
		return;
	
	TArray<FColor> RawPixels;
	
	FSKTexture::ReadDataFromTexture(Texture, &RawPixels);
	
	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

	if (ImageWrapper->SetRaw(RawPixels.GetData(), RawPixels.Num() * sizeof(FColor), 32, 32, ERGBFormat::BGRA, 8))
	{
		const TArray64<uint8>& PngData = ImageWrapper->GetCompressed();
		if (FFileHelper::SaveArrayToFile(PngData, *FilePath))
		{
			UE_LOG(LogTemp, Log, TEXT("Saved texture to %s"), *FilePath);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to save texture to %s"), *FilePath);
		}
	}
}


/**
 *
 * NOTE: Looks like the assets become empty after the session ends, serialisation doesn't commit to disk..?.
 * 
 * @param TexturePath Path to where the .png file will be created/update and the package will be generated
 * @param Texture to be serialised.
 */
UTexture2D* FSKTexture::SerialiseAssetPackageTexture(FString TexturePath, UTexture2D* SourceTexture)
{
	FString PackagePath = ConvertTexturePathToPackagePath(TexturePath);
	FString FileName = FPaths::GetBaseFilename(TexturePath);
	UPackage* Package = FindPackage(nullptr, *PackagePath);
	UTexture2D* TargetTexture = nullptr;

	if (Package)
	{
		TargetTexture = FindObject<UTexture2D>(Package, *FileName);
	}

	if (!TargetTexture)
	{
		// If package or asset doesn't exist, create them
		Package = Package ? Package : CreatePackage(*PackagePath);

		// Duplicate the source texture into the package
		TargetTexture = DuplicateObject<UTexture2D>(SourceTexture, Package, *FileName);
		TargetTexture->SetFlags( RF_Public | RF_Standalone);
		TargetTexture->CompressionSettings = TC_VectorDisplacementmap;
		TargetTexture->MipGenSettings = TMGS_NoMipmaps;
		
		TargetTexture->SetPlatformData(SourceTexture->GetPlatformData());
		TargetTexture->UpdateResource();
		Package->MarkPackageDirty();
	
		FAssetRegistryModule::AssetCreated(TargetTexture);

		// Texture package created, so it is saved automatically
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.Error = GError;
		SaveArgs.SaveFlags = SAVE_KeepDirty;

		const bool bSuccess = UPackage::SavePackage(Package, nullptr, *PackagePath, SaveArgs);
		
		UE_LOG(LogTemp, Log, TEXT("Created new texture asset: %s"), *PackagePath);
	}
	else
	{
		TargetTexture->SetPlatformData(SourceTexture->GetPlatformData());
		TargetTexture->UpdateResource();
		Package->MarkPackageDirty();

		UE_LOG(LogTemp, Log, TEXT("Updated existing texture asset: %s"), *PackagePath);
	}

	FString PackageFilePath = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());

	return TargetTexture;
}

void FSKTexture::CopyMaterialTextureToTexture(UMaterialInterface* Material, UTexture2D* Texture)
{
	TArray<FColor> PixelData;
	
	// Configure Temporary Texture
	Texture->SRGB = true;
	Texture->CompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;
	Texture->MipGenSettings =	TextureMipGenSettings::TMGS_NoMipmaps;
	Texture->Filter = TF_Nearest;
	Texture->UpdateResource();

	UTexture2D* SourceMaterialTexture;
	GetBaseColorTexture(Material,SourceMaterialTexture);

	if (SourceMaterialTexture != nullptr)
	{
		// Checks to make sure image data is present, else return.
		if (SourceMaterialTexture->GetPlatformData()->Mips.Num() == 0) return;
				
		if (!FSKTexture::ValidateTextureResolution(SourceMaterialTexture))
		{
			// Texture is not 32x32 sample it
			FSKTexture::SampleTexture(SourceMaterialTexture, Texture);
			// FSKTexture::ReadDataFromTexture(TextureRT, &PixelData);
		}
		else
		{
			FSKTexture::ReadDataFromTexture(SourceMaterialTexture, &PixelData);
			FSKTexture::WriteDataToTexture(&PixelData, Texture);
		}

		// Assigns the copied texture to the Material.
	
		if (UMaterialInstanceConstant* MI = Cast<UMaterialInstanceConstant>(Material))
		{
			MI->SetTextureParameterValueEditorOnly(FName("texture"), Texture);
			return;
		}

		if (UMaterial* M = Material->GetMaterial())
		{
			FExpressionInput* BaseColorInputExpression = M->GetExpressionInputForProperty(MP_BaseColor);
			UMaterialExpressionTextureSample* TextureSamplerExp = Cast<UMaterialExpressionTextureSample>( BaseColorInputExpression->Expression );
			TextureSamplerExp->Texture = Texture;
			return;
		}
	}
}

void FSKTexture::GetBaseColorTexture(UMaterialInterface* MaterialInterface, UTexture2D*& Texture)
{
	if (!MaterialInterface)
		return;

#if WITH_EDITOR
	
	Texture = nullptr;
	
	if (!MaterialInterface)
		return;
	
	static const FName ParamName(TEXT("texture"));
	
	UTexture* TempTexture = nullptr;
	
	// This works for both UMaterial and UMaterialInstance
	if (MaterialInterface->GetTextureParameterValue(ParamName, TempTexture))
	{
		Texture = Cast<UTexture2D>(TempTexture);
	}
	
#endif
}

bool FSKTexture::SetBaseColorTexture(UMaterial* Material, UTexture2D* Texture)
{
	// TODO: check if this is a better solution then the code below.
	// Material->SetTextureParameterValueEditorOnly(FName("texture"), Texture);
	
	if (Material)
	{
		if (Material->HasBaseColorConnected())
		{
			FExpressionInput* BaseColorInputExpression = Material->GetExpressionInputForProperty(MP_BaseColor);

			if (BaseColorInputExpression->Expression)
			{
				UMaterialExpressionTextureSample* TextureSamplerExp = Cast<UMaterialExpressionTextureSample>( BaseColorInputExpression->Expression );
				
				TextureSamplerExp->Texture = Texture;
				return true;
			}
		}
	}
	return false;
}

FString FSKTexture::ImportTextureAsset(const FString TexturePath)
{
	if (!FPaths::FileExists(TexturePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Texture Cannot Import, Not Found %s."), *TexturePath);
		return "";
	};

	FString PackagePath = FSKTexture::ConvertTexturePathToPackagePath(TexturePath);

	// If Package file exists then it is assumed that the Texture has already been imported
	FString PackageFilename;
	if (FPackageName::DoesPackageExist(PackagePath, &PackageFilename))
	{
		return PackagePath;
	}
	
	FString Extension = FPaths::GetExtension(TexturePath).ToLower();
	FString TextureName = FPaths::GetBaseFilename(TexturePath);
	
	TArray<uint8> RawFileData;
	
	if (!FFileHelper::LoadFileToArray(RawFileData, *TexturePath))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed Loading texture %s"), *TexturePath);
		return "";
	}

	
	UPackage* Package = CreatePackage(*PackagePath);
	Package->FullyLoad();

	UTexture2D* NewTexture = NewObject<UTexture2D>(Package, *TextureName, RF_Public | RF_Standalone | RF_MarkAsRootSet);
	NewTexture->AddToRoot();


	WriteDataToTexture(RawFileData, NewTexture);
	
	
	NewTexture->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
	NewTexture->SRGB = true;
	NewTexture->Filter = TF_Nearest;
	NewTexture->CompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;
	NewTexture->AssetImportData->UpdateFilenameOnly(TexturePath);
	
	NewTexture->UpdateResource();
	NewTexture->PostEditChange();
	Package->MarkPackageDirty();
	
	FAssetRegistryModule::AssetCreated(NewTexture);
	
//	Package->SetLoadedPath(FPackagePath(TexturePath));
	
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_None;
	SaveArgs.Error = GError;

	// todo: The two lines below need to be fixed for apple computers
	// FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	// UPackage::SavePackage(Package, NewTexture, *PackageFileName, SaveArgs);
	// todo: --------
	
	// TextureName = ObjectTools::SanitizeObjectName(TextureName);
	//
	//
	//
	// const uint8* BufferStart = RawFileData.GetData();
	//
	//
	// UTextureFactory* TextureFactory = NewObject<UTextureFactory>();
	// TextureFactory->AddToRoot();
	// TextureFactory->SuppressImportOverwriteDialog();
	// UObject* ImportedObj =TextureFactory->FactoryCreateBinary(
	// 	UTexture2D::StaticClass(),
	// 	Package,
	// 	*TextureName,
	// 	RF_Public | RF_Standalone,
	// 	NULL,
	// 	*Extension,
	// 	BufferStart,
	// 	BufferStart + RawFileData.Num(),
	// 	GWarn
	// );
	//
	// UTexture2D* ImportedTexture = Cast<UTexture2D>(ImportedObj);
	// if (!ImportedTexture)
	// {
	// 	UE_LOG(LogTemp, Error, TEXT("Failed to import texture: %s"), *TexturePath);
	// 	return "";
	// }
	//
	// ImportedTexture->SRGB = true;
	// ImportedTexture->Filter = TF_Nearest;
	// ImportedTexture->CompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;
	// if (ImportedTexture->AssetImportData == nullptr)
	// {
	// 	ImportedTexture->AssetImportData = NewObject<UAssetImportData>(ImportedTexture);
	// }
	// ImportedTexture->AssetImportData->UpdateFilenameOnly(TexturePath);
	//
	// ImportedTexture->MarkPackageDirty();
	// FAssetRegistryModule::AssetCreated(ImportedTexture);
	// Package->SetDirtyFlag(true);
	//
	// // Save the package so it persists in the editor
	// FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	//
	// // Texture package created, so it is saved automatically
	// FSavePackageArgs SaveArgs;
	// SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	// SaveArgs.Error = GError;
	// SaveArgs.SaveFlags = SAVE_FromAutosave;
	//
	// const bool bSuccess = UPackage::SavePackage(Package, ImportedTexture, *PackageFileName, SaveArgs);
	// UE_LOG(LogTemp, Display, TEXT("Successfully imported texture: %s"), *TexturePath)
	//
	// ImportedTexture = nullptr;
	// ImportedObj = nullptr;

	return PackagePath;
}

FString FSKTexture::ConvertTexturePathToPackagePath(FString TexturePath)
{
	FString PackagePath = "";
	
	// Convert Disk File Path to Package Path
	FString ProjectContent = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
	if (TexturePath.StartsWith(ProjectContent))
	{
		FString RelativePath = TexturePath.RightChop(ProjectContent.Len());
		RelativePath = FPaths::ChangeExtension(RelativePath, TEXT("")); // Remove extension
		PackagePath = FString("/Game/") + RelativePath;
		PackagePath = PackagePath.Replace(TEXT("\\"), TEXT("/")); // Ensure correct slashes
	}
	
	return PackagePath;
}

UMaterialInterface* FSKTexture::DuplicateMaterialFromPath(
	const FString& SourceMaterialPath,
	const FString& NewMaterialName,
	const FString& TargetPath)
{
	if (SourceMaterialPath.IsEmpty())
	{
		return nullptr;
	}
	
	UMaterialInterface* SourceMaterial = LoadObject<UMaterialInterface>(nullptr, *SourceMaterialPath);
	if (!SourceMaterial)
	{
		return nullptr;
	}

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");

	UObject* NewAsset = AssetToolsModule.Get().DuplicateAsset(NewMaterialName, TargetPath, SourceMaterial);

	return Cast<UMaterialInterface>(NewAsset);
}