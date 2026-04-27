// Fill out your copyright notice in the Description page of Project Settings.


#include "Utils/AnimBoneTransformLibrary.h"

#if WITH_EDITOR

#include "Animation/AnimSequence.h"

#include "Utils/AttackBoneDataRegistry.h"
#include "Animation/Notifies/ANS_AttackHitWindow.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Curves/CurveVector.h"
#include "UObject/SavePackage.h"
#include "BonePose.h"

// -------------------- utils --------------------

void UAnimBoneTransformLibrary::SaveObjectPackage(UObject* Obj)
{
    if (!Obj) return;

    UPackage* Pkg = Obj->GetOutermost();
    if (!Pkg) return;

    Pkg->MarkPackageDirty();

    const FString PackageName = Pkg->GetName();
    const FString Filename = FPackageName::LongPackageNameToFilename(
        PackageName, FPackageName::GetAssetPackageExtension());

    FSavePackageArgs SaveArgs;
    UPackage::SavePackage(Pkg, Obj, *Filename, SaveArgs);
}

// -------------------- Gather windows from NotifyState --------------------

void UAnimBoneTransformLibrary::GatherAttackHitWindows(const UAnimSequence* Anim, TArray<FHitWindow>& OutWindows)
{
    OutWindows.Reset();

    if (!Anim) return;

    for (const FAnimNotifyEvent& Ev : Anim->Notifies)
    {
        if (!Ev.NotifyStateClass) continue;
        if (Ev.NotifyStateClass->GetClass() != UANS_AttackHitWindow::StaticClass()) continue;

        const UANS_AttackHitWindow* WindowANS = Cast<UANS_AttackHitWindow>(Ev.NotifyStateClass);
        if (!WindowANS) continue;

        const float S = Ev.GetTriggerTime();
        const float E = Ev.GetEndTriggerTime();

        if (E > S)
        {
            OutWindows.Add(FHitWindow(WindowANS->WindowName, WindowANS->TargetBone, S, E));
        }
    }

    OutWindows.Sort([](const FHitWindow& A, const FHitWindow& B)
        {
            return A.StartTime < B.StartTime;
        });
}

// -------------------- Pose evaluation (your original) --------------------

FTransform UAnimBoneTransformLibrary::GetBoneTransformAtTime(UAnimSequence* AnimSequence, const FName& BoneName, double Time)
{
    if (!AnimSequence || !AnimSequence->GetSkeleton())
    {
        return FTransform::Identity;
    }

    const USkeleton* Skeleton = AnimSequence->GetSkeleton();
    const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();

    const int32 BoneIndex = RefSkeleton.FindBoneIndex(BoneName);
    if (BoneIndex == INDEX_NONE)
    {
        return FTransform::Identity;
    }

    
    // 1. Required Bone Indices: 모든 본을 포함
    TArray<FBoneIndexType> RequiredBoneIndices;
    const int32 NumBones = RefSkeleton.GetNum();
    for (int32 i = 0; i < NumBones; ++i)
    {
        RequiredBoneIndices.Add(i);
    }

    // 2. Curve Evaluation Option
    //FCurveEvaluationOption CurveOption(false);
    UE::Anim::FCurveFilterSettings CurveFilterSettings;

    // 3. FBoneContainer 생성
    FBoneContainer RequiredBones(RequiredBoneIndices, CurveFilterSettings, *AnimSequence->GetSkeleton());

    // 4. Pose 생성 준비
    FMemMark Mark(FMemStack::Get());
    FCompactPose Pose;
    FBlendedCurve Curve;
    UE::Anim::FStackAttributeContainer Attributes;
    FAnimationPoseData AnimationPoseData(Pose, Curve, Attributes);

    Pose.SetBoneContainer(&RequiredBones);

    // 5. 애니메이션 포즈 추출 (지정 시간)
    FAnimExtractContext ExtractContext(Time);
    AnimSequence->GetAnimationPose(AnimationPoseData, ExtractContext);

    // 6. 로컬 트랜스폼 배열 생성 (Pose에서 직접 가져오기)
    TArray<FTransform> LocalTransforms;
    LocalTransforms.SetNumUninitialized(Pose.GetNumBones());

    for (FCompactPoseBoneIndex Index(0); Index < Pose.GetNumBones(); ++Index)
    {
        LocalTransforms[Index.GetInt()] = Pose[Index];
    }

    // 7. 컴포넌트 스페이스 트랜스폼 계산
    TArray<FTransform> ComponentSpaceTransforms;
    TArrayView<const FTransform> LocalTransformsView(LocalTransforms);

    FAnimationRuntime::FillUpComponentSpaceTransforms(RefSkeleton, LocalTransformsView, ComponentSpaceTransforms);

    FTransform RootTransform = ComponentSpaceTransforms[0];
    FTransform BoneTransform = ComponentSpaceTransforms[BoneIndex];

    FTransform RelativeTransform = BoneTransform.GetRelativeTransform(RootTransform);

    return RelativeTransform;
}

void UAnimBoneTransformLibrary::BuildSegmentsFromWindows(TMap<FName, FBoneTransformSegment>& OutSegments, const TArray<FHitWindow>& Windows, UAnimSequence* AnimSequence, float SampleInterval)
{
    OutSegments.Reset();
    if (!AnimSequence || Windows.Num() == 0) return;

    for (const auto& W : Windows)
    {
        FBoneTransformSegment Seg;
        Seg.BoneName = W.TargetBone;
        Seg.SampleInterval = SampleInterval;
        Seg.StartTime = W.StartTime;
        Seg.EndTime = W.EndTime;

        for (double T = Seg.StartTime; T <= Seg.EndTime + KINDA_SMALL_NUMBER; T += SampleInterval)
        {
            Seg.Samples.Add(FBoneFrameSample(T, GetBoneTransformAtTime(AnimSequence, W.TargetBone, T)));
        }

        // EndTime을 정확히 찍도록 보정
        if (Seg.Samples.Num() == 0 || !FMath::IsNearlyEqual(Seg.Samples.Last().Time, Seg.EndTime, 1e-4f))
        {
            Seg.Samples.Add(FBoneFrameSample(Seg.EndTime, GetBoneTransformAtTime(AnimSequence, W.TargetBone, Seg.EndTime)));
        }

        // (선택) 여기에 ReduceSamples(Seg.Samples, PosTol, RotTol) 넣으면 B 방식 완성
        OutSegments.Add(W.Name, MoveTemp(Seg));
    }
}

// -------------------- Public entry (CallInEditor) --------------------

void UAnimBoneTransformLibrary::BuildHitDataFromNotifyWindows(
    UAnimSequence* AnimSequence,
    float SampleInterval,
    UAttackBoneDataRegistry* Registry
)
{
    if (!AnimSequence || SampleInterval <= 0.f || !Registry)
        return;

    // 1) 윈도우 자동 수집
    TArray<FHitWindow> Windows;
    GatherAttackHitWindows(AnimSequence, Windows);

    if (Windows.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[BuildHitData] No UANS_AttackHitWindow found in %s"), *AnimSequence->GetName());
        return;
    }

    // 2) 윈도우별로 Key/Asset 생성 + Registry 등록
    for (int32 i = 0; i < Windows.Num(); ++i)
    {
        TArray<FHitWindow> SingleWindow;
        SingleWindow.Add(Windows[i]);

        FHitDataEntry Entry;

        BuildSegmentsFromWindows(
            Entry.Segments,
            SingleWindow,
            AnimSequence,
            SampleInterval
        );

        Registry->UpsertSamples(AnimSequence, Entry);
    }

    // 3) Registry 저장
    SaveObjectPackage(Registry);

    UE_LOG(LogTemp, Log, TEXT("[BuildHitData] Done. Anim=%s, Windows=%d"),
        *AnimSequence->GetName(), Windows.Num());
}

#endif // WITH_EDITOR