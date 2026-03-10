// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Menu/SSKOptionsMenuWidget.h"
#include "Models/MainMenuModel.h"
#include "Widgets/SCompoundWidget.h"

class USKCharacterPreset;
class SSKPresetMenuWidget;
class SSKPartsMenuWidget;
class SSKBodyMenuWidget;
class SSKColorMenuWidget;

/** The available Sidekick UI Tabs */
enum class ESidekickMenuViews : int32
{
	/** Displays the available Presets */
	Presets,

	/** Displays the available Parts */
	Parts,

	/** Displays the blendshape controls */
	Body,

	/** Displays the part colour overrides */
	Colors,

	/** Displays the part options */
	Options,
};

/**
 * 
 */
class SIDEKICKCHARACTERTOOL_API SSKMainMenuWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSKMainMenuWidget)
			: _EditingObject(nullptr)
		{
		}

		SLATE_ARGUMENT(USKCharacterPreset*, EditingObject)
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	~SSKMainMenuWidget();

public:
	/** The active menu being displayed */
	ESidekickMenuViews ActiveMenu;

	/** Get the mode that we are displaying data in */
	ESidekickMenuViews GetActiveMenuMode() const { return ActiveMenu; }

	/** Changes the current open menu */
	void ChangeActiveMenu(ESidekickMenuViews ChosenMenu);

	TSharedPtr<SWidget> MakeActiveMenu();

	/** This holds the main menu's WidgetSwitcher's widget*/
	TSharedPtr<SWidget> ActiveMenuWidget;

	void ResetMenus();

private:
	/** Generates the main menu's banner */
	TSharedPtr<SWidget> MakeMainBanner() const;

	TSharedPtr<SWidget> CharacterImportExportWidget();

	TSharedPtr<SWidget> MenuSelectionWidget();

	// Menu Widgets

	TSharedPtr<SSKPresetMenuWidget> PresetMenu;
	TSharedPtr<SSKPartsMenuWidget> PartsMenu;
	TSharedPtr<SSKBodyMenuWidget> BodyMenu;
	TSharedPtr<SSKColorMenuWidget> ColorMenu;
	TSharedPtr<SSKOptionsMenuWidget> OptionMenu;

public:
	/** The model that is being used to populate the Menus*/
	TSharedPtr<MainMenuModel> UIModel;

	void InitializeModelData();

	// The Sidekick preset that is being altered.
	USKCharacterPreset* SKCharacterPreset = nullptr;
	
	// Helper function that forces the PresetMenu to update as if a species event triggered
	void RefreshPresetSpecie();

private:
	/** Performs all the data validation of file / folder creation before the tool runs */
	void PreFlightChecks();

	void GenerateSyntyDefaultFolderStructure();

	void ValidateDatabaseFile();

	void ValidateDefaultMaterial();
};
