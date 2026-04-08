// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Utils/AnimBoneDataRegistryRoot.h"
#include "AnimBoneDataSubsystem.generated.h"

/**
 * 
 */

struct FBoneTransformSegment;

UCLASS()
class UE5PROJECT_API UAnimBoneDataSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	const FBoneTransformSegment* GetAnimBoneData(const FGameplayTag& SkeletonTag, const UAnimSequence* AnimKey, FName DataKey);

private:
	UPROPERTY(EditDefaultsOnly)
		TObjectPtr<UAnimBoneDataRegistryRoot> AnimBoneDataRegistryRoot;

	
};
