// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Subsystems/GameInstanceSystem/AnimBoneDataSubsystem.h"
#include "Utils/AttackBoneDataRegistry.h"
#include "Utils/CoreLog.h"

void UAnimBoneDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	TSoftObjectPtr<UAnimBoneDataRegistryRoot> AnimBoneDataRef = TSoftObjectPtr<UAnimBoneDataRegistryRoot>(FSoftObjectPath(TEXT("/Game/04_Animations/Data/AnimBoneRootRegistry.AnimBoneRootRegistry")));

	AnimBoneDataRegistryRoot = AnimBoneDataRef.LoadSynchronous();

	if (!AnimBoneDataRegistryRoot)
	{
		UE_LOG(Log_Anim, Error, TEXT("[NPCAnimRegistrySubsystem] NPC Anim DataSet not found"));
		return;
	}
}

const FBoneTransformSegment* UAnimBoneDataSubsystem::GetAnimBoneData(const FGameplayTag& SkeletonTag, const UAnimSequence* AnimKey, FName DataKey)
{
	if (!AnimBoneDataRegistryRoot)
	{
		UE_LOG(Log_Anim, Error, TEXT("[UAnimBoneDataSubsystem] AnimBonedDataRegistryRoot not found"));
		return nullptr;
	}

	if (!AnimKey)
	{
		UE_LOG(Log_Anim, Error, TEXT("[UAnimBoneDataSubsystem] AnimKey is null"));
		return nullptr;
	}

	TSoftObjectPtr<UAttackBoneDataRegistry>* AnimBoneDataPtr = AnimBoneDataRegistryRoot->AnimBoneDataRegistry.Find(SkeletonTag);

	if (!AnimBoneDataPtr)
	{
		UE_LOG(Log_Anim, Error,
			TEXT("[UAnimBoneDataSubsystem] No AnimBoneDataRegistry for SkeletonTag: %s"),
			*SkeletonTag.ToString());
		return nullptr;
	}

	if (!AnimBoneDataPtr->IsValid()) AnimBoneDataPtr->LoadSynchronous();

	const UAttackBoneDataRegistry* AnimBoneData = AnimBoneDataPtr->Get();

	if(!AnimBoneData)
	{
		UE_LOG(Log_Anim, Error,
			TEXT("[UAnimBoneDataSubsystem] AnimProfile asset is null for SkeletonTag: %s"),
			*SkeletonTag.ToString());
		return nullptr;
	}
	
	const FHitDataEntry* FoundDataEntry = AnimBoneData->Find(AnimKey);

	if (!FoundDataEntry)
	{
		UE_LOG(Log_Anim, Error,
			TEXT("[UAnimBoneDataSubsystem] Cant Find Any Entry for Anim: %s"),
			*AnimKey->GetName());
		return nullptr;
	}

	const FBoneTransformSegment* OutData = FoundDataEntry->Segments.Find(DataKey);

	return OutData;
}
