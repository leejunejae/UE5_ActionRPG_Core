// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Animation/Data/NPCAnimSetDataAsset.h"
#include "NPCAnimRegistrySubsystem.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UNPCAnimRegistrySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	const FAnimDataSet* GetAnimProfile(const FGameplayTag& SkeletonTag, const FGameplayTag& ProfileTag = FGameplayTag());

private:
	UPROPERTY(EditDefaultsOnly)
		TObjectPtr<UNPCAnimSetDataAsset> NPCAnimSet;
};
