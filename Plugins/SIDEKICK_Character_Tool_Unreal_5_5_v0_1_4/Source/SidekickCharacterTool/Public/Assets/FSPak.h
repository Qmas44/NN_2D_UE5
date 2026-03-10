#pragma once

#include "CoreMinimal.h"

/**
 * Sidekick Pack - Object that contains tools to interact with the Sidekick Asset Packs
 */
class SIDEKICKCHARACTERTOOL_API FSPak
{
public:
	static void ImportSidekickPak(const FString& SourceFolder, TArray<FString>& OutFiles);
	static void FindUAssets(const TArray<FString>& SourceFolders, TArray<FString>& OutFiles);
};
