// Fill out your copyright notice in the Description page of Project Settings.


#include "SKCharacterPreset.h"

#if WITH_EDITOR
void USKCharacterPreset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	// UE_LOG(LogTemp, Display, TEXT("Property Changed %s"), *PropertyChangedEvent.Property->GetFName().ToString());
	UObject::PostEditChangeProperty(PropertyChangedEvent);
	OnObjectChanged.Broadcast();
	UE_LOG(LogTemp, Warning, TEXT("USKCharacterPreset::PostEditChangeProperty"));

	// Source Texture Changed
	FName ChangedProp = PropertyChangedEvent.MemberProperty 
						   ? PropertyChangedEvent.MemberProperty->GetFName()
						   : NAME_None;

	if (ChangedProp == GET_MEMBER_NAME_CHECKED(USKCharacterPreset, SourceTexture))
	{
		UE_LOG(LogTemp, Warning, TEXT("MyTexture was changed!"));
		// todo: Trigger temporary memory material population

		// TemporaryTexture
	}
}

#endif