// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Models/MainMenuModel.h"
#include "Widgets/SCompoundWidget.h"

class SExpandableArea;
class SVerticalBox;

/**
 * 
 */
class SIDEKICKCHARACTERTOOL_API SSKOptionsMenuWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSKOptionsMenuWidget)
		: _Model()
		{
		}
		SLATE_ARGUMENT( TSharedPtr<MainMenuModel>, Model);

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	FReply OnMergeClicked();
	FReply OnImportPakClicked();
	FReply OnGettingStartedClicked();
	void RefreshMenu();

protected:
	/** The shared UI Model data*/
	TSharedPtr< MainMenuModel > Model;

private:
	// -- Merge Options
	TSharedPtr<SVerticalBox> MergeOptionsVerticalBox;
	TSharedRef<SExpandableArea> InitMergeOptions();
	
};
