// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AnimNotifyState_TimedNiagaraEffect.h"
#include "ANS_TimedWeaponTrail.generated.h"

/**
 * 
 */

class IEquipmentDataInterface;
class UActorComponent;
class UFXSystemComponent;
class UNiagaraComponent;
class USceneComponent;
struct FBoneTransformSegment;

struct FWeaponTrailRuntimeKey
{
	TWeakObjectPtr<USkeletalMeshComponent> MeshComp;
	const FAnimNotifyEvent* NotifyEvent = nullptr;
	TWeakObjectPtr<const UObject> NotifySource;
	int32 MontageInstanceId = INDEX_NONE;

	bool operator==(const FWeaponTrailRuntimeKey& Other) const
	{
		return MeshComp == Other.MeshComp &&
			NotifyEvent == Other.NotifyEvent &&
			NotifySource == Other.NotifySource &&
			MontageInstanceId == Other.MontageInstanceId;
	}

	friend uint32 GetTypeHash(const FWeaponTrailRuntimeKey& Key)
	{
		uint32 Hash = GetTypeHash(Key.MeshComp);
		Hash = HashCombine(Hash, PointerHash(Key.NotifyEvent));
		Hash = HashCombine(Hash, GetTypeHash(Key.NotifySource));
		return HashCombine(Hash, GetTypeHash(Key.MontageInstanceId));
	}
};

struct FWeaponTrailDistanceCacheKey
{
	const FBoneTransformSegment* Segment = nullptr;
	FTransform WeaponRelativeToBone = FTransform::Identity;
	FVector StartSocketInWeapon = FVector::ZeroVector;
	FVector EndSocketInWeapon = FVector::ZeroVector;
	float StartTime = 0.0f;
	float EndTime = 0.0f;
	float SampleInterval = 0.0f;
	float StartWeight = 0.0f;
	float EndWeight = 0.0f;

	bool operator==(const FWeaponTrailDistanceCacheKey& Other) const
	{
		return Segment == Other.Segment &&
			WeaponRelativeToBone.GetTranslation() == Other.WeaponRelativeToBone.GetTranslation() &&
			WeaponRelativeToBone.GetRotation() == Other.WeaponRelativeToBone.GetRotation() &&
			WeaponRelativeToBone.GetScale3D() == Other.WeaponRelativeToBone.GetScale3D() &&
			StartSocketInWeapon == Other.StartSocketInWeapon &&
			EndSocketInWeapon == Other.EndSocketInWeapon &&
			StartTime == Other.StartTime && EndTime == Other.EndTime &&
			SampleInterval == Other.SampleInterval &&
			StartWeight == Other.StartWeight && EndWeight == Other.EndWeight;
	}

	friend uint32 GetTypeHash(const FWeaponTrailDistanceCacheKey& Key)
	{
		uint32 Hash = PointerHash(Key.Segment);
		Hash = HashCombine(Hash, GetTypeHash(Key.WeaponRelativeToBone.GetTranslation()));
		Hash = HashCombine(Hash, GetTypeHash(Key.WeaponRelativeToBone.GetRotation()));
		Hash = HashCombine(Hash, GetTypeHash(Key.WeaponRelativeToBone.GetScale3D()));
		Hash = HashCombine(Hash, GetTypeHash(Key.StartSocketInWeapon));
		Hash = HashCombine(Hash, GetTypeHash(Key.EndSocketInWeapon));
		Hash = HashCombine(Hash, GetTypeHash(Key.StartTime));
		Hash = HashCombine(Hash, GetTypeHash(Key.EndTime));
		Hash = HashCombine(Hash, GetTypeHash(Key.SampleInterval));
		Hash = HashCombine(Hash, GetTypeHash(Key.StartWeight));
		return HashCombine(Hash, GetTypeHash(Key.EndWeight));
	}
};

struct FWeaponTrailRuntimeState
{
	TWeakObjectPtr<UActorComponent> EquipmentComponent;
	TWeakObjectPtr<USceneComponent> TraceComponent;
	TWeakObjectPtr<UNiagaraComponent> EffectComponent;
	const FBoneTransformSegment* Segment = nullptr;
	TArray<FVector> StartSamples;
	TArray<FVector> EndSamples;
	TArray<float> TrailUSamples;
	TArray<float> LinkOrderSamples;
	float NotifyStartTime = 0.0f;
	float NotifyEndTime = 0.0f;
	float LastSampleTime = 0.0f;
	FTransform PreviousRootWorldTransform = FTransform::Identity;
	bool bHasPreviousRootWorldTransform = false;
	bool bNeedsInitialSample = true;
	int32 NextLinkOrder = 0;
	float TotalTrajectoryDistance = 0.0f;
	float AccumulatedTrajectoryDistance = 0.0f;
	bool bHasPreviousDistanceSample = false;
	FVector PreviousDistanceStart = FVector::ZeroVector;
	FVector PreviousDistanceEnd = FVector::ZeroVector;
	bool bHasPreviousDebugSample = false;
	FVector PreviousDebugStart = FVector::ZeroVector;
	FVector PreviousDebugEnd = FVector::ZeroVector;
};

UCLASS()
class UE5PROJECT_API UANS_TimedWeaponTrail : public UAnimNotifyState_TimedNiagaraEffect
{
	GENERATED_BODY()
	
public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, const FAnimNotifyEventReference& EventReference) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData")
		bool bSubWeapon = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData")
		FName TrailStartParameter = TEXT("TrailStart");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData")
		FName TrailEndParameter = TEXT("TrailEnd");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData")
		FName TrailMaterialParameter = TEXT("User.TrailMaterial");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Trajectory")
		FName WindowName = TEXT("Trail");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Trajectory")
		FName TargetBone = TEXT("Hand_R");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Trajectory", meta = (ClampMin = "0.001", UIMin = "0.001"))
		float TrajectorySampleInterval = 0.004f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Trajectory", meta = (ClampMin = "1", UIMin = "1"))
		int32 MaxSamplesPerFrame = 16;

	// Ribbon 진행률 계산에서 Start 소켓 궤적이 차지하는 비율이다.
	// Start/End 가중치는 합계로 정규화되므로 반드시 합이 1일 필요는 없다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Trajectory|Distance",
		meta = (ClampMin = "0.0", UIMin = "0.0"))
		float TrailStartDistanceWeight = 0.5f;

	// Ribbon 진행률 계산에서 End 소켓 궤적이 차지하는 비율이다.
	// 긴 무기의 칼끝 움직임을 강조하려면 이 값을 Start보다 크게 설정한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Trajectory|Distance",
		meta = (ClampMin = "0.0", UIMin = "0.0"))
		float TrailEndDistanceWeight = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Trajectory")
		FName TrailStartSamplesParameter = TEXT("User.TrailStartSamples");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Trajectory")
		FName TrailEndSamplesParameter = TEXT("User.TrailEndSamples");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Trajectory")
		FName TrailSampleCountParameter = TEXT("User.TrailSampleCount");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Trajectory")
		FName TrailBatchDurationParameter = TEXT("User.TrailBatchDuration");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Trajectory")
		FName TrailLinkOrderSamplesParameter = TEXT("User.TrailLinkOrderSamples");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Trajectory")
		FName TrailUSamplesParameter = TEXT("User.TrailUSamples");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Fade")
		FName TrailIsEndingParameter = TEXT("User.TrailIsEnding");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Fade")
		FName TrailFadeDurationParameter = TEXT("User.TrailFadeDuration");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Fade", meta = (ClampMin = "0.01", UIMin = "0.01"))
		float TrailFadeDuration = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Debug")
		bool bDebugDrawTrajectory = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Debug", meta = (ClampMin = "0.0", UIMin = "0.0"))
		float DebugDrawDuration = 2.0f;

protected:
	virtual UFXSystemComponent* SpawnEffect(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation) const override;

private:
	FWeaponTrailRuntimeKey MakeRuntimeKey(
		USkeletalMeshComponent* MeshComp,
		const FAnimNotifyEventReference& EventReference) const;
	FWeaponTrailRuntimeState* FindRuntimeState(
		const FWeaponTrailRuntimeKey& RequestedKey,
		FWeaponTrailRuntimeKey* OutResolvedKey = nullptr);

	TMap<FWeaponTrailRuntimeKey, FWeaponTrailRuntimeState> RuntimeStates;
	TMap<FWeaponTrailDistanceCacheKey, float> TrajectoryDistanceCache;

};
