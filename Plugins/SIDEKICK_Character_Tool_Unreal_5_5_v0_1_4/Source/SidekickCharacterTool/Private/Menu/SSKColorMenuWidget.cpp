// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu/SSKColorMenuWidget.h"

#include "DataTableUtils.h"
#include "Editor.h"
#include "Misc/Paths.h"
#include "ImageWriteBlueprintLibrary.h"
#include "SlateOptMacros.h"
#include "UObject/SavePackage.h"
#include "SidekickDBSubsystem.h"
#include "Assets/FSKTexture.h"
#include "Enums/ColorGroup.h"
#include "Widgets/SSKColorEntryWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Assets/SParts.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SSKColorMenuWidget::Construct(const FArguments& InArgs)
{
	Model = InArgs._Model;

	OnColorSwatchChanged = FOnColorSwatchChanged();
	
	if (!GetPartColourMapDataFromDB())
	{
		UE_LOG(LogTemp, Error, TEXT("SSKColorMenuWidget::Construct - Unable to access the Database."));
		return;
	};
	
	// Create Look Up table with keys as group id's, enabling for a quick reverse lookup.
	TMap<int32, TArray<FString>> PartColorByGroup;
	CreateColorLookupTableByGroupID(PartColorByGroup);

	// Checks to make sure that the Model contains a base texture,
	// else generates a new one from the default material.
	ValidateAndGenerateModelBaseTexture();
	
	// Load Texture into Temporary Texture, and Temporary UI PixelColours
	PopulateTemporaryTexture();

	OnColorSwatchChanged.BindLambda([this](FString ColorSwatchName, FColor ColorSwatchColor)
	{
		// Updates the flat texture array in memory.
		Model->SetSwatchColour(ColorSwatchName, ColorSwatchColor);

		// todo: Look into updating a region instead of the entire texture. This might increase speed.
		// FSKTexture::UpdateTextureRegion(&PixelColors, Model->CharacterPreset->TemporaryTexture);
		
		FSKTexture::WriteDataToTexture(&Model->PixelColors, Model->CharacterPreset->TemporaryTexture);
		FSKTexture::WriteDataToTexture(&Model->PixelColors, Model->CharacterPreset->SourceTexture);
		
		// Tells the Actor to regenerate, causing the dynamic material to be refreshed, as well as assigning the latest temptexture.
		// todo: this might only be required due to the bug where a new texture is generated every time it is opened.
		Model->CharacterPreset->OnObjectChanged.Broadcast();
	});
	
	// Redraws the Material, which flushes the texture buffer.
	if (Model->CharacterPreset->BaseMaterial)
	{
		Model->CharacterPreset->BaseMaterial->PostEditChange();
	}
	
	// Colour swatches
	TSharedRef<SVerticalBox> ColorGroupVerticalBox = SNew(SVerticalBox);
	
	for (int32 i = 1; i < static_cast<int32>(EColorGroup::Max); ++i)
	{
		EColorGroup ColorGroup = static_cast<EColorGroup>(i);
		FString ColorGroupString = ColorGroupToString(ColorGroup);

		TSharedRef<SVerticalBox> PartVerticalBox = SNew(SVerticalBox);

		// Part Colour Swatch Row
		for (auto ColorSectionName : PartColorByGroup[i])
		{
			TSharedRef<SSKColorEntryWidget> ColorEntryWidget = SNew(SSKColorEntryWidget)
				.Text(FText::FromString(ColorSectionName))
				.Model(Model)
				.Color(GetSwatchColour(Model->PartColourMapUdim[ColorSectionName]))
				.OnColorSwatchChangedEvent(OnColorSwatchChanged);
			
			ColorSwatchEntries.Add(ColorSectionName, ColorEntryWidget);
			
			PartVerticalBox->AddSlot()
			.Padding(0,1,10,1)
			.AutoHeight()
			[
				ColorEntryWidget
			];
		}
		
		// Colour Grouping...
		
		TSharedRef<SExpandableArea> PartGroupExpander = SNew(SExpandableArea)
			.AreaTitle(FText::FromString(ColorGroupString))
			.InitiallyCollapsed(false)
			.Padding(FMargin(15.0f, 2.0f, 0.0f, 0.0f))
			.BodyContent()
			[
				PartVerticalBox
			];

		ColorGroupVerticalBox->AddSlot()
		.AutoHeight()
		[

				PartGroupExpander
		];
	}

	
	ChildSlot
	[
		SNew(SVerticalBox)
		
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SButton)
			.Text(FText::FromString("Save Out Texture"))
			.OnPressed_Lambda([this]()
			{
				FString TexturePath = Model->CreateOutputTexturePath();
				if (TexturePath.IsEmpty()) return;

				// auto SerialisedTexture = FSKTexture::SerialiseAssetPackageTexture(TexturePath, Model->CharacterPreset->TemporaryTexture);

				FImageWriteOptions WriteOptions;
				WriteOptions.Format = EDesiredImageFormat::PNG;
				WriteOptions.bOverwriteFile = true;
				WriteOptions.bAsync = true;
				WriteOptions.CompressionQuality = 100;
				
				UImageWriteBlueprintLibrary::ExportToDisk(Model->CharacterPreset->TemporaryTexture, TexturePath, WriteOptions);
				
			})
		]

		// Reset Colours to Original State
		// +SVerticalBox::Slot()
		// .AutoHeight()
		// [
		// 	SNew(SButton)
		// 	.Text(FText::FromString("Reset Colour State"))
		// 	.OnPressed_Lambda([this]()
		// 	{
		// 		UTexture2D* TextureRT = Model->CharacterPreset->TemporaryTexture;
		// 		UTexture2D* SourceTexture = Model->CharacterPreset->SourceTexture;
		// 		UMaterial* BaseMaterial = Model->CharacterPreset->BaseMaterial;
		//
		// 		if (SourceTexture == nullptr) return;
		// 		
		// 		// Clears pixels swatch colours, used to generate texture.
		// 		Model->PixelColors.Empty();
		// 		
		// 		// Copy Data from Source Texture onto Temporary Texture
		// 		// Update Colour Array with Source Data
		// 		FSKTexture::SetBaseColorTexture(BaseMaterial, SourceTexture);
		// 		FSKTexture::CopyMaterialTextureToTexture(BaseMaterial, TextureRT);
		// 		FSKTexture::ReadDataFromTexture(SourceTexture, &Model->PixelColors);
		// 		
		// 		// todo: Update swatches - Add Color Swatch refresh.
		// 		
		// 	})
		// ]
		
		+ SVerticalBox::Slot()
		.FillHeight(1)
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			SNew(SScrollBox)
			.ScrollBarVisibility(EVisibility::All)
			+ SScrollBox::Slot()
			[
				ColorGroupVerticalBox
			]
		]
	];

	PostConstruct();
}


END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SSKColorMenuWidget::PostConstruct()
{
	SSKColorMenuWidget::RefreshColourSwatchVisibility();
}

void SSKColorMenuWidget::RefreshColourSwatchVisibility()
{
	FString SwatchName;
	TSet<FIntPoint> ActiveSwatchPosition;
	TMap<FIntPoint, FString> SwatchNameMap;

	// Creates a unique lists of UV positions, to query for swatch names
	for (auto EntryData : Model->AvailablePartColourSwatches)
	{
		ActiveSwatchPosition.Append(EntryData.Value);
	}
	
	// Generates the Swatch Name Map. Used to find a swatch name by position.
	for (auto Entry : Model->PartColourMapUdim)
	{
		FString ColourSwatchName = Entry.Key;
		SwatchNameMap.Add(FIntPoint(Entry.Value.U, Entry.Value.V), Entry.Key);
		ColorSwatchEntries[ColourSwatchName] -> SetVisibility(EVisibility::Collapsed);
	}

	// Sets swatches that are availalbe for active parts to be visible
	for (FIntPoint SwatchPosition : ActiveSwatchPosition)
	{
		if (!SwatchNameMap.Contains(SwatchPosition))
		{
			if (SwatchPosition.X < 15 && SwatchPosition.Y > 0)
			{
				UE_LOG(LogTemp, Error, TEXT("SSKColorMenuWidget::PostConstruct - Key does not exist %d : %d"), SwatchPosition.X, SwatchPosition.Y);
			}
			continue;
		}
		FString ColourSwatchName = SwatchNameMap[SwatchPosition];
		ColorSwatchEntries[ColourSwatchName] -> SetVisibility(EVisibility::Visible);
	}
}

void SSKColorMenuWidget::PopulateTemporaryTexture()
{
	Model->CharacterPreset->TemporaryTexture = NewObject<UTexture2D>();
	auto* TextureRT = Model->CharacterPreset->TemporaryTexture;
	TextureRT->AddToRoot();	// prevents garbage collection occuring on texture.

	UTexture2D* BaseTexture = Model->CharacterPreset->SourceTexture;
	
	UMaterialInterface* SourceMaterial = Model->CharacterPreset->BaseMaterial;

	if (!SourceMaterial || !BaseTexture)
		return;

	
	// - Update Material with Source Texture
	// FSKTexture::SetBaseColorTexture(BaseMaterial, BaseTexture);
	
	// - Copy Source Texture to Temporary Texture
	// - Update Material with Temporary Texture
	FSKTexture::CopyMaterialTextureToTexture(SourceMaterial, TextureRT);
	

	// Reads the Base Texture into the PixelColors, which is the texture in a flat array
	// structure for easy access.
	FSKTexture::ReadDataFromTexture(TextureRT, &Model->PixelColors);
}

bool SSKColorMenuWidget::GetPartColourMapDataFromDB()
{
	// Get the Part UDIMs position from the Database.
	bool DBSuccess = false;
	if (!GEditor) return false;
	USidekickDBSubsystem* Subsystem = GEditor->GetEditorSubsystem<USidekickDBSubsystem>();
	if (!Subsystem)
	{
		UE_LOG(LogDataTable, Error, TEXT("Unable to access SidekickDB Subsystem"));
		return false;
	}
	Subsystem->GetColorProperties(Model->PartColourMapUdim, DBSuccess);
	return DBSuccess;
}

FColor* SSKColorMenuWidget::GetSwatchColour(int32 U, int32 V)
{
	if (Model->PixelColors.Num() < 32*32) return &Model->NoColor;

	int32 Index = (U * 2) + (((32-1) - V * 2) * 32);
	
	return &Model->PixelColors[Index];
}

FColor* SSKColorMenuWidget::GetSwatchColour(const FColorProperty& ColorEntry)
{
	return GetSwatchColour(ColorEntry.U, ColorEntry.V);
}


/** NOT IMPLEMENTED */
void SSKColorMenuWidget::ResetTemporaryTexture()
{
	UTexture2D* TextureRT = Model->CharacterPreset->TemporaryTexture;
	UTexture2D* SourceTexture = Model->CharacterPreset->SourceTexture;
	UMaterialInterface* BaseMaterial = Model->CharacterPreset->BaseMaterial;

	// todo: Copy the texture buffer from the source Texture into the Temporary Texture
	if (SourceTexture)
	{
		UE_LOG(LogTemp, Error, TEXT("SSKColorMenuWidget::ResetTemporaryTexture not implemented yet.."));
	}
	// todo: Retrieve the 
	else
	{
		UE_LOG(LogTemp, Error, TEXT("SSKColorMenuWidget::ResetTemporaryTexture not implemented yet.."));
	}
}

// todo: When generating the Package, Unreal tries to re import the asset. Try and see if it is possible to implement this without the reimport.
// todo: When generating a new BaseTexture, we need to check that it is actually the base texture and not just another Models texture.
void SSKColorMenuWidget::ValidateAndGenerateModelBaseTexture()
{
	UTexture2D* SourceTexture = Model->CharacterPreset->SourceTexture;
	
	if (SourceTexture) return;

	UMaterialInterface* BaseMaterial = Model->CharacterPreset->BaseMaterial;

	if (BaseMaterial == nullptr)
	{
		return;
	}
	
	FString TexturePath = Model->CreateOutputTexturePath();
	if (TexturePath.IsEmpty()) return;

	// Save out base colour texture to .png file 
	UTexture2D* BaseTexture = nullptr ;
	UTexture2D* SampleTexture = nullptr;
	FSKTexture::GetBaseColorTexture(BaseMaterial, BaseTexture);

	if (BaseTexture == nullptr)
	{
		// Base Texture is null as BaseMaterial has been used and had its texture overwritte.
		return;
	} 
	
	// If Texture is larger then 32x32, sample it down into 32x32 
	if (!FSKTexture::ValidateTextureResolution(BaseTexture))
	{
		SampleTexture = NewObject<UTexture2D>();
		SampleTexture->SRGB = true;
		SampleTexture->CompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;
		SampleTexture->MipGenSettings =	TextureMipGenSettings::TMGS_NoMipmaps;
		SampleTexture->Filter = TF_Nearest;
		SampleTexture->UpdateResource();
		SampleTexture->AddToRoot();
		
		// bug: SampleTexture, still looks to be sampling the texture incorrectly, or colour space is being converted(should be srgb)
		FSKTexture::SampleTexture(BaseTexture, SampleTexture);
		BaseTexture = SampleTexture;
	};
	
	FSKTexture::SaveTexture(BaseTexture, TexturePath);

	// Import the Colour Texture into the project.
	FString TexturePackagePath = FSKTexture::ImportTextureAsset(TexturePath); 
	SourceTexture = LoadObject<UTexture2D>(nullptr, *TexturePackagePath);

	if (SourceTexture)
	{
		// Assign the Colour Texture to the SK Model.
		Model->CharacterPreset->Modify();
		Model->CharacterPreset->SourceTexture = SourceTexture;

		// Save the SK Model.
		UPackage* Package = Model->CharacterPreset->GetOutermost();
		if (!Package) return;

		// Triggers Unreal to detect and try to reimport the Package
		Package->SetDirtyFlag(true);

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Standalone;
		SaveArgs.Error = GError;
		SaveArgs.SaveFlags = SAVE_NoError;

		FString PackageName = Package->GetName();
		FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
  
		const bool bSuccess = UPackage::SavePackage(Package, Model->CharacterPreset, *PackageFileName, SaveArgs);

		if (!bSuccess)
		{
			UE_LOG(LogTemp, Error, TEXT("Unable to silently update Source Texture file %s"), *PackageName);
		}

		// todo: Still finding the best way to import and update a package without having the Editor populate an alert notifying the user.
		// UPackageTools::SavePackagesForObjects({Package});
		// UPackageTools::ReloadPackages({Package});
	}
}

void SSKColorMenuWidget::CreateColorLookupTableByGroupID(TMap<int32, TArray<FString>>& PartColorByGroup)
{
	for (auto DbRow : Model->PartColourMapUdim)
	{
		TArray<FString>& ValueEntry = PartColorByGroup.FindOrAdd(DbRow.Value.ColorGroup);
		ValueEntry.Add(DbRow.Key);
	}
}
