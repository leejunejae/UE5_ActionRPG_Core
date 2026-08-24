// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/ANS_TimedWeaponTrail.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSequenceBase.h"
#include "Characters/CharacterBase.h"
#include "Characters/Interfaces/EquipmentDataInterface.h"
#include "Combat/Data/AttackData.h"
#include "Combat/Interfaces/AttackSourceInterface.h"
#include "Core/Subsystems/GameInstanceSystem/AnimBoneDataSubsystem.h"
#include "DrawDebugHelpers.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Utils/AttackBoneDataRegistry.h"
#include "Utils/CoreLog.h"
#include "Utils/WeaponTrajectoryUtility.h"

namespace
{
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
		const FTransform& PreviousRootWorldTransform,
		const FTransform& CurrentRootWorldTransform)
	{
		ACharacter* Character = MeshComp ? Cast<ACharacter>(MeshComp->GetOwner()) : nullptr;
		IAttackSourceInterface* AttackSource = Character ? Cast<IAttackSourceInterface>(Character) : nullptr;
		if (!Character || !AttackSource || EndTime <= StartTime) return false;

		const EAttackSourceType SourceType = bSubWeapon
			? EAttackSourceType::OffHand : EAttackSourceType::MainHand;
		const FAttackTraceSource TraceSource = AttackSource->GetAttackTraceSource(SourceType);
		if (!TraceSource.TraceComponent) return false;

		const FWeaponTrajectoryGeometry Geometry = FWeaponTrajectoryUtility::BuildGeometry(
			MeshComp, TraceSource.TraceComponent, Segment.BoneName, StartSocket, EndSocket);
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
			OutTrailUSamples.Add(FMath::Clamp(
				(SampleTime - TrailStartTime) / TrailDuration, 0.0f, 1.0f));
		}

		return !OutStartSamples.IsEmpty();
	}
}

void UANS_TimedWeaponTrail::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Anim, TotalDuration, EventReference);
	if (!MeshComp) return;

#if WITH_EDITOR
	if (MeshComp->GetWorld() &&
		MeshComp->GetWorld()->WorldType != EWorldType::Game &&
		MeshComp->GetWorld()->WorldType != EWorldType::PIE)
	{
		return;
	}
#endif

	FWeaponTrailRuntimeState State;
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

	RuntimeStates.Add(MeshComp, State);
}

void UANS_TimedWeaponTrail::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	UActorComponent* EquipmentComponent = FindEquipmentDataComponent(MeshComp);

	if (EquipmentComponent)
	{
		if(UFXSystemComponent* TargetFX = GetSpawnedEffect(MeshComp))
		{
			const FName StartSocket = IEquipmentDataInterface::Execute_GetWeaponTrailStartSocket(EquipmentComponent, bSubWeapon);
			const FName EndSocket = IEquipmentDataInterface::Execute_GetWeaponTrailEndSocket(EquipmentComponent, bSubWeapon);
			const FVector TrailStart = IEquipmentDataInterface::Execute_GetWeaponSocketLocation(EquipmentComponent, StartSocket, bSubWeapon);
			const FVector TrailEnd = IEquipmentDataInterface::Execute_GetWeaponSocketLocation(EquipmentComponent, EndSocket, bSubWeapon);

			TargetFX->SetVectorParameter(TrailStartParameter, TrailStart);
			TargetFX->SetVectorParameter(TrailEndParameter, TrailEnd);

			UNiagaraComponent* NiagaraComponent = Cast<UNiagaraComponent>(TargetFX);
			FWeaponTrailRuntimeState* State = RuntimeStates.Find(MeshComp);
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
				const float StartTime = FMath::Clamp(
					State->LastSampleTime, State->Segment->StartTime, SamplingEndTime);
				const float EndTime = FMath::Clamp(
					StartTime + FrameDeltaTime,
					State->Segment->StartTime, SamplingEndTime);
				TArray<FVector> StartSamples;
				TArray<FVector> EndSamples;
				TArray<float> TrailUSamples;
				if (EndTime > StartTime && BuildWeaponTrailSamples(
					MeshComp, *State->Segment, StartTime, EndTime,
					TrajectorySampleInterval, MaxSamplesPerFrame, State->bNeedsInitialSample, bSubWeapon,
					StartSocket, EndSocket, StartSamples, EndSamples, TrailUSamples,
					FMath::Max(State->NotifyStartTime, State->Segment->StartTime), SamplingEndTime,
					PreviousRootWorldTransform, CurrentRootWorldTransform))
				{
					if (bDebugDrawTrajectory && MeshComp->GetWorld())
					{
						for (int32 Index = 0; Index < StartSamples.Num(); ++Index)
						{
							const FVector& SampleStart = StartSamples[Index];
							const FVector& SampleEnd = EndSamples[Index];
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

					TArray<float> LinkOrderSamples;
					LinkOrderSamples.Reserve(StartSamples.Num());
					for (int32 Index = 0; Index < StartSamples.Num(); ++Index)
					{
						LinkOrderSamples.Add(static_cast<float>(State->NextLinkOrder + Index));
					}

					UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayPosition(
						NiagaraComponent, TrailStartSamplesParameter, StartSamples);
					UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayPosition(
						NiagaraComponent, TrailEndSamplesParameter, EndSamples);
					UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(
						NiagaraComponent, TrailLinkOrderSamplesParameter, LinkOrderSamples);
					UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(
						NiagaraComponent, TrailUSamplesParameter, TrailUSamples);
					NiagaraComponent->SetVariableFloat(
						TrailBatchDurationParameter, EndTime - StartTime);
					NiagaraComponent->SetVariableInt(TrailSampleCountParameter, StartSamples.Num());
					State->NextLinkOrder += StartSamples.Num();
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

				const TArray<FVector> StartSamples { TrailStart };
				const TArray<FVector> EndSamples { TrailEnd };
				const TArray<float> LinkOrderSamples {
					State ? static_cast<float>(State->NextLinkOrder) : 0.0f };
				const float TrailDuration = State
					? FMath::Max(State->NotifyEndTime - State->NotifyStartTime, UE_SMALL_NUMBER)
					: 1.0f;
				const TArray<float> TrailUSamples { State
					? FMath::Clamp((State->LastSampleTime - State->NotifyStartTime) / TrailDuration, 0.0f, 1.0f)
					: 0.0f };
				UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayPosition(
					NiagaraComponent, TrailStartSamplesParameter, StartSamples);
				UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayPosition(
					NiagaraComponent, TrailEndSamplesParameter, EndSamples);
				UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(
					NiagaraComponent, TrailLinkOrderSamplesParameter, LinkOrderSamples);
				UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(
					NiagaraComponent, TrailUSamplesParameter, TrailUSamples);
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
	if (UNiagaraComponent* NiagaraComponent = Cast<UNiagaraComponent>(GetSpawnedEffect(MeshComp)))
	{
		NiagaraComponent->SetVariableFloat(TrailFadeDurationParameter, TrailFadeDuration);
		NiagaraComponent->SetVariableBool(TrailIsEndingParameter, true);
	}

	if (bDebugDrawTrajectory)
	{
		const FWeaponTrailRuntimeState* State = RuntimeStates.Find(MeshComp);
		const FAnimNotifyEvent* Notify = EventReference.GetNotify();
		const float NotifyStartTime = Notify ? Notify->GetTriggerTime() : -1.0f;
		const float NotifyEndTime = Notify ? Notify->GetEndTriggerTime() : -1.0f;
		const float LastSampleTime = State ? State->LastSampleTime : -1.0f;
		const float SegmentStartTime = State && State->Segment ? State->Segment->StartTime : -1.0f;
		const float SegmentEndTime = State && State->Segment ? State->Segment->EndTime : -1.0f;

		UE_LOG(Log_Anim, Warning,
			TEXT("[WeaponTrailDebug] Anim=%s Notify=[%.6f, %.6f] LastSample=%.6f Overshoot=%.6f Segment=[%.6f, %.6f]"),
			*GetNameSafe(Anim), NotifyStartTime, NotifyEndTime, LastSampleTime,
			LastSampleTime - NotifyEndTime, SegmentStartTime, SegmentEndTime);
	}

	RuntimeStates.Remove(MeshComp);
	Super::NotifyEnd(MeshComp, Anim, EventReference);
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
