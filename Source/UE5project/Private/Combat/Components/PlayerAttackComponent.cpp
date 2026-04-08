// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/Components/PlayerAttackComponent.h"

#include "Characters/Player/PlayerBase.h"
#include "Characters/Components/EquipmentComponent.h"

#include "Utils/AnimBoneDataRegistryRoot.h"
#include "Engine/StaticMeshSocket.h"
#include "Combat/Interfaces/HitReactionInterface.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

#include "NiagaraSystem.h"
#include "NiagaraComponent.h"

#include "Utils/CoreLog.h"

void UPlayerAttackComponent::ExecuteAttack(FName AttackName, float Playrate)
{
	APlayerBase* Player = Cast<APlayerBase>(GetOwner());

	if (!Player->GetEquipmentComponent()->GetEquipedWeapon()) return;
	
	Super::ExecuteAttack(AttackName, Playrate);
}

void UPlayerAttackComponent::BeginPlay()
{
	Super::BeginPlay();

	APlayerBase* Player = Cast<APlayerBase>(GetOwner());
	auto& WeaponChangeSig = Player->GetEquipmentComponent()->OnWeaponSetChanged();//IEquipmentDataInterface::Execute_OnWeaponSetChanged(CachedEquipment);
	WeaponChangeSig.AddUObject(this, &UPlayerAttackComponent::SetCurAttackContextSet);
}

void UPlayerAttackComponent::SetCurAttackContextSet(EWeaponType WeaponData)
{
	const EWeaponType WeaponType = WeaponData;
	const UPlayerAttackDataAsset* TypedAsset = Cast<UPlayerAttackDataAsset>(AttackListDA);

	if (!TypedAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("AttackListDA 캐스팅 실패"));
		return;
	}

	FAttackContextSet ContextSet = TypedAsset->FindPlayerAttackContext(WeaponType, /*bLogNotFound=*/true);


	if(APlayerBase* Player = Cast<APlayerBase>(GetOwner()))
		UE_LOG(Log_Attack, Log, TEXT("[PlayerAttackComponent] %s "), *Player->GetName());

	// 현재 
	CurAttackContextSet = ContextSet;
}