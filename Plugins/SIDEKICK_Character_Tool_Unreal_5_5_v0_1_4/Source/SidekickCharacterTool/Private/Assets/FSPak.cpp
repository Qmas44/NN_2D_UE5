#include "Assets/FSPak.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/FileManager.h"

/**
 * Copies all files that are passed into the current project's content folder, and makes sure the directory
 * structure after content matches.
 * 
 * @param FilesToCopy The files that will be copied into the Unreal Project
 */
void CopyAdditionalFilesToFolder( TArray<FString>& FilesToCopy)
{
	FString ContentIdent = TEXT("Content/");

	FString ContentPath = FPaths::ProjectContentDir();

	TArray<FString> FilesCopied;
	
	for (int32 iFile = 0; iFile < FilesToCopy.Num(); ++iFile)
	{
		FString EachFile = FilesToCopy[iFile];
		int32 ContentIndex;
		ContentIndex = EachFile.Find(*ContentIdent);
		
		if (ContentIndex != INDEX_NONE)
		{
			FString ContentFile = EachFile.RightChop(ContentIndex);
			FString MigrationPath = FPaths::Combine(*FPaths::ProjectDir(), *ContentFile);
			
			if (!FPaths::DirectoryExists(FPaths::GetPath(MigrationPath)))
			{
				IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
				bool bCreated = PlatformFile.CreateDirectoryTree(*FPaths::GetPath(MigrationPath));
				UE_LOG(LogTemp, Log, TEXT("[Generated Folder] %s"), *FPaths::GetPath(MigrationPath));
			}

			if (! FPaths::FileExists(MigrationPath))
			{
				if (IFileManager::Get().Copy(*MigrationPath, *EachFile) == COPY_OK)
				{
					FilesCopied.Add(MigrationPath);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("[Import Failed] %s"), *MigrationPath);
				}
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("[File Exists] %s"), *MigrationPath);
			}
		}
	}
	
	UE_LOG(LogTemp, Log, TEXT("[== Files Copied ==]"));
	FString SyntyIdent = TEXT("Synty/");
	for (auto File : FilesCopied)
	{
		int ContentIndex = File.Find(*SyntyIdent);
		FString ContentFile = File.RightChop(ContentIndex);
		UE_LOG(LogTemp, Log, TEXT("    %s"), *ContentFile);
	}
}

void FSPak::ImportSidekickPak(const FString& SourceFolder, TArray<FString>& OutFiles)
{
	CopyAdditionalFilesToFolder(OutFiles);
}

/**
 * Find all .uassets in all provided folders. Creating one monolithic list of uassets
 * found inside all folders.
 * 
 * @param SourceFolders 
 * @param OutFiles 
 */
void FSPak::FindUAssets(const TArray<FString>& SourceFolders, TArray<FString>& OutFiles)
{
	// Search for both .uasset and .umap files
	const TArray<FString> FilePatterns = {
		TEXT("*.uasset"),
		TEXT("*.umap")
	};
	IFileManager& FileManager = IFileManager::Get();

	TArray<FString> PackageFiles;
	TSet<FString> UniqueList;
	OutFiles.Empty();

	for (const FString& PackagePath : SourceFolders)
	{
		for (const FString& Pattern : FilePatterns)
		{
			FileManager.FindFilesRecursive(
				PackageFiles,
				*PackagePath,
				*Pattern,
				true,   // Files
				false,  // Directories
				false); // bClearFileNames

			UniqueList.Append(PackageFiles);
			PackageFiles.Empty();
		}
	}
	OutFiles = UniqueList.Array();
	OutFiles.Sort();
}