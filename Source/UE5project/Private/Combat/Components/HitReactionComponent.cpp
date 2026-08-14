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
	UE_LOG(LogTemp, Warning, TEXT("Set HitReactionDataAsset"));
	HitReactionDataAsset = HitReactionDA;
}

bool UHitReactionComponent::ExecuteHitResponse(const FHitReactionRequest& ReactionData)
{
	if (!HitReactionDataAsset)
	{
		if (bHitReactionActive) CancelHitReaction(EActionExitReason::Interrupted, true);
		else ClearHitReactActionIfCurrent();
		return false;
	}

	const FHitReactionInfo CandidateReaction = HitReactionDataAsset->FindHitReactionInfo(ReactionData.Response);

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

	float MatchScore = 180.0f;

	FHitReactionDetail MatchInfo;

	for (const FHitReactionDetail& Info : CandidateReaction.HitReactionDetail)
	{
		EHitPointHorizontal CurrentPoint = Info.HitPointHorizontal;

		float PointToAngle = DirectionToYaw[CurrentPoint];
		float AngleDiff = FMath::Abs(FMath::FindDeltaAngleDegrees(ReactionData.HitAngle, PointToAngle));

		if (AngleDiff < MatchScore)
		{
			MatchScore = AngleDiff;
			MatchInfo = Info;
		}
	}

	if (!PlayReaction(CandidateReaction, MatchInfo.SectionName))
	{
		if (bHitReactionActive) CancelHitReaction(EActionExitReason::Interrupted, true);
		else ClearHitReactActionIfCurrent();
		return false;
	}

	HitStartDelegate.Broadcast();
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

EHitResponse UHitReactionComponent::EvaluateHitResponse(const FAttackRequest& AttackRequest)
{
	ACharacterBase* OwnerCharacter = Cast<ACharacterBase>(GetOwner());
	if (!OwnerCharacter) return AttackRequest.Response;

	UCharacterStatusComponent* Status = OwnerCharacter->GetCharacterStatusComponent();
	if (!Status) return AttackRequest.Response;

	const FGameplayTag& ActionTag = Status->GetCurrentAction();
	EHitResponse FinalResponse = AttackRequest.Response;

	// === 공중 피격 우선 ===
	if (Status->IsInAir())
	{
		return EHitResponse::HitAir;
	}

	// === 회피 ===
	if (ActionTag.MatchesTagExact(TAG_Action_Dodge))
	{
		if (AttackRequest.CanAvoid)
			return EHitResponse::None;
	}

	// === 패리 (플레이어 / NPC CounterStance 공통) ===
	if (ActionTag.MatchesTagExact(TAG_Action_Parry))
	{
		const float HitAngle = CalculateHitAngle(AttackRequest.HitPoint);
		if (AttackRequest.CanParried && FMath::Abs(HitAngle) <= 60.0f)
		{
			ParryDelegate.Broadcast(AttackRequest);
			return EHitResponse::None;
		}
	}

	// === 가드 ===
	if (ActionTag.MatchesTagExact(TAG_Action_Guard))
	{
		const float HitAngle = CalculateHitAngle(AttackRequest.HitPoint);
		if (AttackRequest.CanBlocked && FMath::Abs(HitAngle) <= 60.0f)
		{
			switch (AttackRequest.Response)
			{
			case EHitResponse::Flinch:
			case EHitResponse::KnockBack:
				FinalResponse = EHitResponse::Block;
				break;
			case EHitResponse::KnockDown:
				FinalResponse = EHitResponse::BlockLarge;
				break;
			}
		}
	}

	return FinalResponse;
}
