// Copyright YourStudio. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"

// -----------------------------------------------------------------------
// FDriveTreeItem
// Represents a single node in the drive/folder tree.
// -----------------------------------------------------------------------
struct FDriveTreeItem
{
    FString         FullPath;           // Absolute path e.g. C:\Users\Foo
    FString         DisplayName;        // Just the folder/drive name
    bool            bIsExpanded;
    bool            bChildrenPopulated;
    bool            bIsSelected;

    TArray<TSharedPtr<FDriveTreeItem>> Children;
    FDriveTreeItem* Parent = nullptr;

    FDriveTreeItem(const FString& InFullPath, const FString& InDisplayName)
        : FullPath(InFullPath)
        , DisplayName(InDisplayName)
        , bIsExpanded(false)
        , bChildrenPopulated(false)
        , bIsSelected(false)
    {}

    // Populate direct child directories on demand
    void PopulateChildren();
};

// -----------------------------------------------------------------------
// SSKFolderPickerPopup
// A standalone Slate window that lets the user browse drives/folders,
// select multiple folders, and confirm or cancel.
// -----------------------------------------------------------------------
class SIDEKICKCHARACTERTOOL_API SSKFolderPickerPopup : public SCompoundWidget
{
public:
    DECLARE_DELEGATE_OneParam(FOnFoldersAccepted, const TArray<FString>& /* SelectedPaths */);

    SLATE_BEGIN_ARGS(SSKFolderPickerPopup) {}
        SLATE_EVENT(FOnFoldersAccepted, OnFoldersAccepted)
        SLATE_EVENT(FSimpleDelegate,    OnCancelled)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    // Opens this widget inside a new SWindow and returns the window
    static TSharedRef<SWindow> OpenInWindow(
        FOnFoldersAccepted InOnAccepted,
        FSimpleDelegate    InOnCancelled = FSimpleDelegate());

private:
    // --- Tree data ---
    TArray<TSharedPtr<FDriveTreeItem>> RootItems;   // One entry per drive / mount point

    void BuildRootItems();

    // --- STreeView callbacks ---
    TSharedRef<ITableRow> OnGenerateRow(
        TSharedPtr<FDriveTreeItem>           Item,
        const TSharedRef<STableViewBase>&    OwnerTable);

    void OnGetChildren(
        TSharedPtr<FDriveTreeItem>           Item,
        TArray<TSharedPtr<FDriveTreeItem>>&  OutChildren);

    void OnExpansionChanged(
        TSharedPtr<FDriveTreeItem> Item,
        bool bExpanded);

    void OnSelectionChanged(
        TSharedPtr<FDriveTreeItem> Item,
        ESelectInfo::Type          SelectInfo);

    // --- Button callbacks ---
    FReply OnAcceptClicked();
    FReply OnCancelClicked();

    // --- Selected path chips (summary bar) ---
    TSharedRef<SWidget> BuildSelectedPathsBar();
    void RebuildSelectedPathsBar();

    // --- Helpers ---
    // Recursively find an item that owns the given path (for checkbox sync)
    TSharedPtr<FDriveTreeItem> FindItemByPath(
        const TArray<TSharedPtr<FDriveTreeItem>>& Items,
        const FString& Path) const;

    // All items the user has checked (multi-select via checkboxes)
    TArray<TSharedPtr<FDriveTreeItem>> CheckedItems;

    TSharedPtr<STreeView<TSharedPtr<FDriveTreeItem>>> TreeView;
    TSharedPtr<SVerticalBox>                          SelectedPathsBox;

    FOnFoldersAccepted  OnFoldersAccepted;
    FSimpleDelegate     OnCancelled;

    TWeakPtr<SWindow>   OwningWindow;

    // Returns true if this exact directory contains a .uproject file
    bool DirectoryContainsUProject(const FString& DirectoryPath) const;

    // Returns true if any ancestor of this directory already contains a .uproject
    // (meaning this folder is nested inside a project and should be hidden)
    bool IsInsideUProjectFolder(const FString& DirectoryPath) const;
};