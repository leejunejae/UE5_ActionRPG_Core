// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/ANS_TimedWeaponTrail.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/ActiveMontageInstanceScope.h"
#include "Characters/CharacterBase.h"
#include "Characters/Interfaces/EquipmentDataInterface.h"
#include "Combat/Data/AttackData.h"
#include "Combat/Interfaces/AttackSourceInterface.h"
#include "Core/Subsystems/GameInstanceSystem/AnimBoneDataSubsystem.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"
#include "Utils/AttackBoneDataRegistry.h"
#include "Utils/CoreLog.h"
#include "Utils/WeaponTrajectoryUtility.h"

namespace
{
	// 이 Notify는 프로젝트 공용 Niagara Trail 규약만 사용한다.
	// 애셋별로 변경할 이유가 없는 파라미터 이름은 에디터 설정으로 노출하지 않는다.
	const FName TrailStartParameter(TEXT("TrailStart"));
	const FName TrailEndParameter(TEXT("TrailEnd"));
	const FName TrailMaterialParameter(TEXT("User.TrailMaterial"));
	const FName TrailStartSamplesParameter(TEXT("User.TrailStartSamples"));
	const FName TrailEndSamplesParameter(TEXT("User.TrailEndSamples"));
	const FName TrailSampleCountParameter(TEXT("User.TrailSampleCount"));
	const FName TrailBatchDurationParameter(TEXT("User.TrailBatchDuration"));
	const FName TrailLinkOrderSamplesParameter(TEXT("User.TrailLinkOrderSamples"));
	const FName TrailUSamplesParameter(TEXT("User.TrailUSamples"));
	const FName TrailIsEndingParameter(TEXT("User.TrailIsEnding"));
	const FName TrailFadeDurationParameter(TEXT("User.TrailFadeDuration"));

	void NormalizeDistanceWeights(float StartWeight, float EndWeight,
		float& OutStartWeight, float& OutEndWeight)
	{
		const float SafeStartWeight = FMath::Max(StartWeight, 0.0f);
		const float SafeEndWeight = FMath::Max(EndWeight, 0.0f);
		const float WeightSum = SafeStartWeight + SafeEndWeight;
		if (WeightSum <= UE_SMALL_NUMBER)
		{
			OutStartWeight = 0.5f;
			OutEndWeight = 0.5f;
			return;
		}

		OutStartWeight = SafeStartWeight / WeightSum;
		OutEndWeight = SafeEndWeight / WeightSum;
	}

	float GetWeightedTrajectoryDistance(
		const FVector& PreviousStart, const FVector& PreviousEnd,
		const FVector& CurrentStart, const FVector& CurrentEnd,
		float StartWeight, float EndWeight)
	{
		return FVector::Distance(PreviousStart, CurrentStart) * StartWeight +
			FVector::Distance(PreviousEnd, CurrentEnd) * EndWeight;
	}

	float CalculateTotalTrajectoryDistance(
		const FWeaponTrajectoryGeometry& Geometry,
		const FBoneTransformSegment& Segment,
		float StartTime,
		float EndTime,
		float SampleInterval,
		float StartWeight,
		float EndWeight)
	{
		if (!Geometry.IsValid() || EndTime <= StartTime) return 0.0f;

		const FTransform IdentityRoot = FTransform::Identity;
		FVector PreviousStart;
		FVector PreviousEnd;
		FWeaponTrajectoryUtility::GetSocketWorldPositions(
			Geometry, Segment.GetTransformAtTime(StartTime), IdentityRoot,
			PreviousStart, PreviousEnd);

		float TotalDistance = 0.0f;
		const float SafeInterval = FMath::Max(SampleInterval, 0.001f);
		for (float SampleTime = StartTime; SampleTime < EndTime;)
		{
			const float NextSampleTime = FMath::Min(SampleTime + SafeInterval, EndTime);
			if (NextSampleTime <= SampleTime) break;
			FVector CurrentStart;
			FVector CurrentEnd;
			FWeaponTrajectoryUtility::GetSocketWorldPositions(
				Geometry, Segment.GetTransformAtTime(NextSampleTime), IdentityRoot,
				CurrentStart, CurrentEnd);
			TotalDistance += GetWeightedTrajectoryDistance(
				PreviousStart, PreviousEnd, CurrentStart, CurrentEnd,
				StartWeight, EndWeight);
			PreviousStart = CurrentStart;
			PreviousEnd = CurrentEnd;
			SampleTime = NextSampleTime;
		}

		return TotalDistance;
	}

	FWeaponTrailDistanceCacheKey BuildTrajectoryDistanceCacheKey(
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

	UActorComponent* FindEquipmentDataComponent(const USkeletalMeshComponent* MeshComp)
	{
		AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
		if (!Owner) return nullptr;

		for (UActorComponent* Component : Owner->GetComponents())
		{
			if (Component && Component->GetClass()->ImplementsInterface(UEquipmentDataInterface::StaticClass()))
			{
				return Component;
			}
		}
		return nullptr;
	}

	bool BuildWeaponTrailSamples(
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
		FWeaponTrailRuntimeState& RuntimeState,
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
}

FWeaponTrailRuntimeKey UANS_TimedWeaponTrail::MakeRuntimeKey(
	USkeletalMeshComponent* MeshComp,
	const FAnimNotifyEventReference& EventReference) const
{
	FWeaponTrailRuntimeKey Key;
	Key.MeshComp = MeshComp;
	Key.NotifyEvent = EventReference.GetNotify();
	Key.NotifySource = EventReference.GetSourceObject();
	if (const UE::Anim::FAnimNotifyMontageInstanceContext* MontageContext =
		EventReference.GetContextData<UE::Anim::FAnimNotifyMontageInstanceContext>())
	{
		Key.MontageInstanceId = MontageContext->MontageInstanceID;
	}
	return Key;
}

FWeaponTrailRuntimeState* UANS_TimedWeaponTrail::FindRuntimeState(
	const FWeaponTrailRuntimeKey& RequestedKey,
	FWeaponTrailRuntimeKey* OutResolvedKey)
{
	if (FWeaponTrailRuntimeState* ExactState = RuntimeStates.Find(RequestedKey))
	{
		if (OutResolvedKey) *OutResolvedKey = RequestedKey;
		return ExactState;
	}

	// NotifyBegin/Tick/End 사이에 Montage context가 제공되지 않는 프레임이 있을 수 있다.
	// 동일 메시에서 후보가 하나뿐일 때만 fallback하여 다른 중첩 Trail을 잘못 선택하지 않는다.
	FWeaponTrailRuntimeKey CandidateKey;
	FWeaponTrailRuntimeState* CandidateState = nullptr;
	for (TPair<FWeaponTrailRuntimeKey, FWeaponTrailRuntimeState>& Pair : RuntimeStates)
	{
		if (Pair.Key.MeshComp != RequestedKey.MeshComp) continue;
		if (CandidateState) return nullptr;
		CandidateKey = Pair.Key;
		CandidateState = &Pair.Value;
	}

	if (CandidateState && OutResolvedKey) *OutResolvedKey = CandidateKey;
	return CandidateState;
}

void UANS_TimedWeaponTrail::RemoveRuntimeState(
	const FWeaponTrailRuntimeKey& RuntimeKey, bool bDestroyEffect)
{
	FWeaponTrailRuntimeState* State = RuntimeStates.Find(RuntimeKey);
	if (!State) return;

	if (UWorld* World = State->World.Get())
	{
		World->GetTimerManager().ClearTimer(State->CleanupTimerHandle);
	}
	if (bDestroyEffect)
	{
		if (UNiagaraComponent* NiagaraComponent = State->EffectComponent.Get())
		{
			NiagaraComponent->DestroyComponent();
		}
	}
	RuntimeStates.Remove(RuntimeKey);
}

void UANS_TimedWeaponTrail::PruneInvalidRuntimeStates()
{
	TArray<FWeaponTrailRuntimeKey> InvalidKeys;
	for (const TPair<FWeaponTrailRuntimeKey, FWeaponTrailRuntimeState>& Pair : RuntimeStates)
	{
		if (!Pair.Key.MeshComp.IsValid() || !Pair.Value.World.IsValid() ||
			!Pair.Value.EffectComponent.IsValid())
		{
			InvalidKeys.Add(Pair.Key);
		}
	}

	for (const FWeaponTrailRuntimeKey& InvalidKey : InvalidKeys)
	{
		RemoveRuntimeState(InvalidKey, true);
	}
}

void UANS_TimedWeaponTrail::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, float TotalDuration, const FAnimNotifyEventReference& EventReference)
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
	PruneInvalidRuntimeStates();
	const FWeaponTrailRuntimeKey RuntimeKey = MakeRuntimeKey(MeshComp, EventReference);
	// 같은 실행 키가 비정상적으로 남아 있으면 새 Effect로 덮어쓰기 전에 정리한다.
	RemoveRuntimeState(RuntimeKey, true);

	FWeaponTrailRuntimeState State;
	State.StartSamples.Reserve(FMath::Max(1, MaxSamplesPerFrame));
	State.EndSamples.Reserve(FMath::Max(1, MaxSamplesPerFrame));
	State.TrailUSamples.Reserve(FMath::Max(1, MaxSamplesPerFrame));
	State.LinkOrderSamples.Reserve(FMath::Max(1, MaxSamplesPerFrame));
	State.EquipmentComponent = FindEquipmentDataComponent(MeshComp);
	if (ACharacter* OwnerCharacter = Cast<ACharacter>(MeshComp->GetOwner()))
	{
		if (IAttackSourceInterface* AttackSource = Cast<IAttackSourceInterface>(OwnerCharacter))
		{
			const FAttackTraceSource TraceSource = AttackSource->GetAttackTraceSource(bSubWeapon
				? EAttackSourceType::OffHand : EAttackSourceType::MainHand);
			State.TraceComponent = TraceSource.TraceComponent;
		}
	}
	State.EffectComponent = Cast<UNiagaraComponent>(SpawnEffect(MeshComp, Anim));
	State.World = MeshComp->GetWorld();
	if (const FAnimNotifyEvent* Notify = EventReference.GetNotify())
	{
		State.NotifyStartTime = Notify->GetTriggerTime();
		State.NotifyEndTime = Notify->GetEndTriggerTime();
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
				Character->GetCharacterProfileTag(), Sequence, WindowName);
		}
	}

	if (!State.Segment)
	{
		UE_LOG(Log_Anim, Warning,
			TEXT("[WeaponTrail] Missing baked trajectory. Profile=%s Animation=%s Window=%s; using live socket fallback."),
			Character ? *Character->GetCharacterProfileTag().ToString() : TEXT("None"),
			*GetNameSafe(Sequence), *WindowName.ToString());
	}

	RuntimeStates.Add(RuntimeKey, MoveTemp(State));
	if (FWeaponTrailRuntimeState* AddedState = RuntimeStates.Find(RuntimeKey))
	{
		if (UWorld* World = AddedState->World.Get())
		{
			// 정상적인 공격 트레일보다 충분히 긴 유예를 두되, NotifyEnd가 유실된 상태는 영구히 남기지 않는다.
			const float CleanupDelay = FMath::Max(TotalDuration + TrailFadeDuration + 1.0f, 5.0f);
			TWeakObjectPtr<UANS_TimedWeaponTrail> WeakThis(this);
			World->GetTimerManager().SetTimer(
				AddedState->CleanupTimerHandle,
				[WeakThis, RuntimeKey]()
				{
					if (UANS_TimedWeaponTrail* TrailNotify = WeakThis.Get())
					{
						TrailNotify->RemoveRuntimeState(RuntimeKey, true);
					}
				},
				CleanupDelay, false);
		}
	}
}

void UANS_TimedWeaponTrail::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	PruneInvalidRuntimeStates();
	FWeaponTrailRuntimeState* State = FindRuntimeState(MakeRuntimeKey(MeshComp, EventReference));
	UActorComponent* EquipmentComponent = State ? State->EquipmentComponent.Get() : nullptr;

	if (EquipmentComponent && State)
	{
		if(UNiagaraComponent* NiagaraComponent = State->EffectComponent.Get())
		{
			const FName StartSocket = IEquipmentDataInterface::Execute_GetWeaponTrailStartSocket(EquipmentComponent, bSubWeapon);
			const FName EndSocket = IEquipmentDataInterface::Execute_GetWeaponTrailEndSocket(EquipmentComponent, bSubWeapon);
			const FVector TrailStart = IEquipmentDataInterface::Execute_GetWeaponSocketLocation(EquipmentComponent, StartSocket, bSubWeapon);
			const FVector TrailEnd = IEquipmentDataInterface::Execute_GetWeaponSocketLocation(EquipmentComponent, EndSocket, bSubWeapon);

			NiagaraComponent->SetVectorParameter(TrailStartParameter, TrailStart);
			NiagaraComponent->SetVectorParameter(TrailEndParameter, TrailEnd);

			if (NiagaraComponent && State && State->Segment)
			{
				const FTransform CurrentRootWorldTransform = MeshComp->GetBoneTransform(0);
				const FTransform& PreviousRootWorldTransform = State->bHasPreviousRootWorldTransform
					? State->PreviousRootWorldTransform
					: CurrentRootWorldTransform;
				const FAnimNotifyEvent* Notify = EventReference.GetNotify();
				const float NotifyEndTime = Notify
					? Notify->GetEndTriggerTime()
					: State->Segment->EndTime;
				const float SamplingEndTime = FMath::Min(
					NotifyEndTime, State->Segment->EndTime);
				float StartDistanceWeight;
				float EndDistanceWeight;
				NormalizeDistanceWeights(TrailStartDistanceWeight, TrailEndDistanceWeight,
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
							TrajectorySampleInterval, StartDistanceWeight, EndDistanceWeight);
						if (FWeaponTrailDistanceCacheEntry* CachedEntry =
							TrajectoryDistanceCache.Find(CacheKey))
						{
							CachedEntry->LastAccessSerial = ++TrajectoryDistanceCacheAccessSerial;
							State->TotalTrajectoryDistance = CachedEntry->Distance;
						}
						else
						{
							State->TotalTrajectoryDistance = CalculateTotalTrajectoryDistance(
								Geometry, *State->Segment, TrajectoryStartTime, SamplingEndTime,
								TrajectorySampleInterval, StartDistanceWeight, EndDistanceWeight);

							if (TrajectoryDistanceCache.Num() >= MaxTrajectoryDistanceCacheEntries)
							{
								FWeaponTrailDistanceCacheKey OldestKey;
								uint64 OldestSerial = MAX_uint64;
								bool bFoundOldestEntry = false;
								for (const TPair<FWeaponTrailDistanceCacheKey,
									FWeaponTrailDistanceCacheEntry>& Pair : TrajectoryDistanceCache)
								{
									if (Pair.Value.LastAccessSerial < OldestSerial)
									{
										OldestKey = Pair.Key;
										OldestSerial = Pair.Value.LastAccessSerial;
										bFoundOldestEntry = true;
									}
								}
								if (bFoundOldestEntry)
								{
									TrajectoryDistanceCache.Remove(OldestKey);
								}
							}

							FWeaponTrailDistanceCacheEntry NewEntry;
							NewEntry.Distance = State->TotalTrajectoryDistance;
							NewEntry.LastAccessSerial = ++TrajectoryDistanceCacheAccessSerial;
							TrajectoryDistanceCache.Add(CacheKey, NewEntry);
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
					TrajectorySampleInterval, MaxSamplesPerFrame, State->bNeedsInitialSample, bSubWeapon,
					StartSocket, EndSocket, State->StartSamples, State->EndSamples, State->TrailUSamples,
					FMath::Max(State->NotifyStartTime, State->Segment->StartTime), SamplingEndTime,
					StartDistanceWeight, EndDistanceWeight, *State,
					PreviousRootWorldTransform, CurrentRootWorldTransform))
				{
					if (bDebugDrawTrajectory && MeshComp->GetWorld())
					{
						for (int32 Index = 0; Index < State->StartSamples.Num(); ++Index)
						{
							const FVector& SampleStart = State->StartSamples[Index];
							const FVector& SampleEnd = State->EndSamples[Index];
							const FVector SampleCenter = (SampleStart + SampleEnd) * 0.5f;
							DrawDebugLine(MeshComp->GetWorld(), SampleStart, SampleEnd,
								FColor::Yellow, false, DebugDrawDuration, 0, 0.75f);
							DrawDebugPoint(MeshComp->GetWorld(), SampleCenter, 4.0f,
								FColor::Green, false, DebugDrawDuration, 0);

							if (State->bHasPreviousDebugSample)
							{
								DrawDebugLine(MeshComp->GetWorld(), State->PreviousDebugStart,
									SampleStart, FColor::Red, false, DebugDrawDuration, 0, 1.0f);
								DrawDebugLine(MeshComp->GetWorld(), State->PreviousDebugEnd,
									SampleEnd, FColor::Blue, false, DebugDrawDuration, 0, 1.0f);
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

					UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayPosition(
						NiagaraComponent, TrailStartSamplesParameter, State->StartSamples);
					UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayPosition(
						NiagaraComponent, TrailEndSamplesParameter, State->EndSamples);
					UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(
						NiagaraComponent, TrailLinkOrderSamplesParameter, State->LinkOrderSamples);
					UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(
						NiagaraComponent, TrailUSamplesParameter, State->TrailUSamples);
					NiagaraComponent->SetVariableFloat(
						TrailBatchDurationParameter, EndTime - StartTime);
					NiagaraComponent->SetVariableInt(TrailSampleCountParameter, State->StartSamples.Num());
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
				UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayPosition(
					NiagaraComponent, TrailStartSamplesParameter, State->StartSamples);
				UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayPosition(
					NiagaraComponent, TrailEndSamplesParameter, State->EndSamples);
				UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(
					NiagaraComponent, TrailLinkOrderSamplesParameter, State->LinkOrderSamples);
				UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(
					NiagaraComponent, TrailUSamplesParameter, State->TrailUSamples);
				NiagaraComponent->SetVariableFloat(TrailBatchDurationParameter, 0.0f);
				NiagaraComponent->SetVariableInt(TrailSampleCountParameter, 1);
				if (State)
				{
					++State->NextLinkOrder;
				}
			}
		}
	}
}

void UANS_TimedWeaponTrail::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, const FAnimNotifyEventReference& EventReference)
{
	FWeaponTrailRuntimeKey RuntimeKey = MakeRuntimeKey(MeshComp, EventReference);
	FWeaponTrailRuntimeState* State = FindRuntimeState(RuntimeKey, &RuntimeKey);
	if (State)
	{
		if (UWorld* World = State->World.Get())
		{
			World->GetTimerManager().ClearTimer(State->CleanupTimerHandle);
		}
	}
	if (UNiagaraComponent* NiagaraComponent = State ? State->EffectComponent.Get() : nullptr)
	{
		NiagaraComponent->SetVariableFloat(TrailFadeDurationParameter, TrailFadeDuration);
		NiagaraComponent->SetVariableBool(TrailIsEndingParameter, true);
		if (bDestroyAtEnd)
		{
			NiagaraComponent->DestroyComponent();
		}
		else
		{
			NiagaraComponent->Deactivate();
		}
	}

	if (bDebugDrawTrajectory)
	{
		const FWeaponTrailRuntimeState* DebugState = State;
		const FAnimNotifyEvent* Notify = EventReference.GetNotify();
		const float NotifyStartTime = Notify ? Notify->GetTriggerTime() : -1.0f;
		const float NotifyEndTime = Notify ? Notify->GetEndTriggerTime() : -1.0f;
		const float LastSampleTime = DebugState ? DebugState->LastSampleTime : -1.0f;
		const float SegmentStartTime = DebugState && DebugState->Segment ? DebugState->Segment->StartTime : -1.0f;
		const float SegmentEndTime = DebugState && DebugState->Segment ? DebugState->Segment->EndTime : -1.0f;

		UE_LOG(Log_Anim, Warning,
			TEXT("[WeaponTrailDebug] Anim=%s Notify=[%.6f, %.6f] LastSample=%.6f Overshoot=%.6f Segment=[%.6f, %.6f]"),
			*GetNameSafe(Anim), NotifyStartTime, NotifyEndTime, LastSampleTime,
			LastSampleTime - NotifyEndTime, SegmentStartTime, SegmentEndTime);
	}

	RemoveRuntimeState(RuntimeKey, false);
}

UFXSystemComponent* UANS_TimedWeaponTrail::SpawnEffect(
	USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) const
{
	UActorComponent* EquipmentComponent = FindEquipmentDataComponent(MeshComp);
	UNiagaraSystem* TrailSystem = EquipmentComponent
		? IEquipmentDataInterface::Execute_GetWeaponTrailSystem(EquipmentComponent, bSubWeapon) : nullptr;
	if (!TrailSystem)
	{
		return Super::SpawnEffect(MeshComp, Animation);
	}

	UNiagaraComponent* NewComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
		TrailSystem,
		MeshComp,
		NAME_None,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::KeepRelativeOffset,
		!bDestroyAtEnd,
		false);
	if (!NewComponent)
	{
		return nullptr;
	}

	if (UMaterialInterface* TrailMaterial =
		IEquipmentDataInterface::Execute_GetWeaponTrailMaterial(EquipmentComponent, bSubWeapon))
	{
		NewComponent->SetVariableMaterial(TrailMaterialParameter, TrailMaterial);
	}

	if (bApplyRateScaleAsTimeDilation && Animation)
	{
		NewComponent->SetCustomTimeDilation(Animation->RateScale);
	}

	const FName StartSocket = IEquipmentDataInterface::Execute_GetWeaponTrailStartSocket(EquipmentComponent, bSubWeapon);
	const FName EndSocket = IEquipmentDataInterface::Execute_GetWeaponTrailEndSocket(EquipmentComponent, bSubWeapon);
	const FVector InitialStart = IEquipmentDataInterface::Execute_GetWeaponSocketLocation(
		EquipmentComponent, StartSocket, bSubWeapon);
	const FVector InitialEnd = IEquipmentDataInterface::Execute_GetWeaponSocketLocation(
		EquipmentComponent, EndSocket, bSubWeapon);
	NewComponent->SetVectorParameter(TrailStartParameter, InitialStart);
	NewComponent->SetVectorParameter(TrailEndParameter, InitialEnd);
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayPosition(
		NewComponent, TrailStartSamplesParameter, TArray<FVector> { InitialStart });
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayPosition(
		NewComponent, TrailEndSamplesParameter, TArray<FVector> { InitialEnd });
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(
		NewComponent, TrailLinkOrderSamplesParameter, TArray<float> { 0.0f });
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(
		NewComponent, TrailUSamplesParameter, TArray<float> { 0.0f });
	NewComponent->SetVariableFloat(TrailBatchDurationParameter, 0.0f);
	NewComponent->SetVariableFloat(TrailFadeDurationParameter, TrailFadeDuration);
	NewComponent->SetVariableBool(TrailIsEndingParameter, false);
	// Notify가 시작된 현재 런타임 포즈와 베이크 궤적의 첫 점을 연결하지 않는다.
	// 첫 Particle은 첫 NotifyTick에서 정확한 베이크 시작 시간으로 생성한다.
	NewComponent->SetVariableInt(TrailSampleCountParameter, 0);
	NewComponent->SetTickBehavior(ENiagaraTickBehavior::ForceTickLast);
	NewComponent->Activate(true);
	return NewComponent;
}
