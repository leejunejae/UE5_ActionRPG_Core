#include "Combat/Trajectory/WeaponTrailRuntime.h"
#include "Combat/ANS_TimedWeaponTrail.h"
#include "Animation/AnimSequence.h"
#include "Characters/CharacterBase.h"
#include "Characters/Interfaces/WeaponRuntimeSourceInterface.h"
#include "Combat/Data/AttackData.h"
#include "Combat/Interfaces/AttackSourceInterface.h"
#include "Core/Subsystems/GameInstanceSystem/AnimBoneDataSubsystem.h"
#include "DrawDebugHelpers.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Utils/CoreLog.h"

#include "Utils/AttackBoneDataRegistry.h"
#include "Animation/ActiveMontageInstanceScope.h"
#include "NiagaraComponent.h"
#include "Engine/World.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"

namespace
{
	// 이 Notify가 지원하는 Niagara 시스템의 고정 파라미터 규약이다.
	const FName TrailStartParameter(TEXT("TrailStart"));
	const FName TrailEndParameter(TEXT("TrailEnd"));
	const FName TrailMaterialParameter(TEXT("User.TrailMaterial"));
	const FName TrailStartSamplesParameter(TEXT("User.TrailStartSamples"));
	const FName TrailEndSamplesParameter(TEXT("User.TrailEndSamples"));
	const FName TrailLinkOrderSamplesParameter(TEXT("User.TrailLinkOrderSamples"));
	const FName TrailUSamplesParameter(TEXT("User.TrailUSamples"));
	const FName TrailSampleCountParameter(TEXT("User.TrailSampleCount"));
	const FName TrailBatchDurationParameter(TEXT("User.TrailBatchDuration"));
	const FName TrailIsEndingParameter(TEXT("User.TrailIsEnding"));
	const FName TrailFadeDurationParameter(TEXT("User.TrailFadeDuration"));
}

static float GetWeightedTrajectoryDistance(
	const FVector& PreviousStart, const FVector& PreviousEnd,
	const FVector& CurrentStart, const FVector& CurrentEnd,
	float StartWeight, float EndWeight)
{
	return FVector::Distance(PreviousStart, CurrentStart) * StartWeight +
		FVector::Distance(PreviousEnd, CurrentEnd) * EndWeight;
}

static FWeaponTrailDistanceCacheKey BuildTrajectoryDistanceCacheKey(
	const FWeaponTrajectoryGeometry& Geometry,
	const FBoneTransformSegment& Segment,
	float StartTime,
	float EndTime,
	float SampleInterval,
	float StartWeight,
	float EndWeight)
{
	FWeaponTrailDistanceCacheKey Key;
	Key.Segment = &Segment;
	Key.WeaponRelativeToBone = Geometry.WeaponRelativeToBone;
	Key.StartSocketInWeapon = Geometry.StartSocketInWeapon;
	Key.EndSocketInWeapon = Geometry.EndSocketInWeapon;
	Key.StartTime = StartTime;
	Key.EndTime = EndTime;
	Key.SampleInterval = SampleInterval;
	Key.StartWeight = StartWeight;
	Key.EndWeight = EndWeight;
	return Key;
}

static UObject* FindWeaponRuntimeSource(const USkeletalMeshComponent* MeshComp)
{
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner) return nullptr;
	if (Owner->GetClass()->ImplementsInterface(UWeaponRuntimeSourceInterface::StaticClass()))
	{
		return Owner;
	}

	for (UActorComponent* Component : Owner->GetComponents())
	{
		if (Component && Component->GetClass()->ImplementsInterface(UWeaponRuntimeSourceInterface::StaticClass()))
		{
			return Component;
		}
	}
	return nullptr;
}

UObject* FWeaponTrailRuntimeManager::FindWeaponSource(const USkeletalMeshComponent* MeshComp)
{
	return FindWeaponRuntimeSource(MeshComp);
}

UNiagaraSystem* FWeaponTrailRuntimeManager::ResolveTrailSystem(
	UObject* WeaponSource, bool bSubWeapon)
{
	return WeaponSource
		? IWeaponRuntimeSourceInterface::Execute_GetWeaponTrailSystem(WeaponSource, bSubWeapon)
		: nullptr;
}

UNiagaraComponent* FWeaponTrailRuntimeManager::SpawnWeaponEffect(
	USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	UObject* WeaponSource, UNiagaraSystem* TrailSystem,
	bool bSubWeapon, bool bDestroyAtEnd,
	bool bApplyRateScaleAsTimeDilation, float FadeDuration)
{
	if (!MeshComp || !WeaponSource || !TrailSystem) return nullptr;
	UNiagaraComponent* Effect = UNiagaraFunctionLibrary::SpawnSystemAttached(
		TrailSystem, MeshComp, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator,
		EAttachLocation::KeepRelativeOffset, !bDestroyAtEnd, false);
	if (!Effect) return nullptr;

	if (UMaterialInterface* Material =
		IWeaponRuntimeSourceInterface::Execute_GetWeaponTrailMaterial(WeaponSource, bSubWeapon))
	{
		Effect->SetVariableMaterial(TrailMaterialParameter, Material);
	}
	if (bApplyRateScaleAsTimeDilation && Animation)
	{
		Effect->SetCustomTimeDilation(Animation->RateScale);
	}

	const FName StartSocket = IWeaponRuntimeSourceInterface::Execute_GetWeaponTrailStartSocket(WeaponSource, bSubWeapon);
	const FName EndSocket = IWeaponRuntimeSourceInterface::Execute_GetWeaponTrailEndSocket(WeaponSource, bSubWeapon);
	const FVector InitialStart = IWeaponRuntimeSourceInterface::Execute_GetWeaponSocketLocation(WeaponSource, StartSocket, bSubWeapon);
	const FVector InitialEnd = IWeaponRuntimeSourceInterface::Execute_GetWeaponSocketLocation(WeaponSource, EndSocket, bSubWeapon);
	Effect->SetVectorParameter(TrailStartParameter, InitialStart);
	Effect->SetVectorParameter(TrailEndParameter, InitialEnd);
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayPosition(Effect, TrailStartSamplesParameter, { InitialStart });
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayPosition(Effect, TrailEndSamplesParameter, { InitialEnd });
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(Effect, TrailLinkOrderSamplesParameter, { 0.0f });
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(Effect, TrailUSamplesParameter, { 0.0f });
	Effect->SetVariableFloat(TrailBatchDurationParameter, 0.0f);
	Effect->SetVariableFloat(TrailFadeDurationParameter, FadeDuration);
	Effect->SetVariableBool(TrailIsEndingParameter, false);
	Effect->SetVariableInt(TrailSampleCountParameter, 0);
	Effect->SetTickBehavior(ENiagaraTickBehavior::ForceTickLast);
	Effect->Activate(true);
	return Effect;
}

static bool BuildWeaponTrailSamples(
	USkeletalMeshComponent* MeshComp,
	USceneComponent* TraceComponent,
	const FBoneTransformSegment& Segment,
	float StartTime,
	float EndTime,
	float SampleInterval,
	int32 MaxSamples,
	bool bIncludeStart,
	bool bSubWeapon,
	FName StartSocket,
	FName EndSocket,
	TArray<FVector>& OutStartSamples,
	TArray<FVector>& OutEndSamples,
	TArray<float>& OutTrailUSamples,
	float TrailStartTime,
	float TrailEndTime,
	float StartDistanceWeight,
	float EndDistanceWeight,
	FWeaponTrailRuntime& RuntimeState,
	const FTransform& PreviousRootWorldTransform,
	const FTransform& CurrentRootWorldTransform)
{
	if (!MeshComp || !TraceComponent || EndTime <= StartTime) return false;

	const FWeaponTrajectoryGeometry Geometry = FWeaponTrajectoryUtility::BuildGeometry(
		MeshComp, TraceComponent, Segment.BoneName, StartSocket, EndSocket);
	if (!Geometry.IsValid()) return false;

	const float SafeInterval = FMath::Max(SampleInterval, 0.001f);
	const int32 IntervalCount = FMath::Clamp(
		FMath::CeilToInt((EndTime - StartTime) / SafeInterval), 1, FMath::Max(1, MaxSamples));
	const int32 SampleCount = FMath::Min(
		IntervalCount + (bIncludeStart ? 1 : 0), FMath::Max(1, MaxSamples));
	OutStartSamples.Reserve(SampleCount);
	OutEndSamples.Reserve(SampleCount);
	OutTrailUSamples.Reserve(SampleCount);
	const float TrailDuration = FMath::Max(TrailEndTime - TrailStartTime, UE_SMALL_NUMBER);
	const FTransform IdentityRoot = FTransform::Identity;

	for (int32 Index = 0; Index < SampleCount; ++Index)
	{
		const float Alpha = bIncludeStart
			? (SampleCount > 1 ? static_cast<float>(Index) / static_cast<float>(SampleCount - 1) : 0.0f)
			: static_cast<float>(Index + 1) / static_cast<float>(SampleCount);
		const float SampleTime = FMath::Lerp(StartTime, EndTime, Alpha);
		FTransform SampleRootWorldTransform;
		SampleRootWorldTransform.Blend(
			PreviousRootWorldTransform, CurrentRootWorldTransform, Alpha);
		FVector SampleStart;
		FVector SampleEnd;
		FWeaponTrajectoryUtility::GetSocketWorldPositions(
			Geometry, Segment.GetTransformAtTime(SampleTime), SampleRootWorldTransform,
			SampleStart, SampleEnd);
		OutStartSamples.Add(SampleStart);
		OutEndSamples.Add(SampleEnd);

		FVector DistanceStart;
		FVector DistanceEnd;
		FWeaponTrajectoryUtility::GetSocketWorldPositions(
			Geometry, Segment.GetTransformAtTime(SampleTime), IdentityRoot,
			DistanceStart, DistanceEnd);
		if (RuntimeState.bHasPreviousDistanceSample)
		{
			RuntimeState.AccumulatedTrajectoryDistance += GetWeightedTrajectoryDistance(
				RuntimeState.PreviousDistanceStart, RuntimeState.PreviousDistanceEnd,
				DistanceStart, DistanceEnd, StartDistanceWeight, EndDistanceWeight);
		}
		RuntimeState.PreviousDistanceStart = DistanceStart;
		RuntimeState.PreviousDistanceEnd = DistanceEnd;
		RuntimeState.bHasPreviousDistanceSample = true;

		const float TrailU = RuntimeState.TotalTrajectoryDistance > UE_SMALL_NUMBER
			? RuntimeState.AccumulatedTrajectoryDistance / RuntimeState.TotalTrajectoryDistance
			: (SampleTime - TrailStartTime) / TrailDuration;
		OutTrailUSamples.Add(FMath::Clamp(TrailU, 0.0f, 1.0f));
	}

	return !OutStartSamples.IsEmpty();
}

void FWeaponTrailRuntimeManager::Begin(UANS_TimedWeaponTrail& Notify, USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp) return;

#if WITH_EDITOR
	if (MeshComp->GetWorld() &&
		MeshComp->GetWorld()->WorldType != EWorldType::Game &&
		MeshComp->GetWorld()->WorldType != EWorldType::PIE)
	{
		return;
	}
#endif
	PruneInvalid();
	const FWeaponTrailRuntimeKey RuntimeKey = FWeaponTrailRuntimeManager::MakeKey(MeshComp, EventReference);
	// 같은 실행 키가 비정상적으로 남아 있으면 새 Effect로 덮어쓰기 전에 정리한다.
	Remove(RuntimeKey, true);

	FWeaponTrailRuntime State;
	State.StartSamples.Reserve(FMath::Max(1, Notify.MaxSamplesPerFrame));
	State.EndSamples.Reserve(FMath::Max(1, Notify.MaxSamplesPerFrame));
	State.TrailUSamples.Reserve(FMath::Max(1, Notify.MaxSamplesPerFrame));
	State.LinkOrderSamples.Reserve(FMath::Max(1, Notify.MaxSamplesPerFrame));
	State.WeaponSource = FindWeaponRuntimeSource(MeshComp);
	if (ACharacter* OwnerCharacter = Cast<ACharacter>(MeshComp->GetOwner()))
	{
		if (IAttackSourceInterface* AttackSource = Cast<IAttackSourceInterface>(OwnerCharacter))
		{
			const FAttackTraceSource TraceSource = AttackSource->GetAttackTraceSource(Notify.bSubWeapon
				? EAttackSourceType::OffHand : EAttackSourceType::MainHand);
			State.TraceComponent = TraceSource.TraceComponent;
		}
	}
	State.EffectComponent = Cast<UNiagaraComponent>(Notify.SpawnEffect(MeshComp, Anim));
	State.World = MeshComp->GetWorld();
	if (const FAnimNotifyEvent* NotifyEvent = EventReference.GetNotify())
	{
		State.NotifyStartTime = NotifyEvent->GetTriggerTime();
		State.NotifyEndTime = NotifyEvent->GetEndTriggerTime();
		State.LastSampleTime = State.NotifyStartTime;
	}
	State.PreviousRootWorldTransform = MeshComp->GetBoneTransform(0);
	State.bHasPreviousRootWorldTransform = true;

	const ACharacterBase* Character = Cast<ACharacterBase>(MeshComp->GetOwner());
	const UAnimSequence* Sequence = Cast<UAnimSequence>(Anim);
	if (Character && Sequence && MeshComp->GetWorld() && MeshComp->GetWorld()->GetGameInstance())
	{
		if (UAnimBoneDataSubsystem* Subsystem = MeshComp->GetWorld()->GetGameInstance()
			->GetSubsystem<UAnimBoneDataSubsystem>())
		{
			State.Segment = Subsystem->GetAnimBoneData(
				Character->GetCharacterProfileTag(), Sequence, Notify.WindowName);
		}
	}

	if (!State.Segment)
	{
		UE_LOG(Log_Anim, Warning,
			TEXT("[WeaponTrail] Missing baked trajectory. Profile=%s Animation=%s Window=%s; using live socket fallback."),
			Character ? *Character->GetCharacterProfileTag().ToString() : TEXT("None"),
			*GetNameSafe(Sequence), *Notify.WindowName.ToString());
	}

	Add(RuntimeKey, MoveTemp(State));
	if (FWeaponTrailRuntime* AddedState = Find(RuntimeKey))
	{
		if (UWorld* World = AddedState->World.Get())
		{
			// 정상적인 공격 트레일보다 충분히 긴 유예를 두되, NotifyEnd가 유실된 상태는 영구히 남기지 않는다.
			const float CleanupDelay = FMath::Max(TotalDuration + Notify.TrailFadeDuration + 1.0f, 5.0f);
			TWeakObjectPtr<UANS_TimedWeaponTrail> WeakThis(&Notify);
			World->GetTimerManager().SetTimer(
				AddedState->CleanupTimerHandle,
				[WeakThis, RuntimeKey]()
				{
					if (UANS_TimedWeaponTrail* TrailNotify = WeakThis.Get())
					{
						if (TrailNotify->RuntimeManager)
						{
							TrailNotify->RuntimeManager->Remove(RuntimeKey, true);
						}
					}
				},
				CleanupDelay, false);
		}
	}
}

void FWeaponTrailRuntimeManager::Tick(UANS_TimedWeaponTrail& Notify, USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	PruneInvalid();
	FWeaponTrailRuntime* State = Find(MakeKey(MeshComp, EventReference));
	UObject* WeaponSource = State ? State->WeaponSource.Get() : nullptr;

	if (WeaponSource && State)
	{
		if(UNiagaraComponent* NiagaraComponent = State->EffectComponent.Get())
		{
			const FName StartSocket = IWeaponRuntimeSourceInterface::Execute_GetWeaponTrailStartSocket(WeaponSource, Notify.bSubWeapon);
			const FName EndSocket = IWeaponRuntimeSourceInterface::Execute_GetWeaponTrailEndSocket(WeaponSource, Notify.bSubWeapon);
			const FVector TrailStart = IWeaponRuntimeSourceInterface::Execute_GetWeaponSocketLocation(WeaponSource, StartSocket, Notify.bSubWeapon);
			const FVector TrailEnd = IWeaponRuntimeSourceInterface::Execute_GetWeaponSocketLocation(WeaponSource, EndSocket, Notify.bSubWeapon);

			NiagaraComponent->SetVectorParameter(TrailStartParameter, TrailStart);
			NiagaraComponent->SetVectorParameter(TrailEndParameter, TrailEnd);

			if (NiagaraComponent && State && State->Segment)
			{
				const FTransform CurrentRootWorldTransform = MeshComp->GetBoneTransform(0);
				const FTransform& PreviousRootWorldTransform = State->bHasPreviousRootWorldTransform
					? State->PreviousRootWorldTransform
					: CurrentRootWorldTransform;
				const FAnimNotifyEvent* NotifyEvent = EventReference.GetNotify();
				const float NotifyEndTime = NotifyEvent
					? NotifyEvent->GetEndTriggerTime()
					: State->Segment->EndTime;
				const float SamplingEndTime = FMath::Min(
					NotifyEndTime, State->Segment->EndTime);
				float StartDistanceWeight;
				float EndDistanceWeight;
				FWeaponTrailSampler::NormalizeDistanceWeights(Notify.TrailStartDistanceWeight, Notify.TrailEndDistanceWeight,
					StartDistanceWeight, EndDistanceWeight);
				if (!State->bHasResolvedTotalTrajectoryDistance)
				{
					const FWeaponTrajectoryGeometry Geometry = FWeaponTrajectoryUtility::BuildGeometry(
						MeshComp, State->TraceComponent.Get(),
						State->Segment->BoneName, StartSocket, EndSocket);
					// 장비가 아직 준비되지 않은 일시적인 실패는 완료로 확정하지 않는다.
					// Geometry가 유효한 경우에는 실제 거리가 0이어도 계산 완료 상태로 기록한다.
					if (Geometry.IsValid())
					{
						const float TrajectoryStartTime = FMath::Max(
							State->NotifyStartTime, State->Segment->StartTime);
						const FWeaponTrailDistanceCacheKey CacheKey = BuildTrajectoryDistanceCacheKey(
							Geometry, *State->Segment, TrajectoryStartTime, SamplingEndTime,
							Notify.TrajectorySampleInterval, StartDistanceWeight, EndDistanceWeight);
						if (FWeaponTrailDistanceCacheEntry* CachedEntry =
							FindDistance(CacheKey))
						{
							State->TotalTrajectoryDistance = CachedEntry->Distance;
						}
						else
						{
							State->TotalTrajectoryDistance = FWeaponTrailSampler::CalculateTotalTrajectoryDistance(
								Geometry, *State->Segment, TrajectoryStartTime, SamplingEndTime,
								Notify.TrajectorySampleInterval, StartDistanceWeight, EndDistanceWeight);

							AddDistance(CacheKey, State->TotalTrajectoryDistance);
						}
						State->bHasResolvedTotalTrajectoryDistance = true;
					}
				}
				const float StartTime = FMath::Clamp(
					State->LastSampleTime, State->Segment->StartTime, SamplingEndTime);
				const float EndTime = FMath::Clamp(
					StartTime + FrameDeltaTime,
					State->Segment->StartTime, SamplingEndTime);
				State->StartSamples.Reset();
				State->EndSamples.Reset();
				State->TrailUSamples.Reset();
				State->LinkOrderSamples.Reset();
				if (EndTime > StartTime && BuildWeaponTrailSamples(
					MeshComp, State->TraceComponent.Get(), *State->Segment, StartTime, EndTime,
					Notify.TrajectorySampleInterval, Notify.MaxSamplesPerFrame, State->bNeedsInitialSample, Notify.bSubWeapon,
					StartSocket, EndSocket, State->StartSamples, State->EndSamples, State->TrailUSamples,
					FMath::Max(State->NotifyStartTime, State->Segment->StartTime), SamplingEndTime,
					StartDistanceWeight, EndDistanceWeight, *State,
					PreviousRootWorldTransform, CurrentRootWorldTransform))
				{
					if (Notify.bDebugDrawTrajectory && MeshComp->GetWorld())
					{
						for (int32 Index = 0; Index < State->StartSamples.Num(); ++Index)
						{
							const FVector& SampleStart = State->StartSamples[Index];
							const FVector& SampleEnd = State->EndSamples[Index];
							const FVector SampleCenter = (SampleStart + SampleEnd) * 0.5f;
							DrawDebugLine(MeshComp->GetWorld(), SampleStart, SampleEnd,
								FColor::Yellow, false, Notify.DebugDrawDuration, 0, 0.75f);
							DrawDebugPoint(MeshComp->GetWorld(), SampleCenter, 4.0f,
								FColor::Green, false, Notify.DebugDrawDuration, 0);

							if (State->bHasPreviousDebugSample)
							{
								DrawDebugLine(MeshComp->GetWorld(), State->PreviousDebugStart,
									SampleStart, FColor::Red, false, Notify.DebugDrawDuration, 0, 1.0f);
								DrawDebugLine(MeshComp->GetWorld(), State->PreviousDebugEnd,
									SampleEnd, FColor::Blue, false, Notify.DebugDrawDuration, 0, 1.0f);
							}

							State->PreviousDebugStart = SampleStart;
							State->PreviousDebugEnd = SampleEnd;
							State->bHasPreviousDebugSample = true;
						}
					}

					for (int32 Index = 0; Index < State->StartSamples.Num(); ++Index)
					{
						State->LinkOrderSamples.Add(static_cast<float>(State->NextLinkOrder + Index));
					}

					State->UploadSamples(EndTime - StartTime);
					State->NextLinkOrder += State->StartSamples.Num();
					State->bNeedsInitialSample = false;
				}
				State->LastSampleTime = EndTime;
				State->PreviousRootWorldTransform = CurrentRootWorldTransform;
				State->bHasPreviousRootWorldTransform = true;
			}
			else if (NiagaraComponent)
			{
				if (State)
				{
					const float NextSampleTime =
						State->LastSampleTime + FMath::Max(FrameDeltaTime, 0.0f);
					State->LastSampleTime = State->NotifyEndTime > State->NotifyStartTime
						? FMath::Min(NextSampleTime, State->NotifyEndTime)
						: NextSampleTime;
				}

				State->StartSamples.Reset();
				State->EndSamples.Reset();
				State->TrailUSamples.Reset();
				State->LinkOrderSamples.Reset();
				State->StartSamples.Add(TrailStart);
				State->EndSamples.Add(TrailEnd);
				State->LinkOrderSamples.Add(static_cast<float>(State->NextLinkOrder));
				const float TrailDuration = State
					? FMath::Max(State->NotifyEndTime - State->NotifyStartTime, UE_SMALL_NUMBER)
					: 1.0f;
				State->TrailUSamples.Add(State
					? FMath::Clamp((State->LastSampleTime - State->NotifyStartTime) / TrailDuration, 0.0f, 1.0f)
					: 0.0f);
				State->UploadSamples(0.0f);
				if (State)
				{
					++State->NextLinkOrder;
				}
			}
		}
	}
}

void FWeaponTrailRuntimeManager::End(UANS_TimedWeaponTrail& Notify, USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, const FAnimNotifyEventReference& EventReference)
{
	FWeaponTrailRuntimeKey RuntimeKey = FWeaponTrailRuntimeManager::MakeKey(MeshComp, EventReference);
	FWeaponTrailRuntime* State = Find(RuntimeKey, &RuntimeKey);
	if (State) State->Finish(Notify.TrailFadeDuration, Notify.bDestroyAtEnd);

	if (Notify.bDebugDrawTrajectory)
	{
		const FWeaponTrailRuntime* DebugState = State;
		const FAnimNotifyEvent* NotifyEvent = EventReference.GetNotify();
		const float NotifyStartTime = NotifyEvent ? NotifyEvent->GetTriggerTime() : -1.0f;
		const float NotifyEndTime = NotifyEvent ? NotifyEvent->GetEndTriggerTime() : -1.0f;
		const float LastSampleTime = DebugState ? DebugState->LastSampleTime : -1.0f;
		const float SegmentStartTime = DebugState && DebugState->Segment ? DebugState->Segment->StartTime : -1.0f;
		const float SegmentEndTime = DebugState && DebugState->Segment ? DebugState->Segment->EndTime : -1.0f;

		UE_LOG(Log_Anim, Warning,
			TEXT("[WeaponTrailDebug] Anim=%s Notify=[%.6f, %.6f] LastSample=%.6f Overshoot=%.6f Segment=[%.6f, %.6f]"),
			*GetNameSafe(Anim), NotifyStartTime, NotifyEndTime, LastSampleTime,
			LastSampleTime - NotifyEndTime, SegmentStartTime, SegmentEndTime);
	}

	Remove(RuntimeKey, false);
}

bool FWeaponTrailRuntimeKey::operator==(const FWeaponTrailRuntimeKey& Other) const
{
	return MeshComp == Other.MeshComp && NotifyEvent == Other.NotifyEvent &&
		NotifySource == Other.NotifySource && MontageInstanceId == Other.MontageInstanceId;
}

uint32 GetTypeHash(const FWeaponTrailRuntimeKey& Key)
{
	uint32 Hash = GetTypeHash(Key.MeshComp);
	Hash = HashCombine(Hash, PointerHash(Key.NotifyEvent));
	Hash = HashCombine(Hash, GetTypeHash(Key.NotifySource));
	return HashCombine(Hash, GetTypeHash(Key.MontageInstanceId));
}

bool FWeaponTrailDistanceCacheKey::operator==(const FWeaponTrailDistanceCacheKey& Other) const
{
	return Segment == Other.Segment &&
		WeaponRelativeToBone.GetTranslation() == Other.WeaponRelativeToBone.GetTranslation() &&
		WeaponRelativeToBone.GetRotation() == Other.WeaponRelativeToBone.GetRotation() &&
		WeaponRelativeToBone.GetScale3D() == Other.WeaponRelativeToBone.GetScale3D() &&
		StartSocketInWeapon == Other.StartSocketInWeapon && EndSocketInWeapon == Other.EndSocketInWeapon &&
		StartTime == Other.StartTime && EndTime == Other.EndTime && SampleInterval == Other.SampleInterval &&
		StartWeight == Other.StartWeight && EndWeight == Other.EndWeight;
}

uint32 GetTypeHash(const FWeaponTrailDistanceCacheKey& Key)
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

void FWeaponTrailRuntime::UploadSamples(float BatchDuration)
{
	UNiagaraComponent* Effect = EffectComponent.Get();
	if (!Effect) return;
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayPosition(Effect, TrailStartSamplesParameter, StartSamples);
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayPosition(Effect, TrailEndSamplesParameter, EndSamples);
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(Effect, TrailLinkOrderSamplesParameter, LinkOrderSamples);
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(Effect, TrailUSamplesParameter, TrailUSamples);
	Effect->SetVariableFloat(TrailBatchDurationParameter, BatchDuration);
	Effect->SetVariableInt(TrailSampleCountParameter, StartSamples.Num());
}

void FWeaponTrailRuntime::Finish(float FadeDuration, bool bDestroyImmediately)
{
	if (UWorld* RuntimeWorld = World.Get()) RuntimeWorld->GetTimerManager().ClearTimer(CleanupTimerHandle);
	UNiagaraComponent* Effect = EffectComponent.Get();
	if (!Effect) return;
	Effect->SetVariableFloat(TrailFadeDurationParameter, FadeDuration);
	Effect->SetVariableBool(TrailIsEndingParameter, true);
	if (bDestroyImmediately) Effect->DestroyComponent();
	else Effect->Deactivate();
}

void FWeaponTrailSampler::NormalizeDistanceWeights(float StartWeight, float EndWeight,
	float& OutStartWeight, float& OutEndWeight)
{
	const float SafeStart = FMath::Max(StartWeight, 0.0f);
	const float SafeEnd = FMath::Max(EndWeight, 0.0f);
	const float Sum = SafeStart + SafeEnd;
	OutStartWeight = Sum > UE_SMALL_NUMBER ? SafeStart / Sum : 0.5f;
	OutEndWeight = Sum > UE_SMALL_NUMBER ? SafeEnd / Sum : 0.5f;
}

float FWeaponTrailSampler::CalculateTotalTrajectoryDistance(
	const FWeaponTrajectoryGeometry& Geometry, const FBoneTransformSegment& Segment,
	float StartTime, float EndTime, float SampleInterval, float StartWeight, float EndWeight)
{
	if (!Geometry.IsValid() || EndTime <= StartTime) return 0.0f;
	FVector PreviousStart, PreviousEnd;
	FWeaponTrajectoryUtility::GetSocketWorldPositions(
		Geometry, Segment.GetTransformAtTime(StartTime), FTransform::Identity, PreviousStart, PreviousEnd);
	float Distance = 0.0f;
	for (float Time = StartTime; Time < EndTime;)
	{
		const float NextTime = FMath::Min(Time + FMath::Max(SampleInterval, 0.001f), EndTime);
		if (NextTime <= Time) break;
		FVector Start, End;
		FWeaponTrajectoryUtility::GetSocketWorldPositions(
			Geometry, Segment.GetTransformAtTime(NextTime), FTransform::Identity, Start, End);
		Distance += FVector::Distance(PreviousStart, Start) * StartWeight +
			FVector::Distance(PreviousEnd, End) * EndWeight;
		PreviousStart = Start;
		PreviousEnd = End;
		Time = NextTime;
	}
	return Distance;
}

FWeaponTrailRuntimeKey FWeaponTrailRuntimeManager::MakeKey(
	USkeletalMeshComponent* MeshComp, const FAnimNotifyEventReference& EventReference)
{
	FWeaponTrailRuntimeKey Key;
	Key.MeshComp = MeshComp;
	Key.NotifyEvent = EventReference.GetNotify();
	Key.NotifySource = EventReference.GetSourceObject();
	if (const UE::Anim::FAnimNotifyMontageInstanceContext* Context =
		EventReference.GetContextData<UE::Anim::FAnimNotifyMontageInstanceContext>())
	{
		Key.MontageInstanceId = Context->MontageInstanceID;
	}
	return Key;
}

FWeaponTrailRuntime* FWeaponTrailRuntimeManager::Find(
	const FWeaponTrailRuntimeKey& Key, FWeaponTrailRuntimeKey* OutResolvedKey)
{
	if (FWeaponTrailRuntime* Exact = Runtimes.Find(Key))
	{
		if (OutResolvedKey) *OutResolvedKey = Key;
		return Exact;
	}
	FWeaponTrailRuntimeKey CandidateKey;
	FWeaponTrailRuntime* Candidate = nullptr;
	for (TPair<FWeaponTrailRuntimeKey, FWeaponTrailRuntime>& Pair : Runtimes)
	{
		if (Pair.Key.MeshComp != Key.MeshComp) continue;
		if (Candidate) return nullptr;
		CandidateKey = Pair.Key;
		Candidate = &Pair.Value;
	}
	if (Candidate && OutResolvedKey) *OutResolvedKey = CandidateKey;
	return Candidate;
}

void FWeaponTrailRuntimeManager::Add(const FWeaponTrailRuntimeKey& Key, FWeaponTrailRuntime&& Runtime)
{
	Remove(Key, true);
	Runtimes.Add(Key, MoveTemp(Runtime));
}

void FWeaponTrailRuntimeManager::Remove(const FWeaponTrailRuntimeKey& Key, bool bDestroyEffect)
{
	FWeaponTrailRuntime* Runtime = Runtimes.Find(Key);
	if (!Runtime) return;
	if (UWorld* World = Runtime->World.Get()) World->GetTimerManager().ClearTimer(Runtime->CleanupTimerHandle);
	if (bDestroyEffect)
	{
		if (UNiagaraComponent* Effect = Runtime->EffectComponent.Get()) Effect->DestroyComponent();
	}
	Runtimes.Remove(Key);
}

void FWeaponTrailRuntimeManager::PruneInvalid()
{
	TArray<FWeaponTrailRuntimeKey> Invalid;
	for (const TPair<FWeaponTrailRuntimeKey, FWeaponTrailRuntime>& Pair : Runtimes)
	{
		if (!Pair.Key.MeshComp.IsValid() || !Pair.Value.World.IsValid() || !Pair.Value.EffectComponent.IsValid())
			Invalid.Add(Pair.Key);
	}
	for (const FWeaponTrailRuntimeKey& Key : Invalid) Remove(Key, true);
}

FWeaponTrailDistanceCacheEntry* FWeaponTrailRuntimeManager::FindDistance(const FWeaponTrailDistanceCacheKey& Key)
{
	FWeaponTrailDistanceCacheEntry* Entry = DistanceCache.Find(Key);
	if (Entry) Entry->LastAccessSerial = ++AccessSerial;
	return Entry;
}

void FWeaponTrailRuntimeManager::AddDistance(const FWeaponTrailDistanceCacheKey& Key, float Distance)
{
	if (DistanceCache.Num() >= MaxDistanceCacheEntries)
	{
		const FWeaponTrailDistanceCacheKey* OldestKey = nullptr;
		uint64 OldestSerial = MAX_uint64;
		for (const TPair<FWeaponTrailDistanceCacheKey, FWeaponTrailDistanceCacheEntry>& Pair : DistanceCache)
		{
			if (Pair.Value.LastAccessSerial < OldestSerial) { OldestKey = &Pair.Key; OldestSerial = Pair.Value.LastAccessSerial; }
		}
		if (OldestKey) DistanceCache.Remove(*OldestKey);
	}
	DistanceCache.Add(Key, { Distance, ++AccessSerial });
}

FWeaponTrailRuntimeManager::~FWeaponTrailRuntimeManager()
{
	TArray<FWeaponTrailRuntimeKey> Keys;
	Runtimes.GetKeys(Keys);
	for (const FWeaponTrailRuntimeKey& Key : Keys)
	{
		Remove(Key, true);
	}
}
