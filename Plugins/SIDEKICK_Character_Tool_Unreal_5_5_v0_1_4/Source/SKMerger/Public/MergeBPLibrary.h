// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SKCharacter/Public/SKCharacterPreset.h"
#include "MergeBPLibrary.generated.h"


/**
 * 
 */
UCLASS()
class SKMERGER_API UMergeBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintCallable, Category = "Synty Merge")
	static void MergeSkeletalMesh(USkeletalMesh* MeshA, USkeletalMesh* MeshB);

	UFUNCTION(BlueprintCallable, Category = "Synty Merge")
	static void SelectSkeletalPackagePath(USKCharacterPreset* SKPreset, FString& OutSaveObjectPath);

	UFUNCTION(BlueprintCallable, Category = "Synty Merge")
	static void GenerateSkeletalMesh(USKCharacterPreset* SKPreset, bool bMergeSection, bool bKeepFacialMorphTargets, bool bKeepBodyMorphTargets, bool bCustomMergeLocation);
	
};
