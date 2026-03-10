#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Interfaces/IHttpRequest.h"
#include "USKDB.generated.h"

DECLARE_DELEGATE_OneParam(FOnRequestFinished, FString /*FilePath*/);
DECLARE_DELEGATE_OneParam(FOnRequestFailed, FString /*Error Message*/);

UCLASS()
class SIDEKICKCHARACTERTOOL_API USKDB: public UObject
{
	GENERATED_BODY()
	
public:
	bool DBFileExists();
	
	void GetLatestVersion();

private:
	void OnZipDownloaded(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	
	FString GetLatestDBFileURL(FHttpResponsePtr Response);

	void DownloadFile(const FString& URL, const FString& ProjectDBPath);
	
public:
	FOnRequestFinished SuccessCallback;
private:
	FOnRequestFailed FailedCallback;
};
