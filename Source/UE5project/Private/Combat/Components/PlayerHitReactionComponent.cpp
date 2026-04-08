// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/Components/PlayerHitReactionComponent.h"
#include "GameFramework/Character.h"
#include "Characters/Player/Components/PlayerStatusComponent.h"
#include "Characters/Components/StatComponent.h"

UPlayerHitReactionComponent::UPlayerHitReactionComponent()
{

}

void UPlayerHitReactionComponent::BeginPlay()
{
	Super::BeginPlay();


}


EHitResponse UPlayerHitReactionComponent::EvaluateHitResponse(const FAttackRequest& AttackRequest)
{
	const EGroundStance CombatState = ICharacterStatusInterface::Execute_GetGroundStance(CachedPlayerStatus.GetObject());

	if (CombatState == EGroundStance::Invincible) return EHitResponse::None;

	EHitResponse FinalResponse = AttackRequest.Response;

	switch (CombatState)
	{
	case EGroundStance::Hit:
	case EGroundStance::Invincible:
	case EGroundStance::SuperArmor:
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