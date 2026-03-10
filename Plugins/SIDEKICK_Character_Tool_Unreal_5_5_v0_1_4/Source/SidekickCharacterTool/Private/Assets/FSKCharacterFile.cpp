#include "Assets/FSKCharacterFile.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

/**
 * Reads the Sidekick Character file
 * 
 * @param FilePath path to the .sk file
 * @return the read data from the file in a more accesible format.
 */
SKFileData FSKCharacterFile::ReadSKFile(const FString FilePath)
{
	SKFileData FileData;	
	TArray<FString> Lines;
	
	if (FFileHelper::LoadFileToStringArray(Lines, *FilePath))
	{
		for (int i = 0; i < Lines.Num(); ++i)
		{
			FString Line = Lines[i];
			
			if (Line.StartsWith(TEXT("Species")))
			{
				// Conforms the Specie data
				
				TArray<FString> SpecieTokens;
				Line.ParseIntoArray(SpecieTokens, TEXT(":"), true);
				FString SpecieName = SpecieTokens[1];
				FileData.SpecieID = FCString::Atoi(*SpecieName);
			}
			else if (Line.StartsWith(TEXT("- Name:")))
			{
				// Conforms the Part data
				
				TArray<FString> NameTokens;
				Line.ParseIntoArray(NameTokens, TEXT(":"), true);
				
				FString PartType = Lines[i+1];
				TArray<FString> TypeTokens;
				PartType.ParseIntoArray(TypeTokens, TEXT(":"), true);

				FString PartName = NameTokens[1].TrimStartAndEnd();
				FString PartTypeName = TypeTokens[1].TrimStartAndEnd();

				FileData.Parts.Add(PartTypeName, PartName);
				
				// skip the next two lines as those were read above
				i+=2;
			}
			else if (Line.StartsWith(TEXT("BlendShapes:")))
			{
				// Assume that the 3 lines after blendshape are the shape attributes
				for (int x = 1; x <= 3; ++x)
				{
					// Check if line exists, as blendshapes appear at the end of the file, and dont
					// have to be included.
					if (i+x >= Lines.Num())
					{
						break;
					}
					
					FString MorphTargetLine = Lines[i+x];
					
					if (!MorphTargetLine.StartsWith(TEXT("  ")))
					{
						continue;
					}
					
					TArray<FString> NameTokens;
					MorphTargetLine.ParseIntoArray(NameTokens, TEXT(":"), true);

					FString MTAttribute = NameTokens[0].TrimStartAndEnd();
					FString MTValueString = NameTokens[1].TrimStartAndEnd();
					
					float MTValue = FCString::Atof(*MTValueString);
					
					if (MTAttribute == "MuscleValue")
					{
						// Muscles value in unity is stored -100 to 100
						MTValue = ( MTValue + 100.0f ) / 200.0f;
						MTAttribute = "MTBodyMusculature";
					}
					else if (MTAttribute == "BodyTypeValue")
					{
						MTValue = MTValue / 100.0f;
						MTAttribute = "MTBodyType";
					}
					else if (MTAttribute == "BodySizeValue")
					{
						MTValue = MTValue / 100.0f;
						MTAttribute = "MTBodySize";
					}
					
					FileData.MorphTargets.Add(MTAttribute, MTValue);
				}
			}
			else if (Line.StartsWith(TEXT("- ColorProperty:")))
			{
				FString DataEntry = "";
				
				TArray<FString> ColorProperyTokens;
				Line.ParseIntoArray(ColorProperyTokens, TEXT(":"), true);
				int32 ColorID = FCString::Atoi(*ColorProperyTokens[1].TrimStartAndEnd());
				
				// TODO: Implement line reading, as there can be a varied amount of attributes under ColorProperty.
				if (i+6 >= Lines.Num())
				{
					continue;
				}
				
				Lines[i+1].ParseIntoArray(ColorProperyTokens, TEXT(":"), true);
				DataEntry = ColorProperyTokens[1].TrimStartAndEnd();
				FileData.MainColor.Add(ColorID, DataEntry);
				Lines[i+2].ParseIntoArray(ColorProperyTokens, TEXT(":"), true);
				DataEntry = ColorProperyTokens[1].TrimStartAndEnd();
				FileData.Metallic.Add(ColorID, DataEntry);
				Lines[i+3].ParseIntoArray(ColorProperyTokens, TEXT(":"), true);
				DataEntry = ColorProperyTokens[1].TrimStartAndEnd();
				FileData.Smoothness.Add(ColorID, DataEntry);
				Lines[i+4].ParseIntoArray(ColorProperyTokens, TEXT(":"), true);
				DataEntry = ColorProperyTokens[1].TrimStartAndEnd();
				FileData.Reflection.Add(ColorID, DataEntry);
				Lines[i+5].ParseIntoArray(ColorProperyTokens, TEXT(":"), true);
				DataEntry = ColorProperyTokens[1].TrimStartAndEnd();
				FileData.Emission.Add(ColorID, DataEntry);
				Lines[i+6].ParseIntoArray(ColorProperyTokens, TEXT(":"), true);
				DataEntry = ColorProperyTokens[1].TrimStartAndEnd();
				FileData.Opacity.Add(ColorID, DataEntry);
				
				i+=6;
			}
		}
	}
	
	return FileData;
}

/**
 * Creates a selection file dialog, to select a .sk file.
 * 
 * @param OutPath path to the .sk file
 * @return true if a file was selected, else false.
 */
bool FSKCharacterFile::OpenFileDialog(FString& OutPath)
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		return false;
	}

	const void* ParentWindowHandle = nullptr;

	// Allow only .yaml files
	FString Filter = TEXT("Sidekick Character Files (*.sk)|*.sk");

	TArray<FString> OutFiles;
	bool bOpened = DesktopPlatform->OpenFileDialog(
		ParentWindowHandle,
		TEXT("Select a Sidekick File"),
		FPaths::ProjectDir(),
		TEXT(""),
		Filter,
		EFileDialogFlags::None,
		OutFiles
	);

	if (bOpened && OutFiles.Num() > 0)
	{
		OutPath = OutFiles[0];
		return true;
	}

	return false;
}
