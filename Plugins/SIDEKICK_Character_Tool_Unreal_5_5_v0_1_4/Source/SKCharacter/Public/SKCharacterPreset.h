// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Materials/Material.h"
#include "SKCharacterPreset.generated.h"

class USkeletalMesh;

DECLARE_MULTICAST_DELEGATE(FOnObjectChanged);

UCLASS()
class SKCHARACTER_API USKCharacterPreset : public UObject
{
	GENERATED_BODY()

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Sidekicks", DisplayName="Parts")
	TMap<FString, USkeletalMesh*> SKMParts;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Sidekicks", DisplayName="Material Overrides")
	TMap<FString, UMaterial*> MaterialOverrides;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Sidekicks", DisplayName="Base Material")
	UMaterialInterface* BaseMaterial = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Sidekicks", DisplayName="Body Type")
	float MTBodyType = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Sidekicks", DisplayName="Body Size")
	float MTBodySize = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Sidekicks", DisplayName="Body Muscular")
	float MTBodyMusculature = 0.0f;
	
	FOnObjectChanged OnObjectChanged;

	/** Offset transform data. NOTE: This is in Unity Space */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Sidekicks", DisplayName="Part Morph Target Offset")
	TMap<FString, FTransform> MorphTargetOffsets;

	// todo: Store a temporary 2D Texture that will be baked onto.
	/* Stores the current colour settings session, for the part. Once the user is happy with the settings it will be baked down into a new texture */
	UPROPERTY(BlueprintReadWrite, CallInEditor, Category = "Sidekicks", DisplayName="Temporary Color Pallette")
	UTexture2D* TemporaryTexture;

	// todo: Store the original 2D Texture, so we can read from it or revert back to it
	/* The original texture that was applied to the material, and read from when the UI interactions start. The Source of truth*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Sidekicks", DisplayName="Source Color Pallette")
	UTexture2D* SourceTexture;
};
