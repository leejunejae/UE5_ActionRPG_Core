// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Utils/AttackBoneDataRegistry.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AnimBoneTransformLibrary.generated.h"

/**
 * 
 */
USTRUCT()
struct FHitWindow
{
    GENERATED_BODY()

public:
    UPROPERTY()
        FName Name;

    UPROPERTY()
        FName TargetBone;

    UPROPERTY()
        float StartTime;

    UPROPERTY()
        float EndTime;


    FHitWindow() = default;
    FHitWindow(FName InName, FName InTargetBone, float InS, float InE)
        : Name(InName), TargetBone(InTargetBone), StartTime(InS), EndTime(InE) {}
};

class UAnimSequence;
class UAttackHitDataRegistry;

UCLASS()
class UE5PROJECT_API UAnimBoneTransformLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
    // ✅ NotifyState 윈도우를 자동 수집해서, 윈도우별 Segment만 프리베이크 후
    // ✅ UAnimBoneDataRegistryRoot 생성 + Registry 자동 등록 + Save 까지
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Attack|HitData")
        static void BuildHitDataFromNotifyWindows(
            UAnimSequence* AnimSequence,
            float SampleInterval,
            UAttackBoneDataRegistry* Registry
        );

private:
#if WITH_EDITOR
    static FTransform GetBoneTransformAtTime(UAnimSequence* AnimSequence, const FName& BoneName, double Time);

    static void GatherAttackHitWindows(const UAnimSequence* Anim, TArray<FHitWindow>& OutWindows);

    static void BuildSegmentsFromWindows(
        TMap<FName, FBoneTransformSegment>& OutSegments,
        const TArray<FHitWindow>& Windows,
        UAnimSequence* AnimSequence,
        float SampleInterval
    );

    static void SaveObjectPackage(UObject* Obj);
#endif
	//UFUNCTION(BlueprintCallable, Category = "Trajectory")
		//static void ExtractAnimBoneTransformToAsset(UAnimSequence* AnimSequence, const FName& BoneName, float FrameRate,const FString& SavePath, const FString& AssetName);
};
