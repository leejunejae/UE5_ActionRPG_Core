// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/Components/PlayerAttackComponent.h"
#include "Characters/Player/PlayerBase.h"
#include "Characters/Components/EquipmentComponent.h"
#include "Combat/Data/DataAsset/PlayerAttackDataAsset.h"
#include "Utils/CoreLog.h"

const FBaseAttackData* UPlayerAttackComponent::ExecuteAttack(FName AttackName, float Playrate)
{
	APlayerBase* Player = Cast<APlayerBase>(GetOwner());
	if (!Player || !Player->GetEquipmentComponent() || !Player->GetEquipmentComponent()->GetEquipedWeapon()) return nullptr;
	
	return Super::ExecuteAttack(AttackName, Playrate);
}

void UPlayerAttackComponent::BeginPlay()
{
	Super::BeginPlay();

	APlayerBase* Player = Cast<APlayerBase>(GetOwner());
	if (!Player || !Player->GetEquipmentComponent()) return;
	auto& WeaponChangeSig = Player->GetEquipmentComponent()->OnWeaponSetChanged();//IEquipmentDataInterface::Execute_OnWeaponSetChanged(CachedEquipment);
	WeaponChangeSig.AddUObject(this, &UPlayerAttackComponent::SetCurAttackContextSet);
}

void UPlayerAttackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APlayerBase* Player = Cast<APlayerBase>(GetOwner()))
	{
		if (UEquipmentComponent* Equipment = Player->GetEquipmentComponent())
		{
			Equipment->OnWeaponSetChanged().RemoveAll(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void UPlayerAttackComponent::SetCurAttackContextSet(EWeaponType WeaponType)
{
	// 공격 도중 무기가 바뀌면 이전 무기의 Trace/몽타주가 새 장비로 이어지지 않게 세션을 종료한다.
	CancelAttack(EActionExitReason::EquipmentChange, true);

	if(!AttackList)
	{
		UE_LOG(Log_Equip_Weapon, Warning, TEXT("[UPlayerAttackComponent] Player Attack Data Not Valid"));
		return;
	}

	const FAttackContextSet* ContextSet = AttackList->FindPlayerAttackContext(WeaponType, /*bLogNotFound=*/true);

	if(APlayerBase* Player = Cast<APlayerBase>(GetOwner()))
		UE_LOG(Log_Attack, Log, TEXT("[PlayerAttackComponent] %s "), *Player->GetName());

	// 현재 
	CurAttackContextSet = ContextSet;
}
