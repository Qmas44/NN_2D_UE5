// Copyright YourStudio. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "Misc/SingleThreadRunnable.h"

DECLARE_DELEGATE(FOnImportCompletedDelegate);

// -----------------------------------------------------------------------
// FCopyFileTask
// Runs the file copy loop on a background thread.
// Progress and completion are communicated back via thread-safe atomics
// that the Slate tick can read safely from the game thread.
// -----------------------------------------------------------------------
class FCopyFileTask : public FRunnable
{
public:
    FCopyFileTask(const TArray<FString>& InFilesToCopy, const FString& InProjectContentDir);
    virtual ~FCopyFileTask() override;

    // FRunnable interface
    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;

    // Thread-safe reads — call these from the game thread
    int32  GetTotalFiles()    const { return TotalFiles; }
    int32  GetCopiedCount()   const { return CopiedCount.load(); }
    int32  GetSkippedCount()  const { return SkippedCount.load(); }
    int32  GetFailedCount()   const { return FailedCount.load(); }
    bool   IsComplete()       const { return bComplete.load(); }
    FString GetCurrentFile()  const;        // returns the file currently being copied

    const TArray<FString>& GetCopiedFiles() const { return FilesCopied; }
    const TArray<FString>& GetFailedFiles() const { return FilesFailed; }
    const TArray<FString>& GetSkippedFiles() const { return FilesSkipped; }

private:
    TArray<FString>       FilesToCopy;
    FString               ProjectContentDir;

    int32                 TotalFiles     = 0;
    std::atomic<int32>    CopiedCount    { 0 };
    std::atomic<int32>    SkippedCount   { 0 };
    std::atomic<int32>    FailedCount    { 0 };
    std::atomic<bool>     bComplete      { false };
    std::atomic<bool>     bStopRequested { false };

    mutable FCriticalSection CurrentFileCS;
    FString                  CurrentFileName;

    // Only written from background thread after completion
    TArray<FString> FilesCopied;
    TArray<FString> FilesFailed;
    TArray<FString> FilesSkipped;

    static constexpr TCHAR ContentIdent[] = TEXT("/Content/");
};

// -----------------------------------------------------------------------
// SSKImportProgressPopup
// A resizable modal-style window that shows per-file progress,
// an overall progress bar, and a scrollable log.
// Polls FCopyFileTask each Slate tick via SNew(SBox).Tick(...)
// -----------------------------------------------------------------------
class SIDEKICKCHARACTERTOOL_API SSKImportProgressPopup : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SSKImportProgressPopup) {}
        SLATE_ARGUMENT(TArray<FString>, FilesToCopy)
        SLATE_ARGUMENT(FOnImportCompletedDelegate, OnImportComplete)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    virtual ~SSKImportProgressPopup() override;

    static TSharedRef<SWindow> OpenInWindow(
        const TArray<FString>& FilesToCopy,
        FOnImportCompletedDelegate OnImportComplete = FOnImportCompletedDelegate());

private:
    // Slate tick — polls the background task and updates UI
    virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

    void StartCopy(const TArray<FString>& FilesToCopy);
    void OnCopyComplete();
    void AppendLog(const FString& Line);

    // Widget attribute callbacks
    TOptional<float> GetProgressFraction()  const;
    FText            GetProgressText()      const;
    FText            GetCurrentFileText()   const;
    FText            GetStatusText()        const;
    FSlateColor      GetProgressBarColor()  const;

    FReply OnCancelClicked();
    FReply OnCloseClicked();

    // Background task
    TSharedPtr<FCopyFileTask>  CopyTask;
    FRunnableThread*           CopyThread  = nullptr;

    // Log entries (game thread only)
    TArray<TSharedPtr<FString>>      LogLines;
    TSharedPtr<SListView<TSharedPtr<FString>>> LogListView;
    TSharedPtr<SButton>              CancelButton;
    TSharedPtr<SButton>              CloseButton;

    // Cached last polled state to avoid redundant log spam
    int32 LastLoggedCount = 0;
    bool  bHasCompleted   = false;

    TWeakPtr<SWindow> OwningWindow;

    // Delegates
    FOnImportCompletedDelegate OnImportComplete;
};