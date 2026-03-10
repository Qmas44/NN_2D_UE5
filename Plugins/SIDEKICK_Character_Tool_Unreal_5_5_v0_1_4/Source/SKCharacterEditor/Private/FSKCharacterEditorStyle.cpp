#include "FSKCharacterEditorStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateTypes.h"
#include "Styling/SlateStyleMacros.h"
#include "Interfaces/IPluginManager.h"
#include "Framework/Application/SlateApplication.h"

#define RootToContentDir Style->RootToContentDir

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);
const FVector2D Icon40x40(40.0f, 40.0f);

TSharedPtr<FSlateStyleSet> FSKCharacterEditorStyle::StyleInstance = nullptr;

void FSKCharacterEditorStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FSKCharacterEditorStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

const ISlateStyle& FSKCharacterEditorStyle::Get()
{
	return *StyleInstance;
}

FName FSKCharacterEditorStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("SKCharacterEditorStyle"));
	return StyleSetName;
}

TSharedRef< FSlateStyleSet > FSKCharacterEditorStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("SKCharacterEditorStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("SidekickCharacterTool")->GetBaseDir() / TEXT("Resources"));
	Style->Set("SKCharacterEditor.ImportSidekickPakAction", new IMAGE_BRUSH_SVG(TEXT("Pak_Import_Icon2_large"), Icon40x40));

	return Style;
}