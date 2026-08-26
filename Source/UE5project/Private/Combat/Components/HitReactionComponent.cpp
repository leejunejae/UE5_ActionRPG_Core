// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/Components/HitReactionComponent.h"
#include "Characters/CharacterBase.h"
#include "Characters/Components/CharacterStatusComponent.h"
#include "Characters/Interfaces/CharacterStatusInterface.h"
#include "Characters/Data/StatusData.h"
#include "Utils/CoreLog.h"
#include "Utils/GameplayTagsBase.h"

// Sets default values for this component's properties
UHitReactionComponent::UHitReactionComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.

	// ...
}


// Called when the game starts
void UHitReactionComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UHitReactionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FinishHitReaction(true, false, EActionExitReason::Interrupted);
	Super::EndPlay(EndPlayReason);
}

void UHitReactionComponent::InitializeComponentLogic()
{
	
}

void UHitReactionComponent::SetHitReactionDA(UHitReactionDataAsset* HitReactionDA)
{
	HitReactionDataAsset = HitReactionDA;
	UE_LOG(Log_Hit, Log, TEXT("[HitReactionComponent] Owner=%s ActiveData=%s"),
		*GetNameSafe(GetOwner()), *GetPathNameSafe(HitReactionDataAsset));
}

bool UHitReactionComponent::ExecuteHitResponse(const FHitReactionRequest& ReactionData)
{
	if (!HitReactionDataAsset)
	{
		UE_LOG(Log_Hit, Warning, TEXT("[HitReactionComponent] Owner=%s Response=%s failed: active data asset is null."),
			*GetNameSafe(GetOwner()), *StaticEnum<ECombatReaction>()->GetNameStringByValue(static_cast<int64>(ReactionData.Response)));
		if (bHitReactionActive) CancelHitReaction(EActionExitReason::Interrupted, true);
		else ClearHitReactActionIfCurrent();
		return false;
	}

	if (!HitReactionDataAsset->HitReactionInfoList.Contains(ReactionData.Response))
	{
		UE_LOG(Log_Hit, Warning,
			TEXT("[HitReactionComponent] Owner=%s Response=%s failed: response is missing from %s."),
			*GetNameSafe(GetOwner()),
			*StaticEnum<ECombatReaction>()->GetNameStringByValue(static_cast<int64>(ReactionData.Response)),
			*GetPathNameSafe(HitReactionDataAsset));
		if (bHitReactionActive) CancelHitReaction(EActionExitReason::Interrupted, true);
		else ClearHitReactActionIfCurrent();
		return false;
	}

	const FHitReactionInfo CandidateReaction = HitReactionDataAsset->FindHitReactionInfo(ReactionData.Response);
	if (!CandidateReaction.IsValid())
	{
		UE_LOG(Log_Hit, Warning,
			TEXT("[HitReactionComponent] Owner=%s Response=%s failed: invalid reaction data. Anim=%s Details=%d Asset=%s"),
			*GetNameSafe(GetOwner()),
			*StaticEnum<ECombatReaction>()->GetNameStringByValue(static_cast<int64>(ReactionData.Response)),
			*GetNameSafe(CandidateReaction.Anim), CandidateReaction.HitReactionDetail.Num(),
			*GetPathNameSafe(HitReactionDataAsset));
		if (bHitReactionActive) CancelHitReaction(EActionExitReason::Interrupted, true);
		else ClearHitReactActionIfCurrent();
		return false;
	}

	static const TMap<EHitPointHorizontal, float> DirectionToYaw = {
		{ EHitPointHorizontal::Front, 0.0f },
		{ EHitPointHorizontal::FrontRight, 45.0f },
		{ EHitPointHorizontal::Right, 90.0f },
		{ EHitPointHorizontal::BackRight, 135.0f },
		{ EHitPointHorizontal::Back, 180.0f },
		{ EHitPointHorizontal::BackLeft, -135.0f },
		{ EHitPointHorizontal::Left, -90.0f },
		{ EHitPointHorizontal::FrontLeft, -45.0f }
	};

	float MatchScore = TNumericLimits<float>::Max();
	FHitReactionDetail MatchInfo;
	const FHitReactionDetail* NeutralFallback = nullptr;
	bool bFoundDirectionalMatch = false;

	for (const FHitReactionDetail& Info : CandidateReaction.HitReactionDetail)
	{
		const EHitPointHorizontal CurrentPoint = Info.HitPointHorizontal;
		if (CurrentPoint == EHitPointHorizontal::Neutral)
		{
			// Neutral은 방향별 항목이 하나도 없을 때만 사용하는 명시적 fallback이다.
			if (!NeutralFallback || Info.SectionName.LexicalLess(NeutralFallback->SectionName))
			{
				NeutralFallback = &Info;
			}
			continue;
		}

		const float* PointToAngle = DirectionToYaw.Find(CurrentPoint);
		if (!PointToAngle)
		{
			continue;
		}

		const float AngleDiff = FMath::Abs(
			FMath::FindDeltaAngleDegrees(ReactionData.HitAngle, *PointToAngle));
		const bool bCloser = AngleDiff < MatchScore;
		const bool bSameDistance = FMath::IsNearlyEqual(AngleDiff, MatchScore);
		const bool bStableTieBreak = bSameDistance &&
			static_cast<uint8>(CurrentPoint) < static_cast<uint8>(MatchInfo.HitPointHorizontal);

		if (!bFoundDirectionalMatch || bCloser || bStableTieBreak)
		{
			MatchScore = AngleDiff;
			MatchInfo = Info;
			bFoundDirectionalMatch = true;
		}
	}

	if (!bFoundDirectionalMatch && NeutralFallback)
	{
		MatchInfo = *NeutralFallback;
	}

	if (!PlayReaction(CandidateReaction, MatchInfo.SectionName))
	{
		UE_LOG(Log_Hit, Warning,
			TEXT("[HitReactionComponent] Owner=%s Response=%s failed to play. Montage=%s Section=%s HitAngle=%.2f Asset=%s"),
			*GetNameSafe(GetOwner()),
			*StaticEnum<ECombatReaction>()->GetNameStringByValue(static_cast<int64>(ReactionData.Response)),
			*GetNameSafe(CandidateReaction.Anim), *MatchInfo.SectionName.ToString(), ReactionData.HitAngle,
			*GetPathNameSafe(HitReactionDataAsset));
		if (bHitReactionActive) CancelHitReaction(EActionExitReason::Interrupted, true);
		else ClearHitReactActionIfCurrent();
		return false;
	}

	ActiveReaction = ReactionData.Response;
	HitStartDelegate.Broadcast();
	return true;
}

bool UHitReactionComponent::CanUpgradeActiveReaction(ECombatReaction IncomingReaction) const
{
	if (!bHitReactionActive)
	{
		return false;
	}

	auto GetDirectHitRank = [](ECombatReaction Reaction) -> int32
	{
		switch (Reaction)
		{
		case ECombatReaction::Flinch: return 1;
		case ECombatReaction::KnockBack: return 2;
		case ECombatReaction::KnockDown: return 3;
		default: return INDEX_NONE;
		}
	};

	const int32 ActiveRank = GetDirectHitRank(ActiveReaction);
	const int32 IncomingRank = GetDirectHitRank(IncomingReaction);
	return ActiveRank != INDEX_NONE && IncomingRank > ActiveRank;
}

bool UHitReactionComponent::TransitionHitAirToRecovery(FName RecoverySection)
{
	if (!IsHitAirReactionActive() || !ActiveHitMontage)
	{
		return false;
	}

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	UAnimInstance* AnimInstance = Character && Character->GetMesh()
		? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance || !ActiveHitMontage->IsValidSectionName(RecoverySection))
	{
		UE_LOG(Log_Hit, Warning,
			TEXT("[HitReactionComponent] Owner=%s HitAir recovery section '%s' is invalid in %s."),
			*GetNameSafe(GetOwner()), *RecoverySection.ToString(), *GetNameSafe(ActiveHitMontage));
		return false;
	}

	AnimInstance->Montage_JumpToSection(RecoverySection, ActiveHitMontage);
	return true;
}

bool UHitReactionComponent::PlayReaction(const FHitReactionInfo& HitReaction, const FName SectionName)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	UAnimInstance* AnimInstance = Character && Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance || !HitReaction.Anim || SectionName.IsNone()) return false;

	FHitReactionDetail DataForFind;
	DataForFind.SectionName = SectionName;
	const FHitReactionDetail* FoundData = HitReaction.HitReactionDetail.Find(DataForFind);

	if (!FoundData || !HitReaction.Anim->IsValidSectionName(FoundData->SectionName))
	{
		UE_LOG(Log_Hit, Warning, TEXT("[HitReactionComponent] Invalid reaction section '%s' in montage '%s'."),
		       *SectionName.ToString(), *GetNameSafe(HitReaction.Anim));
		return false;
	}

	// 연속 피격 시 이전 몽타주의 종료 콜백이 새 HitReact 행동을 지우지 않게 교체 전에 해제한다.
	if (ActiveHitMontage)
	{
		FOnMontageBlendingOutStarted EmptyDelegate;
		AnimInstance->Montage_SetBlendingOutDelegate(EmptyDelegate, ActiveHitMontage);
	}

	const float PlayedDuration = AnimInstance->Montage_Play(
		HitReaction.Anim, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, false);
	if (PlayedDuration <= 0.0f) return false;

	CurHitReaction = HitReaction;
	ActiveHitMontage = HitReaction.Anim;
	bHitReactionActive = true;
	AnimInstance->Montage_JumpToSection(FoundData->SectionName, HitReaction.Anim);

	FOnMontageBlendingOutStarted BlendingOutDelegate;
	BlendingOutDelegate.BindUObject(this, &UHitReactionComponent::OnHitReactionEnded);
	AnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, HitReaction.Anim);
	return true;
}

void UHitReactionComponent::OnHitReactionEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != ActiveHitMontage) return;

	if (AActor* Owner = GetOwner())UE_LOG(Log_Hit, Log, TEXT("[HitReactionComponent] %s stagger response End"), *Owner->GetName());
	FinishHitReaction(false, true, EActionExitReason::Completed);
}

void UHitReactionComponent::CancelHitReaction(EActionExitReason ExitReason, bool bStopMontage)
{
	FinishHitReaction(bStopMontage, true, ExitReason);
}

void UHitReactionComponent::FinishHitReaction(bool bStopMontage, bool bBroadcastEnd, EActionExitReason ExitReason)
{
	if (bFinishingHitReaction || !bHitReactionActive) return;
	TGuardValue<bool> FinishingGuard(bFinishingHitReaction, true);

	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		if (UAnimInstance* AnimInstance = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr)
		{
			if (ActiveHitMontage)
			{
				FOnMontageBlendingOutStarted EmptyDelegate;
				AnimInstance->Montage_SetBlendingOutDelegate(EmptyDelegate, ActiveHitMontage);
				if (bStopMontage && AnimInstance->Montage_IsPlaying(ActiveHitMontage))
				{
					FAlphaBlendArgs BlendOut = ActiveHitMontage->GetBlendOutArgs();
					const float OverrideTime = CurHitReaction.ExitBlendSettings.GetOverride(ExitReason);
					if (OverrideTime >= 0.0f)
					{
						BlendOut.BlendTime = OverrideTime;
					}
					AnimInstance->Montage_StopWithBlendOut(BlendOut, ActiveHitMontage);
				}
			}
		}
	}

	ActiveHitMontage = nullptr;
	CurHitReaction = FHitReactionInfo();
	ActiveReaction = ECombatReaction::None;
	bHitReactionActive = false;
	ClearHitReactActionIfCurrent();
	if (bBroadcastEnd)
	{
		HitEndDelegate.Broadcast();
	}
}

void UHitReactionComponent::ClearHitReactActionIfCurrent() const
{
	const ACharacterBase* Character = Cast<ACharacterBase>(GetOwner());
	UCharacterStatusComponent* Status = Character ? Character->GetCharacterStatusComponent() : nullptr;
	if (Status && Status->GetCurrentAction().MatchesTagExact(TAG_Action_HitReact))
	{
		Status->ClearAction();
	}
}

float UHitReactionComponent::CalculateHitAngle(const FVector HitPoint)
{
	FVector CharacterLocation = GetOwner()->GetActorLocation();
	FVector ImpactVector = HitPoint - CharacterLocation;
	FRotator HitRotator = ImpactVector.Rotation();

	float HitYaw = HitRotator.Yaw;
	float CharacterYaw = GetOwner()->GetActorRotation().Yaw;
	float HitAngle = FMath::FindDeltaAngleDegrees(CharacterYaw, HitYaw);

	return HitAngle;
}

float UHitReactionComponent::CalculateAttackAngle(const FAttackRequest& AttackRequest) const
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return 0.f;
	}

	const FVector DirectionPoint = IsValid(AttackRequest.AttackCauser)
		? AttackRequest.AttackCauser->GetActorLocation()
		: AttackRequest.HitPoint;
	const float DirectionYaw = (DirectionPoint - OwnerActor->GetActorLocation()).Rotation().Yaw;
	return FMath::FindDeltaAngleDegrees(OwnerActor->GetActorRotation().Yaw, DirectionYaw);
}

FHitResolution UHitReactionComponent::ResolveHit(const FAttackRequest& AttackRequest) const
{
	ACharacterBase* OwnerCharacter = Cast<ACharacterBase>(GetOwner());
	const float HitAngle = CalculateAttackAngle(AttackRequest);
	if (!OwnerCharacter)
	{
		return { EHitOutcome::Hit, ToCombatReaction(AttackRequest.DamageLevel), HitAngle };
	}

	UCharacterStatusComponent* Status = OwnerCharacter->GetCharacterStatusComponent();
	if (!Status)
	{
		return { EHitOutcome::Hit, ToCombatReaction(AttackRequest.DamageLevel), HitAngle };
	}

	const FGameplayTag& ActionTag = Status->GetCurrentAction();

	// === 공중 피격 우선 ===
	if (Status->IsInAir())
	{
		return { EHitOutcome::Hit, ECombatReaction::HitAir, HitAngle };
	}

	// === 회피 ===
	if (ActionTag.MatchesTagExact(TAG_Action_Dodge))
	{
		if (AttackRequest.CanAvoid)
			return { EHitOutcome::Avoided, ECombatReaction::None, HitAngle };
	}

	// === 패리 (플레이어 / NPC CounterStance 공통) ===
	if (ActionTag.MatchesTagExact(TAG_Action_Parry) && IsParryActiveWindowOpen())
	{
		if (AttackRequest.CanParried && FMath::Abs(HitAngle) <= 60.0f)
		{
			return { EHitOutcome::Parried, ECombatReaction::None, HitAngle };
		}
	}

	// === 가드 ===
	if (ActionTag.MatchesTagExact(TAG_Action_Guard))
	{
		if (AttackRequest.CanBlocked && FMath::Abs(HitAngle) <= 60.0f)
		{
			switch (AttackRequest.DamageLevel)
			{
			case EHitDamageLevel::KnockBack:
			case EHitDamageLevel::KnockDown:
				return { EHitOutcome::Blocked, ECombatReaction::GuardHitHeavy, HitAngle };
			default:
				return { EHitOutcome::Blocked, ECombatReaction::GuardHit, HitAngle };
			}
		}
	}

	return { EHitOutcome::Hit, ToCombatReaction(AttackRequest.DamageLevel), HitAngle };
}

void UHitReactionComponent::BeginParryActiveWindow()
{
	++ParryActiveWindowCount;
}

void UHitReactionComponent::EndParryActiveWindow()
{
	ParryActiveWindowCount = FMath::Max(0, ParryActiveWindowCount - 1);
}

void UHitReactionComponent::ResetParryActiveWindow()
{
	ParryActiveWindowCount = 0;
}
