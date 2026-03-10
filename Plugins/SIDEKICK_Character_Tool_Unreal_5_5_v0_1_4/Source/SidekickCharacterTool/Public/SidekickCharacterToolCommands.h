// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "SidekickCharacterToolStyle.h"

class FSidekickCharacterToolCommands : public TCommands<FSidekickCharacterToolCommands>
{
public:

	FSidekickCharacterToolCommands()
		: TCommands<FSidekickCharacterToolCommands>(TEXT("SidekickCharacterTool"),
			NSLOCTEXT("Contexts", "SidekickCharacterTool", "SidekickCharacterTool Plugin"),
			NAME_None,
			FSidekickCharacterToolStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenSidekickPluginWindow;
};