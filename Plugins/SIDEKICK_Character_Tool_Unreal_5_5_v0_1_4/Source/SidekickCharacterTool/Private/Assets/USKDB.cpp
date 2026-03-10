#include "Assets/USKDB.h"

#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Interfaces/IHttpRequest.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"

/**
 * 
 * @param URL Url to file
 * @param ProjectDBPath Path where file will be saved to 
 */
void USKDB::DownloadFile(const FString& URL, const FString& ProjectDBPath)
{
	const FString& SavePath = ProjectDBPath;
	
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(URL);
	Request->SetVerb("GET");

	Request->OnProcessRequestComplete().BindLambda([this, SavePath](FHttpRequestPtr Req, FHttpResponsePtr Response, bool bWasSuccessful)
	{
		if (bWasSuccessful && Response.IsValid())
		{
			const TArray<uint8>& Data = Response->GetContent();
			FFileHelper::SaveArrayToFile(Data, *SavePath);

			UE_LOG(LogTemp, Log, TEXT("Downloaded file to: %s"), *SavePath);

			SuccessCallback.ExecuteIfBound(SavePath);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Download failed"));
		}
	});

	Request->ProcessRequest();
}

/**
 * Checks if the Sidekicks Database file exists.
 * 
 * @return False if not found. True if found. 
 */
bool USKDB::DBFileExists()
{
	FString FolderStructure = "Synty/SidekickCharacters/Database/";
	FString DBFileName = "Synty_Sidekick.db";
	FString Path = FPaths::ProjectContentDir() + "/" + FolderStructure + "/" + DBFileName;
	
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	
	return PlatformFile.FileExists(*Path);
}

void USKDB::GetLatestVersion()
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL("http://api.github.com/repos/SyntyStudios/SidekicksToolRelease/releases/latest");
	Request->SetVerb("GET");
	Request->SetHeader("Accept", "application/vnd.github+json");
	Request->SetHeader("X-GitHub-Api-Version", "2022-11-28");
	Request->SetHeader("User-Agent", "SyntySidekick-PackageManager");

	Request->OnProcessRequestComplete().BindUObject(this, &USKDB::OnZipDownloaded);
	Request->ProcessRequest();
}

void USKDB::OnZipDownloaded(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	bool bSuccess = bWasSuccessful;
	FString ZipUrl = "";
	
	if (bWasSuccessful && Response.IsValid())
	{
		
		ZipUrl = GetLatestDBFileURL(Response);
		
		if (!ZipUrl.IsEmpty())
		{

			UE_LOG(LogTemp, Log, TEXT("Downloaded file: %s"), *ZipUrl);
			
			// download the package from git
			FString FolderStructure = "Synty/SidekickCharacters/Database/";
			FString DBFileName = "Synty_Sidekick.db";
			FString Path = FPaths::ProjectContentDir() + "/" + FolderStructure + "/" + DBFileName;
			DownloadFile(ZipUrl, Path);
			return;
		}
	}
	
	FailedCallback.ExecuteIfBound("Failed to download database from github.");
}

// [x] Get the payload responce
// [x] Convert the payload into a json object
// [x] From 'assets' find Synty_Sidekick.db, then get the browser download url
FString USKDB::GetLatestDBFileURL(FHttpResponsePtr Response)
{
	FString ZipUrl = "";
	TSharedPtr<FJsonObject> JsonObject;

	const TArray<uint8>& Data = Response->GetContent();
	
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		if ( JsonObject->HasField("assets"))
		{
			const TArray<TSharedPtr<FJsonValue>>& AssetsEntries = JsonObject->GetArrayField("assets");
				
			for (auto JsonEntry : AssetsEntries)
			{
				TSharedPtr<FJsonObject> JSonAssets = JsonEntry->AsObject();
					
				if (JSonAssets->HasField("name") && JSonAssets->GetStringField("name") == "Synty_Sidekick.db")
				{
					ZipUrl = JSonAssets->GetStringField("browser_download_url");
				}
			}
		}
	}

	return ZipUrl;
}