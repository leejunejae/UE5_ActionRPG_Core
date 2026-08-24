// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/Components/AttackComponent.h"
#include "GameFramework/Character.h"

#include "Combat/Interfaces/HitReactionInterface.h"
#include "Combat/Interfaces/CombatInterface.h"
#include "Combat/Interfaces/AttackSourceInterface.h"

#include "GameFramework/CharacterMovementComponent.h"

#include "Engine/StaticMeshSocket.h"
#include "Utils/AttackBoneDataRegistry.h"
#include "Utils/AnimBoneDataRegistryRoot.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

#include "NiagaraSystem.h"
#include "NiagaraComponent.h"

#include "Core/Subsystems/GameInstanceSystem/AnimBoneDataSubsystem.h"
#include "Engine/GameInstance.h"
#include "GameplayTags.h"

#include "Utils/CoreLog.h"
#include "Utils/WeaponTrajectoryUtility.h"


// Sets default values for this component's properties
UAttackComponent::UAttackComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
}


// Called when the game starts
void UAttackComponent::BeginPlay()
{
	Super::BeginPlay();

	ACharacter* Character = Cast<ACharacter>(GetOwner());

	if (!Character) return;

}

void UAttackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FinishAttackSession(true, true, EActionExitReason::Interrupted);
	Super::EndPlay(EndPlayReason);
}

const FBaseAttackData* UAttackComponent::ExecuteAttack(FName AttackName, float Playrate)
{
	if (!CurAttackContextSet)
	{
		UE_LOG(Log_Attack, Warning, TEXT("[AttackComponent] Attack context set is not configured for '%s'."),
		       *GetNameSafe(GetOwner()));
		return nullptr;
	}

	bool CanPlayAttack = false;
	int32 CandidateIndex = ComboIndex;
	FAttackContext CandidateContext = CurAttackContext;

	if (CurAttackContext.AttackName == AttackName)
	{
		CanPlayAttack = true;
		CandidateIndex = CurAttackContext.AttackDetail.IsValidIndex(ComboIndex + 1) ? ComboIndex + 1 : 0;
	}
	else
	{
		if (CurAttackContextSet->Contexts.IsEmpty()) return nullptr;
		FAttackContext DataForFind;
		DataForFind.AttackName = AttackName;
		const FAttackContext* FoundData = CurAttackContextSet->Contexts.Find(DataForFind);
		CandidateContext = FoundData ? *FoundData : FAttackContext{};

		if (CandidateContext.Anim && !CandidateContext.AttackDetail.IsEmpty())
		{
			CanPlayAttack = true;
			CandidateIndex = 0;
		}
	}

	if (!CanPlayAttack || !CandidateContext.AttackDetail.IsValidIndex(CandidateIndex)) return nullptr;

	if (!PlayAnimation(CandidateContext, CandidateIndex, Playrate))
	{
		CancelAttack(EActionExitReason::Interrupted, true);
		return nullptr;
	}

	CurAttackContext = CandidateContext;
	ComboIndex = CandidateIndex;
	++AttackSessionId;
	AttackSessionState = EAttackSessionState::Active;
	ResetAttackTrace();
	HitActorListCurrentAttack.Empty();

	return &CurAttackContext.AttackDetail[ComboIndex]; // 실행한 단계 데이터
}

const FBaseAttackData* UAttackComponent::GetNextAttackData(FName AttackName) const
{
	if (!CurAttackContextSet)
	{
		return nullptr;
	}

	if (CurAttackContext.AttackName == AttackName)
	{
		const int32 CandidateIndex = CurAttackContext.AttackDetail.IsValidIndex(ComboIndex + 1)
			? ComboIndex + 1 : 0;
		return CurAttackContext.AttackDetail.IsValidIndex(CandidateIndex)
			? &CurAttackContext.AttackDetail[CandidateIndex] : nullptr;
	}

	FAttackContext DataForFind;
	DataForFind.AttackName = AttackName;
	const FAttackContext* FoundData = CurAttackContextSet->Contexts.Find(DataForFind);
	return FoundData && FoundData->Anim && FoundData->AttackDetail.IsValidIndex(0)
		? &FoundData->AttackDetail[0] : nullptr;
}

bool UAttackComponent::PlayAnimation(const FAttackContext& AttackInfo, int32 Index, float Playrate)
{
	ACharacter* Char = Cast<ACharacter>(GetOwner());
	if (!Char || !AttackInfo.Anim || !AttackInfo.AttackDetail.IsValidIndex(Index)) return false;

	UAnimInstance* Anim = Char->GetMesh()->GetAnimInstance();
	if (!Anim) return false;

	const FName SectionName = AttackInfo.AttackDetail[Index].SectionName;
	if (SectionName.IsNone() || !AttackInfo.Anim->IsValidSectionName(SectionName))
	{
		UE_LOG(Log_Attack, Warning, TEXT("[AttackComponent] Invalid attack section '%s' in montage '%s'."),
		       *SectionName.ToString(), *GetNameSafe(AttackInfo.Anim));
		return false;
	}

	// 콤보 다음 단계가 같은 몽타주를 다시 시작할 때 이전 단계의 EndDelegate가
	// 새 공격 세션을 중단 처리하지 않도록 먼저 해제한다.
	if (ActiveAttackMontage)
	{
		FOnMontageEnded EmptyDelegate;
		Anim->Montage_SetEndDelegate(EmptyDelegate, ActiveAttackMontage);
	}
	ResetAttackTrace();

	const float PlayedDuration = Anim->Montage_Play(AttackInfo.Anim, Playrate);
	if (PlayedDuration <= 0.0f)
	{
		UE_LOG(Log_Attack, Warning, TEXT("[AttackComponent] Failed to play attack montage '%s'."),
		       *GetNameSafe(AttackInfo.Anim));
		return false;
	}

	ActiveAttackMontage = AttackInfo.Anim;
	Anim->Montage_JumpToSection(SectionName, AttackInfo.Anim);

	FOnMontageEnded MontageEndDelegate;
	MontageEndDelegate.BindUObject(this, &UAttackComponent::OnMontageEnded);
	Anim->Montage_SetEndDelegate(MontageEndDelegate, AttackInfo.Anim);
	return true;
}

void UAttackComponent::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != ActiveAttackMontage)
	{
		return;
	}

	FinishAttackSession(bInterrupted, false, EActionExitReason::Completed);
}

void UAttackComponent::CancelAttack(EActionExitReason ExitReason, bool bStopMontage)
{
	FinishAttackSession(true, bStopMontage, ExitReason);
}

void UAttackComponent::FinishAttackSession(bool bInterrupted, bool bStopMontage, EActionExitReason ExitReason)
{
	if (bFinishingAttackSession || AttackSessionState == EAttackSessionState::Idle)
	{
		ResetAttackTrace();
		return;
	}

	TGuardValue<bool> FinishingGuard(bFinishingAttackSession, true);
	++AttackSessionId;
	UAnimMontage* MontageToStop = ActiveAttackMontage;

	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		if (UAnimInstance* AnimInstance = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr)
		{
			if (MontageToStop)
			{
				FOnMontageEnded EmptyDelegate;
				AnimInstance->Montage_SetEndDelegate(EmptyDelegate, MontageToStop);
				if (bStopMontage && AnimInstance->Montage_IsPlaying(MontageToStop))
				{
					FAlphaBlendArgs BlendOut = MontageToStop->GetBlendOutArgs();
					const float OverrideTime = CurAttackContext.ExitBlendSettings.GetOverride(ExitReason);
					if (OverrideTime >= 0.0f)
					{
						BlendOut.BlendTime = OverrideTime;
					}
					AnimInstance->Montage_StopWithBlendOut(BlendOut, MontageToStop);
				}
			}
		}
	}

	ResetAttackTrace();
	HitActorListCurrentAttack.Empty();
	CurAttackContext = FAttackContext();
	ComboIndex = 0;
	ActiveAttackMontage = nullptr;
	AttackSessionState = EAttackSessionState::Idle;
	OnAttackFinished.Broadcast(bInterrupted);
}

void UAttackComponent::ResetAttackTrace()
{
	CurrentSeg = nullptr;
	LastTraceTime = 0.0f;
	PreviousTraceRootWorldTransform = FTransform::Identity;
	bHasPreviousTraceRootWorldTransform = false;
	bAttackTraceActive = false;
}

void UAttackComponent::ExecuteAttackTrace(float StartTime, float EndTime, bool bDrawDebug)
{
	if (!IsAttackActive() || !bAttackTraceActive || EndTime <= StartTime) return;
	if (!CurAttackContext.AttackDetail.IsValidIndex(ComboIndex)) return;
	const uint64 TraceSessionId = AttackSessionId;

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		UE_LOG(Log_Attack, Error, TEXT("[UAttackComponent] Owner Character Invalid"));
		return;
	}

	if (!CurrentSeg)
	{
		UE_LOG(Log_Attack, Error, TEXT("[UAttackComponent] AnimBoneData was not set"));
		return;
	}

	// 이전 프레임과 현재 프레임 사이를 0.001초 간격으로 균등하게 나눈다.
	const int32 TraceCorrectionCount = FMath::Max(1, FMath::CeilToInt((EndTime - StartTime) / 0.001f));

	const FTransform CurrentRootWorldTransform = Character->GetMesh()->GetBoneTransform(0);
	const FTransform& PreviousRootWorldTransform = bHasPreviousTraceRootWorldTransform
		? PreviousTraceRootWorldTransform
		: CurrentRootWorldTransform;

	FAttackTraceSource TraceSource;
	if (IAttackSourceInterface* AttackSource = Cast<IAttackSourceInterface>(Character))
	{
		TraceSource = AttackSource->GetAttackTraceSource(CurAttackContext.AttackDetail[ComboIndex].AttackSource);
	}

	if (!TraceSource.TraceComponent)
	{
		UE_LOG(Log_Attack, Error, TEXT("[UAttackComponent] TraceComponent Invalid"));
		return;
	}

	const FWeaponTrajectoryGeometry WeaponGeometry = FWeaponTrajectoryUtility::BuildGeometry(
		Character->GetMesh(), TraceSource.TraceComponent, CurrentSeg->BoneName,
		TEXT("Start"), TEXT("End"));
	if (!WeaponGeometry.IsValid())
	{
		UE_LOG(Log_Attack, Error, TEXT("[UAttackComponent] Weapon trajectory geometry invalid"));
		return;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	const FBaseAttackData& Detail = CurAttackContext.AttackDetail[ComboIndex];
	FAttackDamageSource DamageSource;
	if (IAttackSourceInterface* AttackSource = Cast<IAttackSourceInterface>(Character))
	{
		DamageSource = AttackSource->GetAttackDamageSource();
	}

	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(GetOwner());
	const float Radius = TraceSource.Radius;
	TArray<FHitResult> HitResults;

	for (int32 i = 1; i <= TraceCorrectionCount; ++i)
	{
		const float SampleAlpha = static_cast<float>(i) / static_cast<float>(TraceCorrectionCount);
		const float PrevTime = FMath::Lerp(StartTime, EndTime, SampleAlpha);
		FTransform SampleRootWorldTransform;
		SampleRootWorldTransform.Blend(
			PreviousRootWorldTransform, CurrentRootWorldTransform, SampleAlpha);

		FVector StartLoc;
		FVector EndLoc;
		FWeaponTrajectoryUtility::GetSocketWorldPositions(
			WeaponGeometry, CurrentSeg->GetTransformAtTime(PrevTime),
			SampleRootWorldTransform, StartLoc, EndLoc);

		const FVector CapsuleAxis = EndLoc - StartLoc;
		if (CapsuleAxis.IsNearlyZero())
		{
			continue;
		}
		const float CurHalfHeight = FMath::Max(CapsuleAxis.Size() * 0.5f, Radius);
		const FVector CapsuleCenter = (StartLoc + EndLoc) * 0.5f;
		const FQuat CapsuleRotation = FRotationMatrix::MakeFromZ(CapsuleAxis.GetSafeNormal()).ToQuat();
		const FCollisionShape DetectShape = FCollisionShape::MakeCapsule(Radius, CurHalfHeight);

		HitResults.Reset();
		const bool bHit = World->SweepMultiByChannel(
			HitResults,
			CapsuleCenter,
			CapsuleCenter,
			CapsuleRotation,
			ECC_GameTraceChannel3,
			DetectShape,
			CollisionParams
		);
		
		if (bHit)
		{
			for (const FHitResult& Result : HitResults)
			{
				AActor* HitActor = Result.GetActor();
				if (HitActor && !HitActorListCurrentAttack.Contains(HitActor))
				{
					HitActorListCurrentAttack.Add(HitActor);

					UE_LOG(Log_Attack, Log, TEXT("[AttackComponent] %s Attacked %s"), *Character->GetName(), *HitActor->GetName());

					if (HitActor->Implements<UHitReactionInterface>())
					{
						float OutDamage = DamageSource.AttackRating * Detail.DamageMultiplier;
						float OutPoiseDamage = DamageSource.PoiseRating * Detail.PoiseDamageMultiplier;
						float OutStanceDamage = DamageSource.StanceRating * Detail.StanceDamageMultiplier;
						ECombatReaction OutResponse = Detail.Response;
						EDamageType OutAttackType = Detail.DamageType;
						EElementalType OutElementType = Detail.ElementType;
						float OutElementalBuildup = Detail.ElementalBuildup;
						FVector OutHitPoint = Result.ImpactPoint;
						FString OutHitPointName = Result.PhysMaterial.IsValid() ? Result.PhysMaterial->GetName() : FString();
						bool OutCanBlocked = Detail.CanBlocked;
						bool OutCanParried = Detail.CanParried;
						bool OutCanAvoid = Detail.CanAvoid;

						FAttackRequest OutAttackData(
							OutDamage,
							OutStanceDamage,
							OutPoiseDamage,
							OutResponse,
							OutAttackType,
							OutElementType,
							OutElementalBuildup,
							OutHitPoint,
							OutHitPointName,
							OutCanBlocked,
							OutCanParried,
							OutCanAvoid,
							Character
						);

						IHitReactionInterface::Execute_OnHit(HitActor, OutAttackData);

						// OnHit은 동기 호출이며 패리/사망 등의 처리에서 이 공격 세션을
						// 즉시 취소할 수 있다. 취소된 세션의 CurrentSeg를 다음 보정
						// 샘플에서 다시 사용하지 않는다.
						if (AttackSessionId != TraceSessionId ||
							!IsAttackActive() || !bAttackTraceActive || !CurrentSeg)
						{
							return;
						}

					}
				}
			}
		}
		
		if (bDrawDebug)
		{
			DrawDebugCapsule(World, CapsuleCenter, CurHalfHeight, Radius,
				CapsuleRotation, FColor::Red, false, 5.0f);
		}
	}

	PreviousTraceRootWorldTransform = CurrentRootWorldTransform;
	bHasPreviousTraceRootWorldTransform = true;
	LastTraceTime = EndTime;
}

void UAttackComponent::BeginAttackTrace(FGameplayTag Profile, const UAnimSequence* AnimKey, FName WindowName, float StartTime)
{
	ResetAttackTrace();
	if (!IsAttackActive() || !AnimKey) return;

	UAnimBoneDataSubsystem* Subsys = GetWorld()->GetGameInstance()->GetSubsystem<UAnimBoneDataSubsystem>();
	if (!Subsys) return;

	CurrentSeg = Subsys->GetAnimBoneData(Profile, AnimKey, WindowName);
	if (!CurrentSeg)
	{
		UE_LOG(Log_Attack, Warning,
		       TEXT("[AttackComponent] Missing baked trace data. Profile=%s Animation=%s Window=%s"),
		       *Profile.ToString(), *GetNameSafe(AnimKey), *WindowName.ToString());
		return;
	}

	LastTraceTime = StartTime;
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		PreviousTraceRootWorldTransform = Character->GetMesh()->GetBoneTransform(0);
		bHasPreviousTraceRootWorldTransform = true;
	}
	bAttackTraceActive = true;
}

void UAttackComponent::TickAttackTrace(float DeltaTime, bool bDrawDebug)
{
	if (!IsAttackActive() || !bAttackTraceActive || !CurrentSeg) return;
	const uint64 TraceSessionId = AttackSessionId;

	const float PrevTime = LastTraceTime;
	const float CurrentTime = LastTraceTime + DeltaTime;

	if (CurrentSeg->EndTime < CurrentTime || CurrentSeg->StartTime > CurrentTime) return;

	ExecuteAttackTrace(PrevTime, CurrentTime, bDrawDebug);
	if (AttackSessionId != TraceSessionId ||
		!IsAttackActive() || !bAttackTraceActive || !CurrentSeg)
	{
		return;
	}

	LastTraceTime = CurrentTime;
}

void UAttackComponent::EndAttackTrace(float EndTime, bool bDrawDebug)
{
	const uint64 TraceSessionId = AttackSessionId;
	if (IsAttackActive() && bAttackTraceActive && CurrentSeg && EndTime > LastTraceTime)
	{
		ExecuteAttackTrace(LastTraceTime, FMath::Min(EndTime, CurrentSeg->EndTime), bDrawDebug);
	}
	if (AttackSessionId == TraceSessionId)
	{
		ResetAttackTrace();
	}
}
