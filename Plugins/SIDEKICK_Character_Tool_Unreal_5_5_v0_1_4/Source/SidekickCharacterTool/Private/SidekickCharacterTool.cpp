// Copyright Epic Games, Inc. All Rights Reserved.

#include "SidekickCharacterTool.h"
#include "SidekickCharacterToolStyle.h"
#include "SidekickCharacterToolCommands.h"
#include "LevelEditor.h"
#include "SSKMainMenuWidget.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"

static const FName SidekickCharacterToolTabName("SidekickCharacterTool");

#define LOCTEXT_NAMESPACE "FSidekickCharacterToolModule"

void FSidekickCharacterToolModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FSidekickCharacterToolStyle::Initialize();
	FSidekickCharacterToolStyle::ReloadTextures();

	FSidekickCharacterToolCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FSidekickCharacterToolCommands::Get().OpenSidekickPluginWindow,
		FExecuteAction::CreateRaw(this, &FSidekickCharacterToolModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FSidekickCharacterToolModule::RegisterMenus));
}

void FSidekickCharacterToolModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FSidekickCharacterToolStyle::Shutdown();

	FSidekickCharacterToolCommands::Unregister();
}

TSharedRef<SDockTab> FSidekickCharacterToolModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SSKMainMenuWidget)
		];
}

void FSidekickCharacterToolModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(SidekickCharacterToolTabName);
}

void FSidekickCharacterToolModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSidekickCharacterToolModule, SidekickCharacterTool)