// Fill out your copyright notice in the Description page of Project Settings.


#include "SKCharacterFactory.h"

#include "AssetToolsModule.h"
#include "SKCharacterToolkit.h"
#include "Interfaces/IPluginManager.h"
#include "Engine/Texture2D.h"

#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInterface.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Editor.h"
#include "Factories/MaterialFactoryNew.h"


USKCharacterFactory::USKCharacterFactory(const FObjectInitializer& ObjectInitializer): Super(ObjectInitializer)
{
	SupportedClass = USKCharacterPreset::StaticClass();
	
	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

UObject* USKCharacterFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{

	UE_LOG(LogTemp, Display, TEXT("USKCharacterFactory::FactoryCreateNew"));

	USKCharacterPreset* NewPreset = NewObject<USKCharacterPreset>(InParent, Class, Name, Flags | RF_Transactional); 

	FString PresetPath = NewPreset->GetPathName();
	
	FString PluginBase = "";
	FString DefaultTextureToken = "Content/Textures";
	FString DefaultTextureName = "T_Sidekicks_Default.uasset";
	FString DefaultMaterialPath = "Materials/M_Default_Sidekick.M_Default_Sidekick";

	// Gets the modules path, find the default texture and copies it
	static const FName ModuleName(TEXT("SidekickCharacterTool"));

	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(ModuleName.ToString());
	
	if (Plugin.IsValid())
	{
		PluginBase = Plugin->GetBaseDir();

		FString PluginP = FPaths::ProjectPluginsDir();
		PluginP = FPaths::ConvertRelativePathToFull(PluginP);
		PluginBase = FPaths::ConvertRelativePathToFull(PluginBase);
		
		FString PluginTexturePath = FPaths::Combine(PluginBase, DefaultTextureToken, DefaultTextureName);
		FString PluginMaterialPath = "/" + FPaths::Combine(Plugin->GetName(), DefaultMaterialPath);

		FString ContentPath = FPaths::ProjectContentDir();
		
		// if(FPaths::FileExists(PluginTexturePath))
		{
			FPaths::NormalizeFilename(PluginTexturePath);
			FPackageName::TryConvertFilenameToLongPackageName(PluginTexturePath,ContentPath);
			
			UTexture2D* DefaultTexture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, *ContentPath));

			FString OriginalFolder = FPaths::GetPath(PresetPath);
			
			if(!DefaultTexture)
			{
				UE_LOG(LogTemp, Display, TEXT("Failed to find default texture"));
			}
			else
			{
				UE_LOG(LogTemp, Display, TEXT("DefaultTexture: %s"), *PluginTexturePath);

				
				FString NewPresetTextureName = "T_" + FPaths::GetBaseFilename(PresetPath);
				UTexture2D* NewTexture = DuplicateTexture(DefaultTexture, NewPresetTextureName,OriginalFolder);

				NewPreset->SourceTexture = NewTexture;
			}

			// Creates Material Instance
			FString NewMaterialName = "M_" + FPaths::GetBaseFilename(PresetPath);
			UMaterialInstanceConstant* NewMaterial = CreateMaterialInstanceFromPath(PluginMaterialPath,
				OriginalFolder,
				NewMaterialName);

			NewPreset->BaseMaterial = NewMaterial;

			// Assign Material texture
			NewMaterial->SetTextureParameterValueEditorOnly(FName("texture"), NewPreset->SourceTexture);
			
		}
	}
	
	return NewPreset;
}

EAssetCommandResult UAssetDefinition_SKCharacterPreset::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
	for (USKCharacterPreset* EditingAsset : OpenArgs.LoadObjects<USKCharacterPreset>())
	{
		const TSharedRef<FSKCharacterToolkit> CustomObjectToolkit(new FSKCharacterToolkit());
		CustomObjectToolkit->Initialize(OpenArgs.ToolkitHost, EditingAsset);
	}

	return EAssetCommandResult::Handled;
}

UTexture2D* USKCharacterFactory::DuplicateTexture(UTexture2D* SourceTexture, const FString& NewName, const FString& TargetPath)
{
	if (!SourceTexture)
	{
	return nullptr;
	}

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");

	UObject* DuplicateObject = AssetToolsModule.Get().DuplicateAsset(NewName, TargetPath, SourceTexture);

	return Cast<UTexture2D>(DuplicateObject);
}

#if WITH_EDITOR

/**
 * Creates an Instance Material from the Material Path provided
 * 
 * @param ParentMaterialPath "/Game/Materials/M_Master.M_Master"
 * @param DestinationPath "/Game/MyFolder"
 * @param NewAssetName "MI_NewMaterial"
 * @return 
 */
UMaterialInstanceConstant* USKCharacterFactory::CreateMaterialInstanceFromPath(
	const FString& ParentMaterialPath,
	const FString& DestinationPath,
	const FString& NewAssetName
)
{
	// Load Parent Material
	UMaterialInterface* ParentMaterial = LoadObject<UMaterialInterface>(nullptr, *ParentMaterialPath);
	
	if (!ParentMaterial)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load parent material."));
		return nullptr;
	}
	
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	UMaterialInstanceConstantFactoryNew* Factory =NewObject<UMaterialInstanceConstantFactoryNew>();
	Factory->InitialParent = ParentMaterial;

	// Create Asset
	UObject* NewAsset = AssetToolsModule.Get().CreateAsset(
		NewAssetName,
		DestinationPath,
		UMaterialInstanceConstant::StaticClass(),
		Factory
	);

	UMaterialInstanceConstant* MIC = Cast<UMaterialInstanceConstant>(NewAsset);
	
	if (MIC)
	{
		MIC->MarkPackageDirty();
		FAssetRegistryModule::AssetCreated(MIC);
		MIC->PostEditChange();
	}

	return MIC;
}

#endif
