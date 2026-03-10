// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu/SSKPresetMenuWidget.h"

#include "Styling/SlateStyleRegistry.h"
// #include "Styling/SlateStyle.h"
#include "SlateOptMacros.h"
#include "Assets/FSKTexture.h"
#include "Assets/SParts.h"
#include "Enums/ColorGroup.h"
#include "Enums/PartGroups.h"
#include "Menu/SSKBodyMenuWidget.h"
#include "Widgets/SSKColorPresetWidgetElement.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SExpandableArea.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

// todo: Implement the Colour drop downs to update depending on the Species selected

void SSKPresetMenuWidget::Construct(const FArguments& InArgs)
{
	Model = InArgs._Model;

	PopulateData();
	
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			GenerateSpeciesSection()
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			GenerateFiltersSection()
		]
		
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			GenerateRandomizeSection()
		]
		
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			GeneratePresetSection()
		]
	];
	AddAvailablePresetParts();
	PopulateAvailablePresets();
	
}

TSharedRef<SExpandableArea> SSKPresetMenuWidget::GenerateSpeciesSection()
{
	TSharedRef<SExpandableArea> SpeciesSection = SNew(SExpandableArea)
	.AreaTitle(FText::FromString("Select - Species"))
	.InitiallyCollapsed(false)
	// .Padding(FMargin(8.0f, 8.0f, 8.0f, 4.0f))
	.BodyContent()
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString("Species"))
		]
		
		+ SHorizontalBox::Slot()
		[
			SNew(SComboBox<TSharedPtr<FString>>)
			.OptionsSource(&Model->AvailableSpecies)

			.OnGenerateWidget(this, &SSKPresetMenuWidget::GenerateSpeciesComboItem)
			.OnSelectionChanged(this, &SSKPresetMenuWidget::HandleSpeciesComboChanged)
			.Content()
			[
				SNew(STextBlock)
				.Text(this, &SSKPresetMenuWidget::GetSpeciesComboText)
			]
		]
	];

	return SpeciesSection;
};

// UI Filter Presets Elements
TSharedRef<SExpandableArea> SSKPresetMenuWidget::GenerateFiltersSection()
{
	TSharedRef<SVerticalBox> VBox = SNew(SVerticalBox);

	PresetVerticalBox = VBox;
	
	TSharedRef<SExpandableArea> FilterSection = SNew(SExpandableArea)
	.AreaTitle(FText::FromString("Select - Preset Part Filter"))
	.InitiallyCollapsed(false)
	.BodyContent()
		[
			VBox	
		];

	PopulatePresetFilterUI();
	
	return FilterSection;
};

// UI Randomize Elements
TSharedRef<SExpandableArea> SSKPresetMenuWidget::GenerateRandomizeSection()
{
	const ISlateStyle* SyntyStyle = FSlateStyleRegistry::FindSlateStyle("SidekickCharacterToolStyle");
	
	TSharedRef<SExpandableArea> RansomizeSection = SNew(SExpandableArea)
	.AreaTitle(FText::FromString("Randomize Character"))
	.BodyContent()
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		[
			SNew(SButton)
			.ToolTipText(FText::FromString("Generate a character at the push of a button"))
			.Content()
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				.AutoWidth()
				.Padding(10)
				[
					SNew(SImage)
					.Image(SyntyStyle->GetBrush("SidekickCharacterTool.RandomIcon"))
				]
				
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				.AutoWidth()
				.Padding(10)
				[
					SNew(STextBlock)
					.Text(FText::FromString("Randomize Character"))
				]
			]
			.OnClicked_Lambda([this]()
			{
				RandomizePresets();
				return FReply::Handled();
			})
		]
	];
	return RansomizeSection;
};

void SSKPresetMenuWidget::PopulateData()
{
	FString HeadTitle = PartGroupToString(EPartGroup::Head);
	FString UpperTitle = PartGroupToString(EPartGroup::UpperBody);
	FString LowerTitle = PartGroupToString(EPartGroup::LowerBody);
	
	SelectedPartsOption.FindOrAdd(HeadTitle, 0);
	SelectedPartsOption.FindOrAdd(UpperTitle, 0);
	SelectedPartsOption.FindOrAdd(LowerTitle, 0);
	AvailablePartPresets.FindOrAdd(HeadTitle, TArray<TSharedPtr<FString>>());
	AvailablePartPresets.FindOrAdd(UpperTitle, TArray<TSharedPtr<FString>>());
	AvailablePartPresets.FindOrAdd(LowerTitle, TArray<TSharedPtr<FString>>());
	
	// Adds the default None state
	AvailablePartPresets[HeadTitle].Add(MakeShared<FString>("None"));
	AvailablePartPresets[UpperTitle].Add(MakeShared<FString>("None"));
	AvailablePartPresets[LowerTitle].Add(MakeShared<FString>("None"));
}

TSharedRef<SExpandableArea> SSKPresetMenuWidget::GeneratePresetSection()
{
	UpdateAvailableColorGroups();
	
	TSharedRef<SVerticalBox> MainVbox = SNew(SVerticalBox);
	
	TSharedRef<SExpandableArea> PartsSubSection = SNew(SExpandableArea)
		.AreaTitle(FText::FromString("Parts"))
		.BodyContent()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.Padding(15,0,0,0)
			[
				// Head Preset UI
				SNew(SSKColorPresetWidgetElement)
				.Model(Model)
				.AddClearButton(true)
				.Text(FText::FromString(PartGroupToString(EPartGroup::Head)))
				.ComboOptionsSource(&AvailablePartPresets[PartGroupToString(EPartGroup::Head)])
				.ComboBoxOnGenerateWidget(this, &SSKPresetMenuWidget::GeneratePartsComboItem)
				.ComboIndex(SelectedPartsOption[PartGroupToString(EPartGroup::Head)])
				.ComboBoxOnSelectionChanged_Lambda([this](TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo)
				{
					return HandlePresetParteChanged(PartGroupToString(EPartGroup::Head), Item, SelectInfo);
				})
				.ComboBoxActiveText_Lambda([this]() 
				{
					return GetPartComboText(PartGroupToString(EPartGroup::Head));
				})
			]
			
			+ SVerticalBox::Slot()
			.Padding(15,0,0,0)
			[
				// Upper Body Preset UI
				SNew(SSKColorPresetWidgetElement)
				.Model(Model)
				.AddClearButton(true)
				.Text(FText::FromString(PartGroupToString(EPartGroup::UpperBody)))
				.ComboOptionsSource(&AvailablePartPresets[PartGroupToString(EPartGroup::UpperBody)])
				.ComboBoxOnGenerateWidget(this, &SSKPresetMenuWidget::GeneratePartsComboItem)
				.ComboIndex(SelectedPartsOption[PartGroupToString(EPartGroup::UpperBody)])
				.ComboBoxOnSelectionChanged_Lambda([this](TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo)
				{
					return HandlePresetParteChanged(PartGroupToString(EPartGroup::UpperBody), Item, SelectInfo);
				})
				.ComboBoxActiveText_Lambda([this]() 
				{
					return GetPartComboText(PartGroupToString(EPartGroup::UpperBody));
				})
			]

			+ SVerticalBox::Slot()
			.Padding(15,0,0,0)
			[
				// Lower Body Preset UI
				SNew(SSKColorPresetWidgetElement)
				.Model(Model)
				.AddClearButton(true)
				.Text(FText::FromString(PartGroupToString(EPartGroup::LowerBody)))
				.ComboOptionsSource(&AvailablePartPresets[PartGroupToString(EPartGroup::LowerBody)])
				.ComboBoxOnGenerateWidget(this, &SSKPresetMenuWidget::GeneratePartsComboItem)
				.ComboIndex(SelectedPartsOption[PartGroupToString(EPartGroup::LowerBody)])
				.ComboBoxOnSelectionChanged_Lambda([this](TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo)
				{
					return HandlePresetParteChanged(PartGroupToString(EPartGroup::LowerBody), Item, SelectInfo);
				})
				.ComboBoxActiveText_Lambda([this]() 
				{
					return GetPartComboText(PartGroupToString(EPartGroup::LowerBody));
				})
				
			]
		];

	// Body Morph Targets
	
	TSharedRef<SExpandableArea> BodySubSection = SNew(SExpandableArea)
		.AreaTitle(FText::FromString("Body"))
		.BodyContent()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.Padding(15,0,0,0)
			[
				SNew(SSKColorPresetWidgetElement)
				.Model(Model)
				.AddClearButton(false)
				.Text(FText::FromString("Body Type"))
				.ComboBoxOnGenerateWidget(this, &SSKPresetMenuWidget::GenerateBodyTypeComboItem)
				.ComboBoxOnSelectionChanged(this, &SSKPresetMenuWidget::HandleBodyTypeChanged)
				.ComboBoxActiveText(this, &SSKPresetMenuWidget::GetBodyTypeComboText)
				.ComboOptionsSource(&Model->AvailableBodyPresets)
				.ComboIndex(SelectedComboBoxBodyPresetOption)
			]
		];

	// Color Section
	
	auto ColorVerticalBox = SNew(SVerticalBox);
	
	for (int32 i = 1; i < static_cast<int32>(EColorGroup::Max); ++i)
	{
		EColorGroup ColorGroup = static_cast<EColorGroup>(i);
		FString ColorGroupString = ColorGroupToString(ColorGroup);

		SelectedSpeciesColorOption.Add(ColorGroupString, 0);

		ColorVerticalBox->AddSlot()
		.Padding(15,0,0,0)
		[
			SNew(SSKColorPresetWidgetElement)
				.Model(Model)
				.AddClearButton(false)
				.Text(FText::FromString(ColorGroupString))
				.ComboBoxOnGenerateWidget(this, &SSKPresetMenuWidget::GenerateColorComboItem)
				.Visibility_Lambda([this, ColorGroupString]()
				{
					return ColourUIVisibility(ColorGroupString);
				})
				.ComboOptionsSource(&AvailableColorPresets[ColorGroupString])
				.ComboIndex(SelectedSpeciesColorOption[ColorGroupString])
				.ComboBoxActiveText_Lambda([this, ColorGroupString]() 
				{
					return GetColorComboText(ColorGroupString);
				})
				.ComboBoxOnSelectionChanged_Lambda([this, ColorGroupString](TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo)
				{
					return HandleColorChanged(ColorGroupString, Item, SelectInfo);
				})
		];
	}

	TSharedRef<SExpandableArea> ColorSubSection = SNew(SExpandableArea)
		.AreaTitle(FText::FromString("Colors"))
		.BodyContent()
		[
			ColorVerticalBox
		];
	
	// Add Sub Subsections
	MainVbox->AddSlot()
	.Padding(10,0,0,0)
	.AutoHeight()
	[
		PartsSubSection
	];
	
	MainVbox->AddSlot()
	.Padding(10,0,0,0)
	.AutoHeight()
	[
		BodySubSection
	];
	
	MainVbox->AddSlot()
	.Padding(10,0,0,0)
	.AutoHeight()
	[
		ColorSubSection
	];

	// Adds the master verticalbox 
	TSharedRef<SExpandableArea> PresetSection = SNew(SExpandableArea)
		.AreaTitle(FText::FromString("Presets"))
		.InitiallyCollapsed(false)
		.BodyContent()
	[
		MainVbox	
	];
	
	return PresetSection;
};

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#pragma region Species Combo Helpers

TSharedRef<SWidget> SSKPresetMenuWidget::GenerateSpeciesComboItem(TSharedPtr<FString> InItem)
{
	return SNew(STextBlock).Text(FText::FromString(*InItem));
}

FText SSKPresetMenuWidget::GetSpeciesComboText() const
{
	if (Model->AvailableSpecies.Num() > 0)
	{
		return FText::FromString(*Model->AvailableSpecies[Model->SelectedComboBoxSpeciesOptions]);
	}
	return FText::GetEmpty();
}

void SSKPresetMenuWidget::HandleSpeciesComboChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo)
{
	if (Item.IsValid())
	{
		Model->SelectedComboBoxSpeciesOptions = Model->AvailableSpecies.IndexOfByKey(Item);

		Model->SelectedSpecie = Item;
		
		ClearAvailablePresetParts();
		AddAvailablePresetParts();
		PopulateAvailablePresets();
		PopulateAvailableColors();
		UpdateAvailableColorGroups();
		
		Model->PostActiveSpecieUpdate();

		// Creates Collections of Parts for the active specie.
		// This is required for the UpdateSpeciesParts to evaluate correctly.
		Model->SetupGroupedPartDataStructure();
		// Updates all base species parts to the newly activated specie.
		Model->UpdateSpecieParts();
	}
}

void SSKPresetMenuWidget::RefreshUI()
{
	ClearAvailablePresetParts();
	AddAvailablePresetParts();
	PopulateAvailablePresets();
	PopulateAvailableColors();
	UpdateAvailableColorGroups();
		
	Model->PostActiveSpecieUpdate();

	// Creates Collections of Parts for the active specie.
	// This is required for the UpdateSpeciesParts to evaluate correctly.
	Model->SetupGroupedPartDataStructure();
	// Updates all base species parts to the newly activated specie.
	Model->UpdateSpecieParts();
}

#pragma endregion Species Combo Helpers

#pragma region Body Type Combo Helpers

TSharedRef<SWidget> SSKPresetMenuWidget::GenerateBodyTypeComboItem(TSharedPtr<FString> InItem)
{
	return SNew(STextBlock).Text(FText::FromString(*InItem));
}

FText SSKPresetMenuWidget::GetBodyTypeComboText() const
{
	if (Model->BodyShapePresetLookup.Num() > 0)
	{
		return FText::FromString(*Model->AvailableBodyPresets[SelectedComboBoxBodyPresetOption]);
	}
	return FText::GetEmpty();
}

void SSKPresetMenuWidget::HandleBodyTypeChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo)
{
	if (Item.IsValid())
	{
		SelectedComboBoxBodyPresetOption = Model->AvailableBodyPresets.IndexOfByKey(Item);
		
		FString BodyKey = *Item;

		if (!Model->BodyShapePresetLookup.Contains(BodyKey)) return;

		// Perform Blendshape update
		
		auto MorphTargetData = Model->BodyShapePresetLookup[BodyKey];
		Model->CharacterPreset->Modify();
		Model->CharacterPreset->MTBodyMusculature = (MorphTargetData.Musculature + 100.f) / 200.f;
		Model->CharacterPreset->MTBodySize = MorphTargetData.BodySize / 100.f;
		Model->CharacterPreset->MTBodyType = (MorphTargetData.BodyType + 100.f) / 200.f;

		// Moves the attachments into position that matches the blendshapes
		SSKBodyMenuWidget::CalculateMTOffset(Model.Get());
	}
}

#pragma endregion Body Type Combo Helpers

#pragma region Color Combo Helpers

TSharedRef<SWidget> SSKPresetMenuWidget::GenerateColorComboItem(TSharedPtr<FString> InItem)
{
	return SNew(STextBlock).Text(FText::FromString(*InItem));
}

FText SSKPresetMenuWidget::GetColorComboText(FString ColorGroup) const
{
	auto& SpeciesName = Model->SelectedSpecie;

	if (SpeciesName.IsValid() && !SpeciesName->IsEmpty())
	{
		if (Model->AvailableColorPresets[*SpeciesName][ColorGroup].Num() > 0 &&
			Model->AvailableColorPresets[*SpeciesName][ColorGroup].Num() > SelectedSpeciesColorOption[ColorGroup])
		{
			return FText::FromString(*Model->AvailableColorPresets[*SpeciesName][ColorGroup][SelectedSpeciesColorOption[ColorGroup]]);
		}
	}
	
	return FText::GetEmpty();
}

void SSKPresetMenuWidget::HandleColorChanged(FString ColorGroup, TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo)
{
	if (Item.IsValid())
	{
		FString SpeciesName = GetSpeciesComboText().ToString();
		int& ComboIndex = SelectedSpeciesColorOption[ColorGroup];
		auto& ComboEntries = Model->AvailableColorPresets[SpeciesName][ColorGroup];
		
		ComboIndex = ComboEntries.IndexOfByKey(Item);

		if (Model->ColorPresetsToDBTable[SpeciesName][ColorGroup].Num() < ComboIndex) return;
		
		FColorPreset* ColourPresetEntry = Model->ColorPresetsToDBTable[SpeciesName][ColorGroup][ComboIndex]; 
		int32 ColorPresetIndex = ColourPresetEntry->Id;

		if (Model->ColorPresetRowLookup.Contains(ColorPresetIndex))
		{
			// Updates the texture data with the preset swatch colors
			
			for (auto Element : Model->ColorPresetRowLookup[ColorPresetIndex])
			{
				Model->SetSwatchColour(Element.Name, FColor::FromHex(Element.Color));
			}
			FSKTexture::WriteDataToTexture(&Model->PixelColors, Model->CharacterPreset->TemporaryTexture);
			FSKTexture::WriteDataToTexture(&Model->PixelColors, Model->CharacterPreset->SourceTexture);
			// Model->CharacterPreset->BaseMaterial->PostEditChange(); // causes flickering, maybe only needed when a material alteration occurs
			Model->CharacterPreset->OnObjectChanged.Broadcast(); // forces Actor in world to rebuild
		};
	}
}

EVisibility SSKPresetMenuWidget::ColourUIVisibility(FString ColorGroup)
{
	auto& SpeciesName = Model->SelectedSpecie;

	if (!SpeciesName.IsValid() || Model->AvailableColorPresets.IsEmpty())
	{
		return EVisibility::Collapsed;
	}
	
	if (Model->AvailableColorPresets.Contains(*SpeciesName) &&
		Model->AvailableColorPresets[*SpeciesName].Contains(ColorGroup) &&
		Model->AvailableColorPresets[*SpeciesName][ColorGroup].Num() > 0)
	{
		return EVisibility::Visible;
	}
	
	return EVisibility::Collapsed;
}

#pragma endregion Color Combo Helpers

#pragma region Parts Type Combo Helpers

TSharedRef<SWidget> SSKPresetMenuWidget::GeneratePartsComboItem(TSharedPtr<FString> InItem)
{
	if (InItem.IsValid())
	{
		return SNew(STextBlock).Text(FText::FromString(*InItem));
	}
	
	return SNew(STextBlock);
}

void SSKPresetMenuWidget::HandlePresetParteChanged(FString PartType, TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo)
{
	if (Item.IsValid())
	{
		SelectedPartsOption[PartType] = AvailablePartPresets[PartType].IndexOfByKey(Item);

		// Gets the enum index from the name. Names have a space, which has to be removed
		FString EnumName = PartType;
		EnumName.RemoveSpacesInline();

		EPartGroup PartGroup = PartGroupFromString(EnumName);
		int32 PartTypeIndex = static_cast<int32>(PartGroup);
		
		Model->CharacterPreset->Modify();

		// Updates parts that are found and empties parts that are not
		for (ECharacterPartType PartTypeE : PartsGrouping[PartGroup])
		{
			FString PartTypeName = CharacterPartTypeToString(PartTypeE);
			int32 PartTypeValue = static_cast<int32>(PartTypeE);
			
			// If the Item is None, then remove all associated parts.
			if (*Item == "None")
			{
				Model->CharacterPreset->SKMParts.FindOrAdd(PartTypeName);
				Model->CharacterPreset->SKMParts[PartTypeName] = nullptr;
				continue;
			}
			
			if (Model->PresetToParts[PartTypeIndex][*Item].Contains(PartTypeValue))
			{
				FString PartName = Model->PresetToParts[PartTypeIndex][*Item][PartTypeValue];

				Model->CharacterPreset->SKMParts.FindOrAdd(PartTypeName);
				Model->CharacterPreset->SKMParts[PartTypeName] = Model->GetPartByName(PartName);
			}
			else
			{
				Model->CharacterPreset->SKMParts.FindOrAdd(PartTypeName);
				Model->CharacterPreset->SKMParts[PartTypeName] = nullptr;
			}
		}
		
		Model->CharacterPreset->OnObjectChanged.Broadcast();
		
	}
}

FText SSKPresetMenuWidget::GetPartComboText(FString PartType) const
{
	if (!AvailablePartPresets.IsEmpty() && AvailablePartPresets[PartType].Num() > 0)
	{
		if (SelectedPartsOption.IsEmpty() ||
			!SelectedPartsOption.Contains(PartType) ||
			!AvailablePartPresets.Contains(PartType) ||
			SelectedPartsOption[PartType] < 0 ||
			SelectedPartsOption[PartType] >= AvailablePartPresets[PartType].Num())
		{
			return FText::GetEmpty();
		}
		
		return FText::FromString(*AvailablePartPresets[PartType][SelectedPartsOption[PartType]]);
	}
	return FText::GetEmpty();
}

#pragma endregion Parts Type Combo Helpers

void SSKPresetMenuWidget::RandomizePresets()
{
	FString SpeciesName = GetSpeciesComboText().ToString();

	FString PGHead = PartGroupToString(EPartGroup::Head);
	FString PGUpperBody = PartGroupToString(EPartGroup::UpperBody);
	FString PGLowerBody = PartGroupToString(EPartGroup::LowerBody);
	
	int HeadRandomIndex = FMath::RandRange(1,AvailablePartPresets[PGHead].Num() -1);
	int UpperBodyRandomIndex = FMath::RandRange(1,AvailablePartPresets[PGUpperBody].Num() -1);
	int LowerBodyRandomIndex = FMath::RandRange(1,AvailablePartPresets[PGLowerBody].Num() -1);
	int BodyMorphRandomIndex = FMath::RandRange(0,Model->AvailableBodyPresets.Num() -1);

	for (int32 i = 1; i < static_cast<int32>(EColorGroup::Max); ++i)
	{
		EColorGroup ColorGroup = static_cast<EColorGroup>(i);
		FString ColorGroupString = ColorGroupToString(ColorGroup);

		if (Model->AvailableColorPresets.Contains(SpeciesName) && Model->AvailableColorPresets[SpeciesName].Contains(ColorGroupString) && Model->AvailableColorPresets[SpeciesName][ColorGroupString].Num() > 0)
		{
			int ColourRandomIndex = FMath::RandRange(0, Model->AvailableColorPresets[SpeciesName][ColorGroupString].Num() -1);
			HandleColorChanged(ColorGroupString, Model->AvailableColorPresets[SpeciesName][ColorGroupString][ColourRandomIndex], ESelectInfo::OnMouseClick);
		}
	}

	// As Presets can be filtered out, we cannot assign a random index to a list preset list that might be empty
	
	if (AvailablePartPresets[PGHead].Num() > 1 && AvailablePartPresets[PGHead].Num() >= HeadRandomIndex)
	{
		HandlePresetParteChanged(PGHead, AvailablePartPresets[PGHead][HeadRandomIndex], ESelectInfo::OnMouseClick);
	}
	if (AvailablePartPresets[PGUpperBody].Num() > 1 && AvailablePartPresets[PGUpperBody].Num() >= UpperBodyRandomIndex)
	{
		HandlePresetParteChanged(PGUpperBody, AvailablePartPresets[PGUpperBody][UpperBodyRandomIndex], ESelectInfo::OnMouseClick);
	}
	if (AvailablePartPresets[PGLowerBody].Num() > 1 && AvailablePartPresets[PGLowerBody].Num() >= LowerBodyRandomIndex)
	{
		HandlePresetParteChanged(PGLowerBody, AvailablePartPresets[PGLowerBody][LowerBodyRandomIndex], ESelectInfo::OnMouseClick);
	}
	if (!Model->AvailableBodyPresets.IsEmpty())
	{
		HandleBodyTypeChanged(Model->AvailableBodyPresets[BodyMorphRandomIndex], ESelectInfo::OnMouseClick);
	}
}

// Preset Helpers

void SSKPresetMenuWidget::PopulateAvailablePresets()
{
	if (!PresetVerticalBox.IsValid()) return;
	
	PresetVerticalBox->ClearChildren();
	
	PopulatePresetFilterUI();
}

/**
 * Generates the Preset Filter's UI Checkboxes
 */
void SSKPresetMenuWidget::PopulatePresetFilterUI()
{
	FString ActiveSpecie = SSKPresetMenuWidget::GetSpeciesComboText().ToString();

	// Early out, no specie found
	if ( ActiveSpecie.IsEmpty() )
	{
		return;
	}
	
	// Early out if the Specie does not exist in the presets
	if ( !Model->SpeciesToPresetFilter.Contains(ActiveSpecie) )
	{
		return;
	}
	
	for (auto PresetPartFilterName : Model->SpeciesToPresetFilter[ActiveSpecie])
	{
		PresetVerticalBox->AddSlot()
		[
			SNew(SCheckBox)
			.Style(FCoreStyle::Get(), "ToggleButtonCheckbox")
			.IsChecked(ECheckBoxState::Checked)
			.OnCheckStateChanged_Lambda([this, PresetPartFilterName](ECheckBoxState NewState)
			{
				UE_LOG(LogTemp, Display, TEXT("PresetMenuWidget::OnCheckStateChanged %s"), *PresetPartFilterName);
				if (NewState == ECheckBoxState::Checked)
				{
					// Add
					Model -> AddPresetPartsFromFilter(PresetPartFilterName, AvailablePartPresets);
				}
				else
				{
					// Remove
					Model -> RemovePresetPartsFromFilter(PresetPartFilterName, AvailablePartPresets);
				}
			})
			[
				SNew(STextBlock)
				.Text(FText::FromString(PresetPartFilterName))
			]
		];
	}
}

void SSKPresetMenuWidget::ClearAvailablePresetParts()
{
	for (int32 EIndex = 1; EIndex < static_cast<int32>(EPartGroup::Max); ++EIndex)
	{
		EPartGroup PartGroup = static_cast<EPartGroup>(EIndex);
		FString PartGroupString = PartGroupToString(PartGroup);

		if (AvailablePartPresets.Contains(PartGroupString))
		{
			AvailablePartPresets[PartGroupString].Empty();
			AvailablePartPresets[PartGroupString].Add(MakeShared<FString>("None"));
		} 
	}
}

void SSKPresetMenuWidget::AddAvailablePresetParts()
{
	FString ActiveSpecie = SSKPresetMenuWidget::GetSpeciesComboText().ToString();

	// Specie empty, most likely no DB for population, exit early.
	if ( ActiveSpecie.IsEmpty() )
	{
		return;
	}
	
	if (!Model->SpeciesToPresetFilter.Contains(ActiveSpecie))
	{
		UE_LOG(LogTemp, Warning, TEXT("SSKPresetMenuWidget::AddAvailablePresetParts - Unable to find presets for %s"), *ActiveSpecie);
		return;
	}
	
	for (auto PresetPartFilterName : Model->SpeciesToPresetFilter[ActiveSpecie])
	{
		Model -> AddPresetPartsFromFilter(PresetPartFilterName, AvailablePartPresets);
	}
}

void SSKPresetMenuWidget::PopulateAvailableColors()
{
	// Resets the active colour group back to default 0
	for (int32 i = 1; i < static_cast<int32>(EColorGroup::Max); ++i)
	{
		EColorGroup ColorGroup = static_cast<EColorGroup>(i);
		SelectedSpeciesColorOption[ColorGroupToString(ColorGroup)] = 0;
	}
}

/**
 *  Populates AvailableColorPresets, with the Colour Groups and the available name
 *  
 * @param ColorGroup 
 */
void SSKPresetMenuWidget::UpdateAvailableColorGroups(FString ColorGroup)
{
	FString ActiveSpecie = GetSpeciesComboText().ToString();

	if ( Model->AvailableColorPresets.IsEmpty() || ActiveSpecie.IsEmpty() )
	{
		// No Data available, most likely due to Database missing, populate empty fields
		for (int32 i = 1; i < static_cast<int32>(EColorGroup::Max); ++i)
		{
			EColorGroup ColorGroupEnum = static_cast<EColorGroup>(i);
			ColorGroup = ColorGroupToString(ColorGroupEnum);
			AvailableColorPresets.FindOrAdd(ColorGroup);
		}
		return;
	}
	
	if (ColorGroup.IsEmpty())
	{
		for (int32 i = 1; i < static_cast<int32>(EColorGroup::Max); ++i)
		{
			EColorGroup ColorGroupEnum = static_cast<EColorGroup>(i);
			ColorGroup = ColorGroupToString(ColorGroupEnum);
			
			AvailableColorPresets.FindOrAdd(ColorGroup);
			if (Model->AvailableColorPresets[ActiveSpecie].Contains(ColorGroup))
			{
				AvailableColorPresets[ColorGroup] = Model->AvailableColorPresets[ActiveSpecie][ColorGroup]; 
			}
			else
			{
				AvailableColorPresets[ColorGroup].Empty();
				AvailableColorPresets[ColorGroup].Add(MakeShared<FString>(TEXT("Empty")));
			}
		}
	}
	else
	{
		AvailableColorPresets.FindOrAdd(ColorGroup);			
		AvailableColorPresets[ColorGroup] = Model->AvailableColorPresets[ActiveSpecie][ColorGroup];
	}
}
