// Fill out your copyright notice in the Description page of Project Settings.


#include "SSKMainMenuWidget.h"

#include "Editor.h"
#include "SlateOptMacros.h"
#include "EditorViewportCommands.h"
#include "FileHelpers.h"
#include "Styling/SlateStyleMacros.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateStyle.h"
#include "Styling/CoreStyle.h"

#include "UObject/SavePackage.h"

#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Input/SSegmentedControl.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SCheckBox.h"

#include "SidekickDBSubsystem.h"
#include "Assets/FSKCharacterFile.h"
#include "Assets/FSKTexture.h"
#include "Assets/USKDB.h"
#include "Menu/SSKPresetMenuWidget.h"
#include "Menu/SSKPartsMenuWidget.h"
#include "Menu/SSKBodyMenuWidget.h"
#include "Menu/SSKColorMenuWidget.h"
#include "Menu/SSKOptionsMenuWidget.h"

#include "HAL/PlatformFileManager.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

FString BasePathToken = "Synty/SidekickCharacters/Resources";

void SSKMainMenuWidget::Construct(const FArguments& InArgs)
{
	SKCharacterPreset = InArgs._EditingObject;

	PreFlightChecks();
	
	InitializeModelData();
	
	// Initialises UI Widgets
	TSharedPtr<SWidget> MainBanner = MakeMainBanner();
	ActiveMenuWidget = MakeActiveMenu();

	ChangeActiveMenu(ESidekickMenuViews::Presets);
	
	ChildSlot
	[
		SNew(SVerticalBox)

		// Main Banner
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			MainBanner.ToSharedRef()
		]
		
		// Tab button selection
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			MenuSelectionWidget().ToSharedRef()
		]
		
		// Dynamic Menu Section
		+ SVerticalBox::Slot()
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		.FillHeight(1)
		[
			ActiveMenuWidget.ToSharedRef()
		]
		
		// Load Save Export Section
		+ SVerticalBox::Slot()
		.AutoHeight()
		.VAlign(VAlign_Bottom)
		.HAlign(HAlign_Fill)
		[
			CharacterImportExportWidget().ToSharedRef()
		]
	];
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

SSKMainMenuWidget::~SSKMainMenuWidget()
{
	UE_LOG(LogTemp, Log, TEXT("SSKMainMenuWidget Destructor"));
}


void SSKMainMenuWidget::ChangeActiveMenu(ESidekickMenuViews ChosenMenu)
{
	ActiveMenu = ChosenMenu;

	// Refresh Colour Swatches when Tab is selected
	if (ChosenMenu == ESidekickMenuViews::Colors)
	{
		if (ColorMenu.IsValid())
		{
			ColorMenu->RefreshColourSwatchVisibility();
		}
	}

	// NOTE: The refresh is an ugly fix to keep the species in sync, we should have a boolean that flags on when a
	// species changes and then when a tab changes flag the boolean off. This will reduce unneeded cpu cycles and
	// excess ui clearing.
	
	else if (ChosenMenu == ESidekickMenuViews::Parts)
	{
		PartsMenu->RefreshUI();
	}
	else if (ChosenMenu == ESidekickMenuViews::Presets)
	{
		PresetMenu->RefreshUI();
	}
}

// todo: Refactor this to be called by a callback

/**
 * Creates the active menu area, that gets switched between when the main menu button selection changes.
 *
 * This holds the widget that the user will interact with depending on the activated menu.
 * @return The widget switcher that holds each menu widget.
 */
TSharedPtr<SWidget> SSKMainMenuWidget::MakeActiveMenu()
{
	return SNew(SWidgetSwitcher)
		.WidgetIndex_Lambda([this](){ return (int32)ActiveMenu; })
		+ SWidgetSwitcher::Slot()
		[
			SAssignNew(PresetMenu, SSKPresetMenuWidget)
			.Model(UIModel)
		]
		+ SWidgetSwitcher::Slot()
		[
			SAssignNew(PartsMenu, SSKPartsMenuWidget)
			.Model(UIModel)
		]
		+ SWidgetSwitcher::Slot()
		[
			SAssignNew(BodyMenu, SSKBodyMenuWidget)
			.Model(UIModel)
		]
		+ SWidgetSwitcher::Slot()
		[
			SAssignNew(ColorMenu, SSKColorMenuWidget)
			.Model(UIModel)
		]
		+ SWidgetSwitcher::Slot()
		[
			SAssignNew(OptionMenu, SSKOptionsMenuWidget)
			.Model(UIModel)
		];
}

/**
 * Created the Main Banner Widget for the UI
 * @return 
 */
TSharedPtr<SWidget> SSKMainMenuWidget::MakeMainBanner() const
{
	const ISlateStyle* BannerStyle = FSlateStyleRegistry::FindSlateStyle("SidekickCharacterToolStyle");
	
	TSharedPtr<SOverlay> Overlay = SNew(SOverlay);

	// Background Colour
	Overlay->AddSlot()
		[
			SNew(SScaleBox)
			.Stretch(EStretch::Fill)
			[
				SNew(SImage)
				.Image(BannerStyle->GetBrush("SidekickCharacterTool.SKDefaultRed"))
			]
		];

	// Synty Sidekick Banner
	Overlay->AddSlot()
		[
			SNew(SScaleBox)
			.Stretch(EStretch::ScaleToFit)
			[
				SNew(SImage)
				.Image(BannerStyle->GetBrush("SidekickCharacterTool.MainMenuBanner"))
			]
		];
	
	return Overlay;
}

/**
 * Creates the Character Sheet import / export widget
 * @return 
 */
TSharedPtr<SWidget> SSKMainMenuWidget::CharacterImportExportWidget()
{
	return SNew(SVerticalBox)
		
		+ SVerticalBox::Slot()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Text(FText::FromString("Load Character (.sk)"))
				.IsEnabled(true) // todo: Implement functionality
				.OnClicked_Lambda([this]()
				{
					// todo: ask user to select file
					FString SKPath;
					bool bSuccess = FSKCharacterFile::OpenFileDialog(SKPath);
					if (bSuccess){
					
						SKFileData SKFileData = FSKCharacterFile::ReadSKFile(SKPath);
						UIModel->LoadSKData(SKFileData);
					}
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot()
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Text(FText::FromString("Save Character (.sk)"))
				.IsEnabled(false)// todo: Implement functionality
			]
		]
		// NOTE: Removed as Unreal utilises the SKPreset asset.
		// + SVerticalBox::Slot()
		// [
		// 	SNew(SButton)
		// 	.HAlign(HAlign_Center)
		// 	.VAlign(VAlign_Center)
		// 	.Text(FText::FromString("Export Characters as asset"))
		// 	.IsEnabled(false)// todo: Implement functionality
		// ]
		;
}

/**
 * Create the Menu Buttons that appear as tabs
 * @return 
 */
TSharedPtr<SWidget> SSKMainMenuWidget::MenuSelectionWidget()
{
	return SNew(SSegmentedControl<ESidekickMenuViews>)
			.OnValueChanged_Lambda([this] (ESidekickMenuViews InMode)
			{
				if (InMode != GetActiveMenuMode()) ChangeActiveMenu(InMode);
			} )
			.Value(this, &SSKMainMenuWidget::GetActiveMenuMode)
		
			+ SSegmentedControl<ESidekickMenuViews>::Slot(ESidekickMenuViews::Presets)
			[
				SNew(STextBlock)
				.Text(FText::FromString("Presets"))
			]
		
			+ SSegmentedControl<ESidekickMenuViews>::Slot(ESidekickMenuViews::Parts)
			[
				SNew(STextBlock)
				.Text(FText::FromString("Parts"))
			]
		
			+ SSegmentedControl<ESidekickMenuViews>::Slot(ESidekickMenuViews::Body)
			[
				SNew(STextBlock)
				.Text(FText::FromString("Body"))
			]
		
			+ SSegmentedControl<ESidekickMenuViews>::Slot(ESidekickMenuViews::Colors)
			[
				SNew(STextBlock)
				.Text(FText::FromString("Colors"))
			]
		
			+ SSegmentedControl<ESidekickMenuViews>::Slot(ESidekickMenuViews::Options)
			[
				SNew(STextBlock)
				.Text(FText::FromString("Options"))
			];
}

/**
 * Initialises the UI's ViewModel object.
 */
void SSKMainMenuWidget::InitializeModelData()
{
	// Initializes the Model Data
	UIModel = MakeShared<MainMenuModel>();
	UIModel->Initialize();
	UIModel->CharacterPreset = SKCharacterPreset;
	UIModel->PreflightChecksCharacterPreset();
	
	// Detect all Part's Colour Swatches
	UIModel->UpdateAvailableColourSwatches();
}

/**
 * Generates the default Synty Sidekick folder structure
 */
void SSKMainMenuWidget::GenerateSyntyDefaultFolderStructure()
{
	FString ContentPath = FPaths::ProjectContentDir();

	TArray<FString> FolderNames = { TEXT( "Materials"), TEXT( "Meshes"), TEXT( "Skeletons"), TEXT( "Textures") };

	FString CreationPath = ContentPath + "/" + BasePathToken;

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	
	bool bCreated = true;
	
	if (!PlatformFile.DirectoryExists(*CreationPath))
		bCreated = PlatformFile.CreateDirectoryTree(*CreationPath);

	if (bCreated)
	{
		FString NewFolderPath = "";
		for (auto Folder : FolderNames)
		{
			NewFolderPath = CreationPath + "/" + Folder;

			if (!PlatformFile.DirectoryExists(*NewFolderPath))
				PlatformFile.CreateDirectoryTree(*NewFolderPath);
		}
	}
}

/**
 * Checks Sidekick's database file exists, else download it
 */
void SSKMainMenuWidget::ValidateDatabaseFile()
{
	USKDB* DBFileInteraction = NewObject<USKDB>();
	
	if (!DBFileInteraction->DBFileExists())
	{
		// Callback
		DBFileInteraction->SuccessCallback.BindLambda([this](FString ZipPath)
		{
			UE_LOG(LogTemp, Error, TEXT("SSKMainMenuWidget::PreFlightChecks - DB Downloaded Sucessfully"));

			USidekickDBSubsystem* Subsystem = GEditor->GetEditorSubsystem<USidekickDBSubsystem>();
			if (Subsystem)
			{
				bool DBOpened;
				Subsystem->OpenDatabase(DBOpened);
			}

			UIModel->Initialize();
			UIModel->PreflightChecksCharacterPreset();
			UIModel->UpdateAvailableColourSwatches();
			
			ResetMenus();
			ActiveMenuWidget = MakeActiveMenu();

		});

		// Trigger DB query and retrieval
		UE_LOG(LogTemp, Error, TEXT("Database file not found.. Downloading from Github."));
		DBFileInteraction->GetLatestVersion();
	}
}

void SSKMainMenuWidget::ResetMenus()
{
	PresetMenu.Reset();
	PartsMenu.Reset();
	BodyMenu.Reset();
	ColorMenu.Reset();
	OptionMenu.Reset();
}


/**
 * Checks to make sure that the default material exists in the correct location with the correct name.
 */
void SSKMainMenuWidget::ValidateDefaultMaterial()
{
	FString ContentPath = FPaths::ProjectContentDir();
	FString MaterialsPath = ContentPath + BasePathToken + "/Materials";
	FString MaterialsGamePath = "/Game/" + BasePathToken + "/Materials";
	FString FileName = "M_Default_Sidekick";
	FString FilePath = MaterialsPath + "/" + FileName + ".uasset";
	FString PluginMaterialPath = "/SidekickCharacterTool/Materials/M_Default_Sidekick";
	FString RelativeMaterialPath = FPaths::ConvertRelativePathToFull(FilePath);

	IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
	
	if (FPaths::DirectoryExists(*MaterialsPath))
	{
			
		if (!FileManager.FileExists(*RelativeMaterialPath))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Sidekicks] Default Material not found, duplicating from plugin..."))
			
			UMaterialInterface* NewMaterial = FSKTexture::DuplicateMaterialFromPath(PluginMaterialPath, FileName, MaterialsGamePath);

			if (NewMaterial)
			{
				UPackage* Package = NewMaterial->GetPackage();
				const FString PackageName = Package->GetName();
				const FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
				
				Package->MarkAsFullyLoaded();  // Ensure fully loaded
				
				FSavePackageArgs SaveArgs;
				SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
				SaveArgs.SaveFlags = SAVE_FromAutosave;
				SaveArgs.Error = GError;
				
				const bool bSuccess = UPackage::SavePackage(Package, nullptr, *PackageFileName, SaveArgs);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[Sidekicks] Failed to duplicate Default Material from plugin folder."))
			}
		}
	}
}


void SSKMainMenuWidget::PreFlightChecks()
{
	GenerateSyntyDefaultFolderStructure();

	ValidateDefaultMaterial();
	
	ValidateDatabaseFile();
}

void SSKMainMenuWidget::RefreshPresetSpecie()
{
	if (PresetMenu.IsValid())
	{
		PresetMenu->RefreshUI();
	}
	
	if (PartsMenu.IsValid())
	{
		PartsMenu->RefreshUI();
	}
}
