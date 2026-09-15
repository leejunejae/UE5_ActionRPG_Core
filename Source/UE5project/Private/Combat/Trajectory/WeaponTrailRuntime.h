#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"
#include "Utils/WeaponTrajectoryUtility.h"

class UNiagaraComponent;
class UNiagaraSystem;
class USceneComponent;
class USkeletalMeshComponent;
class UWorld;
class UANS_TimedWeaponTrail;
class UAnimSequenceBase;
struct FAnimNotifyEvent;
struct FAnimNotifyEventReference;
struct FBoneTransformSegment;

struct FWeaponTrailRuntimeKey
{
	TWeakObjectPtr<USkeletalMeshComponent> MeshComp;
	const FAnimNotifyEvent* NotifyEvent = nullptr;
	TWeakObjectPtr<const UObject> NotifySource;
	int32 MontageInstanceId = INDEX_NONE;
	bool operator==(const FWeaponTrailRuntimeKey& Other) const;
	friend uint32 GetTypeHash(const FWeaponTrailRuntimeKey& Key);
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
	bool operator==(const FWeaponTrailDistanceCacheKey& Other) const;
	friend uint32 GetTypeHash(const FWeaponTrailDistanceCacheKey& Key);
};

struct FWeaponTrailDistanceCacheEntry
{
	float Distance = 0.0f;
	uint64 LastAccessSerial = 0;
};

/** Mutable state for one concrete execution of a weapon-trail notify. */
struct FWeaponTrailRuntime
{
	TWeakObjectPtr<UObject> WeaponSource;
	TWeakObjectPtr<USceneComponent> TraceComponent;
	TWeakObjectPtr<UNiagaraComponent> EffectComponent;
	TWeakObjectPtr<UWorld> World;
	FTimerHandle CleanupTimerHandle;
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
	bool bHasResolvedTotalTrajectoryDistance = false;
	float AccumulatedTrajectoryDistance = 0.0f;
	bool bHasPreviousDistanceSample = false;
	FVector PreviousDistanceStart = FVector::ZeroVector;
	FVector PreviousDistanceEnd = FVector::ZeroVector;
	bool bHasPreviousDebugSample = false;
	FVector PreviousDebugStart = FVector::ZeroVector;
	FVector PreviousDebugEnd = FVector::ZeroVector;

	void UploadSamples(float BatchDuration);
	void Finish(float FadeDuration, bool bDestroyImmediately);
};

/** Stateless sampling operations shared by trail runtimes. */
struct FWeaponTrailSampler
{
	static void NormalizeDistanceWeights(float StartWeight, float EndWeight,
		float& OutStartWeight, float& OutEndWeight);
	static float CalculateTotalTrajectoryDistance(
		const FWeaponTrajectoryGeometry& Geometry, const FBoneTransformSegment& Segment,
		float StartTime, float EndTime, float SampleInterval,
		float StartWeight, float EndWeight);
};

/** Owns concurrent notify executions and the bounded trajectory-distance cache. */
class FWeaponTrailRuntimeManager
{
public:
	~FWeaponTrailRuntimeManager();
	static UObject* FindWeaponSource(const USkeletalMeshComponent* MeshComp);
	static UNiagaraSystem* ResolveTrailSystem(UObject* WeaponSource, bool bSubWeapon);
	static UNiagaraComponent* SpawnWeaponEffect(
		USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		UObject* WeaponSource, UNiagaraSystem* TrailSystem,
		bool bSubWeapon, bool bDestroyAtEnd,
		bool bApplyRateScaleAsTimeDilation, float FadeDuration);
	void Begin(UANS_TimedWeaponTrail& Notify, USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Anim, float TotalDuration, const FAnimNotifyEventReference& EventReference);
	void Tick(UANS_TimedWeaponTrail& Notify, USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Anim, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference);
	void End(UANS_TimedWeaponTrail& Notify, USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Anim, const FAnimNotifyEventReference& EventReference);
	static FWeaponTrailRuntimeKey MakeKey(USkeletalMeshComponent* MeshComp,
		const FAnimNotifyEventReference& EventReference);
	FWeaponTrailRuntime* Find(const FWeaponTrailRuntimeKey& RequestedKey,
		FWeaponTrailRuntimeKey* OutResolvedKey = nullptr);
	void Add(const FWeaponTrailRuntimeKey& Key, FWeaponTrailRuntime&& Runtime);
	void Remove(const FWeaponTrailRuntimeKey& Key, bool bDestroyEffect);
	void PruneInvalid();
	FWeaponTrailDistanceCacheEntry* FindDistance(const FWeaponTrailDistanceCacheKey& Key);
	void AddDistance(const FWeaponTrailDistanceCacheKey& Key, float Distance);

private:
	TMap<FWeaponTrailRuntimeKey, FWeaponTrailRuntime> Runtimes;
	TMap<FWeaponTrailDistanceCacheKey, FWeaponTrailDistanceCacheEntry> DistanceCache;
	uint64 AccessSerial = 0;
	static constexpr int32 MaxDistanceCacheEntries = 64;
};
