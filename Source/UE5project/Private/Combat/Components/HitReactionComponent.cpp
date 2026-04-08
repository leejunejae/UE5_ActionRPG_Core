// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/Components/HitReactionComponent.h"
#include "GameFramework/Character.h"
#include "Characters/Components/CharacterStatusComponent.h"
#include "Characters/Interfaces/CharacterStatusInterface.h"
#include "Characters/Data/StatusData.h"
#include "Utils/CoreLog.h"

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

	CachedCharacter = Cast<ACharacter>(GetOwner());

	if (!CachedCharacter.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Owner Character Not Valid"));
		return;
	}
	
	for (UActorComponent* Comp : CachedCharacter->GetComponents())
	{
		if (Comp->GetClass()->ImplementsInterface(UCharacterStatusInterface::StaticClass()))
			CachedPlayerStatus = TScriptInterface<ICharacterStatusInterface>(Comp);;
	}

	if (!CachedPlayerStatus.GetObject() || !CachedPlayerStatus.GetInterface())
	{
		UE_LOG(LogTemp, Warning, TEXT("Character Status Component Not Valid"));
		return;
	}
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
}

void UHitReactionComponent::PlayReaction(const FHitReactionInfo HitReaction, const FName SectionName)
{
	UAnimInstance* AnimInstance = Cast<ACharacter>(GetOwner())->GetMesh()->GetAnimInstance();
		//GetOwner()->GetComponentByClass<USkeletalMeshComponent>()->GetAnimInstance();

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

	//if (HitMontageEndDelegate.IsBound())
		//HitMontageEndDelegate.Unbind();

	if (HitMontageBlendingOutDelegate.IsBound())
		HitMontageBlendingOutDelegate.Unbind();

	//HitMontageEndDelegate.BindUObject(this, &UHitReactionComponent::OnHitReactionEnded);
	HitMontageBlendingOutDelegate.BindUObject(this, &UHitReactionComponent::OnHitReactionEnded);

	//AnimInstance->Montage_SetEndDelegate(HitMontageEndDelegate, HitReaction.Anim);
	AnimInstance->Montage_SetBlendingOutDelegate(HitMontageBlendingOutDelegate, HitReaction.Anim);
}

void UHitReactionComponent::OnHitReactionEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != CurHitReaction.Anim) return;

	if (AActor* Owner = GetOwner())UE_LOG(Log_Hit, Log, TEXT("[HitReactionComponent] %s stagger response End"), *Owner->GetName());

	//ICharacterStatusInterface::Execute_SetGroundStance(CachedPlayerStatus.GetObject(), EGroundStance::Normal);
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
	const EGroundStance CombatState = ICharacterStatusInterface::Execute_GetGroundStance(CachedPlayerStatus.GetObject());

	if (CombatState == EGroundStance::Invincible) return EHitResponse::None;

	EHitResponse FinalResponse = AttackRequest.Response;

	switch (CombatState)
	{
	case EGroundStance::Hit:
	{
		FinalResponse = EHitResponse::NoStagger;
		break;
	}
	case EGroundStance::Invincible:
	{
		FinalResponse = EHitResponse::None;
		break;
	}
	case EGroundStance::Jump:
	{
		FinalResponse = EHitResponse::HitAir;
		break;
	}
	case EGroundStance::Dodge:
	{
		if (AttackRequest.CanAvoid)
		{
			FinalResponse = EHitResponse::None;
		}
		break;
	}
	case EGroundStance::Block:
	{
		float HitAngle = CalculateHitAngle(AttackRequest.HitPoint);
		if (AttackRequest.CanBlocked && (FMath::Abs(HitAngle) <= 60.0f))
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
		break;
	}
	case EGroundStance::Parry:
	{
		float HitAngle = CalculateHitAngle(AttackRequest.HitPoint);
		if (AttackRequest.CanParried && (HitAngle >= -60.0f || HitAngle <= 60.0f))
		{
			FinalResponse = EHitResponse::None;
			// 패링시 발동될 델리게이트 추가
		}
		break;
	}
	}

	return FinalResponse;
}