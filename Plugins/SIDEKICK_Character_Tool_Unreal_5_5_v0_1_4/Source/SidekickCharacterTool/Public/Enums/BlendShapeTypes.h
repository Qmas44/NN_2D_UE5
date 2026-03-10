#pragma once


UENUM()
enum class EBlendShapeType : int32
{
	// Specify the starting value as 0 to ensure values match server side values.
	Feminine = 0,
	Heavy,
	Skinny,
	Bulk,
	/** MAX - invalid */
	Max UMETA(Hidden),
};