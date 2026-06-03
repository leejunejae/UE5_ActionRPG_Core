// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/Components/PlayerHitReactionComponent.h"
#include "Characters/Player/PlayerBase.h"
#include "Characters/Components/EquipmentComponent.h"
#include "Combat/Data/DataAsset/PlayerHitReactionDataAsset.h"
#include "Utils/CoreLog.h"

void UPlayerHitReactionComponent::BeginPlay()
{
    Super::BeginPlay();

    APlayerBase* Player = Cast<APlayerBase>(GetOwner());
    if (!Player) return;

    auto& WeaponChangeSig = Player->GetEquipmentComponent()->OnWeaponSetChanged();
    WeaponChangeSig.AddUObject(this, &UPlayerHitReactionComponent::HandleWeaponChange);
}

void UPlayerHitReactionComponent::HandleWeaponChange(EWeaponType WeaponType)
{
    if (!HitReactionList)
    {
        UE_LOG(Log_Equip_Weapon, Warning, TEXT("[UPlayerHitReactionComponent] Player HitReaction Data Not Valid"));
        return;
    }

    // 베이스가 소비하는 활성 DA만 교체 (어택의 CurAttackContextSet 갈아끼기와 동일 위치)
    SetHitReactionDA(HitReactionList->FindHitReactionDA(WeaponType, /*bLogNotFound=*/true));
}