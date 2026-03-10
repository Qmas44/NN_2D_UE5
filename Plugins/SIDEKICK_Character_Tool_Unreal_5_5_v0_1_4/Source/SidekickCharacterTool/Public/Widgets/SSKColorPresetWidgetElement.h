// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Models/MainMenuModel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Layout/SExpandableArea.h"

/**
 * 
 */
class SIDEKICKCHARACTERTOOL_API SSKColorPresetWidgetElement : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSKColorPresetWidgetElement)
		: _Model(),
		  _Text(),
		  _AddClearButton(false),
		  _ComboBoxActiveText(),
		  _ComboOptionsSource(),
		  _ComboIndex(0)
		{}

		// Model data for the MVVM
		SLATE_ARGUMENT( TSharedPtr< MainMenuModel >, Model)
		// Text that will be displayed with in the widget
		SLATE_ATTRIBUTE( FText, Text )
		// Adds a clear button to the selection
		SLATE_ATTRIBUTE( bool, AddClearButton )

		SLATE_EVENT(SComboBox<TSharedPtr<FString>>::FOnGenerateWidget, ComboBoxOnGenerateWidget)
		SLATE_EVENT(SComboBox<TSharedPtr<FString>>::FOnSelectionChanged, ComboBoxOnSelectionChanged)
		SLATE_ATTRIBUTE(FText, ComboBoxActiveText)
		/** Source for the newly created combo box drop down */
		SLATE_ARGUMENT(TArray<TSharedPtr<FString>>*, ComboOptionsSource)
		/** Shares the Index of the combobox */
		SLATE_ATTRIBUTE(int, ComboIndex)
		
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);
	
private:

	/** The shared UI Model data*/
	TSharedPtr<MainMenuModel> Model;
	
	/** Text that will appear in the widget*/
	TAttribute< FText > TextContent;

	TAttribute< bool > ClearButtonVisible;

	TArray<TSharedPtr<FString>>* OptionsSource;
	TAttribute< int > ComboboxIndex;
	
	// UI element construction
	TSharedRef<SButton> ConstructClearButton();
	TSharedRef<SButton> ConstructPrevButton();
	TSharedRef<SButton> ConstructNextButton();
	TSharedRef<SButton> ConstructRandomButton();
	TSharedRef<SComboBox<TSharedPtr<FString>>> ConstructDropdown();

public:
	TSharedPtr<SComboBox<TSharedPtr<FString>>> DropdownWidgetPtr;
};
