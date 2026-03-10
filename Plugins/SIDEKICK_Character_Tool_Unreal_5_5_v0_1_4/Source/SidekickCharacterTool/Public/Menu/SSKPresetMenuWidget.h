// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Models/MainMenuModel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SExpandableArea.h"

/**
 * 
 */
class SIDEKICKCHARACTERTOOL_API SSKPresetMenuWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSKPresetMenuWidget)
		: _Model()
		{
		}

		SLATE_ARGUMENT(TSharedPtr<MainMenuModel>, Model);
		
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

private:
	
	/** Updates the available presets for the active specie */
	void PopulateAvailablePresets();

	/** Removes all Parts from the available Preset Parts lists (Head, Upper Body, Lower Body) */
	void ClearAvailablePresetParts();
	
	/** Adds all the Parts groups for the available presets */
	void AddAvailablePresetParts();

	/** Resets the Color Preset Selection to the first index */
	void PopulateAvailableColors();
	
private:

	/** Randomize each preset dropdown.*/
	void RandomizePresets();
	
	/** The shared UI Model data*/
	TSharedPtr<MainMenuModel> Model;
	
	TSharedRef<SExpandableArea> GenerateSpeciesSection();
	TSharedRef<SExpandableArea> GenerateFiltersSection();
	TSharedRef<SExpandableArea> GenerateRandomizeSection();
	TSharedRef<SExpandableArea> GeneratePresetSection();

	void PopulateData();
	
	// -- Species, dropdown event handling
	
	TSharedRef<SWidget> GenerateSpeciesComboItem(TSharedPtr<FString> InItem);
	FText GetSpeciesComboText() const;
	void HandleSpeciesComboChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo);

	// -- Preset Filtering
	TSharedPtr<SVerticalBox> PresetVerticalBox;
	void PopulatePresetFilterUI();
	// TArray<TSharedPtr<FString>> SpeciesComboItems;
	
	// -- Parts
	
	TMap<FString, int> SelectedPartsOption;
	TMap<FString, TArray<TSharedPtr<FString>>> AvailablePartPresets;
	TSharedRef<SWidget> GeneratePartsComboItem(TSharedPtr<FString> InItem);
	FText GetPartComboText(FString PartType) const;
	void HandlePresetParteChanged(FString PartType, TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo);
	
	// -- Body Type handling and variables

	/** Selected body type from the dropdown */
	int SelectedComboBoxBodyPresetOption = 0;
	TSharedRef<SWidget> GenerateBodyTypeComboItem(TSharedPtr<FString> InItem);
	FText GetBodyTypeComboText() const;
	void HandleBodyTypeChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo);

	// -- Color

	TMap<FString, int> SelectedSpeciesColorOption;
	TMap<FString, TArray<TSharedPtr<FString>>> AvailableColorPresets;
	TSharedRef<SWidget> GenerateColorComboItem(TSharedPtr<FString> InItem);
	FText GetColorComboText(FString ColorGroup) const;
	void HandleColorChanged(FString ColorGroup, TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo);
	EVisibility ColourUIVisibility(FString ColorGroup);
	void UpdateAvailableColorGroups(FString ColorGroup="");

public:
	/** Forces UI elements to repopulate there data */
	void RefreshUI();
};
