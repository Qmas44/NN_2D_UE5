// Copyright YourStudio. All Rights Reserved.

#include "Menu/Popup/SKFolderPickerPopup.h"

#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"

#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STreeView.h"
#include "Styling/AppStyle.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif

#define LOCTEXT_NAMESPACE "SSKFolderPickerPopup"

// -----------------------------------------------------------------------
// FDriveTreeItem
// -----------------------------------------------------------------------

void FDriveTreeItem::PopulateChildren()
{
    if (bChildrenPopulated) return;
    bChildrenPopulated = true;
    Children.Empty();

    IFileManager& FM = IFileManager::Get();
    TArray<FString> FoundDirs;
    FM.FindFiles(FoundDirs, *(FullPath / TEXT("*")), false, true);

    for (const FString& DirName : FoundDirs)
    {
        if (DirName.StartsWith(TEXT("."))) continue;

        const FString ChildFullPath = FullPath / DirName;

        // --- Filter rule 1: skip folders nested inside a .uproject folder ---
        // If the current folder (FullPath) already contains a .uproject,
        // none of its children should be shown.
        {
            TArray<FString> UProjectFiles;
            FM.FindFiles(UProjectFiles, *(FullPath / TEXT("*.uproject")), true, false);
            if (UProjectFiles.Num() > 0) continue;
        }

        // --- Filter rule 2: only show folders that either contain a .uproject
        // themselves, or contain subdirectories that eventually lead to one ---
        // We do a shallow check here; deeper filtering happens on expansion.
        // To avoid deep recursion at build time we only filter at the
        // immediate child level — folders with no uproject anywhere nearby
        // are still shown so the user can navigate into them.
        // The checkbox is what controls what gets returned, not visibility.

        TSharedPtr<FDriveTreeItem> Child = MakeShareable(new FDriveTreeItem(ChildFullPath, DirName));
        Child->Parent = this;

        // Check if this child itself is a uproject root — if so it's a leaf
        // (no point expanding its children since they're inside a project)
        TArray<FString> ChildUProjects;
        FM.FindFiles(ChildUProjects, *(ChildFullPath / TEXT("*.uproject")), true, false);

        if (ChildUProjects.Num() > 0)
        {
            // This IS a project folder — show it but give it no children
            // so the user cannot navigate inside it
        }
        else
        {
            // Not a project folder — allow expansion to keep navigating
            TArray<FString> GrandChildren;
            FM.FindFiles(GrandChildren, *(ChildFullPath / TEXT("*")), false, true);
            if (GrandChildren.Num() > 0)
            {
                Child->Children.Add(MakeShareable(new FDriveTreeItem(TEXT("__dummy__"), TEXT(""))));
            }
        }

        Children.Add(Child);
    }
}

// -----------------------------------------------------------------------
// SSKFolderPickerPopup  –  Static factory
// -----------------------------------------------------------------------

TSharedRef<SWindow> SSKFolderPickerPopup::OpenInWindow(
    FOnFoldersAccepted InOnAccepted,
    FSimpleDelegate    InOnCancelled)
{
    TSharedRef<SSKFolderPickerPopup> PickerWidget =
        SNew(SSKFolderPickerPopup)
        .OnFoldersAccepted(InOnAccepted)
        .OnCancelled(InOnCancelled);

    TSharedRef<SWindow> Window = SNew(SWindow)
        .Title(LOCTEXT("WindowTitle", "Select Folders"))
        .ClientSize(FVector2D(600.f, 520.f))
        .SupportsMaximize(true)
        .SupportsMinimize(false)
        .SizingRule(ESizingRule::UserSized)
        [
            PickerWidget
        ];

    PickerWidget->OwningWindow = Window;

    FSlateApplication::Get().AddWindow(Window);
    return Window;
}

// -----------------------------------------------------------------------
// Construct
// -----------------------------------------------------------------------

void SSKFolderPickerPopup::Construct(const FArguments& InArgs)
{
    OnFoldersAccepted = InArgs._OnFoldersAccepted;
    OnCancelled       = InArgs._OnCancelled;

    BuildRootItems();

    // Pre-build the selected paths box (starts empty)
    SAssignNew(SelectedPathsBox, SVerticalBox);

    // Build the tree view
    SAssignNew(TreeView, STreeView<TSharedPtr<FDriveTreeItem>>)
        .TreeItemsSource(&RootItems)
        .OnGenerateRow(this, &SSKFolderPickerPopup::OnGenerateRow)
        .OnGetChildren(this, &SSKFolderPickerPopup::OnGetChildren)
        .OnExpansionChanged(this, &SSKFolderPickerPopup::OnExpansionChanged)
        .OnSelectionChanged(this, &SSKFolderPickerPopup::OnSelectionChanged)
        .SelectionMode(ESelectionMode::None) // Selection driven by checkboxes
        .HeaderRow(
            SNew(SHeaderRow)
            + SHeaderRow::Column("FolderName")
            .DefaultLabel(LOCTEXT("FolderColumnHeader", "Folder"))
            .FillWidth(1.f)
        );

    ChildSlot
[
    SNew(SVerticalBox)

    // ---- Instructions ----
    + SVerticalBox::Slot()
    .AutoHeight()
    .Padding(12.f, 10.f, 12.f, 4.f)
    [
        SNew(STextBlock)
        .Text(LOCTEXT("Instructions", "Check folders to include, then press Accept."))
        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
        .ColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)))
    ]

    + SVerticalBox::Slot().AutoHeight().Padding(12.f, 0.f)
    [ SNew(SSeparator) ]

    // ---- Resizable splitter between tree and selected paths ----
    + SVerticalBox::Slot()
    .FillHeight(1.f)
    .Padding(12.f, 6.f)
    [
        SNew(SSplitter)
        .Orientation(Orient_Vertical)
        .ResizeMode(ESplitterResizeMode::Fill)

        // Top slot: folder tree
        + SSplitter::Slot()
        .Value(0.7f)                           // tree takes 70% by default
        [
            SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
            .Padding(2.f)
            [
                TreeView.ToSharedRef()
            ]
        ]

        // Bottom slot: selected paths summary
        + SSplitter::Slot()
        .Value(0.3f)                           // selected paths takes 30%
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 2.f)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("SelectedLabel", "Selected Folders:"))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
            ]
            + SVerticalBox::Slot().FillHeight(1.f)
            [
                SNew(SScrollBox)
                + SScrollBox::Slot()
                [
                    SelectedPathsBox.ToSharedRef()
                ]
            ]
        ]
    ]

    + SVerticalBox::Slot().AutoHeight().Padding(12.f, 0.f)
    [ SNew(SSeparator) ]

    // ---- Accept / Cancel ---- (unchanged)
    + SVerticalBox::Slot()
    .AutoHeight()
    .HAlign(HAlign_Right)
    .Padding(12.f, 8.f)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 8.f, 0.f)
        [
            SNew(SButton)
            .Text(LOCTEXT("AcceptButton", "Accept"))
            .ButtonColorAndOpacity(FLinearColor(0.1f, 0.4f, 0.1f, 1.f))
            .OnClicked(this, &SSKFolderPickerPopup::OnAcceptClicked)
        ]
        + SHorizontalBox::Slot().AutoWidth()
        [
            SNew(SButton)
            .Text(LOCTEXT("CancelButton", "Cancel"))
            .OnClicked(this, &SSKFolderPickerPopup::OnCancelClicked)
        ]
    ]
];
}

// -----------------------------------------------------------------------
// Build root items (drives on Windows, mount points on Mac/Linux)
// -----------------------------------------------------------------------

void SSKFolderPickerPopup::BuildRootItems()
{
    RootItems.Empty();

#if PLATFORM_WINDOWS
    #include "Windows/WindowsHWrapper.h"

    // Enumerate drive letters A-Z via Win32
    const DWORD DriveMask = ::GetLogicalDrives();
    for (int32 i = 0; i < 26; ++i)
    {
        if (DriveMask & (1 << i))
        {
            const FString DriveLetter = FString::Printf(TEXT("%c:\\"), TEXT('A') + i);
            TSharedPtr<FDriveTreeItem> DriveItem =
                MakeShareable(new FDriveTreeItem(DriveLetter, DriveLetter));
            DriveItem->Children.Add(
                MakeShareable(new FDriveTreeItem(TEXT("__dummy__"), TEXT(""))));
            RootItems.Add(DriveItem);
        }
    }

#elif PLATFORM_MAC
    // On Mac, /Volumes contains all mounted drives and external disks
    TArray<FString> VolumeDirs;
    IFileManager::Get().FindFiles(VolumeDirs, TEXT("/Volumes/*"), false, true);

    for (const FString& VolumeName : VolumeDirs)
    {
        const FString VolumePath = FString::Printf(TEXT("/Volumes/%s"), *VolumeName);
        TSharedPtr<FDriveTreeItem> VolumeItem =
            MakeShareable(new FDriveTreeItem(VolumePath, VolumeName));
        VolumeItem->Children.Add(
            MakeShareable(new FDriveTreeItem(TEXT("__dummy__"), TEXT(""))));
        RootItems.Add(VolumeItem);
    }

    // Always add home directory as a top-level convenience shortcut
    const FString HomeDir = FPlatformMisc::GetEnvironmentVariable(TEXT("HOME"));
    if (!HomeDir.IsEmpty())
    {
        const FString HomeName = FString::Printf(TEXT("Home (%s)"), *HomeDir);
        TSharedPtr<FDriveTreeItem> HomeItem =
            MakeShareable(new FDriveTreeItem(HomeDir, HomeName));
        HomeItem->Children.Add(
            MakeShareable(new FDriveTreeItem(TEXT("__dummy__"), TEXT(""))));
        RootItems.Add(HomeItem);
    }

#elif PLATFORM_LINUX
    // On Linux there are no drive letters — expose a curated set of
    // useful root-level mount points rather than dumping all of /
    TArray<FString> LinuxRoots = { TEXT("/"), TEXT("/mnt"), TEXT("/media"), TEXT("/home") };

    for (const FString& RootPath : LinuxRoots)
    {
        // Only add entries that actually exist on this machine
        if (!IFileManager::Get().DirectoryExists(*RootPath)) continue;

        TSharedPtr<FDriveTreeItem> RootItem =
            MakeShareable(new FDriveTreeItem(RootPath, RootPath));
        RootItem->Children.Add(
            MakeShareable(new FDriveTreeItem(TEXT("__dummy__"), TEXT(""))));
        RootItems.Add(RootItem);
    }

    // Add the current user's home directory as a shortcut
    const FString HomeDir = FPlatformMisc::GetEnvironmentVariable(TEXT("HOME"));
    if (!HomeDir.IsEmpty() && IFileManager::Get().DirectoryExists(*HomeDir))
    {
        const FString HomeName = FString::Printf(TEXT("Home (%s)"), *HomeDir);
        TSharedPtr<FDriveTreeItem> HomeItem =
            MakeShareable(new FDriveTreeItem(HomeDir, HomeName));
        HomeItem->Children.Add(
            MakeShareable(new FDriveTreeItem(TEXT("__dummy__"), TEXT(""))));
        RootItems.Add(HomeItem);
    }
#endif
}

// -----------------------------------------------------------------------
// STreeView callbacks
// -----------------------------------------------------------------------

TSharedRef<ITableRow> SSKFolderPickerPopup::OnGenerateRow(
    TSharedPtr<FDriveTreeItem>        Item,
    const TSharedRef<STableViewBase>& OwnerTable)
{
    // Determine if this folder contains a .uproject
    TArray<FString> UProjectFiles;
    IFileManager::Get().FindFiles(
        UProjectFiles,
        *(Item->FullPath / TEXT("*.uproject")),
        true,   // files
        false); // not directories

    const bool bHasUProject = UProjectFiles.Num() > 0;

    // White for project folders, grey for navigation-only folders
    const FLinearColor FolderColour = bHasUProject
        ? FLinearColor::White
        : FLinearColor(0.4f, 0.4f, 0.4f, 1.f);
    
    // Build the left-side widget — checkbox if project, label if not
    TSharedRef<SWidget> LeftWidget = bHasUProject
        // --- Has .uproject: show a selectable checkbox ---
        ? TSharedRef<SWidget>(
            SNew(SCheckBox)
            .IsChecked_Lambda([this, Item]()
            {
                return CheckedItems.Contains(Item)
                    ? ECheckBoxState::Checked
                    : ECheckBoxState::Unchecked;
            })
            .OnCheckStateChanged_Lambda([this, Item](ECheckBoxState NewState)
            {
                if (NewState == ECheckBoxState::Checked)
                    CheckedItems.AddUnique(Item);
                else
                    CheckedItems.Remove(Item);

                RebuildSelectedPathsBar();
            }))
        // --- No .uproject: show a non-interactive navigation label ---
        : TSharedRef<SWidget>(
            SNew(STextBlock)
            .Text(LOCTEXT("NavigationOnly", "-"))
            .Font(FCoreStyle::GetDefaultFontStyle("Italic", 7))
            .ColorAndOpacity(FSlateColor(FLinearColor(0.3f, 0.3f, 0.3f))));
    
    return SNew(STableRow<TSharedPtr<FDriveTreeItem>>, OwnerTable)
    .Padding(FMargin(2.f, 1.f))
    [
        SNew(SHorizontalBox)

        // Left widget — checkbox or nav label depending on bHasUProject
        + SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        .Padding(0.f, 0.f, 6.f, 0.f)
        [
            LeftWidget
        ]

        // Folder icon — tinted by project status
        + SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        .Padding(0.f, 0.f, 4.f, 0.f)
        [
            SNew(SImage)
            .Image(FAppStyle::GetBrush("ContentBrowser.AssetTreeFolderOpen"))
            .DesiredSizeOverride(FVector2D(14.f, 14.f))
            .ColorAndOpacity(FolderColour)
        ]

        // Display name — tinted by project status
        + SHorizontalBox::Slot()
        .FillWidth(1.f)
        .VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text(FText::FromString(Item->DisplayName))
            .ColorAndOpacity(FSlateColor(FolderColour))
        ]
    ];
}

void SSKFolderPickerPopup::OnGetChildren(
    TSharedPtr<FDriveTreeItem>          Item,
    TArray<TSharedPtr<FDriveTreeItem>>& OutChildren)
{
    // If the only child is our dummy sentinel, replace it with real children
    if (Item->Children.Num() == 1 && Item->Children[0]->FullPath == TEXT("__dummy__"))
    {
        Item->Children.Empty();
        Item->bChildrenPopulated = false;
    }

    Item->PopulateChildren();
    OutChildren = Item->Children;
}

void SSKFolderPickerPopup::OnExpansionChanged(
    TSharedPtr<FDriveTreeItem> Item,
    bool bExpanded)
{
    Item->bIsExpanded = bExpanded;

    // Trigger child population when expanding for the first time
    if (bExpanded && !Item->bChildrenPopulated)
    {
        Item->PopulateChildren();
        TreeView->RequestTreeRefresh();
    }
}

void SSKFolderPickerPopup::OnSelectionChanged(
    TSharedPtr<FDriveTreeItem> Item,
    ESelectInfo::Type          SelectInfo)
{
    // Selection is handled entirely via checkboxes; nothing needed here
}

// -----------------------------------------------------------------------
// Selected paths summary bar
// -----------------------------------------------------------------------

void SSKFolderPickerPopup::RebuildSelectedPathsBar()
{
    if (!SelectedPathsBox.IsValid()) return;

    SelectedPathsBox->ClearChildren();

    if (CheckedItems.IsEmpty())
    {
        SelectedPathsBox->AddSlot().AutoHeight()
        [
            SNew(STextBlock)
            .Text(LOCTEXT("NoneSelected", "  (none)"))
            .Font(FCoreStyle::GetDefaultFontStyle("Italic", 8))
            .ColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)))
        ];
        return;
    }

    for (const TSharedPtr<FDriveTreeItem>& Item : CheckedItems)
    {
        SelectedPathsBox->AddSlot()
        .AutoHeight()
        .Padding(0.f, 1.f)
        [
            SNew(SHorizontalBox)

            // Small remove button
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0.f, 0.f, 4.f, 0.f)
            [
                SNew(SButton)
                .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                .ContentPadding(FMargin(2.f, 0.f))
                .OnClicked_Lambda([this, Item]() -> FReply
                {
                    CheckedItems.Remove(Item);
                    RebuildSelectedPathsBar();
                    if (TreeView.IsValid()) TreeView->RequestTreeRefresh();
                    return FReply::Handled();
                })
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TEXT("✕")))
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
                    .ColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.3f, 0.3f)))
                ]
            ]

            + SHorizontalBox::Slot()
            .FillWidth(1.f)
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text(FText::FromString(Item->FullPath))
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
            ]
        ];
    }
}

// -----------------------------------------------------------------------
// Accept / Cancel
// -----------------------------------------------------------------------

FReply SSKFolderPickerPopup::OnAcceptClicked()
{
    TArray<FString> SelectedPaths;
    for (const TSharedPtr<FDriveTreeItem>& Item : CheckedItems)
    {
        SelectedPaths.Add(Item->FullPath);
    }

    OnFoldersAccepted.ExecuteIfBound(SelectedPaths);

    if (OwningWindow.IsValid())
    {
        OwningWindow.Pin()->RequestDestroyWindow();
    }

    return FReply::Handled();
}

FReply SSKFolderPickerPopup::OnCancelClicked()
{
    OnCancelled.ExecuteIfBound();

    if (OwningWindow.IsValid())
    {
        OwningWindow.Pin()->RequestDestroyWindow();
    }

    return FReply::Handled();
}

// -----------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------

TSharedPtr<FDriveTreeItem> SSKFolderPickerPopup::FindItemByPath(
    const TArray<TSharedPtr<FDriveTreeItem>>& Items,
    const FString& Path) const
{
    for (const TSharedPtr<FDriveTreeItem>& Item : Items)
    {
        if (Item->FullPath == Path) return Item;

        TSharedPtr<FDriveTreeItem> Found = FindItemByPath(Item->Children, Path);
        if (Found.IsValid()) return Found;
    }
    return nullptr;
}

bool SSKFolderPickerPopup::DirectoryContainsUProject(const FString& DirectoryPath) const
{
    TArray<FString> UProjectFiles;
    IFileManager::Get().FindFiles(UProjectFiles, *(DirectoryPath / TEXT("*.uproject")), true, false);
    return UProjectFiles.Num() > 0;
}

bool SSKFolderPickerPopup::IsInsideUProjectFolder(const FString& DirectoryPath) const
{
    // Walk up the directory tree. If any parent contains a .uproject
    // then this folder is a subdirectory of a project and should be filtered out.
    FString CurrentPath = FPaths::GetPath(DirectoryPath); // step up one level

    while (!CurrentPath.IsEmpty())
    {
        if (DirectoryContainsUProject(CurrentPath))
        {
            return true;
        }

        const FString ParentPath = FPaths::GetPath(CurrentPath);

        // Stop if we've hit the root and can't go further up
        if (ParentPath == CurrentPath) break;

        CurrentPath = ParentPath;
    }
    return false;
}

#undef LOCTEXT_NAMESPACE