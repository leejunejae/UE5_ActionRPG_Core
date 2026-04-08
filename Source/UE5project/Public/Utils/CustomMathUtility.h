// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CustomMathUtility.generated.h"

/**
 * 
 */

namespace CustomMath
{
    namespace Spatial
    {
        FORCEINLINE float Distance2D(const AActor* A, const AActor* B)
        {
            if (!A || !B) return FLT_MAX;
            return FVector::Dist2D(A->GetActorLocation(), B->GetActorLocation());
        }

        FORCEINLINE float AbsDeltaYawDeg(const APawn* SelfPawn, const AActor* Target)
        {
            if (!SelfPawn || !Target) return 180.f;

            const FVector ToTarget = (Target->GetActorLocation() - SelfPawn->GetActorLocation()).GetSafeNormal2D();
            const FVector Forward = SelfPawn->GetActorForwardVector().GetSafeNormal2D();

            const float Dot = FVector::DotProduct(Forward, ToTarget);
            const float CrossZ = FVector::CrossProduct(Forward, ToTarget).Z;

            const float AngleRad = FMath::Atan2(CrossZ, Dot);
            return FMath::Abs(FMath::RadiansToDegrees(AngleRad));
        }
    }

    namespace Math
    {
        static float SmoothStepNormal(float T)
        {
            T = FMath::Clamp(T, 0.f, 1.f);
            return T * T * (3.f - 2.f * T);
        }

        static float Tent(float D, float Min, float Ideal, float Max)
        {
            if (D <= Min || D >= Max) return 0.f;
            if (D < Ideal) return (D - Min) / FMath::Max(KINDA_SMALL_NUMBER, (Ideal - Min));
            return (Max - D) / FMath::Max(KINDA_SMALL_NUMBER, (Max - Ideal));
        }

        static float RampNormal(float D, float Start, float Full)
        {
            return FMath::Clamp((D - Start) / FMath::Max(KINDA_SMALL_NUMBER, (Full - Start)), 0.f, 1.f);
        }

        static float AngleWindowNormal(float AbsDeltaYawDeg, float Good, float Bad)
        {
            const float T = (AbsDeltaYawDeg - Good) / FMath::Max(KINDA_SMALL_NUMBER, (Bad - Good));
            return 1.f - FMath::Clamp(T, 0.f, 1.f);
        }

        static float SoftRange(float X, float Min, float Max, float Feather)
        {
            if (Feather <= 0.f)
                return (X >= Min && X <= Max) ? 1.f : 0.f;

            if (X < Min - Feather) return 0.f;
            if (X < Min) return SmoothStepNormal((X - (Min - Feather)) / Feather);

            if (X <= Max) return 1.f;
            if (X <= Max + Feather) return 1.f - SmoothStepNormal((X - Max) / Feather);

            return 0.f;
        }

        static float Cooldown01(float Remaining, float Cooldown)
        {
            if (Cooldown <= 0.f) return 1.f;
            return 1.f - FMath::Clamp(Remaining / Cooldown, 0.f, 1.f);
        }
    }

    namespace BlendCurve
    {
        static float EaseIn(float T)
        {
            T = FMath::Clamp(T, 0.f, 1.f);
            return T * T;
        }

        /** Quadratic Out - 천천히 끝 */
        static float EaseOut(float T)
        {
            T = FMath::Clamp(T, 0.f, 1.f);
            return T * (2.f - T);
        }

        /** Cubic EaseInOut */
        static float EaseInOutCubic(float T)
        {
            T = FMath::Clamp(T, 0.f, 1.f);
            return T < 0.5f
                ? 4.f * T * T * T
                : 1.f - FMath::Pow(-2.f * T + 2.f, 3.f) * 0.5f;
        }

        /** Elastic Out - 오버슈트 후 안착 */
        static float ElasticOut(float T)
        {
            T = FMath::Clamp(T, 0.f, 1.f);
            if (T <= 0.f) return 0.f;
            if (T >= 1.f) return 1.f;
            const float c4 = (2.f * PI) / 3.f;
            return FMath::Pow(2.f, -10.f * T)
                * FMath::Sin((T * 10.f - 0.75f) * c4) + 1.f;
        }

        /** Bounce Out - 튀기며 안착 */
        static float BounceOut(float T)
        {
            T = FMath::Clamp(T, 0.f, 1.f);
            constexpr float n1 = 7.5625f, d1 = 2.75f;
            if (T < 1.f / d1) { return n1 * T * T; }
            else if (T < 2.f / d1) { T -= 1.500f / d1; return n1 * T * T + 0.75f; }
            else if (T < 2.5f / d1) { T -= 2.250f / d1; return n1 * T * T + 0.9375f; }
            else { T -= 2.625f / d1; return n1 * T * T + 0.984375f; }
        }

        /** 기존 SmoothStepNormal의 In 버전 alias */
        static float EaseInOut(float T) { return FMath::SmoothStep(0.f, 1.f, T); }
    }
}

UCLASS()
class UE5PROJECT_API UCustomMathUtility : public UObject
{
	GENERATED_BODY()
	
};
