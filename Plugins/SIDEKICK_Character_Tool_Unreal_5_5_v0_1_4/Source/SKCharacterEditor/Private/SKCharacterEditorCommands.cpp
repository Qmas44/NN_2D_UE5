// Fill out your copyright notice in the Description page of Project Settings.


#include "SKCharacterEditorCommands.h"

#define LOCTEXT_NAMESPACE "FSKCharacterEditorModule"

void FSKCharacterEditorCommands::RegisterCommands()
{
	UI_COMMAND(ImportSidekickPakAction, "Import Pack", "Import Sidekick Package.", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE