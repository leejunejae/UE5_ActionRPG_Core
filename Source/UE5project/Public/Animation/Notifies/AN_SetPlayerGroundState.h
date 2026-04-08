// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Characters/Interfaces/CharacterStatusInterface.h"
#include "AN_SetPlayerGroundState.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UAN_SetPlayerGroundState : public UAnimNotify
{
	GENERATED_BODY()
	
public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notify")
		EGroundStance NewState;
};
