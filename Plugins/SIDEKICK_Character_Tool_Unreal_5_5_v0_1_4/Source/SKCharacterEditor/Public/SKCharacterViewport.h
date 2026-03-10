// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CharacterBase.h"
#include "SCommonEditorViewportToolbarBase.h"
#include "SEditorViewport.h"

class FAdvancedPreviewScene;

class SSKCharacterViewport : public SEditorViewport, public ICommonEditorViewportToolbarInfoProvider
{

public:
	SLATE_BEGIN_ARGS(SSKCharacterViewport)
		: _EditingObject(nullptr)
		{}
		
		SLATE_ARGUMENT(USKCharacterPreset*, EditingObject)
	SLATE_END_ARGS()

	void Construct(const FArguments& _args);

	// Toolbar interface
	virtual TSharedRef<SEditorViewport> GetViewportWidget() override { return SharedThis(this); }
	virtual TSharedPtr<FExtender> GetExtenders() const override { return MakeShareable(new FExtender); }
	virtual void OnFloatingButtonClicked() override {};

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 7
	virtual TSharedPtr<SWidget> MakeViewportToolbar() override;
#endif
	
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	
	TSharedPtr<FAdvancedPreviewScene> AdvancedPreviewScene = nullptr;
	TSoftObjectPtr<USKCharacterPreset> SKCharacterPreset = nullptr;

protected:
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
	TSharedPtr<FEditorViewportClient> LevelViewportClient;

	ASidekickCharacter* PreviewCharacter = nullptr;
};
