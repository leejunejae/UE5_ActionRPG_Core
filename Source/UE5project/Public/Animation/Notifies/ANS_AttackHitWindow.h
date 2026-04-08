// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_AttackHitWindow.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UANS_AttackHitWindow : public UAnimNotifyState
{
	GENERATED_BODY()
	
public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, const FAnimNotifyEventReference& EventReference) override;

	UPROPERTY(EditAnywhere)
		FName WindowName;

protected:
	float PrevTime = 0.0f;
	float EndTime = 0.0f;
	bool bFinished = false;
	
	UPROPERTY(EditAnywhere)
		bool bDrawDebug = false;
};
