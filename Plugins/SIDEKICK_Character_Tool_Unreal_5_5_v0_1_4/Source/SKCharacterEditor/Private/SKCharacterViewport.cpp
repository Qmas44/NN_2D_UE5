// Fill out your copyright notice in the Description page of Project Settings.


#include "SKCharacterViewport.h"
#include "AdvancedPreviewScene.h"
#include "CameraController.h"
#include "SKCharacterEditor.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Application/ThrottleManager.h"

void SSKCharacterViewport::Construct(const FArguments& InArgs)
{

	FSlateThrottleManager::Get().DisableThrottle(true);
	
	AdvancedPreviewScene = MakeShareable(new FAdvancedPreviewScene(FPreviewScene::ConstructionValues()));

	SEditorViewport::Construct(SEditorViewport::FArguments());

	SKCharacterPreset = InArgs._EditingObject;
	if (!SKCharacterPreset.IsValid())
	{
		UE_LOG(LogSKCharacterEditor, Error, TEXT("Editing asset is not valid."));
		return;
	}

	
	
	// Generates The Character using th C++ Constructor
	// 	ASidekickCharacter* PreviewCharacter = GetWorld()->SpawnActor<ASidekickCharacter>();
	// 	PreviewCharacter->SKPreset = SKCharacterPreset.Get();
	// 	PreviewCharacter->RerunConstructionScripts();

	// Generates The Viewport using the Blueprint that exists in the project.
	FString AssetPath = TEXT("/SidekickCharacterTool/Blueprints/SK_Manual_LeaderPose_Setup");
	UClass* BP_TargetItemClass = ConstructorHelpersInternal::FindOrLoadClass(AssetPath, UObject::StaticClass());

	if (BP_TargetItemClass)
	{
			PreviewCharacter = GetWorld()->SpawnActor<ASidekickCharacter>(BP_TargetItemClass);
			PreviewCharacter->SKPreset = SKCharacterPreset.Get();
			PreviewCharacter->RerunConstructionScripts();
		
			PreviewCharacter->GetMesh()->PlayAnimation(PreviewCharacter->GetMesh()->AnimationData.AnimToPlay, true);
			PreviewCharacter->GetMesh()->SetUpdateAnimationInEditor(true);
	}
	
}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 7
/** Shows the viewport options */
TSharedPtr<SWidget> SSKCharacterViewport::MakeViewportToolbar()
{
	return SNew(SCommonEditorViewportToolbarBase, SharedThis(this));
}
#endif


TSharedRef<FEditorViewportClient> SSKCharacterViewport::MakeEditorViewportClient()
{
	LevelViewportClient = MakeShareable(new FEditorViewportClient(nullptr, AdvancedPreviewScene.Get(), SharedThis(this)));
	
	LevelViewportClient->ViewportType = LVT_Perspective;
	LevelViewportClient->bSetListenerPosition = false;
	LevelViewportClient->SetRealtime(true);
	LevelViewportClient->ViewFOV = 60.f;

	LevelViewportClient->SetViewLocation( FVector(0.f,0.f,0.f));
	LevelViewportClient->SetViewRotation( FRotator(-20.0f, 180.0f, 0.0f) );
	LevelViewportClient->OverrideNearClipPlane(0.001f);
	LevelViewportClient->SetViewLocationForOrbiting(FVector(0, 0, 80.f), 150);
	LevelViewportClient->SetOrthoZoom(1.f);
	
	return LevelViewportClient.ToSharedRef();
}

/** Overloaded in order to get animation playback working */
void SSKCharacterViewport::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	GetWorld()->Tick(LEVELTICK_All, InDeltaTime);
}

// NOTE: Slate.bAllowThrottling false < to deactivate slate throttling