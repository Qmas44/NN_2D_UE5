// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu/SSKPartsMenuWidget.h"

// #include "SCheckBoxList.h"
#include "SlateOptMacros.h"
#include "Algo/ForEach.h"


#include "Enums/PartGroups.h"
#include "Assets/SParts.h"
#include "Widgets/SSKPartTypeWidgetElement.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SScrollBar.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SCheckBox.h"


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SSKPartsMenuWidget::Construct(const FArguments& InArgs)
{

	Model = InArgs._Model;

	OnOufitCheckboxChanged = MakeShared<FOnOufitCheckboxChanged>();
	OnRandomizeButtonPressed = MakeShared<FOnRandomizeButtonPressed>();
	
	// Outfits
	
	// todo: Look at creating our own SSKCheckBoxList
	// Setup of the outfits checkboxes
	CheckBoxList = SNew(SCheckBoxList)
		.ItemHeaderLabel(FText::FromString("All Outfits"))
		.IncludeGlobalCheckBoxInHeaderRow(true)
		.OnItemCheckStateChanged(this, &SSKPartsMenuWidget::OnOutfitCheckStateChanged)
		;
	
	for (auto OutfitName : Model->AvailableOutfits)
	{
		CheckBoxList->AddItem(FText::FromString(*OutfitName), true);	
	}
	
	// Parts

	TSharedRef<SVerticalBox> PartGroupVerticalBox = SNew(SVerticalBox);
	for (int32 i = 1; i < static_cast<int32>(EPartGroup::Max); ++i)
	{
		EPartGroup PartGroup = static_cast<EPartGroup>(i);
		FString PartGroupString = PartGroupToString(PartGroup);

		TSharedRef<SVerticalBox> PartVerticalBox = SNew(SVerticalBox);

		PartVerticalBox->AddSlot()
		.Padding(FMargin(0.0f, 0.0f, 15.0f, 2.0f))
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString("Randomize " + PartGroupString))
			]
			.OnClicked_Lambda([this, PartGroup]()
			{
				OnRandomizeButtonPressed->Broadcast(PartGroup);
				return FReply::Handled();
			})
		];

		// Creates a Row for each part that exists in the Part Group
		for (auto PartType : PartsGrouping[PartGroup])
		{
			FString PartName = CharacterPartTypeToString(PartType);
			PartVerticalBox->AddSlot()
			.AutoHeight()
			[
				SNew(SSKPartTypeWidgetElement)
				.Text(FText::FromString(*PartName))
				.PartType(PartType)
				.Model(Model)
				.OnCustomOutfitChangedEvent(OnOufitCheckboxChanged)
				.OnRandomizeCollectionEvent(OnRandomizeButtonPressed)
			];
		}
		
		TSharedRef<SExpandableArea> PartGroupExpander = SNew(SExpandableArea)
			.AreaTitle(FText::FromString(PartGroupString))
			.InitiallyCollapsed(false)
			.Padding(FMargin(15.0f, 2.0f, 0.0f, 0.0f))
			.BodyContent()
			[
				PartVerticalBox
			];
		
		PartGroupVerticalBox->AddSlot()
		.AutoHeight()
		[
			PartGroupExpander
		];
	}
	
	ChildSlot
	[
		SNew(SVerticalBox)

		// Species
		
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SExpandableArea)
			.AreaTitle(FText::FromString("Select - Species"))
			.InitiallyCollapsed(false)
			// .Padding(FMargin(8.0f, 8.0f, 8.0f, 4.0f))
			.BodyContent()
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString("Species"))
				]
				
				+ SHorizontalBox::Slot()
				[
					SNew(SComboBox<TSharedPtr<FString>>)
					.OptionsSource(&Model->AvailableSpecies)
					// .InitiallySelectedItem()
					.OnGenerateWidget(this, &SSKPartsMenuWidget::GenerateSpeciesComboItem)
					.OnSelectionChanged(this, &SSKPartsMenuWidget::HandleSpeciesComboChanged)
					.Content()
					[
						// note: Methods are passed into slate object, and get called every tick 
						SNew(STextBlock)
						.Text(this, &SSKPartsMenuWidget::GetSpeciesComboText)
					]
				]
			]
		]

		// Outfits
		
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SExpandableArea)
			.AreaTitle(FText::FromString("Select - Outfit Filter"))
			.InitiallyCollapsed(false)
			.BodyContent()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				[
					// todo: Implement Checkbox list
					CheckBoxList.ToSharedRef()
				]
			]
		]

		// Parts
		+ SVerticalBox::Slot()
		.FillHeight(1)
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			SNew(SScrollBox)
			.ScrollBarVisibility(EVisibility::All)
			+ SScrollBox::Slot()
			[
				SNew(SExpandableArea)
				.AreaTitle(FText::FromString("Select - Parts"))
				.InitiallyCollapsed(false)
				.Padding(FMargin(15.0f, 2.0f, 0.0f, 0.0f))
				.BodyContent()
				[
					PartGroupVerticalBox
				]
			]
		]
	];
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

TSharedRef<SWidget> SSKPartsMenuWidget::GenerateSpeciesComboItem(TSharedPtr<FString> InItem)
{
	return SNew(STextBlock).Text(FText::FromString(*InItem));
}

FText SSKPartsMenuWidget::GetSpeciesComboText() const
{
	if (Model->AvailableSpecies.Num() > 0)
	{
		return FText::FromString(*Model->AvailableSpecies[Model->SelectedComboBoxSpeciesOptions]);
	}
	return FText::GetEmpty();
}

void SSKPartsMenuWidget::HandleSpeciesComboChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo)
{
	if (Item.IsValid())
	{
		Model->SelectedComboBoxSpeciesOptions = Model->AvailableSpecies.IndexOfByKey(Item);

		Model->SelectedSpecie = Item;

		Model->PostActiveSpecieUpdate();
		
		RefreshOutfitCheckBoxes();

        // Creates Collections of Parts for the dropdown UI
        Model->SetupGroupedPartDataStructure();

		// Triggers Delegate when outfit state changes
		OnOufitCheckboxChanged->Broadcast();
		
		Model->UpdateSpecieParts();
	}
}

void SSKPartsMenuWidget::RefreshUI()
{
	Model->PostActiveSpecieUpdate();
		
	RefreshOutfitCheckBoxes();

	// Creates Collections of Parts for the dropdown UI
	Model->SetupGroupedPartDataStructure();

	// Triggers Delegate when outfit state changes
	OnOufitCheckboxChanged->Broadcast();
		
	Model->UpdateSpecieParts();
}

void SSKPartsMenuWidget::RefreshOutfitCheckBoxes()
{
	// Clears all Items
	int ItemCount = CheckBoxList->GetNumCheckboxes();
	for (int i = 0; i <= ItemCount; i++)
	{
		CheckBoxList->RemoveItem(0);
	}
		
	for (auto OutfitName : Model->AvailableOutfits)
	{
		CheckBoxList->AddItem(FText::FromString(*OutfitName), true);	
	}
}


void SSKPartsMenuWidget::OnOutfitCheckStateChanged(int Index)
{
	UE_LOG(LogTemp, Display, TEXT("SSKPartsMenuWidget::OnOutfitCheckStateChanged: %i"), Index);

	// All checkbox was selected
	if (Index == -1)
	{
		int count = 0;
		for (auto bIsChecked : CheckBoxList->GetValues())
		{
			TSharedPtr<FString> TempName = Model->AvailableOutfits[count];
			count+=1;

			if (bIsChecked)
			{
				Model->ActivateOutfit(TempName);
			}
			else
			{
				Model->DeactivateOutfit(TempName);
			}
			
		}
	}
	// Individual checkbox was selected
	else
	{
		bool bIsChecked = CheckBoxList->IsItemChecked(Index);
		TSharedPtr<FString> TempName = Model->AvailableOutfits[Index];

		if (bIsChecked)
		{
			Model->ActivateOutfit(TempName);
		}
		else
		{
			Model->DeactivateOutfit(TempName);
		}
	}

	Model->SetupGroupedPartDataStructure();
	
	// Triggers Delegate when outfit state changes
	OnOufitCheckboxChanged->Broadcast();
}