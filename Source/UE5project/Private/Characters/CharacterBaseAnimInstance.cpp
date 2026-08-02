// Fill out your copyright notice in the Description page of Project Settings.

// 엔진 헤더
#include "Characters/CharacterBaseAnimInstance.h"
#include "Components/CapsuleComponent.h"

#include "Characters/Components/BaseCharacterMovementComponent.h"
#include "Characters/Player/Components/PlayerStatusComponent.h"

// 애니메이션 모드
#include "Animation/Mode/AnimMode_Ground.h"
#include "Animation/Mode/AnimMode_Ladder.h"

// 참조할 액터
#include "Characters/CharacterBase.h"

// Kismet 유틸리티
#include "KismetAnimationLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"

// 컴포넌트
#include "Interaction/Climb/Components/ClimbComponent.h"
#include "Utils/GameplayTagsBase.h"

UCharacterBaseAnimInstance::UCharacterBaseAnimInstance()
{
	IsInAir = false;
}

void UCharacterBaseAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	Character = Cast<ACharacterBase>(TryGetPawnOwner());

	if (Character)
	{
		Character->GetCharacterStatusComponent()->OnDeathStarted.AddUObject(this, &UCharacterBaseAnimInstance::HandleDeathStarted);

		AnimModeMap.Empty();
		AnimModeMap.Add(TAG_State_Ground, NewObject<UAnimMode_Ground>(this));
		AnimModeMap.Add(TAG_State_Ladder, NewObject<UAnimMode_Ladder>(this));

		SetIKPhaseAlpha_Native(TAG_IK_Phase_Ground, 1.0f);
		SetIKPhaseAlpha_Native(TAG_IK_Phase_Ladder, 0.0f);
		SetIKPhaseAlpha_Native(TAG_IK_Phase_Ride, 0.0f);
		InitializeIKLayers();

		for (auto& Pair : AnimModeMap)
		{
			if (!IsValid(Pair.Value))
			{
				// 무효 엔트리 발견 시 재빌드 or 건너뛰기
				// 여기선 안전하게 스킵
				continue;
			}

			Pair.Value->Character = Character;
			Pair.Value->AnimInst = this;
		}

		SwitchAnimMode(TAG_State_Ground);
	}
}

void UCharacterBaseAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	if (Character)
	{
		CurrentState = Character->GetCharacterStatusComponent()->GetCurrentState();

		if (!CurrentMode || AnimModeMap.FindRef(CurrentState) != CurrentMode)
		{
			SwitchAnimMode(CurrentState);
		}

		if (CurrentMode && !Character->GetCharacterStatusComponent()->IsDead())
			CurrentMode->Tick(DeltaSeconds);
	}
}

void UCharacterBaseAnimInstance::SwitchAnimMode(const FGameplayTag TargetMode)
{
	UAnimModeBase* NextMode = AnimModeMap.FindRef(TargetMode);
	if (!NextMode || CurrentMode == NextMode) return;

	if (CurrentMode) CurrentMode->OnModeExit();
	CurrentMode = NextMode;
	CurrentMode->OnModeEnter();
}

TTuple<FVector, float> UCharacterBaseAnimInstance::FootTrace(FName SocketName)
{
	FVector FootLoc = Character->GetMesh()->GetSocketLocation(SocketName);
	float TraceDistance = 50.0f;
	FVector Start = FVector(FootLoc.X, FootLoc.Y, Character->GetActorLocation().Z);
	FVector End = Start - FVector(0.0f, 0.0f, Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + TraceDistance);

	FHitResult DistHitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.bTraceComplex = true;
	enum ECollisionChannel TraceChannel = Speed > 50.0f ? ECC_Visibility : ECC_GameTraceChannel8;

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		DistHitResult,
		Start,
		End,
		TraceChannel,
		CollisionParams
	);

	//FColor DrawColor = bHit ? FColor::Green : FColor::Red;
	//DrawDebugLine(GetWorld(), Start, End, DrawColor, false, 1.0f);

	if (bHit)
	{
		float Offset = ((DistHitResult.ImpactPoint - DistHitResult.TraceEnd).Length()) - (TraceDistance - 3.0f);
		return MakeTuple(DistHitResult.ImpactNormal, Offset);
	}
	else
	{
		return MakeTuple(FVector::ZeroVector, 0.0f);
	}
}

void UCharacterBaseAnimInstance::FootRotation(float DeltaTime, FVector TargetNormal, FRotator *FootRotator, float fInterpSpeed)
{
	float AtanX = -1.0f * UKismetMathLibrary::DegAtan2(TargetNormal.X, TargetNormal.Z);
	float AtanY = UKismetMathLibrary::DegAtan2(TargetNormal.Y, TargetNormal.Z);
	FRotator TargetRotator = FRotator(AtanX, 0.0f, AtanY);

	*FootRotator = UKismetMathLibrary::RInterpTo(*FootRotator, TargetRotator, DeltaTime, fInterpSpeed);
}

void UCharacterBaseAnimInstance::SetHitAir(bool HitState)
{
	bIsHitAir = HitState;
}

void UCharacterBaseAnimInstance::ResetHitAir_Implementation()
{
	SetHitAir(false);
}

void UCharacterBaseAnimInstance::HandleDeathStarted()
{
	
}

void UCharacterBaseAnimInstance::AnimNotify_NOT_EnableRootLock()
{
	SetRootMotionMode(ERootMotionMode::RootMotionFromEverything);
	UE_LOG(LogTemp, Warning, TEXT("Enable Root Motion"));
}

void UCharacterBaseAnimInstance::AnimNotify_NOT_DisableRootLock()
{
	SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
}

void UCharacterBaseAnimInstance::AnimNotify_NOT_ResetClimbState()
{
	Character->GetClimbComponent()->ResetClimbState();
}

void UCharacterBaseAnimInstance::SetIKPhaseAlpha_Native(FGameplayTag TargetIKPhase, float Weight)
{
	if (!ensure(TargetIKPhase.MatchesTag(TAG_IK_Phase))) return;

	const float ClampedWeight = FMath::Clamp(Weight, 0.0f, 1.0f);
	if (TargetIKPhase.MatchesTagExact(TAG_IK_Phase_Ground))
	{
		GroundIKPhaseAlpha = ClampedWeight;
	}
	else if (TargetIKPhase.MatchesTagExact(TAG_IK_Phase_Ladder))
	{
		LadderIKPhaseAlpha = ClampedWeight;
	}
	else if (TargetIKPhase.MatchesTagExact(TAG_IK_Phase_Ride))
	{
		RideIKPhaseAlpha = ClampedWeight;
	}
	else
	{
		ensureMsgf(
			false,
			TEXT("Unsupported IK phase tag: %s"),
			*TargetIKPhase.ToString());
		return;
	}

	BaseIKPhaseAlpha = 1.0f - FMath::Clamp(
		GroundIKPhaseAlpha +
		LadderIKPhaseAlpha +
		RideIKPhaseAlpha,
		0.0f,
		1.0f);

}

float UCharacterBaseAnimInstance::GetIKPhaseAlpha_Native(FGameplayTag TargetIKPhase)
{
	if (TargetIKPhase.MatchesTagExact(TAG_IK_Phase_Ground))
	{
		return GroundIKPhaseAlpha;
	}
	if (TargetIKPhase.MatchesTagExact(TAG_IK_Phase_Ladder))
	{
		return LadderIKPhaseAlpha;
	}
	if (TargetIKPhase.MatchesTagExact(TAG_IK_Phase_Ride))
	{
		return RideIKPhaseAlpha;
	}

	return 0.0f;
}

void UCharacterBaseAnimInstance::SetIKLayerAlpha_Native(FGameplayTag TargetIKLayer, ELimbList Limb, float Weight)
{
	if (!ensure(TargetIKLayer.MatchesTag(TAG_IK_Layer))) return;

	const float ClampedWeight = FMath::Clamp(Weight, 0.0f, 1.0f);
	if (FIKLimbWeights* LayerWeights = FindIKLayerWeights(TargetIKLayer))
	{
		LayerWeights->Set(Limb, ClampedWeight);
	}
	else
	{
		ensureMsgf(false, TEXT("Unsupported IK layer tag: %s"),
			*TargetIKLayer.ToString());
	}
}

float UCharacterBaseAnimInstance::GetIKLayerAlpha_Native(FGameplayTag TargetIKLayer, ELimbList Limb)
{
	const FIKLimbWeights* LayerWeights = FindIKLayerWeights(TargetIKLayer);
	return LayerWeights ? LayerWeights->Get(Limb) : 0.0f;
}

void UCharacterBaseAnimInstance::InitializeIKLayers()
{
	GroundLocomotionIKLayer = FIKLimbWeights{};
	GroundHandWeaponIKLayer = FIKLimbWeights{};
	LadderClimbIKLayer = FIKLimbWeights{};
	RideLocomotionIKLayer = FIKLimbWeights{};

	// Ground locomotion is the initial mode. Transition notifies enable the
	// ladder and ride limbs when those modes become active.
	GroundLocomotionIKLayer.Set(ELimbList::FootL, 1.0f);
	GroundLocomotionIKLayer.Set(ELimbList::FootR, 1.0f);
}

FIKLimbWeights* UCharacterBaseAnimInstance::FindIKLayerWeights(
	FGameplayTag TargetIKLayer)
{
	if (TargetIKLayer.MatchesTagExact(TAG_IK_Layer_Ground_Locomotion))
	{
		return &GroundLocomotionIKLayer;
	}
	if (TargetIKLayer.MatchesTagExact(TAG_IK_Layer_Ground_HandWeapon))
	{
		return &GroundHandWeaponIKLayer;
	}
	if (TargetIKLayer.MatchesTagExact(TAG_IK_Layer_Ladder_Climb))
	{
		return &LadderClimbIKLayer;
	}
	if (TargetIKLayer.MatchesTagExact(TAG_IK_Layer_Ride_Locomotion))
	{
		return &RideLocomotionIKLayer;
	}

	return nullptr;
}

const FIKLimbWeights* UCharacterBaseAnimInstance::FindIKLayerWeights(
	FGameplayTag TargetIKLayer) const
{
	if (TargetIKLayer.MatchesTagExact(TAG_IK_Layer_Ground_Locomotion))
	{
		return &GroundLocomotionIKLayer;
	}
	if (TargetIKLayer.MatchesTagExact(TAG_IK_Layer_Ground_HandWeapon))
	{
		return &GroundHandWeaponIKLayer;
	}
	if (TargetIKLayer.MatchesTagExact(TAG_IK_Layer_Ladder_Climb))
	{
		return &LadderClimbIKLayer;
	}
	if (TargetIKLayer.MatchesTagExact(TAG_IK_Layer_Ride_Locomotion))
	{
		return &RideLocomotionIKLayer;
	}

	return nullptr;
}

void UCharacterBaseAnimInstance::SetIKPhaseAlpha_Implementation(FGameplayTag TargetIKPhase, float Weight)
{
	SetIKPhaseAlpha_Native(TargetIKPhase, Weight);
}

float UCharacterBaseAnimInstance::GetIKPhaseAlpha_Implementation(FGameplayTag TargetIKPhase)
{
	return GetIKPhaseAlpha_Native(TargetIKPhase);
}

void UCharacterBaseAnimInstance::SetIKLayerAlpha_Implementation(FGameplayTag TargetIKLayer, ELimbList Limb, float Weight)
{
	SetIKLayerAlpha_Native(TargetIKLayer, Limb, Weight);
}

float UCharacterBaseAnimInstance::GetIKLayerAlpha_Implementation(FGameplayTag TargetIKLayer, ELimbList Limb)
{
	return GetIKLayerAlpha_Native(TargetIKLayer, Limb);
}
