// Copyright Epic Games, Inc. All Rights Reserved.

#include "SidekickCharacterToolStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FSidekickCharacterToolStyle::StyleInstance = nullptr;

void FSidekickCharacterToolStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FSidekickCharacterToolStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FSidekickCharacterToolStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("SidekickCharacterToolStyle"));
	return StyleSetName;
}

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);

TSharedRef< FSlateStyleSet > FSidekickCharacterToolStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("SidekickCharacterToolStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("SidekickCharacterTool")->GetBaseDir() / TEXT("Resources"));

	Style->Set("SidekickCharacterTool.OpenSidekickPluginWindow", new IMAGE_BRUSH(TEXT("syntystudios_icon_red"), Icon20x20));

	Style->Set("SidekickCharacterTool.MainMenuBanner", new IMAGE_BRUSH(TEXT("T_SidekickTitle"), FVector2D(500,125)));

	Style->Set("SidekickCharacterTool.SKDefaultRed", new FSlateColorBrush( FLinearColor(FColor(209, 34, 51)) ) );

	Style->Set("SidekickCharacterTool.ClearIcon", new IMAGE_BRUSH(TEXT("T_Clear"), Icon20x20));
	Style->Set("SidekickCharacterTool.RandomIcon", new IMAGE_BRUSH(TEXT("T_Random"), Icon20x20));
	
	return Style;
}

void FSidekickCharacterToolStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FSidekickCharacterToolStyle::Get()
{
	return *StyleInstance;
}
