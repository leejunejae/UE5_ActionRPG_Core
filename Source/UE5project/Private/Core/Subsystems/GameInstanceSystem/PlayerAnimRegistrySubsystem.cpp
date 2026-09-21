// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Subsystems/GameInstanceSystem/PlayerAnimRegistrySubsystem.h"

void UPlayerAnimRegistrySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	TSoftObjectPtr<UPlayerAnimSetDataAsset> AnimSetRef = TSoftObjectPtr<UPlayerAnimSetDataAsset>(FSoftObjectPath(TEXT("/Game/00_Character/Data/AnimData/PlayerAnimSet_DA.PlayerAnimSet_DA")));
	if (AnimSetRef.IsValid() == false)
	{
		AnimSetRef.LoadSynchronous();
	}

	PlayerAnimAsset = AnimSetRef.Get();
}

FPlayerAnimSet UPlayerAnimRegistrySubsystem::ResolvePlayerAnimSet(FGameplayTag CombatStyle) const
{
	return PlayerAnimAsset ? PlayerAnimAsset->ResolvePlayerAnimSet(CombatStyle) : FPlayerAnimSet{};
}

FGameplayTag UPlayerAnimRegistrySubsystem::ResolveCombatStyle(
	EWeaponCategory MainWeaponCategory,
	EWeaponCategory OffHandWeaponCategory,
	EWeaponGripMode GripMode) const
{
	return PlayerAnimAsset
		? PlayerAnimAsset->ResolveCombatStyle(MainWeaponCategory, OffHandWeaponCategory, GripMode)
		: FGameplayTag();
}

UAnimMontage* UPlayerAnimRegistrySubsystem::ResolveCombatStyleTransition(
	FGameplayTag FromStyle, FGameplayTag ToStyle) const
{
	const FPlayerCombatStyleTransition* Transition = PlayerAnimAsset
		? PlayerAnimAsset->FindCombatStyleTransition(FromStyle, ToStyle) : nullptr;
	return Transition ? Transition->Montage.LoadSynchronous() : nullptr;
}
