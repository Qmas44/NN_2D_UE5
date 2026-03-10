// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu/SSKOptionsMenuWidget.h"

#include "AssetToolsModule.h"
#include "DesktopPlatformModule.h"
#include "IAssetTools.h"
#include "PackageTools.h"
#include "SkeletalMergingLibrary.h"
#include "SlateOptMacros.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetUtils/CreateSkeletalMeshUtil.h"
#include "SkeletalMeshModelingToolsMeshConverter.generated.h"
#include "SSKMainMenuWidget.h"
#include "Assets/FSPak.h"
#include "Menu/Popup/SKFolderPickerPopup.h"
#include "Menu/Popup/SKImportProgressPopup.h"
#include "SKMerger/Public/MergeBPLibrary.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SExpandableArea.h"

#define LOCTEXT_NAMESPACE "SKOptionsMenuWidget"

#define VIDEO_GETTING_STARTED "https://www.youtube.com/watch?v=oChl7pEPfP0"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SSKOptionsMenuWidget::Construct(const FArguments& InArgs)
{

	Model = InArgs._Model;

	InitMergeOptions();
	
	ChildSlot
	[
		SNew(SVerticalBox)

#pragma region Removed For Debug		
		// +SVerticalBox::Slot()
		// .AutoHeight()
		// [
		// 	InitMergeOptions()
		// ]
		//
		// +SVerticalBox::Slot()
		// .AutoHeight()
		// [
		// 	SNew(SButton)
		// 	.Text(LOCTEXT("SK_MergeParts","Merge Sidekick"))
		// 	.OnClicked(this, &SSKOptionsMenuWidget::OnMergeClicked)
		// ]
#pragma endregion Removed For Debug
		
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SButton)
			.Text(LOCTEXT("SK_ImportAssetPak","Import Sidekick Package"))
			.OnClicked(this, &SSKOptionsMenuWidget::OnImportPakClicked)
		]

		+SVerticalBox::Slot()
		.Padding(20)
		.AutoHeight()
		[
			SNew(SButton)
			.Text(LOCTEXT("SK_VidGettingStarter","Getting Started Video"))
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.OnClicked(this, &SSKOptionsMenuWidget::OnGettingStartedClicked)
		]
	];
}

TSharedRef<SExpandableArea> SSKOptionsMenuWidget::InitMergeOptions()
{
	TSharedRef<SVerticalBox> VBox = SNew(SVerticalBox);

	MergeOptionsVerticalBox = VBox;
	
	TSharedRef<SExpandableArea> FilterSection = SNew(SExpandableArea)
	.AreaTitle(FText::FromString("Merge SkeletalMesh Settings"))
	.InitiallyCollapsed(false)
	.BodyContent()
		[
			VBox	
		];

	VBox->AddSlot()
	.Padding(10,0,0,0)
	[
		SNew(SCheckBox)
		.IsChecked_Lambda([this]()
		{
			return Model->MergeSettings->bCustomMergeLocation ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
		})
		.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
		{
			if (NewState == ECheckBoxState::Checked)
			{
				Model->MergeSettings->bCustomMergeLocation = true;
			}
			else
			{
				Model->MergeSettings->bCustomMergeLocation = false;
			}
			
		})
		[
			SNew(STextBlock)
			.Text(LOCTEXT("MergeSettings", "Custom Merge Location"))
		]
	];
		
	VBox->AddSlot()
	.Padding(10,0,0,0)
	[
		SNew(SCheckBox)
		.IsChecked_Lambda([this]()
		{
			return Model->MergeSettings->bKeepBodyMorphTargets ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
		})
		.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
		{
			if (NewState == ECheckBoxState::Checked)
			{
				Model->MergeSettings->bKeepBodyMorphTargets = true;
			}
			else
			{
				Model->MergeSettings->bKeepBodyMorphTargets = false;
			}
			
		})
		[
			SNew(STextBlock)
			.Text(LOCTEXT("MergeSettings", "Keep Body Morph Targets"))
		]
	];

	VBox->AddSlot()
	.Padding(10,0,0,0)
	[
		SNew(SCheckBox)
		.IsChecked_Lambda([this]()
		{
			return Model->MergeSettings->bKeepFacialMorphTargets ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
		})
		.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
		{
			if (NewState == ECheckBoxState::Checked)
			{
				Model->MergeSettings->bKeepFacialMorphTargets = true;
			}
			else
			{
				Model->MergeSettings->bKeepFacialMorphTargets = false;
			}
			
		})
		[
			SNew(STextBlock)
			.Text(LOCTEXT("MergeSettings", "Keep Facial Morph Targets"))
		]
	];
	
	return FilterSection;
}


END_SLATE_FUNCTION_BUILD_OPTIMIZATION

// Unreal UE Retargeting options
// - Realtime retargeting
// - Baking retargeting

FReply SSKOptionsMenuWidget::OnMergeClicked()
{
	bool bMergeSection = Model->MergeSettings->bMergeSection;
	bool bKeepFacialMorphTargets = Model->MergeSettings->bKeepFacialMorphTargets;
	bool bKeepBodyMorphTargets = Model->MergeSettings->bKeepBodyMorphTargets;
	bool bCustomMergeLocation = Model->MergeSettings->bCustomMergeLocation;

	UMergeBPLibrary::GenerateSkeletalMesh(Model->CharacterPreset, bMergeSection, bKeepFacialMorphTargets, bKeepBodyMorphTargets, bCustomMergeLocation);
	
	return FReply::Handled();
}

/**
 * Imports all assets found in the Sidekick project folder, once the folder is specified.
 * @return 
 */
FReply SSKOptionsMenuWidget::OnImportPakClicked()
{
	SSKFolderPickerPopup::OpenInWindow(

		// --- Accept callback ---
		SSKFolderPickerPopup::FOnFoldersAccepted::CreateLambda(
			[this](const TArray<FString>& SelectedFolders)
			{
				TArray<FString> PartFiles;
				FSPak::FindUAssets(SelectedFolders, PartFiles);
				SSKImportProgressPopup::OpenInWindow(
					PartFiles,
					FOnImportCompletedDelegate::CreateSP(this, &SSKOptionsMenuWidget::RefreshMenu));

				
			}),

		// --- Cancel callback (optional) ---
		FSimpleDelegate::CreateLambda([this]()
		{
			UE_LOG(LogTemp, Log, TEXT("Folder picker cancelled."));
		})
	);

	return FReply::Handled();
}

FReply SSKOptionsMenuWidget::OnGettingStartedClicked()
{
	FPlatformProcess::LaunchURL(TEXT(VIDEO_GETTING_STARTED), nullptr, nullptr);
	return FReply::Handled();
}

void SSKOptionsMenuWidget::RefreshMenu()
{
	// Your refresh logic here
	UE_LOG(LogTemp, Log, TEXT("[SKMenu] Import complete — refreshing menu."));
	
	// Walks up the UI hierarchy to find the main menu and then resets the Menu
	TWeakPtr<SWidget> CurrentWeak = GetParentWidget();
				
	while (TSharedPtr<SWidget> Current = CurrentWeak.Pin())
	{
		if (TSharedPtr<SSKMainMenuWidget> Found = StaticCastSharedPtr<SSKMainMenuWidget>(Current))
		{
			Model->Initialize();
			Model->PreflightChecksCharacterPreset();
			Model->UpdateAvailableColourSwatches();
			break;
		}
		CurrentWeak = Current->GetParentWidget();
	}
}

#undef LOCTEXT_NAMESPACE