// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Models/MainMenuModel.h"
#include "Widgets/SCompoundWidget.h"
#include "Menu/MenuDelegates.h"

class SSKColorEntryWidget;

/**
 * 
 */
class SIDEKICKCHARACTERTOOL_API SSKColorMenuWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSKColorMenuWidget)
		: _Model()
		{
		}

		SLATE_ARGUMENT( TSharedPtr< MainMenuModel >, Model);
		
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);
	
	/** The shared UI Model data*/
	TSharedPtr< MainMenuModel > Model;
private:
	
	/** Stores a lookup table for each colour swatch and the name that the swatch relates too. */
	TMap<FString, TSharedRef<SSKColorEntryWidget>> ColorSwatchEntries;
	
	/** Queries the DB for the Part Udims */
	bool GetPartColourMapDataFromDB();
	
	/**
	 * Queries the PixelColor for the specific colour and returns
	 * a pointer to it.
	 *
	 * Texture is 32x32 pixels, and stored in a flat array. 
	 * 
	 * @param U Udim position
	 * @param V Udim position
	 * @return Pointer to the pixels color value
	 */
	FColor* GetSwatchColour(int32 U, int32 V);

	/**
	 * Queries the PixelColor for the specific colour and returns
	 * a pointer to it.
	 *
	 * @param ColorEntry The color property you want to retrieve the colour value for.
	 * @return Pointer to the pixels color value
	 */
	FColor* GetSwatchColour(const FColorProperty& ColorEntry);



	/**
	 * Resets the Temporary Texture to that of the original texture file.
	 *
	 * The original texture file can be the source texture, or the texture that is associated
	 * to the Sidekick Base Material.
	 */
	void ResetTemporaryTexture();

	/**
	 * Checks if a Base Texture exists in the SK Model, if not generates the file and populates the Model
	 */
	void ValidateAndGenerateModelBaseTexture();
	
	/**
	 * Reads the data out of the Texture and copies it into a temporary texture in memory.
	 */
	void PopulateTemporaryTexture();
	
	FOnColorSwatchChanged OnColorSwatchChanged;

	/**
	 * Generates a lookup table that uses the colour group as a key and the name as the value.
	 *
	 * This is the reversed layout of the structure that comes from the DB query.
	 */
	void CreateColorLookupTableByGroupID(TMap<int32, TArray<FString>>& PartColorByGroup);

	void PostConstruct();

public:
	
	/**
	 * Refreshes the color swatch visibilities.
	 *
	 * Parts that have been made active will have there associated colour swatches made visible.
	 * */
	void RefreshColourSwatchVisibility();
};
