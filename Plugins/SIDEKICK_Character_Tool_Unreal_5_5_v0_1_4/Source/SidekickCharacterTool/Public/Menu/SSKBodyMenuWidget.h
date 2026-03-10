// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Models/MainMenuModel.h"
#include "Widgets/SCompoundWidget.h"

/**
 * 
 */
class SIDEKICKCHARACTERTOOL_API SSKBodyMenuWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSKBodyMenuWidget)
		: _Model()
		{
		}
		SLATE_ARGUMENT( TSharedPtr<MainMenuModel>, Model);

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

protected:
	/** The shared UI Model data*/
	TSharedPtr< MainMenuModel > Model;

	/** Populates the Part Morph Target offset with keys */
	void PopulateMTOffsets();

public:	
	/** Calculates the part offset transforms for the active morph targets. */
	static void CalculateMTOffset(MainMenuModel* Model);

	float GetBodySize();
	float GetBodyType();
	float GetBodyMuscular();
};
