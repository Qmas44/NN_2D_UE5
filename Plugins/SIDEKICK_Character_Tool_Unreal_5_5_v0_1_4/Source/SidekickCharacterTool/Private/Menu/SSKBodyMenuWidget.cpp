// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu/SSKBodyMenuWidget.h"

#include "AnimationCoreLibrary.h"
#include "SkeletalMeshBuilder.h"
#include "SlateOptMacros.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Enums/BlendShapeTypes.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SButton.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

/*
 * defaultBuff
 * defaultHeavy
 * defaultSkinny
 * masculineFeminine
 */

void SSKBodyMenuWidget::Construct(const FArguments& InArgs)
{
	Model = InArgs._Model;

	PopulateMTOffsets(); // todo: Moved to character creation

	// Moves the attachments into position that matches the blendshapes
	CalculateMTOffset(Model.Get());
	
	ChildSlot
	[
		SNew(SVerticalBox)

		// Body Type
		+SVerticalBox::Slot()
		.Padding(10, 10, 10, 10)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				SNew(STextBlock)
				.MinDesiredWidth(150)
				.Text(FText::FromString("Body Type"))
			]
			+SHorizontalBox::Slot()
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				[
					SNew(SSlider)
					.MinValue(0)
					.MaxValue(1)
					.Value_Lambda([this]()
					{
						return GetBodyType();
					})
					.OnValueChanged_Lambda([this](float NewValue)
					{
						Model->CharacterPreset->MTBodyType = NewValue;
						CalculateMTOffset(Model.Get());
					})
					.OnMouseCaptureBegin_Lambda([this]()
					{
						Model->CharacterPreset->Modify();
					})
					
				]
				+SVerticalBox::Slot()
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Top)
					[
						SNew(STextBlock)
						.Text(FText::FromString("Masculine"))
					]
					+SHorizontalBox::Slot()
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Top)
					[
						SNew(STextBlock)
						.Text(FText::FromString("Feminine"))
					]
				]
			]
		]

		// Body Size
		
		+SVerticalBox::Slot()
		.Padding(10, 10, 10, 10)
		.AutoHeight()
		[
			SNew(SHorizontalBox)

			+SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				SNew(STextBlock)
				.MinDesiredWidth(150)
				.Text(FText::FromString("Body Size"))
			]

			+SHorizontalBox::Slot()
			[
				SNew(SVerticalBox)

				+SVerticalBox::Slot()
				[
					SNew(SSlider)
					.MinValue(-1)
					.MaxValue(1)
					.Value_Lambda([this]()
					{
						return GetBodySize();
					})
					.OnValueChanged_Lambda([this](float NewValue)
					{
						Model->CharacterPreset->MTBodySize = NewValue;
						CalculateMTOffset(Model.Get());
					})
					.OnMouseCaptureBegin_Lambda([this]()
					{
						Model->CharacterPreset->Modify();
					})
				]
			
				+SVerticalBox::Slot()
				[
					SNew(SHorizontalBox)

					+SHorizontalBox::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Top)
					[
						SNew(STextBlock)
						.Text(FText::FromString("Slim"))
					]
					
					+SHorizontalBox::Slot()
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Top)
					[
						SNew(STextBlock)
						.Text(FText::FromString("Heavy"))
					]
				]
			]
		]

		// Musculature
		
		+SVerticalBox::Slot()
		.Padding(10, 10, 10, 10)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				SNew(STextBlock)
				.MinDesiredWidth(150)
				.Text(FText::FromString("Musculature"))
			]
			+SHorizontalBox::Slot()
			[
				SNew(SVerticalBox)

				+SVerticalBox::Slot()
				[
					SNew(SSlider)
					.MinValue(0)
					.MaxValue(1)
					.Value_Lambda([this]()
					{
						return GetBodyMuscular();
					})
					.OnValueChanged_Lambda([this](float NewValue)
					{
						Model->CharacterPreset->MTBodyMusculature = NewValue;
						CalculateMTOffset(Model.Get());
					})
					.OnMouseCaptureBegin_Lambda([this]()
					{
						Model->CharacterPreset->Modify();
					})
				]
			
				+SVerticalBox::Slot()
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Top)
					[
						SNew(STextBlock)
						.Text(FText::FromString("Lean"))
					]
					+SHorizontalBox::Slot()
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Top)
					[
						SNew(STextBlock)
						.Text(FText::FromString("Muscular"))
					]
				]
			]
		]
	];
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

// todo: This should be run on creation of the character
void SSKBodyMenuWidget::PopulateMTOffsets()
{
	Model->CharacterPreset->MorphTargetOffsets.Empty();
	
	for (auto MtOffsetEntry : Model->MorphTargetOffsetLookup)
	{
		
		if (!Model->CharacterPreset->MorphTargetOffsets.Contains(MtOffsetEntry.Key))
		{
			Model->CharacterPreset->MorphTargetOffsets.Add(MtOffsetEntry.Key);
		}
	}
}

/**
 * Calculates the Offset transform for each attachment part against the three blendshape attributes.
 */
void SSKBodyMenuWidget::CalculateMTOffset(MainMenuModel* Model)
{
	// The three body shape slider values
	float* MusculartureDriver = &Model->CharacterPreset->MTBodyMusculature;
	float* SizeDriver = &Model->CharacterPreset->MTBodySize;
	float* TypeDriver = &Model->CharacterPreset->MTBodyType;
	
	for (auto DBOffsetEntry : Model->MorphTargetOffsetLookup)
	{
		FTransform PartOffsetTransform = FTransform();

		// SocketName, is the socket name with spaces e.g. `Attachment Back`
		FString SocketName = DBOffsetEntry.Key;
		
		// Wold position for each blend for the current socket.
		TMap<int32, FTransform>* BlendshapeWorldPosition = &DBOffsetEntry.Value.PartBlendOffsets;
		
		
		if (*SizeDriver < 0.f)
		{
			int32 MTType = static_cast<int32>(EBlendShapeType::Skinny); // Morph Target Type

			if (BlendshapeWorldPosition->Contains(MTType))
			{
				FTransform MTOffset = (*BlendshapeWorldPosition)[MTType];

				FVector UnrealTransform;
				UnrealTransform.X = MTOffset.GetTranslation().Z;
				UnrealTransform.Y = MTOffset.GetTranslation().X;
				UnrealTransform.Z = MTOffset.GetTranslation().Y;
				
				PartOffsetTransform.AddToTranslation(UnrealTransform * (*SizeDriver * -1));
				// PartOffsetTransform.SetRotation(  FQuat::MakeFromEuler( PartOffsetTransform.GetRotation().Euler() + MTOffset.GetRotation().Euler() * *SizeDriver * -1) );
			}
		}
		else
		{
			int32 MTType = static_cast<int32>(EBlendShapeType::Heavy); // Morph Target Type

			if (BlendshapeWorldPosition->Contains(MTType))
			{
				FTransform MTOffset = (*BlendshapeWorldPosition)[MTType];

				FVector UnrealTransform;
				UnrealTransform.X = MTOffset.GetTranslation().Z;
				UnrealTransform.Y = MTOffset.GetTranslation().X;
				UnrealTransform.Z = MTOffset.GetTranslation().Y;
				
				PartOffsetTransform.AddToTranslation(UnrealTransform * *SizeDriver);
				// PartOffsetTransform.SetRotation(  FQuat::MakeFromEuler( PartOffsetTransform.GetRotation().Euler() + MTOffset.GetRotation().Euler() * *SizeDriver) );
			}
		}

		if (*MusculartureDriver > 0.f)
		{
			int32 MTType = static_cast<int32>(EBlendShapeType::Bulk); // Morph Target Type

			if (BlendshapeWorldPosition->Contains(MTType))
			{
				FTransform MTOffset = (*BlendshapeWorldPosition)[MTType];

				FVector UnrealTransform;
				UnrealTransform.X = MTOffset.GetTranslation().Z;
				UnrealTransform.Y = MTOffset.GetTranslation().X;
				UnrealTransform.Z = MTOffset.GetTranslation().Y;
				
				PartOffsetTransform.AddToTranslation(UnrealTransform * *MusculartureDriver);
				// PartOffsetTransform.SetRotation(  FQuat::MakeFromEuler( PartOffsetTransform.GetRotation().Euler() + MTOffset.GetRotation().Euler() * *MusculartureDriver) );
			}
		}
	
		if (*TypeDriver > 0.f)
		{
			int32 MTType = static_cast<int32>(EBlendShapeType::Feminine); // Morph Target Type
			
			if (BlendshapeWorldPosition->Contains(MTType))
			{
				FTransform MTOffset = (*BlendshapeWorldPosition)[MTType];

				FVector UnrealTransform;
				UnrealTransform.X = MTOffset.GetTranslation().Z;
				UnrealTransform.Y = MTOffset.GetTranslation().X;
				UnrealTransform.Z = MTOffset.GetTranslation().Y;
				
				PartOffsetTransform.AddToTranslation(UnrealTransform * *TypeDriver);
				// PartOffsetTransform.SetRotation(  FQuat::MakeFromEuler( PartOffsetTransform.GetRotation().Euler() + MTOffset.GetRotation().Euler() * *TypeDriver) );
			}
		}

		if (Model->CharacterPreset->MorphTargetOffsets.Contains(SocketName))
		{
			Model->CharacterPreset->MorphTargetOffsets[SocketName] = PartOffsetTransform;
		}
	}
}

float SSKBodyMenuWidget::GetBodySize()
{
	return Model->CharacterPreset->MTBodySize;
}
float SSKBodyMenuWidget::GetBodyType()
{
	return Model->CharacterPreset->MTBodyType;
}
float SSKBodyMenuWidget::GetBodyMuscular()
{
	return Model->CharacterPreset->MTBodyMusculature;
}