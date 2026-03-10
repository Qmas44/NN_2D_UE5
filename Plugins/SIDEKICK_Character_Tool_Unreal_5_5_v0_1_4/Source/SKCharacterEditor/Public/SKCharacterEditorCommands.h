// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "FSKCharacterEditorStyle.h"

class SKCHARACTEREDITOR_API FSKCharacterEditorCommands : public TCommands<FSKCharacterEditorCommands>
{
public:
	
	FSKCharacterEditorCommands()
		: TCommands<FSKCharacterEditorCommands>(
			TEXT("SKCharacterEditor"),
			NSLOCTEXT("Contexts", "SKCharacterEditor", "My Editor"),
			NAME_None,
			FSKCharacterEditorStyle::GetStyleSetName())
	{}
	
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > ImportSidekickPakAction;
};
