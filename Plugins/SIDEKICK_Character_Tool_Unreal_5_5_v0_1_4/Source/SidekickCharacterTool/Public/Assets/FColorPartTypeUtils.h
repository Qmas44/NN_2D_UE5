#pragma once
#include "Enums/ColorPartType.h"

class FColorPartTypeUtils
{
public:
    
    /**
     * Get a Collection of parts for specific ColorPartType's
     * 
     * @param basePartType
     * @return 
     */
    static TArray<EColorPartType> GetPartTypes(EColorPartType basePartType)
    {
        switch (basePartType)
        {
            case EColorPartType::AllParts:
                return TArray<EColorPartType>
                {
                    EColorPartType::Head,
                    EColorPartType::Hair,
                    EColorPartType::EyebrowLeft,
                    EColorPartType::EyebrowRight,
                    EColorPartType::EyeLeft,
                    EColorPartType::EyeRight,
                    EColorPartType::EarLeft,
                    EColorPartType::EarRight,
                    EColorPartType::FacialHair,
                    EColorPartType::Torso,
                    EColorPartType::ArmUpperLeft,
                    EColorPartType::ArmUpperRight,
                    EColorPartType::ArmLowerLeft,
                    EColorPartType::ArmLowerRight,
                    EColorPartType::HandLeft,
                    EColorPartType::HandRight,
                    EColorPartType::Hips,
                    EColorPartType::LegLeft,
                    EColorPartType::LegRight,
                    EColorPartType::FootLeft,
                    EColorPartType::FootRight,
                    EColorPartType::AttachmentHead,
                    EColorPartType::AttachmentFace,
                    EColorPartType::AttachmentBack,
                    EColorPartType::AttachmentHipsFront,
                    EColorPartType::AttachmentHipsBack,
                    EColorPartType::AttachmentHipsLeft,
                    EColorPartType::AttachmentHipsRight,
                    EColorPartType::AttachmentShoulderLeft,
                    EColorPartType::AttachmentShoulderRight,
                    EColorPartType::AttachmentElbowLeft,
                    EColorPartType::AttachmentElbowRight,
                    EColorPartType::AttachmentKneeLeft,
                    EColorPartType::AttachmentKneeRight,
                    EColorPartType::Nose,
                    EColorPartType::Teeth,
                    EColorPartType::Tongue,
                    // EColorPartType::Wrap,
                    // EColorPartType::AttachmentHandLeft,
                    // EColorPartType::AttachmentHandRight,
                };

            case EColorPartType::CharacterHead:
                return TArray<EColorPartType>
                {
                    EColorPartType::Head,
                    EColorPartType::Hair,
                    EColorPartType::EyebrowLeft,
                    EColorPartType::EyebrowRight,
                    EColorPartType::EyeLeft,
                    EColorPartType::EyeRight,
                    EColorPartType::EarLeft,
                    EColorPartType::EarRight,
                    EColorPartType::FacialHair,
                    EColorPartType::AttachmentHead,
                    EColorPartType::AttachmentFace,
                    EColorPartType::Nose,
                    EColorPartType::Teeth,
                    EColorPartType::Tongue,
                };

            case EColorPartType::CharacterUpperBody:
                return TArray<EColorPartType>
                {
                    EColorPartType::Torso,
                    // EColorPartType::Wrap,
                    EColorPartType::ArmUpperLeft,
                    EColorPartType::ArmUpperRight,
                    EColorPartType::ArmLowerLeft,
                    EColorPartType::ArmLowerRight,
                    EColorPartType::HandLeft,
                    EColorPartType::HandRight,
                    EColorPartType::AttachmentBack,
                    EColorPartType::AttachmentShoulderLeft,
                    EColorPartType::AttachmentShoulderRight,
                    EColorPartType::AttachmentElbowLeft,
                    EColorPartType::AttachmentElbowRight,
                    // EColorPartType::AttachmentHandLeft,
                    // EColorPartType::AttachmentHandRight,
                };

            case EColorPartType::CharacterLowerBody:
                return TArray<EColorPartType>
                {
                    EColorPartType::Hips,
                    EColorPartType::LegLeft,
                    EColorPartType::LegRight,
                    EColorPartType::FootLeft,
                    EColorPartType::FootRight,
                    EColorPartType::AttachmentHipsFront,
                    EColorPartType::AttachmentHipsBack,
                    EColorPartType::AttachmentHipsLeft,
                    EColorPartType::AttachmentHipsRight,
                    EColorPartType::AttachmentKneeLeft,
                    EColorPartType::AttachmentKneeRight,
                };

            default:
                // if it's not a group, then it only represents itself, so only return itself
                return TArray<EColorPartType>
                {
                    basePartType,
                };
        }
    }
};
