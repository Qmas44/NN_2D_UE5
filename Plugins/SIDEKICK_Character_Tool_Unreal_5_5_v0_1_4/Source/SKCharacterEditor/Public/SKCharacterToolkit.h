// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Toolkits/AssetEditorToolkit.h"
#include "CharacterBase.h"
#include "SSKMainMenuWidget.h"

class USKCharacterPreset;

class SKCHARACTEREDITOR_API FSKCharacterToolkit : public FAssetEditorToolkit
{
public:

	void Initialize(const TSharedPtr<IToolkitHost>& InitToolkitHost, USKCharacterPreset* Character);
	
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& TabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& TabManager) override;
	
	virtual FName GetToolkitFName() const override { return FName("Sidekick Character Editor"); }
	virtual FText GetBaseToolkitName() const override { return INVTEXT("Sidekick Character Editor"); }
	virtual FString GetWorldCentricTabPrefix() const override { return "SidekickCharacterEditor"; }
	virtual FLinearColor GetWorldCentricTabColorScale() const override { return FLinearColor::White; }

	virtual void RegisterToolbarExtension();
	void ExtendToolbar(FToolBarBuilder& ToolbarBuilder);
	
	void ImportSidekickPak();

	// Call when importing is complete and the UI needs to be refreshed.
	void RefreshMenu();
	
private:
	TObjectPtr<USKCharacterPreset> SidekickCharacterPreset = nullptr;

	TSharedPtr<class FUICommandList> PluginCommands;

	TWeakPtr<SSKMainMenuWidget> SKMenuWidget;
};
