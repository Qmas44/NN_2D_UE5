#pragma once
#include "ColorGroup.generated.h"

/**
 * Color Group is used in the `sk_color_property`,
 * to query what udim the specific part should reference
 */
UENUM()
enum class EColorGroup : int32
{
	Species = 1,
	Outfits,
	Attachments,
	Materials,
	Elements,

	/** MAX - invalid */
	Max UMETA(Hidden),
};

/** Converts the Color Group Enum to a String */
inline const FString ColorGroupToString(EColorGroup InValue)
{
	return UEnum::GetDisplayValueAsText(InValue).ToString();
}

/** Converts a String name that matches the enum name into an Enum */
inline const EColorGroup ColorGroupFromString(const FString& EnumName)
{
	UEnum* EnumPtr = StaticEnum<EColorGroup>();
	if (!EnumPtr) return EColorGroup::Max;

	int64 Value = EnumPtr->GetValueByName(FName(*EnumName));

	if (Value == INDEX_NONE)
	{
		return EColorGroup::Max;
	}
	return static_cast<EColorGroup>(Value);
}

/** Converts am integer to a string enum name */
inline FString ColorGroupFromIntToString(const int32 Index)
{
	UEnum* EnumPtr = StaticEnum<EColorGroup>();
	if (!EnumPtr) return "";

	if (EnumPtr->IsValidEnumValue(Index))
	{
		EColorGroup ColorGroup = static_cast<EColorGroup>(Index);
		return ColorGroupToString(ColorGroup);
	
	}
	return "";
}