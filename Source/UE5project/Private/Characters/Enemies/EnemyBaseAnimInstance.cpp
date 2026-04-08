// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Enemies/EnemyBaseAnimInstance.h"
#include "Characters/Enemies/EnemyBase.h"
#include "Animation/Mode/AnimMode_Ground_NPC.h"
#include "Utils/CoreLog.h"

UEnemyBaseAnimInstance::UEnemyBaseAnimInstance()
{

}

void UEnemyBaseAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	Enemy = Cast<AEnemyBase>(Character); // ★ Player 전용


	UAnimMode_Ground_NPC* GroundMode = NewObject<UAnimMode_Ground_NPC>(this);
	GroundMode->Character = Enemy;
	GroundMode->AnimInst = this;

	AnimModeMap.Add(ECharacterState::Ground, GroundMode);

	UE_LOG(Log_Anim, Log, TEXT("[EnemyBaseAnimInstance] Check AnimModeList"));

	for (auto& Pair : AnimModeMap)
	{
		if (!IsValid(Pair.Value))
		{
			// 무효 엔트리 발견 시 재빌드 or 건너뛰기
			// 여기선 안전하게 스킵
			continue;
		}

		UE_LOG(Log_Anim, Log, TEXT("[EnemyBaseAnimInstance] AnimMode : %s")
			, *StaticEnum<ECharacterState>()->GetValueAsString(Pair.Key));
	}
}

void UEnemyBaseAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	//UE_LOG(Log_Anim, Log, TEXT("[EnemyBaseAnimInstance] Direction : %f")
	//	, Direction);
}

void UEnemyBaseAnimInstance::InitAnimationData(FAnimDataSet AnimData)
{
	UE_LOG(Log_Spawn_NPC, Log, TEXT("[EnemyBaseAnimInstance] Init Animation Data"));
	Locomotion = AnimData;
	/*
	Locomotion_BS = AnimData.Locomotion_CycleBS;
	Locomotion_Idle = AnimData.Locomotion_Idle;
	Locomotion_Phase = AnimData.LocomotionPhaseAnim;
	/*
	Locomotion_WalkSlowBS = AnimData.Locomotion_WalkSlowBS.LoadSynchronous();
	Locomotion_WalkBS = AnimData.Locomotion_WalkBS.LoadSynchronous();
	Locomotion_JogBS = AnimData.Locomotion_JogBS.LoadSynchronous();
	Locomotion_RunBS = AnimData.Locomotion_RunBS.LoadSynchronous();
	*/
}
