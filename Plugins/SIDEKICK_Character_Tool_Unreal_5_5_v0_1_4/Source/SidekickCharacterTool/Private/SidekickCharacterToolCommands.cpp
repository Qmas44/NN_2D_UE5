// Copyright Epic Games, Inc. All Rights Reserved.

#include "SidekickCharacterToolCommands.h"

#define LOCTEXT_NAMESPACE "FSidekickCharacterToolModule"

void FSidekickCharacterToolCommands::RegisterCommands()
{
	UI_COMMAND(OpenSidekickPluginWindow, "SidekickCharacterTool", "Bring up SidekickCharacterTool window", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
