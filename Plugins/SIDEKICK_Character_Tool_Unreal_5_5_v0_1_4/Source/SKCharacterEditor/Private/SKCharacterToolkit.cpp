// Fill out your copyright notice in the Description page of Project Settings.

#include "SKCharacterToolkit.h"

#include "DesktopPlatformModule.h"
#include "Assets/FSPak.h"
#include "SKCharacterEditor.h"
#include "SKCharacterViewport.h"
#include "SSKMainMenuWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Docking/SDockTab.h"

#include "PropertyEditorModule.h"
#include "SKCharacterEditorCommands.h"
#include "Modules/ModuleManager.h"
#include "Menu/Popup/SKFolderPickerPopup.h"
#include "Menu/Popup/SKImportProgressPopup.h"

FName ViewportTab = FName(TEXT("ViewportTab"));
FName StatsTab = FName(TEXT("StatsTab"));
FName DetailsTab = FName(TEXT("DetailsTab"));
FName SKMenuTab = FName(TEXT("SKMenuTab"));

#define LOCTEXT_NAMESPACE "FSKCharacterEditorModule"

void FSKCharacterToolkit::Initialize(const TSharedPtr<IToolkitHost>& InitToolkitHost, USKCharacterPreset* CharacterPreset)
{
	SidekickCharacterPreset = CharacterPreset;

	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout(FName(TEXT("SKCharacterEditorLayout")))
	->AddArea
	(
		FTabManager::NewPrimaryArea()
		->SetOrientation(Orient_Horizontal)
		->Split
		(
			FTabManager::NewStack()
			 ->SetSizeCoefficient(.65f)
			 ->AddTab(ViewportTab, ETabState::OpenedTab)
			 ->SetHideTabWell(true)
		)
		->Split
		(
			FTabManager::NewStack()
			->SetSizeCoefficient(.35f)
			->AddTab(SKMenuTab, ETabState::OpenedTab)
			->SetHideTabWell(true)

		)
	);

	FSKCharacterEditorCommands::Register();

	ToolkitCommands->MapAction(
		FSKCharacterEditorCommands::Get().ImportSidekickPakAction,
		FExecuteAction::CreateSP(this, &FSKCharacterToolkit::ImportSidekickPak),
		FCanExecuteAction()
	);
	
	RegisterToolbarExtension();
	
	InitAssetEditor(EToolkitMode::Standalone, InitToolkitHost, GetToolkitFName(), Layout, true, true, CharacterPreset);

	RegenerateMenusAndToolbars();
}

void FSKCharacterToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& TabManagerRef)
{
	WorkspaceMenuCategory = TabManagerRef->AddLocalWorkspaceMenuCategory(INVTEXT("Sidekicks"));

	UE_LOG(LogSKCharacterEditor, Display, TEXT("Registered TabSpawners"));

	// VIEWPORT
	TabManagerRef->RegisterTabSpawner(ViewportTab, FOnSpawnTab::CreateLambda([this](const FSpawnTabArgs&)
	{
		return SNew(SDockTab)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SSKCharacterViewport)
				.EditingObject(SidekickCharacterPreset.Get())
			]
		];
	}))
	.SetDisplayName(INVTEXT("Viewport"))
	.SetGroup(WorkspaceMenuCategory.ToSharedRef());
	
	// STATS PLACEHOLDER
	TabManagerRef->RegisterTabSpawner(StatsTab, FOnSpawnTab::CreateLambda([=](const FSpawnTabArgs&)
	{
		return SNew(SDockTab)
		.TabRole(PanelTab)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			.Padding(10)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Visibility(EVisibility::HitTestInvisible)
				.TextStyle(FAppStyle::Get(), "Graph.CornerText")
				.Text(FText::FromString("STATS PLACEHOLDER"))
			]
		];
	}))
	.SetDisplayName(INVTEXT("Stats"))
	.SetGroup(WorkspaceMenuCategory.ToSharedRef());

	// DETAILS VIEW
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;

	const TSharedRef<IDetailsView> DetailsView = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(DetailsViewArgs);
	
	DetailsView->SetObjects(TArray<UObject*> { SidekickCharacterPreset.Get() });

	//todo: Working here to try and figure out why the Details view causes it to crash. It may be due to it being an ACharacter and not a simple UObject..
	
	TabManagerRef->RegisterTabSpawner(DetailsTab, FOnSpawnTab::CreateLambda([=](const FSpawnTabArgs&)
	{
		return SNew(SDockTab)
			.TabRole(PanelTab)
			[
				DetailsView
			];
	}))
	.SetDisplayName(INVTEXT("Details"))
	.SetGroup(WorkspaceMenuCategory.ToSharedRef());

	// SIDEKICK Character Menu
	TabManagerRef->RegisterTabSpawner(SKMenuTab, FOnSpawnTab::CreateLambda([=, this](const FSpawnTabArgs&)
	{
		TSharedRef<SSKMainMenuWidget> NewMenuWidget = SNew(SSKMainMenuWidget)
			.EditingObject(SidekickCharacterPreset.Get());

		SKMenuWidget = NewMenuWidget;
		
		return SNew(SDockTab)
		.TabRole(PanelTab)
		[
			NewMenuWidget
		];
	}))
	.SetDisplayName(INVTEXT("Sidekicks Tool"))
	.SetGroup(WorkspaceMenuCategory.ToSharedRef());
	
}

void FSKCharacterToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& TabManagerRef)
{
	SidekickCharacterPreset = nullptr;

	TabManagerRef->UnregisterAllTabSpawners();

	FAssetEditorToolkit::UnregisterTabSpawners(TabManagerRef);
}

void FSKCharacterToolkit::RegisterToolbarExtension()
{
	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

	ToolbarExtender->AddToolBarExtension(
		"Asset",                               // Extension hook name (anchor point)
		EExtensionHook::After,                 // Position relative to anchor
		GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateSP(this, &FSKCharacterToolkit::ExtendToolbar)
	);

	AddToolbarExtender(ToolbarExtender);
}

void FSKCharacterToolkit::ExtendToolbar(FToolBarBuilder& ToolbarBuilder)
{
	ToolbarBuilder.BeginSection("MyCustomSection");
	{
		// ToolbarBuilder.SetStyle("SKCharacterEditorStyle");
		ToolbarBuilder.AddToolBarButton(FSKCharacterEditorCommands::Get().ImportSidekickPakAction);
	}
	ToolbarBuilder.EndSection();
}

/*
* Triggers the import Sidekick pak logic
* Action triggered from toolbar button.
*/
void FSKCharacterToolkit::ImportSidekickPak()
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
					FOnImportCompletedDelegate::CreateSP(this, &FSKCharacterToolkit::RefreshMenu));
			}),

		// --- Cancel callback (optional) ---
		FSimpleDelegate::CreateLambda([this]()
		{
			UE_LOG(LogTemp, Log, TEXT("Folder picker cancelled."));
		})
	);
}

void FSKCharacterToolkit::RefreshMenu()
{
	// Your refresh logic here
	UE_LOG(LogTemp, Log, TEXT("[SKMenu] Import complete — refreshing menu."));

	// TSharedPtr<FTabManager> TabManager = GetTabManager();
	if (TabManager.IsValid())
	{
		TSharedPtr<SDockTab> Tab = TabManager->FindExistingLiveTab(SKMenuTab);
		if (Tab.IsValid())
		{
			Tab->DrawAttention();
			
			TSharedPtr<SSKMainMenuWidget> MenuWidget = SKMenuWidget.Pin();
			if (MenuWidget.IsValid())
			{
				MenuWidget->UIModel->Initialize();
				MenuWidget->UIModel->PreflightChecksCharacterPreset();
				MenuWidget->UIModel->UpdateAvailableColourSwatches();
				MenuWidget->RefreshPresetSpecie();
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE