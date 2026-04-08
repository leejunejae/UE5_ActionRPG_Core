// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/GameInstance/MainGameInstance.h"
#include "Core/Subsystems/GameInstanceSystem/WeaponDataSubsystem.h"
#include "Core/Subsystems/GameInstanceSystem/EnemyDataSubsystem.h"
#include "Core/Subsystems/GameInstanceSystem/PlayerAnimRegistrySubsystem.h"
//#include "UObject/SoftObjectPath.h"

void UMainGameInstance::Init()
{
	Super::Init();

    UWeaponDataSubsystem* WeaponSubsystem = GetSubsystem<UWeaponDataSubsystem>();
    if (WeaponSubsystem)
    {
        UE_LOG(LogTemp, Log, TEXT("WeaponSubSystem Loaded Successful"));
    }

    UPlayerAnimRegistrySubsystem* PlayerAnimRegistrySubsystem = GetSubsystem<UPlayerAnimRegistrySubsystem>();
    if (PlayerAnimRegistrySubsystem)
    {
        UE_LOG(LogTemp, Log, TEXT("PlayerAnimRegistrySubsystem Loaded Successful"));
    }

    UEnemyDataSubsystem* EnemyDataSubsystem = GetSubsystem<UEnemyDataSubsystem>();
    if (EnemyDataSubsystem)
    {
        UE_LOG(LogTemp, Log, TEXT("EnemyDataSubSystem Loaded Successful"));
    }
}