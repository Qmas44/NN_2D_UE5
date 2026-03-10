#pragma once
#include "CoreMinimal.h"
#include "UObject/Class.h"
#include "CharacterPartTypes.generated.h"

UENUM()
enum class ECharacterPartType : int32
{
	// Specify the starting value as 1 to ensure values match server side values.
	Head = 1,
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
	// AttachmentHandLeft,
	// AttachmentHandRight,
	
	/** MAX - invalid */
	Max UMETA(Hidden),
};

/** Converts the Character Part Enum to a String */
inline const FString CharacterPartTypeToString(ECharacterPartType InValue)
{
	return UEnum::GetDisplayValueAsText(InValue).ToString();
}

/** Converts a String into an Enum */
inline const ECharacterPartType CharacterPartTypeFromString(const FString& EnumName)
{
	UEnum* EnumPtr = StaticEnum<ECharacterPartType>();
	if (!EnumPtr) return ECharacterPartType::Max;

	int64 Value = EnumPtr->GetValueByName(FName(*EnumName));

	if (Value == INDEX_NONE)
	{
		return ECharacterPartType::Max;
	}
	return static_cast<ECharacterPartType>(Value);
}