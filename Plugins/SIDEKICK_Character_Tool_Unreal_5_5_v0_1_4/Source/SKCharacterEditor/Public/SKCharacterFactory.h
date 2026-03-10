// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AssetDefinitionDefault.h"
// #include "CoreMinimal.h"
#include "CharacterBase.h"
#include "Factories/Factory.h"
#include "UObject/NameTypes.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectGlobals.h"
#include "Materials/MaterialInstanceConstant.h"
#include "SKCharacterFactory.generated.h"


/**
 * 
 */
UCLASS(MinimalAPI, hidecategories=Object)
class USKCharacterFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

private:
	/**
	 * Duplicates a texture to another location in the project.
	 *
	 * @param SourceTexture The texture that will be copied.
	 * @param NewName The name of the new texture file.
	 * @param TargetPath The path where the new texture file will reside.
	 * @return 
	 */
	static UTexture2D* DuplicateTexture(UTexture2D* SourceTexture, const FString& NewName, const FString& TargetPath);

	UMaterialInstanceConstant* CreateMaterialInstanceFromPath(const FString& ParentMaterialPath, const FString& DestinationPath, const FString& NewAssetName
	);
	
};

UCLASS()
class UAssetDefinition_SKCharacterPreset : public UAssetDefinitionDefault
{
	GENERATED_BODY()

public:
	
	virtual FText GetAssetDisplayName() const override { return FText::FromString("Sidekick Character Preset"); }
	virtual FLinearColor GetAssetColor() const override { return FLinearColor(FColor(148, 65, 71)); }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return USKCharacterPreset::StaticClass(); }

	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const FAssetCategoryPath Categories[] = { FText::FromString("Synty")};
		return Categories;
	}

	virtual EAssetCommandResult OpenAssets(const FAssetOpenArgs& OpenArgs) const override;
};