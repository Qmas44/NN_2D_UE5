#include "Widgets/SSKColorEntryWidget.h"

#include "CoreMinimal.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Colors/SColorPicker.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Math/Color.h"


void SSKColorEntryWidget::Construct(const FArguments& InArgs)
{
	Name = InArgs._Text;
	Model = InArgs._Model;
	Color = InArgs._Color;
	OnColorSwatchChangedEvent = InArgs._OnColorSwatchChangedEvent;
	
	SwatchColor = FLinearColor::FromSRGBColor(*Color);
	
	ChildSlot
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(STextBlock)
			.MinDesiredWidth(150)
			.Text(Name)
		]
		+ SHorizontalBox::Slot()
		.FillWidth(2.0f)
		[
			SNew(SColorBlock)
			.UseSRGB(true)
			.Color(this, &SSKColorEntryWidget::GetColor)
			.ShowBackgroundForAlpha(false)
			.Size(FVector2D(10, 10))
			.OnMouseButtonDown_Lambda([this](const FGeometry& MyGeometry,	 const FPointerEvent& MouseEvent)
			{
				UE_LOG(LogTemp, Display, TEXT("%s"), *Name.Get().ToString());
				
				if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
				{
					FColorPickerArgs PickerArgs;
					PickerArgs.bIsModal = false; // set to true for only update on click
					PickerArgs.bUseAlpha = false;
					PickerArgs.bOnlyRefreshOnMouseUp = false;
					PickerArgs.InitialColor = FLinearColor::FromSRGBColor(*Color);
					
					// Live update
					PickerArgs.OnColorCommitted = FOnLinearColorValueChanged::CreateLambda(
						[this](FLinearColor NewColor)
						{
							SwatchColor = NewColor;

							if (OnColorSwatchChangedEvent.IsBound())
							{
								OnColorSwatchChangedEvent.Execute(Name.Get().ToString(), SwatchColor.ToFColorSRGB());
							}
						}
					);

					OpenColorPicker(PickerArgs);

					return FReply::Handled();
				}
				return FReply::Unhandled();
			})
		]
	];
}

FLinearColor SSKColorEntryWidget::GetColor() const
{
	return FLinearColor::FromSRGBColor(*Color);
}