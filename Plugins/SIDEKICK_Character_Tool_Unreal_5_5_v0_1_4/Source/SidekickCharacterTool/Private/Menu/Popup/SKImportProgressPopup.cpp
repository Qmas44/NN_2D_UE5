// Copyright YourStudio. All Rights Reserved.

#include "Menu/Popup/SKImportProgressPopup.h"

#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"

#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "SSKImportProgressPopup"

constexpr TCHAR FCopyFileTask::ContentIdent[];

// -----------------------------------------------------------------------
// FCopyFileTask
// -----------------------------------------------------------------------

FCopyFileTask::FCopyFileTask(const TArray<FString>& InFilesToCopy, const FString& InProjectContentDir)
    : FilesToCopy(InFilesToCopy)
    , ProjectContentDir(InProjectContentDir)
    , TotalFiles(InFilesToCopy.Num())
{}

FCopyFileTask::~FCopyFileTask() {}

bool FCopyFileTask::Init()
{
    return true;
}

uint32 FCopyFileTask::Run()
{
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

    for (int32 i = 0; i < FilesToCopy.Num(); ++i)
    {
        if (bStopRequested.load()) break;

        const FString& EachFile = FilesToCopy[i];

        // Update the current file name for the UI to read
        {
            FScopeLock Lock(&CurrentFileCS);
            CurrentFileName = FPaths::GetCleanFilename(EachFile);
        }

        // Find "Content/" in the source path and derive the destination
        int32 ContentIndex = INDEX_NONE;
        ContentIndex = EachFile.Find(ContentIdent, ESearchCase::IgnoreCase, ESearchDir::FromStart);


        if (ContentIndex == INDEX_NONE)
        {
            // Path doesn't contain Content/ — skip it
            SkippedCount.fetch_add(1);
            continue;
        }

        // Chop to get the relative path from Content/ onwards
        const FString ContentRelative  = EachFile.RightChop(ContentIndex);       // e.g. Content/Characters/Hero.uasset
        const FString DestinationPath  = FPaths::Combine(ProjectContentDir, ContentRelative.RightChop(FCString::Strlen(ContentIdent)));

        // Ensure the destination directory exists
        const FString DestDir = FPaths::GetPath(DestinationPath);
        if (!FPaths::DirectoryExists(DestDir))
        {
            const bool bDirCreated = PlatformFile.CreateDirectoryTree(*DestDir);
            checkf(bDirCreated, TEXT("SKImportProgressPopup: Failed to create directory: %s"), *DestDir);
            if (!bDirCreated)
            {
                FilesFailed.Add(EachFile);
                FailedCount.fetch_add(1);
                continue;
            }
        }

        // Skip if file already exists at destination
        if (FPaths::FileExists(DestinationPath))
        {
            FilesSkipped.Add(DestinationPath);
            SkippedCount.fetch_add(1);
            continue;
        }

        // Perform the copy
        const ECopyResult Result = static_cast<ECopyResult>(IFileManager::Get().Copy(*DestinationPath, *EachFile, true, true));
        if (Result == COPY_OK)
        {
            FilesCopied.Add(DestinationPath);
            CopiedCount.fetch_add(1);
        }
        else
        {
            FilesFailed.Add(EachFile);
            FailedCount.fetch_add(1);
        }
    }

    bComplete.store(true);
    return 0;
}

void FCopyFileTask::Stop()
{
    bStopRequested.store(true);
}

FString FCopyFileTask::GetCurrentFile() const
{
    FScopeLock Lock(&CurrentFileCS);
    return CurrentFileName;
}

// -----------------------------------------------------------------------
// SSKImportProgressPopup — static factory
// -----------------------------------------------------------------------

TSharedRef<SWindow> SSKImportProgressPopup::OpenInWindow(const TArray<FString>& FilesToCopy, FOnImportCompletedDelegate OnImportComplete)
{
    TSharedRef<SSKImportProgressPopup> Widget =
        SNew(SSKImportProgressPopup)
        .FilesToCopy(FilesToCopy)
        .OnImportComplete(OnImportComplete);

    TSharedRef<SWindow> Window = SNew(SWindow)
        .Title(LOCTEXT("WindowTitle", "Importing Assets..."))
        .ClientSize(FVector2D(560.f, 400.f))
        .MinWidth(400.f)
        .MinHeight(300.f)
        .SizingRule(ESizingRule::UserSized)
        .SupportsMaximize(true)
        .SupportsMinimize(false)
        .IsTopmostWindow(true)
        [
            Widget
        ];

    Widget->OwningWindow = Window;

    FSlateApplication::Get().AddWindow(Window);
    return Window;
}

// -----------------------------------------------------------------------
// Construct
// -----------------------------------------------------------------------

void SSKImportProgressPopup::Construct(const FArguments& InArgs)
{
    OnImportComplete = InArgs._OnImportComplete;
    
    // Build the log list view
    SAssignNew(LogListView, SListView<TSharedPtr<FString>>)
        .ListItemsSource(&LogLines)
        .OnGenerateRow_Lambda([](TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& Owner)
        {
            // Colour-code lines by prefix
            FLinearColor LineColor = FLinearColor::White;
            if (Item->StartsWith(TEXT("[FAILED]")))       LineColor = FLinearColor(1.f, 0.3f, 0.3f);
            else if (Item->StartsWith(TEXT("[SKIPPED]"))) LineColor = FLinearColor(0.8f, 0.6f, 0.0f);
            else if (Item->StartsWith(TEXT("[COPIED]")))  LineColor = FLinearColor(0.4f, 1.f, 0.4f);

            return SNew(STableRow<TSharedPtr<FString>>, Owner)
            [
                SNew(STextBlock)
                .Text(FText::FromString(*Item))
                .Font(FCoreStyle::GetDefaultFontStyle("Mono", 8))
                .ColorAndOpacity(FSlateColor(LineColor))
            ];
        });

    SAssignNew(CancelButton, SButton)
        .Text(LOCTEXT("CancelButton", "Cancel"))
        .OnClicked(this, &SSKImportProgressPopup::OnCancelClicked);

    SAssignNew(CloseButton, SButton)
        .Text(LOCTEXT("CloseButton", "Close"))
        .IsEnabled(false)       // enabled only once copy is done
        .OnClicked(this, &SSKImportProgressPopup::OnCloseClicked);

    ChildSlot
    [
        SNew(SVerticalBox)

        // ---- Current file label ----
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(12.f, 10.f, 12.f, 2.f)
        [
            SNew(STextBlock)
            .Text(this, &SSKImportProgressPopup::GetCurrentFileText)
            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
            .ColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.8f, 0.8f)))
        ]

        // ---- Overall progress bar ----
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(12.f, 4.f)
        [
            SNew(SProgressBar)
            .Percent(this, &SSKImportProgressPopup::GetProgressFraction)
            .FillColorAndOpacity(this, &SSKImportProgressPopup::GetProgressBarColor)
        ]

        // ---- Progress counts label ----
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(12.f, 2.f, 12.f, 6.f)
        [
            SNew(STextBlock)
            .Text(this, &SSKImportProgressPopup::GetProgressText)
            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
            .ColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)))
        ]

        + SVerticalBox::Slot().AutoHeight().Padding(12.f, 0.f)
        [ SNew(SSeparator) ]

        // ---- Scrollable log ----
        + SVerticalBox::Slot()
        .FillHeight(1.f)
        .Padding(12.f, 6.f)
        [
            SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
            .Padding(4.f)
            [
                LogListView.ToSharedRef()
            ]
        ]

        + SVerticalBox::Slot().AutoHeight().Padding(12.f, 0.f)
        [ SNew(SSeparator) ]

        // ---- Status line + buttons ----
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(12.f, 8.f)
        [
            SNew(SHorizontalBox)

            // Status text (left-aligned)
            + SHorizontalBox::Slot()
            .FillWidth(1.f)
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text(this, &SSKImportProgressPopup::GetStatusText)
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
            ]

            // Cancel / Close (right-aligned)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(0.f, 0.f, 8.f, 0.f)
            [
                CancelButton.ToSharedRef()
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                CloseButton.ToSharedRef()
            ]
        ]
    ];

    // Kick off the background copy task
    StartCopy(InArgs._FilesToCopy);
}

SSKImportProgressPopup::~SSKImportProgressPopup()
{
    // If the window is closed before the task completes, stop the thread cleanly
    if (CopyTask.IsValid())
    {
        CopyTask->Stop();
    }
    if (CopyThread)
    {
        CopyThread->WaitForCompletion();
        delete CopyThread;
        CopyThread = nullptr;
    }
}

// -----------------------------------------------------------------------
// Start background task
// -----------------------------------------------------------------------

void SSKImportProgressPopup::StartCopy(const TArray<FString>& FilesToCopy)
{
    CopyTask = MakeShareable(new FCopyFileTask(FilesToCopy, FPaths::ProjectContentDir()));

    CopyThread = FRunnableThread::Create(
        CopyTask.Get(),
        TEXT("SKMCompare_FileCopyThread"),
        0,                              // default stack size
        TPri_BelowNormal               // below normal so editor stays responsive
    );

    AppendLog(FString::Printf(TEXT("Starting import of %d file(s)..."), FilesToCopy.Num()));
}

// -----------------------------------------------------------------------
// Tick — polls the background task, flushes new log entries
// -----------------------------------------------------------------------

void SSKImportProgressPopup::Tick(
    const FGeometry& AllottedGeometry,
    const double     InCurrentTime,
    const float      InDeltaTime)
{
    SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

    if (!CopyTask.IsValid() || bHasCompleted) return;

    const int32 CurrentCopied  = CopyTask->GetCopiedCount();
    const int32 CurrentSkipped = CopyTask->GetSkippedCount();
    const int32 CurrentFailed  = CopyTask->GetFailedCount();
    const int32 TotalProcessed = CurrentCopied + CurrentSkipped + CurrentFailed;

    // Flush any new log entries since last tick
    // We derive log lines from the running totals rather than queuing
    // strings across threads, keeping thread interaction minimal.
    if (TotalProcessed > LastLoggedCount)
    {
        // The task exposes final file lists only after completion,
        // so mid-run we just log the current filename + result code
        const FString CurrentFile = CopyTask->GetCurrentFile();
        if (!CurrentFile.IsEmpty())
        {
            // Log the most recently completed file
            const int32 NewEntries = TotalProcessed - LastLoggedCount;
            for (int32 i = 0; i < NewEntries; ++i)
            {
                AppendLog(FString::Printf(TEXT("[COPIED]  %s"), *CurrentFile));
            }
        }
        LastLoggedCount = TotalProcessed;
    }

    // Check for completion
    if (CopyTask->IsComplete() && !bHasCompleted)
    {
        OnCopyComplete();
    }
}

// -----------------------------------------------------------------------
// On copy complete
// -----------------------------------------------------------------------

void SSKImportProgressPopup::OnCopyComplete()
{
    bHasCompleted = true;

    LogLines.Empty();

    // Copied
    for (const FString& File : CopyTask->GetCopiedFiles())
    {
        AppendLog(FString::Printf(TEXT("[COPIED]  %s"), *FPaths::GetCleanFilename(File)));
    }

    // Skipped
    for (const FString& File : CopyTask->GetSkippedFiles())
    {
        AppendLog(FString::Printf(TEXT("[SKIPPED] %s"), *FPaths::GetCleanFilename(File)));
    }

    // Failed
    for (const FString& File : CopyTask->GetFailedFiles())
    {
        AppendLog(FString::Printf(TEXT("[FAILED]  %s"), *File));
    }

    AppendLog(TEXT(""));
    AppendLog(FString::Printf(TEXT("Done — %d copied, %d skipped, %d failed."),
        CopyTask->GetCopiedCount(),
        CopyTask->GetSkippedCount(),
        CopyTask->GetFailedCount()));

    if (CancelButton.IsValid()) CancelButton->SetEnabled(false);
    if (CloseButton.IsValid())  CloseButton->SetEnabled(true);

    if (OwningWindow.IsValid())
    {
        OwningWindow.Pin()->SetTitle(LOCTEXT("WindowTitleDone", "Import Complete"));
        OnImportComplete.ExecuteIfBound();
    }

    if (LogListView.IsValid() && LogLines.Num() > 0)
    {
        LogListView->RequestScrollIntoView(LogLines.Last());
    }
}

// -----------------------------------------------------------------------
// Widget attribute callbacks
// -----------------------------------------------------------------------

TOptional<float> SSKImportProgressPopup::GetProgressFraction() const
{
    if (!CopyTask.IsValid() || CopyTask->GetTotalFiles() == 0)
        return 0.f;

    const float Processed = static_cast<float>(
        CopyTask->GetCopiedCount() +
        CopyTask->GetSkippedCount() +
        CopyTask->GetFailedCount());

    return Processed / static_cast<float>(CopyTask->GetTotalFiles());
}

FText SSKImportProgressPopup::GetProgressText() const
{
    if (!CopyTask.IsValid()) return FText::GetEmpty();

    return FText::Format(
        LOCTEXT("ProgressFmt", "{0} copied   {1} skipped   {2} failed   of {3} total"),
        CopyTask->GetCopiedCount(),
        CopyTask->GetSkippedCount(),
        CopyTask->GetFailedCount(),
        CopyTask->GetTotalFiles());
}

FText SSKImportProgressPopup::GetCurrentFileText() const
{
    if (!CopyTask.IsValid() || bHasCompleted) return FText::GetEmpty();

    const FString File = CopyTask->GetCurrentFile();
    return File.IsEmpty()
        ? LOCTEXT("Preparing", "Preparing...")
        : FText::Format(LOCTEXT("CopyingFmt", "Copying: {0}"), FText::FromString(File));
}

FText SSKImportProgressPopup::GetStatusText() const
{
    if (!CopyTask.IsValid()) return FText::GetEmpty();

    if (bHasCompleted)
    {
        const int32 Failed = CopyTask->GetFailedCount();
        return Failed > 0
            ? FText::Format(LOCTEXT("DoneWithErrors", "Done — {0} error(s)"), Failed)
            : LOCTEXT("DoneClean", "Done — all files imported successfully.");
    }
    return LOCTEXT("InProgress", "Importing...");
}

FSlateColor SSKImportProgressPopup::GetProgressBarColor() const
{
    if (bHasCompleted && CopyTask.IsValid() && CopyTask->GetFailedCount() > 0)
        return FSlateColor(FLinearColor(1.f, 0.3f, 0.3f));   // red if any failures

    if (bHasCompleted)
        return FSlateColor(FLinearColor(0.2f, 0.8f, 0.2f));  // green when done

    return FSlateColor(FLinearColor(0.2f, 0.5f, 1.f));        // blue while in progress
}

// -----------------------------------------------------------------------
// Buttons
// -----------------------------------------------------------------------

FReply SSKImportProgressPopup::OnCancelClicked()
{
    if (CopyTask.IsValid()) CopyTask->Stop();
    AppendLog(TEXT("Cancelled by user."));

    if (CancelButton.IsValid()) CancelButton->SetEnabled(false);
    if (CloseButton.IsValid())  CloseButton->SetEnabled(true);

    return FReply::Handled();
}

FReply SSKImportProgressPopup::OnCloseClicked()
{
    if (OwningWindow.IsValid())
    {
        OwningWindow.Pin()->RequestDestroyWindow();
    }
    return FReply::Handled();
}

// -----------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------

void SSKImportProgressPopup::AppendLog(const FString& Line)
{
    LogLines.Add(MakeShareable(new FString(Line)));
    if (LogListView.IsValid())
    {
        LogListView->RequestListRefresh();
        LogListView->RequestScrollIntoView(LogLines.Last());
    }
}

#undef LOCTEXT_NAMESPACE