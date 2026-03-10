#pragma once

#include "CoreMinimal.h"

struct SKFileData
{
	int32 SpecieID;
	TMap<FString, FString> Parts;
	TMap<FString, float> MorphTargets;

	// Material Colour properties
	TMap<int32, FString> MainColor;
	TMap<int32, FString> Metallic;
	TMap<int32, FString> Smoothness;
	TMap<int32, FString> Reflection;
	TMap<int32, FString> Emission;
	TMap<int32, FString> Opacity;
};

class FSKCharacterFile
{
public:
	/** Converts the file into a usable SKFileData */
	static SKFileData ReadSKFile(const FString FilePath);
	/** Open Dialog to select .sk file */
	static bool OpenFileDialog(FString& OutPath);
};
