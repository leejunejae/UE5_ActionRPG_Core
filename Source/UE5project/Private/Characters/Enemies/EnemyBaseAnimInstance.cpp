// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Enemies/EnemyBaseAnimInstance.h"
#include "Characters/Enemies/EnemyBase.h"
#include "Animation/Mode/AnimMode_Ground_NPC.h"

#include "Characters/Player/Components/PlayerStatusComponent.h"

#include "Utils/CoreLog.h"
#include "Utils/GameplayTagsBase.h"

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

	AnimModeMap.Add(TAG_State_Ground, GroundMode);

	for (auto& Pair : AnimModeMap)
	{
		if (!IsValid(Pair.Value))
		{
			// 무효 엔트리 발견 시 재빌드 or 건너뛰기
			// 여기선 안전하게 스킵
			continue;
		}
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
	Locomotion_BS = AnimData.Locomotion_CycleBS;
	DeathMontage = AnimData.DeathMontage;
}

void UEnemyBaseAnimInstance::HandleDeathStarted()
{
	Montage_Play(DeathMontage);
}
