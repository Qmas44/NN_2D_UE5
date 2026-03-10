#pragma once
#include "CoreMinimal.h"
#include "UObject/Class.h"
#include "PartGroups.generated.h"

UENUM()
enum class EPartGroup : int32
{
	Head = 1,
	UpperBody,
	LowerBody,
	
	/** MAX - invalid */
	Max UMETA(Hidden),
};

inline const FString PartGroupToString(EPartGroup InValue)
{
	return UEnum::GetDisplayValueAsText(InValue).ToString();
}

/** Converts a String into an Enum */
inline const EPartGroup PartGroupFromString(const FString& EnumName)
{
	UEnum* EnumPtr = StaticEnum<EPartGroup>();
	if (!EnumPtr) return EPartGroup::Max;

	int64 Value = EnumPtr->GetValueByName(FName(*EnumName));

	if (Value == INDEX_NONE)
	{
		return EPartGroup::Max;
	}
	return static_cast<EPartGroup>(Value);
}