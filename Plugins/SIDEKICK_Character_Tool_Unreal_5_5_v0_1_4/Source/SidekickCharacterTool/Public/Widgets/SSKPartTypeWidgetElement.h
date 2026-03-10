// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Containers/UnrealString.h"
#include "Menu/SSKPartsMenuWidget.h"
#include "Models/MainMenuModel.h"
#include "Widgets/SCompoundWidget.h"

/**
 * 
 */
class SIDEKICKCHARACTERTOOL_API SSKPartTypeWidgetElement : public SCompoundWidget
{
	
public:
	
	SLATE_BEGIN_ARGS(SSKPartTypeWidgetElement)
		: _Text(),
		  _Model(),
		  _PartType(),
		  _OnCustomOutfitChangedEvent(),
		  _OnRandomizeCollectionEvent()
		{}
		
		/** The text displayed in this text block */
		SLATE_ATTRIBUTE( FText, Text )
		SLATE_ARGUMENT( TSharedPtr< MainMenuModel >, Model);
		SLATE_ARGUMENT( ECharacterPartType, PartType);
		SLATE_ARGUMENT(TSharedPtr<FOnOufitCheckboxChanged>, OnCustomOutfitChangedEvent)
		SLATE_ARGUMENT(TSharedPtr<FOnRandomizeButtonPressed>, OnRandomizeCollectionEvent)

	SLATE_END_ARGS()
	
	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	void GetAvailablePartsForGroup();

	FText GetPartComboText();

	SSKPartTypeWidgetElement();
	
	/** Triggered when combobox changes */
	void HandlePartComboChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo);
	
	/** Returns the state of the lock boolean. If the lock has been enabled then buttons cannot be pressed.*/
	bool CanPressButton() const;
	
private:
	TAttribute< FText > TextContent;
	
	ECharacterPartType EPartType;

	/** The shared UI Model data*/
	TSharedPtr< MainMenuModel > Model;

	/** List of Part names that appear in the dropdown */
	TArray<TSharedPtr<FString>> PartList;

	int SelectedComboBoxPartIndex = 0;

	TSharedPtr<FOnOufitCheckboxChanged> OnCustomEvent;

	TSharedPtr<FOnRandomizeButtonPressed> OnRandomizeCollectionEvent;

	void UpdatePreset();

	void PopulateDropdownsFromPreset();

	void RandomizePart();

	bool bPartLocked = false;

protected:


	int FindPartInList(FString PartName) const;
};
