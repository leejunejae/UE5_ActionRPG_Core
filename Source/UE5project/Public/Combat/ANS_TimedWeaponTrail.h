// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AnimNotifyState_TimedNiagaraEffect.h"
#include "ANS_TimedWeaponTrail.generated.h"

/**
 * 
 */

class IEquipmentDataInterface;

UCLASS()
class UE5PROJECT_API UANS_TimedWeaponTrail : public UAnimNotifyState_TimedNiagaraEffect
{
	GENERATED_BODY()
	
public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, const FAnimNotifyEventReference& EventReference) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData")
		bool bSubWeapon = false;

protected:
	TScriptInterface<IEquipmentDataInterface> CachedWeaponInterface;
};
