// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UObject/PrimaryAssetId.h"
#include "AttackBoneDataRegistry.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FBoneFrameSample
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        float Time;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        FTransform BoneTransform;

    FBoneFrameSample()
        : Time(0.f), BoneTransform(FTransform::Identity)
    {
    }

    FBoneFrameSample(float InTime, const FTransform& InTransform)
        : Time(InTime), BoneTransform(InTransform) {}
};

USTRUCT(BlueprintType)
struct FBoneTransformSegment
{
    GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
        FName BoneName = FName("Hand_R");
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
        float SampleInterval = 0.001f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
        float StartTime = 0.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
        float EndTime = 0.f;

    // Time은 "Anim 절대 시간" 저장 추천
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
        TArray<FBoneFrameSample> Samples;

    public:
    FTransform GetTransformAtTime(float QueryTime) const;
};

USTRUCT(BlueprintType)
struct FHitDataEntry
{
    GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
        TMap<FName, FBoneTransformSegment> Segments; // 네가 만든 Segments 그대로
};

UCLASS(BlueprintType)
class UE5PROJECT_API UAttackBoneDataRegistry : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
    // Key -> Soft reference
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitData")
        TMap<FPrimaryAssetId, FHitDataEntry> HitDataMap;

    const FHitDataEntry* Find(const UAnimSequence* Anim) const
    {
        FPrimaryAssetId AssetId = Anim->GetPrimaryAssetId();

        if (!AssetId.IsValid())
        {
            AssetId = FPrimaryAssetId(FName("AnimSequence"), Anim->GetFName());
        }

        return HitDataMap.Find(AssetId);
    }

#if WITH_EDITOR
    void ReplaceSamples(const UAnimSequence* Anim, FHitDataEntry Entry)
    {
        if (!Anim)
        {
            UE_LOG(LogTemp, Warning, TEXT("ReplaceSamples: Anim Invalid"));
            return;
        }

        FPrimaryAssetId AssetId = Anim->GetPrimaryAssetId();

        if (!AssetId.IsValid())
        {
            AssetId = FPrimaryAssetId(FName("AnimSequence"), Anim->GetFName());
        }

        if (Entry.Segments.IsEmpty())
        {
            HitDataMap.Remove(AssetId);
        }
        else
        {
            HitDataMap.FindOrAdd(AssetId) = MoveTemp(Entry);
        }

        MarkPackageDirty();
    }
#endif
};
