// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SCheckBoxList.h"
#include "Enums/PartGroups.h"
#include "Models/MainMenuModel.h"
#include "Widgets/SCompoundWidget.h"


DECLARE_MULTICAST_DELEGATE(FOnOufitCheckboxChanged);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnRandomizeButtonPressed, EPartGroup);


/**
 * 
 */
class SIDEKICKCHARACTERTOOL_API SSKPartsMenuWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSKPartsMenuWidget)
		: _Model()
		{
		}

		SLATE_ARGUMENT( TSharedPtr<MainMenuModel>, Model);
		
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);
	

private:

	/** The shared UI Model data*/
	TSharedPtr< MainMenuModel > Model;

	// Dropdown event handling for species
	TSharedRef<SWidget> GenerateSpeciesComboItem(TSharedPtr<FString> InItem);
	FText GetSpeciesComboText() const;
	void HandleSpeciesComboChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo);

	// Outfit Events
	void OnOutfitCheckStateChanged(int Index);
	
	TSharedPtr<FOnOufitCheckboxChanged> OnOufitCheckboxChanged;
	TSharedPtr<FOnRandomizeButtonPressed> OnRandomizeButtonPressed;

	/** Clears all checkbox entries and adds the available outfits */
	void RefreshOutfitCheckBoxes();
	
protected:
	/** Outfits checkbox list */
	TSharedPtr< SCheckBoxList > CheckBoxList;

public:
	/** Forces UI elements to repopulate there data */
	void RefreshUI();
};
