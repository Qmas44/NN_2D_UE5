// Fill out your copyright notice in the Description page of Project Settings.

#include "Widgets/SSKPartTypeWidgetElement.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Images/SImage.h"

#include "Styling/SlateStyleRegistry.h"
#include "Assets/SParts.h"
#include "SlateOptMacros.h"
#include "Menu/SSKPartsMenuWidget.h"
#include "SKCharacter/Public/SKCharacterPreset.h"

SSKPartTypeWidgetElement::SSKPartTypeWidgetElement()
{
	EPartType = ECharacterPartType::Max;
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SSKPartTypeWidgetElement::Construct(const FArguments& InArgs)
{
	TextContent = InArgs._Text;
	Model = InArgs._Model;
	EPartType = InArgs._PartType;
	OnCustomEvent = InArgs._OnCustomOutfitChangedEvent;
	OnRandomizeCollectionEvent = InArgs._OnRandomizeCollectionEvent;
	
	OnCustomEvent->AddLambda([this]()
	{
		GetAvailablePartsForGroup();
	});

	OnRandomizeCollectionEvent->AddLambda([this](EPartGroup PartGroup)
	{
		auto PartsInGroup = PartsGrouping[PartGroup];
		
		if (PartsInGroup.FindByKey(EPartType))
		{
			RandomizePart();
		}
	});

	const ISlateStyle* SyntyStyle = FSlateStyleRegistry::FindSlateStyle("SidekickCharacterToolStyle");
	
	GetAvailablePartsForGroup();
	
	PopulateDropdownsFromPreset();
	
	ChildSlot
	[
		SNew(SHorizontalBox)
		
		+ SHorizontalBox::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			SNew(STextBlock)
			.MinDesiredWidth(150)
			.Text(TextContent)
			.ColorAndOpacity(FSlateColor::UseForeground())
		]
		
		+ SHorizontalBox::Slot()
		.Padding(0,0,20,0)
		.HAlign(HAlign_Right)
		.FillWidth(1)
		[
			SNew(SHorizontalBox)

			// Lock Button
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.ForegroundColor(FSlateColor::UseStyle())
				.Content()
				[
					SNew(SImage)
					.DesiredSizeOverride(FVector2D(16.0f, 16.0f))
					.Image(FAppStyle::Get().GetBrush("ContentBrowser.Private.Large"))
					
					.ColorAndOpacity_Lambda([this, SyntyStyle]{
						if(bPartLocked)
						{
							return SyntyStyle->GetBrush("SidekickCharacterTool.SKDefaultRed")->TintColor.GetSpecifiedColor();
						}
						return FLinearColor::White;
					})
				]
				.OnClicked_Lambda([this]()
				{
					bPartLocked = !bPartLocked;
					return FReply::Handled();
				})
			]
			
			// Clear Button
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.IsEnabled(this, &SSKPartTypeWidgetElement::CanPressButton)
				.ForegroundColor(FSlateColor::UseStyle())
				.Content()
				[
					SNew(SImage)
					.Image(SyntyStyle->GetBrush("SidekickCharacterTool.ClearIcon"))
				]
				.OnClicked_Lambda([this]()
				{
					SelectedComboBoxPartIndex = 0;
					FString MeshKey = TextContent.Get().ToString();
					Model->CharacterPreset->SKMParts[MeshKey] = nullptr;
					UpdatePreset();
					return FReply::Handled();
				})
			]
			
			// Previous Button
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.IsEnabled(this, &SSKPartTypeWidgetElement::CanPressButton)
				.ForegroundColor(FSlateColor::UseStyle())
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Content()
				[
					SNew(STextBlock)
					.Text(FText::FromString("<"))
				]
				.OnClicked_Lambda([this]()
				{
					if (PartList.IsEmpty())
					{
						return FReply::Handled();
					}
					
					FString MeshKey = TextContent.Get().ToString();
					if (Model->CharacterPreset->SKMParts[MeshKey] == nullptr)
					{
						// no part is currently specified
					}
					else
					{
						FString PartName = Model->CharacterPreset->SKMParts[MeshKey]->GetName();
						SelectedComboBoxPartIndex = FindPartInList(PartName);
					}
					
					SelectedComboBoxPartIndex -= 1;
					if (SelectedComboBoxPartIndex < 1)
					{
						SelectedComboBoxPartIndex = PartList.Num() - 1;
					}

					if (PartList.Num() == 1)
					{
						SelectedComboBoxPartIndex = 0;
					}
					
					Model->CharacterPreset->SKMParts[MeshKey] = Model->GetPartByName(*PartList[SelectedComboBoxPartIndex]);
					
					UpdatePreset();
					return FReply::Handled();
				})
			]

			// Next Button
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.IsEnabled(this, &SSKPartTypeWidgetElement::CanPressButton)
				.ForegroundColor(FSlateColor::UseStyle())
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Content()
				[
					SNew(STextBlock)
					.Text(FText::FromString(">"))

				]
				.OnClicked_Lambda([this]()
				{
					if (PartList.IsEmpty())
					{
						return FReply::Handled();
					}
					
					FString MeshKey = TextContent.Get().ToString();
					if (Model->CharacterPreset->SKMParts[MeshKey] == nullptr)
					{
						// no part is currently specified
					}
					else
					{
						FString PartName = Model->CharacterPreset->SKMParts[MeshKey]->GetName();
						SelectedComboBoxPartIndex = FindPartInList(PartName);
					}
					
					SelectedComboBoxPartIndex += 1;
					if (SelectedComboBoxPartIndex >= PartList.Num())
					{
						SelectedComboBoxPartIndex = 1;
					}

					if (PartList.Num() == 1)
					{
						SelectedComboBoxPartIndex = 0;
					}

					Model->CharacterPreset->SKMParts[MeshKey] = Model->GetPartByName(*PartList[SelectedComboBoxPartIndex]);
					
					UpdatePreset();
					return FReply::Handled();
				})
			]

			// Random Button
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.IsEnabled(this, &SSKPartTypeWidgetElement::CanPressButton)
				.ForegroundColor(FSlateColor::UseStyle())
				.Content()
				[
					SNew(SImage)
					.Image(SyntyStyle->GetBrush("SidekickCharacterTool.RandomIcon"))
				]
				.OnClicked_Lambda([this]()
				{
					RandomizePart();
					return FReply::Handled();
				})
			]

			// Part ComboBox
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Fill)
			[
				SNew(SComboBox<TSharedPtr<FString>>)
				.IsEnabled(this, &SSKPartTypeWidgetElement::CanPressButton)
				.OptionsSource(&PartList)
				.OnGenerateWidget_Lambda([this](TSharedPtr<FString> Item)
				{
					return SNew(STextBlock).Text(FText::FromString(*Item));
				})
				.Content()
				[
					SNew(STextBlock)
					.MinDesiredWidth(230)
					.Text_Lambda([this]()
					{
						return GetPartComboText();
					})
				]
				.OnSelectionChanged(this, &SSKPartTypeWidgetElement::HandlePartComboChanged) // Widget updates recorded data
			]
		]
	];
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

/** Populates the PartsList variable, that is used to populate the dropdown options of the combobox */
void SSKPartTypeWidgetElement::GetAvailablePartsForGroup()
{

	if (!Model.IsValid() || Model->AvailablePartsDropDowns == nullptr)
		return;
	
	FString tempEntry;
	if (!PartList.IsEmpty())
	{
		 tempEntry= *PartList[SelectedComboBoxPartIndex];
	}
	
	// Bruteforce check to make sure that the Selected Index is still valid with the data assigned to the SKMParts.
	// As they can become out of sync when Presets are set no `None`.
	FString MeshKey = TextContent.Get().ToString();
	if (Model->CharacterPreset->SKMParts[MeshKey] == nullptr)
	{
		tempEntry = "None";
	}
	
	PartList.Empty();

	TArray<FString>* PartsArray = Model->AvailablePartsDropDowns->Find(EPartType);

	if (PartsArray == nullptr)
	{
		PartList.Add( MakeShareable( new FString("Empty")) );
		SelectedComboBoxPartIndex = 0;
		return;
	}
	
	if (PartsArray->IsEmpty())
	{
		PartList.Add( MakeShareable( new FString("Empty")) );
		SelectedComboBoxPartIndex = 0;
		return;
	}

	PartList.Add( MakeShareable( new FString("None")) );
	for (const FString& Part : *PartsArray)
	{
		PartList.Add( MakeShared<FString>(FString(Part)) );
	}

	// Validate check to make sure that the removal of entries from the dropdown box has not invalidated the
	// selected entry.
	auto FoudIndex = PartList.IndexOfByPredicate([&](const TSharedPtr<FString>& Ptr)
	{
		return Ptr.IsValid() && *Ptr == tempEntry;
	});

	if (FoudIndex >= 0)
		SelectedComboBoxPartIndex = FoudIndex;
	else
		SelectedComboBoxPartIndex = 0;
}

FText SSKPartTypeWidgetElement::GetPartComboText()
{
	if (PartList.Num() > 0)
	{
		if (SelectedComboBoxPartIndex > PartList.Num() - 1)
			return FText::GetEmpty();

		FString MeshKey = TextContent.Get().ToString();
		
		if (Model->CharacterPreset->SKMParts.Contains(MeshKey) && Model->CharacterPreset->SKMParts[MeshKey])
		{
			FString PartName = Model->CharacterPreset->SKMParts[MeshKey]->GetName();

			if ( *PartList[SelectedComboBoxPartIndex].Get() != PartName )
			{
				// Validate check to make sure that the removal of entries from the dropdown box has not invalidated the
				// selected entry.
				int FoudIndex = PartList.IndexOfByPredicate([&](const TSharedPtr<FString>& Ptr)
				{
					return Ptr.IsValid() && *Ptr == PartName;
				});

				if (FoudIndex >= 0)
					SelectedComboBoxPartIndex = FoudIndex;
				else
					SelectedComboBoxPartIndex = 0;
			}
		}
		
		// Catch to check incase something slipped through.
		if (SelectedComboBoxPartIndex < 0)
		{
			SelectedComboBoxPartIndex = 0;
		}
		
		return FText::FromString(*PartList[SelectedComboBoxPartIndex]);
	}
	return FText::GetEmpty();
}

void SSKPartTypeWidgetElement::HandlePartComboChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo)
{
	if (Item.IsValid())
	{
		FString MeshKey = TextContent.Get().ToString();
		SelectedComboBoxPartIndex = PartList.IndexOfByKey(Item.ToSharedRef());

		if (!Model->CharacterPreset->SKMParts.Contains(MeshKey))
		{
			Model->CharacterPreset->SKMParts.Add(MeshKey);
		}
		
		if (SelectedComboBoxPartIndex < 0)
		{
			Model->CharacterPreset->SKMParts[MeshKey] = nullptr;
			return;
		}
		
		Model->CharacterPreset->SKMParts[MeshKey] = Model->GetPartByName(*PartList[SelectedComboBoxPartIndex]);
	}
	UpdatePreset();
}

/** Populates the Preset with the Specified Skeletal Mesh and triggers the update call. */
void SSKPartTypeWidgetElement::UpdatePreset()
{
	FString MeshKey = TextContent.Get().ToString();
	if (!Model->CharacterPreset->SKMParts.Contains(MeshKey))
	{
		Model->CharacterPreset->SKMParts.Add(MeshKey);
	}

	Model->CharacterPreset->Modify();

	Model->CharacterPreset->SKMParts[MeshKey] = Model->GetPartByName(GetPartComboText().ToString());
	
	Model->CharacterPreset->OnObjectChanged.Broadcast();

	TSet<FIntPoint> ColourIndices = SParts::GetAvailableColourSwatchIndices(Model->CharacterPreset->SKMParts[MeshKey]);

	Model->AvailablePartColourSwatches.FindOrAdd(MeshKey);
	Model->AvailablePartColourSwatches[MeshKey] = ColourIndices;
}

/** Populates the dropdowns with the assets that are currently active in the Preset */
void SSKPartTypeWidgetElement::PopulateDropdownsFromPreset()
{
	if (PartList.IsEmpty() || Model->CharacterPreset == nullptr) return;
	
	// Type of Part ie. Head, Torso...
	FString PartTypeKey = TextContent.Get().ToString();

	// does the current Part key exist in the preset, if not exit early
	if (!Model->CharacterPreset->SKMParts.Contains(PartTypeKey))
	{
		return;
	}

	if (!Model->CharacterPreset->SKMParts[PartTypeKey]) return;
	
	FString ActivePresetPartName = Model->CharacterPreset->SKMParts[PartTypeKey]->GetName();
	
	int32 FoundIndex = PartList.IndexOfByPredicate([&](const TSharedPtr<FString>& Ptr)
	{
		return Ptr.IsValid() && *Ptr == ActivePresetPartName;
	});

	if (FoundIndex >= 0)
	{
		SelectedComboBoxPartIndex = FoundIndex;
	}
	else
	{
		SelectedComboBoxPartIndex = 0; // default to `None`
	}

}

void SSKPartTypeWidgetElement::RandomizePart()
{
	if (bPartLocked)
	{
		return;
	}
	FString MeshKey = TextContent.Get().ToString();
	
	if (PartList.Num() <= 1)
	{
		Model->CharacterPreset->SKMParts[MeshKey] = nullptr;
		return;
	}
	
	SelectedComboBoxPartIndex = FMath::RandRange(1, PartList.Num() - 1);
	Model->CharacterPreset->SKMParts[MeshKey] = Model->GetPartByName(*PartList[SelectedComboBoxPartIndex]);
	
	UpdatePreset();
}

int SSKPartTypeWidgetElement::FindPartInList(FString PartName) const
{
	int FoudIndex = PartList.IndexOfByPredicate([&](const TSharedPtr<FString>& Ptr)
	{
		return Ptr.IsValid() && *Ptr == PartName;
	});

	if (FoudIndex >= 0)
		return FoudIndex;
	return -1;
}

bool SSKPartTypeWidgetElement::CanPressButton() const
{
	return !bPartLocked;
}