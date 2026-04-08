// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Subsystems/GameInstanceSystem/NPCAnimRegistrySubsystem.h"
#include "Utils/CoreLog.h"

void UNPCAnimRegistrySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

	UE_LOG(Log_Spawn_NPC, Log, TEXT("[NPCAnimRegistrySubsystem] NPCAnimRegistrySubsystem Initialize"));

	TSoftObjectPtr<UNPCAnimSetDataAsset> AnimSetRef = TSoftObjectPtr<UNPCAnimSetDataAsset>(FSoftObjectPath(TEXT("/Game/03_Enemy/Human/AnimData/NPCAnimDataSet_DA.NPCAnimDataSet_DA")));
	if (AnimSetRef.IsValid() == false)
	{
		UE_LOG(Log_Spawn_NPC, Error, TEXT("[NPCAnimRegistrySubsystem] NPC Anim DataSet not found"));
		AnimSetRef.LoadSynchronous();
	}

	NPCAnimSet = AnimSetRef.Get();
}

const FAnimDataSet* UNPCAnimRegistrySubsystem::GetAnimProfile(const FGameplayTag& SkeletonTag, const FGameplayTag& ProfileTag)
{
	if (!SkeletonTag.IsValid())
	{
		UE_LOG(Log_Spawn_NPC, Error, TEXT("[NPCAnimRegistrySubsystem] Skeleton Tag Not Valid"));
		return nullptr;
	}

	if (!NPCAnimSet)
	{
		UE_LOG(Log_Spawn_NPC, Error, TEXT("[NPCAnimRegistrySubsystem] NPCAnimSet is null"));
		return nullptr;
	}

	TSoftObjectPtr<UNPCAnimProfileDataAsset>* AnimProfilePtr =
		NPCAnimSet->NPCAnimSets.Find(SkeletonTag);

	if (!AnimProfilePtr)
	{
		UE_LOG(Log_Spawn_NPC, Error,
			TEXT("[AnimRegistry] No AnimProfile entry for SkeletonTag: %s"),
			*SkeletonTag.ToString());
		return nullptr;
	}

	// 2) 소프트 포인터 로드
	if (!AnimProfilePtr->IsValid())
	{
		AnimProfilePtr->LoadSynchronous();
	}

	const UNPCAnimProfileDataAsset* AnimProfile = AnimProfilePtr->Get();
	if (!AnimProfile)
	{
		UE_LOG(Log_Spawn_NPC, Error,
			TEXT("[AnimRegistry] AnimProfile asset is null for SkeletonTag: %s"),
			*SkeletonTag.ToString());
		return nullptr;
	}

	FGameplayTag FindProfile = ProfileTag;
	if (!FindProfile.IsValid())
	{
		FindProfile = FGameplayTag::RequestGameplayTag(TEXT("Weapon.Unarmed"));
	}

	const FAnimDataSet* FoundProfile = AnimProfile->AnimProfiles.Find(FindProfile);
	if (!FoundProfile)
	{
		UE_LOG(Log_Spawn_NPC, Error,
			TEXT("[AnimRegistry] AnimProfile '%s' not found in asset '%s' (SkeletonTag: %s)"),
			*FindProfile.ToString(),
			*AnimProfile->GetName(),
			*SkeletonTag.ToString());
	}

	return FoundProfile;
}
