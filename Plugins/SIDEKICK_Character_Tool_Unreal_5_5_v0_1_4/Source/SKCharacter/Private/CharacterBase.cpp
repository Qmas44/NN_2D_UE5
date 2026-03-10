// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterBase.h"

// #include "Components/CapsuleComponent.h"


// Sets default values
ASidekickCharacter::ASidekickCharacter()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SKPreset = nullptr;
}

// Called when the game starts or when spawned
void ASidekickCharacter::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ASidekickCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ASidekickCharacter::OnConstruction(const FTransform& Transform)
{
	if (!SKPreset)
	{
		UE_LOG(LogTemp, Warning, TEXT("No Sidekick Character Preset Selected."));
		return;
	}

#if WITH_EDITOR
	if (!SKPreset->OnObjectChanged.IsBoundToObject(this))
	{
		SKPreset->OnObjectChanged.AddUObject(this, &ASidekickCharacter::RerunConstructionScripts);
	}
//
// 	// Cleans all presets that might exist when editing the Character in the UI, else you end up with stale SKM's in the World.
// 	TArray<USceneComponent*> ChildComponents;
// 	GetMesh()->GetChildrenComponents(true, ChildComponents);
// 	for (USceneComponent* Child : ChildComponents)
// 	{
// 		if (USkeletalMeshComponent* SkeletalChild = Cast<USkeletalMeshComponent>(Child))
// 		{
// 			SkeletalChild->DestroyComponent();
// 		}
// 	}
#endif
	
	
	// if (SKPreset)
	// {
	// 	// Generates a SkeletonMeshComponent instance for each mesh object assigned.
	// 	for (auto Element : SKPreset->SKMParts)
	// 	{
	// 		USkeletalMesh* SMesh = Element.Value;
	// 		FString PartKey = Element.Key;
	//
	// 		if ( SMesh == nullptr ) continue;
	//
	// 		USkeletalMeshComponent* NewComp = NewObject<USkeletalMeshComponent>(this);
	//
	// 		NewComp->SetupAttachment(GetMesh());
	//
	// 		NewComp->RegisterComponent();
	// 		NewComp->SetSkeletalMesh(SMesh);
	// 		NewComp->SetComponentTickEnabled(true);
	//
	// 		// Material overloading for each Part, if the part key exists in the override list.
	// 		if (SKPreset->MaterialOverrides.Contains(PartKey))
	// 		{
	// 			NewComp->SetMaterial(0, SKPreset->MaterialOverrides[PartKey]);
	// 		}
	// 		else
	// 		{
	// 			if (SKPreset->BaseMaterial)
	// 			{
	// 				NewComp->SetMaterial(0, SKPreset->BaseMaterial);
	// 			}
	// 		}
	// 		
	// 		// todo: Populate the base SKM Mesh attribute, to trigger the rendering of the thumbnail. Looks like construction population doesn't help
	// 		// GetMesh()->SetSkeletalMesh(SMesh);
	// 	}
	// 	
	// 	// Offset The SK Character, so its feet are sitting on the ground plane
	// 	float ZOffset = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	// 	FTransform TempTrans = GetTransform();
	// 	TempTrans.SetTranslation(FVector(0,0, -ZOffset));
	// 	GetMesh()->SetRelativeTransform(TempTrans);
	// }
}


void ASidekickCharacter::Destroyed()
{
	if (SKPreset && SKPreset->OnObjectChanged.IsBoundToObject(this))
	{
		SKPreset->OnObjectChanged.RemoveAll(this);
	}
	
	Super::Destroyed();
}