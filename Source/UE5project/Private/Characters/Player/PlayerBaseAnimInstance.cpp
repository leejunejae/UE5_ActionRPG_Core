// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Player/PlayerBaseAnimInstance.h"
#include "Characters/Player/PlayerBase.h"
#include "Animation/Mode/AnimMode_Ride.h"
#include "Animation/Mode/AnimMode_Ground_Player.h"
#include "Utils/CoreLog.h"
#include "Utils/GameplayTagsBase.h"

#include "Core/Subsystems/GameInstanceSystem/PlayerAnimRegistrySubsystem.h"

// 컴포넌트
#include "Characters/Components/EquipmentComponent.h"

void UPlayerBaseAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	Player = Cast<APlayerBase>(Character); // ★ Player 전용

	if (Player)
	{
		Player->GetEquipmentComponent()->OnWeaponChangedDelegate.AddUObject(this, &UPlayerBaseAnimInstance::HandleWeaponChange);
	}

	UAnimMode_Ride* RideMode = NewObject<UAnimMode_Ride>(this);
	RideMode->Character = Player;
	RideMode->AnimInst = this;

	UAnimMode_Ground_Player* GroundMode = NewObject<UAnimMode_Ground_Player>(this);
	GroundMode->Character = Player;
	GroundMode->AnimInst = this;

	AnimModeMap.Add(TAG_State_Ride, RideMode);
	AnimModeMap.Add(TAG_State_Ground, GroundMode);
}

void UPlayerBaseAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (Player)
	{
		IsMovementInput = Player->GetIsMovementInput();
	}
}

void UPlayerBaseAnimInstance::HandleWeaponChange(EWeaponType WeaponData)
{
	UWorld* World = GetWorld();
	if (!World)
		return;

	UPlayerAnimRegistrySubsystem* PlayerAnimSubsystem = World->GetGameInstance()->GetSubsystem<UPlayerAnimRegistrySubsystem>();
	if (!PlayerAnimSubsystem)
	{
		UE_LOG(Log_Anim_Equip, Warning, TEXT("[PlayerBaseAnimInstance] PlayerAnimSubsystem not found"));
		return;
	}

	const EWeaponType TargetAnimType = WeaponData;

	const FPlayerAnimSet* TargetAnimSet = PlayerAnimSubsystem->GetPlayerAnimSet(TargetAnimType);

	Locomotion_Normal_CycleBS = TargetAnimSet->Locomotion_Normal_CycleBS.IsNull() ? nullptr : TargetAnimSet->Locomotion_Normal_CycleBS.LoadSynchronous();
	Locomotion_Combat_Forward_BS = TargetAnimSet->Locomotion_Combat_Forward_BS.IsNull() ? nullptr : TargetAnimSet->Locomotion_Combat_Forward_BS.LoadSynchronous();
	Locomotion_Combat_Backward_BS = TargetAnimSet->Locomotion_Combat_Backward_BS.IsNull() ? nullptr : TargetAnimSet->Locomotion_Combat_Backward_BS.LoadSynchronous();
	Locomotion_Idle = TargetAnimSet->Locomotion_Idle.IsNull() ? nullptr : TargetAnimSet->Locomotion_Idle.LoadSynchronous();
	Locomotion_Start = TargetAnimSet->Locomotion_Start.IsNull() ? nullptr : TargetAnimSet->Locomotion_Start.LoadSynchronous();
	Locomotion_Stop_Jog = TargetAnimSet->Locomotion_Stop_Jog.IsNull() ? nullptr : TargetAnimSet->Locomotion_Stop_Jog.LoadSynchronous();
	Locomotion_Stop_Run = TargetAnimSet->Locomotion_Stop_Run.IsNull() ? nullptr : TargetAnimSet->Locomotion_Stop_Run.LoadSynchronous();

	Jump_Start = TargetAnimSet->Jump_Start.IsNull() ? nullptr : TargetAnimSet->Jump_Start.LoadSynchronous();
	Jump_Loop = TargetAnimSet->Jump_Loop.IsNull() ? nullptr : TargetAnimSet->Jump_Loop.LoadSynchronous();
	Fall_Loop = TargetAnimSet->Fall_Loop.IsNull() ? nullptr : TargetAnimSet->Fall_Loop.LoadSynchronous();
	Land_Jump = TargetAnimSet->Land_Jump.IsNull() ? nullptr : TargetAnimSet->Land_Jump.LoadSynchronous();
	Land_Fall = TargetAnimSet->Land_Fall.IsNull() ? nullptr : TargetAnimSet->Land_Fall.LoadSynchronous();
	Land_Jog = TargetAnimSet->Land_Jog.IsNull() ? nullptr : TargetAnimSet->Land_Jog.LoadSynchronous();
	Land_High = TargetAnimSet->Land_High.IsNull() ? nullptr : TargetAnimSet->Land_High.LoadSynchronous();

	HitAir_Start = TargetAnimSet->HitAir_Start.IsNull() ? nullptr : TargetAnimSet->HitAir_Start.LoadSynchronous();
	HitAir_Loop = TargetAnimSet->HitAir_Loop.IsNull() ? nullptr : TargetAnimSet->HitAir_Loop.LoadSynchronous();
	HitAir_End = TargetAnimSet->HitAir_End.IsNull() ? nullptr : TargetAnimSet->HitAir_End.LoadSynchronous();
	GetUp = TargetAnimSet->GetUp.IsNull() ? nullptr : TargetAnimSet->GetUp.LoadSynchronous();
	Guard = TargetAnimSet->Guard.IsNull() ? nullptr : TargetAnimSet->Guard.LoadSynchronous();
	HitAir_Death = TargetAnimSet->HitAir_Death.IsNull() ? nullptr : TargetAnimSet->HitAir_Death.LoadSynchronous();
	Ground_Death = TargetAnimSet->Ground_Death.IsNull() ? nullptr : TargetAnimSet->Ground_Death.LoadSynchronous();

	float WeaponIKAlpha = TargetAnimSet->bUseWeaponIK ? 1.0f : 0.0f;
	SetIKLayerAlpha_Native(FGameplayTag::RequestGameplayTag(FName("IK.Layer.Ground.HandWeapon")), ELimbList::HandL, WeaponIKAlpha);

	UE_LOG(Log_Anim_Equip, Log, TEXT("WeaponIKAlpha = %f"), WeaponIKAlpha);

	bInitAnimSet = true;

	UE_LOG(Log_Anim_Equip, Log, TEXT("[PlayerBaseAnimInstance] Character ID : %s Anim Has been Set By Weapon Change"), *Character->GetName());
}

void UPlayerBaseAnimInstance::AnimNotify_NOT_MountEnd()
{
	OnMountEnd.Broadcast();
}

void UPlayerBaseAnimInstance::AnimNotify_NOT_DisMountEnd()
{
	OnDisMountEnd.Broadcast();
}