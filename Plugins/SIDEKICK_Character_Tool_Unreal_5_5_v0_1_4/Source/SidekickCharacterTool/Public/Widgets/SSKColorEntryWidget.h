#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Models/MainMenuModel.h"
#include "Menu/MenuDelegates.h"

/**
 * Sidekick Colour Entry UI Slate Widget
 *
 * Inputs:
 * - Name of the Colour swatch. This will be used as a key to update the Swatch Texture
 * - Original colour of the swatch. This value is the value that was read from the Texture.
 */
class SIDEKICKCHARACTERTOOL_API SSKColorEntryWidget : public SCompoundWidget
{
	
public:
	SLATE_BEGIN_ARGS(SSKColorEntryWidget)
		: _Text(),
		  _Color(),
		  _Model()
		{}

		SLATE_ATTRIBUTE(FText, Text)
		SLATE_ARGUMENT(FColor*, Color)
		SLATE_ARGUMENT(TSharedPtr< MainMenuModel >, Model)
		SLATE_EVENT(FOnColorSwatchChanged, OnColorSwatchChangedEvent)
		
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);
	
private:

	/** Name to appear in the UI */
	TAttribute< FText > Name;

	// NOTE: This Model could be replaced with passing in a delegate, and then when the colour changed, triggers delegate to update 
	/** The shared UI Model data */
	TSharedPtr< MainMenuModel > Model;

	/** The colour being displayed in the UI */
	FColor* Color;
	FLinearColor SwatchColor;

	FLinearColor GetColor() const;

	/** Delegate that will be triggered when a color change occurs */
	FOnColorSwatchChanged OnColorSwatchChangedEvent;
};
