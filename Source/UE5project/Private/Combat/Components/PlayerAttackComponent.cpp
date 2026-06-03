// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/Components/PlayerAttackComponent.h"
#include "Characters/Player/PlayerBase.h"
#include "Characters/Components/EquipmentComponent.h"
#include "Combat/Data/DataAsset/PlayerAttackDataAsset.h"
#include "Utils/CoreLog.h"

const FBaseAttackData* UPlayerAttackComponent::ExecuteAttack(FName AttackName, float Playrate)
{
	APlayerBase* Player = Cast<APlayerBase>(GetOwner());

	if (!Player->GetEquipmentComponent()->GetEquipedWeapon()) return nullptr;
	
	return Super::ExecuteAttack(AttackName, Playrate);
}

void UPlayerAttackComponent::BeginPlay()
{
	Super::BeginPlay();

	APlayerBase* Player = Cast<APlayerBase>(GetOwner());
	auto& WeaponChangeSig = Player->GetEquipmentComponent()->OnWeaponSetChanged();//IEquipmentDataInterface::Execute_OnWeaponSetChanged(CachedEquipment);
	WeaponChangeSig.AddUObject(this, &UPlayerAttackComponent::SetCurAttackContextSet);
}

void UPlayerAttackComponent::SetCurAttackContextSet(EWeaponType WeaponType)
{
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