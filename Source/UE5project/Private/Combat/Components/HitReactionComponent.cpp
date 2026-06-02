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

void UHitReactionComponent::InitializeComponentLogic()
{
	
}

void UHitReactionComponent::SetHitReactionDA(UHitReactionDataAsset* HitReactionDA)
{
	UE_LOG(LogTemp, Warning, TEXT("Set HitReactionDataAsset"));
	HitReactionDataAsset = HitReactionDA;
}

void UHitReactionComponent::ExecuteHitResponse(const FHitReactionRequest ReactionData)
{
	if (!HitReactionDataAsset) return;

	const UEnum* EnumPtr = StaticEnum<EHitResponse>();
	if (!EnumPtr) return;

	FName ResponseRowName = FName(EnumPtr->GetNameStringByValue(static_cast<int64>(ReactionData.Response)));

	CurHitReaction = HitReactionDataAsset->FindHitReactionInfo(ReactionData.Response);

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

	for (FHitReactionDetail Info : CurHitReaction.HitReactionDetail)
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

	PlayReaction(CurHitReaction, MatchInfo.SectionName);

	HitStartDelegate.Broadcast();
}

void UHitReactionComponent::PlayReaction(const FHitReactionInfo HitReaction, const FName SectionName)
{
	UAnimInstance* AnimInstance = Cast<ACharacter>(GetOwner())->GetMesh()->GetAnimInstance();

	if (!AnimInstance) return;

	AnimInstance->Montage_Play(HitReaction.Anim, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, false);

	UE_LOG(LogTemp, Warning, TEXT("%s"), *SectionName.ToString());

	FHitReactionDetail DataForFind;
	DataForFind.SectionName = SectionName;
	const FHitReactionDetail* FoundData = HitReaction.HitReactionDetail.Find(DataForFind);

	if (!FoundData)
	{
		UE_LOG(LogTemp, Warning, TEXT("Data Not Found"));
		return;
	}

	AnimInstance->Montage_JumpToSection(FoundData->SectionName);


	if (HitMontageBlendingOutDelegate.IsBound())
		HitMontageBlendingOutDelegate.Unbind();

	HitMontageBlendingOutDelegate.BindUObject(this, &UHitReactionComponent::OnHitReactionEnded);

	AnimInstance->Montage_SetBlendingOutDelegate(HitMontageBlendingOutDelegate, HitReaction.Anim);
}

void UHitReactionComponent::OnHitReactionEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != CurHitReaction.Anim) return;

	if (AActor* Owner = GetOwner())UE_LOG(Log_Hit, Log, TEXT("[HitReactionComponent] %s stagger response End"), *Owner->GetName());

	HitEndDelegate.Broadcast();
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