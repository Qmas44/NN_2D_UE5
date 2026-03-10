#pragma once
#include "CoreMinimal.h"
#include "UObject/Class.h"
#include "ColorPartType.generated.h"

UENUM()
enum class EColorPartType : int32
{
	// Specify the starting value as 1 to ensure values match server side values.
	AllParts = 1,
	Species,
	Outfit,
	Attachments,
	Materials,
	Elements,
	CharacterHead,
	CharacterUpperBody,
	CharacterLowerBody,
	Head,
	Hair,
	EyebrowLeft,
	EyebrowRight,
	EyeLeft,
	EyeRight,
	EarLeft,
	EarRight,
	FacialHair,
	Torso,
	ArmUpperLeft,
	ArmUpperRight,
	ArmLowerLeft,
	ArmLowerRight,
	HandLeft,
	HandRight,
	Hips,
	LegLeft,
	LegRight,
	FootLeft,
	FootRight,
	AttachmentHead,
	AttachmentFace,
	AttachmentBack,
	AttachmentHipsFront,
	AttachmentHipsBack,
	AttachmentHipsLeft,
	AttachmentHipsRight,
	AttachmentShoulderLeft,
	AttachmentShoulderRight,
	AttachmentElbowLeft,
	AttachmentElbowRight,
	AttachmentKneeLeft,
	AttachmentKneeRight,
	Nose,
	Teeth,
	Tongue,
	Wrap,
	
	/** MAX - invalid */
	Max UMETA(Hidden),
};

/** Converts the ColorPartType Enum to a String */
inline const FString ColorPartTypeToString(EColorPartType InValue)
{
	return UEnum::GetDisplayValueAsText(InValue).ToString();
}

/** Converts a String into an Enum */
inline const EColorPartType ColorPartTypeFromString(const FString& EnumName)
{
	UEnum* EnumPtr = StaticEnum<EColorPartType>();
	if (!EnumPtr) return EColorPartType::Max;

	int64 Value = EnumPtr->GetValueByName(FName(*EnumName));

	if (Value == INDEX_NONE)
	{
		return EColorPartType::Max;
	}
	return static_cast<EColorPartType>(Value);
}