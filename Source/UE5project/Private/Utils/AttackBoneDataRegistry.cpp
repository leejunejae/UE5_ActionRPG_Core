// Fill out your copyright notice in the Description page of Project Settings.


#include "Utils/AttackBoneDataRegistry.h"

static FTransform SampleArrayAtTime(const TArray<FBoneFrameSample>& CachedSamples, float QueryTime)
{
    if (CachedSamples.Num() == 0) return FTransform::Identity;
    if (CachedSamples.Num() == 1) return CachedSamples[0].BoneTransform;

    const float StartTime = CachedSamples[0].Time;
    const float EndTime = CachedSamples.Last().Time;

    if (QueryTime <= StartTime) return CachedSamples[0].BoneTransform;
    if (QueryTime >= EndTime)   return CachedSamples.Last().BoneTransform;

    // 균등 샘플링 가정(추출 시 일정 간격) - 너 방식 그대로
    const float Dt = CachedSamples[1].Time - CachedSamples[0].Time;
    if (Dt <= KINDA_SMALL_NUMBER) return CachedSamples[0].BoneTransform;

    const float Exact = (QueryTime - StartTime) / Dt;
    int32 A = FMath::FloorToInt(Exact);
    int32 B = A + 1;
    const float Alpha = Exact - (float)A;

    A = FMath::Clamp(A, 0, CachedSamples.Num() - 1);
    B = FMath::Clamp(B, 0, CachedSamples.Num() - 1);

    FTransform Out;
    Out.Blend(CachedSamples[A].BoneTransform, CachedSamples[B].BoneTransform, Alpha);
    return Out;
}

FTransform FBoneTransformSegment::GetTransformAtTime(float QueryTime) const
{
    if (Samples.Num() == 0)
        return FTransform::Identity;

    const TArray<FBoneFrameSample>& CachedSamples = Samples;
    int32 NumCachedSamples = CachedSamples.Num();

    float Start = CachedSamples[0].Time;
    float End = CachedSamples.Last().Time;

    if (QueryTime <= Start)
        return CachedSamples[0].BoneTransform;
    if (QueryTime >= End)
        return CachedSamples.Last().BoneTransform;

    float DeltaTime = CachedSamples[1].Time - CachedSamples[0].Time;
    float ExactIndex = (QueryTime - Start) / DeltaTime;

    int32 IndexA = FMath::FloorToInt(ExactIndex);
    int32 IndexB = IndexA + 1;
    float Alpha = ExactIndex - IndexA;

    IndexA = FMath::Clamp(IndexA, 0, NumCachedSamples - 1);
    IndexB = FMath::Clamp(IndexB, 0, NumCachedSamples - 1);

    const FTransform LowerTransfrom = CachedSamples[IndexA].BoneTransform;
    const FTransform UpperTransfrom =    CachedSamples[IndexB].BoneTransform;

    FTransform BoneTransformAtTime;
    BoneTransformAtTime.Blend(LowerTransfrom, UpperTransfrom, Alpha);
    return BoneTransformAtTime;
}