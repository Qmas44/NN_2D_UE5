// Fill out your copyright notice in the Description page of Project Settings.

#include "Widgets/SSKColorPresetWidgetElement.h"
#include "Styling/SlateStyleRegistry.h"

#include "SlateOptMacros.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
// todo: Rename tp SSKPresetWidgetElement
void SSKColorPresetWidgetElement::Construct(const FArguments& InArgs)
{
	Model = InArgs._Model;
	TextContent = InArgs._Text;
	ClearButtonVisible = InArgs._AddClearButton;
	OptionsSource = InArgs._ComboOptionsSource;
	ComboboxIndex = InArgs._ComboIndex;

	TSharedRef<SComboBox<TSharedPtr<FString>>> DropdownWidget = SNew(SComboBox<TSharedPtr<FString>>)
		.OptionsSource(OptionsSource)
		.OnGenerateWidget(InArgs._ComboBoxOnGenerateWidget)
		.OnSelectionChanged(InArgs._ComboBoxOnSelectionChanged)
		.Content()
		[
			SNew(STextBlock)
			.MinDesiredWidth(230)
			.Text(InArgs._ComboBoxActiveText)
		];

	DropdownWidgetPtr = DropdownWidget;
	
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
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				ConstructClearButton()
			]
			
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				ConstructPrevButton()
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				ConstructNextButton()
			]
			
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				ConstructRandomButton()
			]

			// Preset ComboBox
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Fill)
			[
				DropdownWidget
			]
		]
	];
}

TSharedRef<SButton> SSKColorPresetWidgetElement::ConstructClearButton()
{
	const ISlateStyle* SyntyStyle = FSlateStyleRegistry::FindSlateStyle("SidekickCharacterToolStyle");
	TAttribute<EVisibility> VisibilitySettings = EVisibility::Visible;
	
	if (!ClearButtonVisible.Get())
	{
		VisibilitySettings = EVisibility::Hidden;
	}
	
	return SNew(SButton)
		.ForegroundColor(FSlateColor::UseStyle())
		.Visibility(VisibilitySettings)
		.Content()
		[
			SNew(SImage)
			.Image(SyntyStyle->GetBrush("SidekickCharacterTool.ClearIcon"))
		]
		.OnClicked_Lambda([this]()
		{
			DropdownWidgetPtr.Get()->SetSelectedItem( (*OptionsSource)[0]);
			return FReply::Handled();
		});
}

TSharedRef<SButton> SSKColorPresetWidgetElement::ConstructPrevButton()
{
	return SNew(SButton)
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
			if (OptionsSource == nullptr || OptionsSource->Num() == 0) return FReply::Handled();
			
			int Index = ComboboxIndex.Get();
			Index = Index - 1 < 1 ? OptionsSource->Num() - 1 : Index - 1;
			ComboboxIndex.Set(Index);
			
			auto RandomEntry = (*OptionsSource)[Index];
			
			DropdownWidgetPtr.Get()->SetSelectedItem(RandomEntry);
			
			return FReply::Handled();
		});
}

TSharedRef<SButton> SSKColorPresetWidgetElement::ConstructNextButton()
{
	return SNew(SButton)
		.ForegroundColor(FSlateColor::UseStyle())
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Content()
		[
			SNew(STextBlock)
			.Text(FText::FromString(">"))

		].OnClicked_Lambda([this]()
		{
			if (OptionsSource == nullptr || OptionsSource->Num() <= 1) return FReply::Handled();
			
			int Index = ComboboxIndex.Get();
			Index = Index + 1 > (OptionsSource->Num() - 1) ? 1 : Index + 1;
			ComboboxIndex.Set(Index);
			
			auto RandomEntry = (*OptionsSource)[Index];
			
			DropdownWidgetPtr.Get()->SetSelectedItem(RandomEntry);
			
			return FReply::Handled();
		});
}

TSharedRef<SButton> SSKColorPresetWidgetElement::ConstructRandomButton()
{
	const ISlateStyle* SyntyStyle = FSlateStyleRegistry::FindSlateStyle("SidekickCharacterToolStyle");
	
	return SNew(SButton)
				.ForegroundColor(FSlateColor::UseStyle())
				.Content()
				[
					SNew(SImage)
					.Image(SyntyStyle->GetBrush("SidekickCharacterTool.RandomIcon"))
				]
				.OnClicked_Lambda([this]()
				{
					if (OptionsSource == nullptr || OptionsSource->Num() <= 1) return FReply::Handled();
					
					auto RandomIndex = FMath::RandRange(1, OptionsSource->Num() - 1);

					auto RandomEntry = (*OptionsSource)[RandomIndex];
					
					DropdownWidgetPtr.Get()->SetSelectedItem(RandomEntry);
					
					return FReply::Handled();
				});
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
