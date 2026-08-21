// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "ANS_PlayerActionWindow.generated.h"

/**
 * 
 */
UCLASS(DisplayName = "Action Window", meta = (ToolTip = "Opens/Closes Action Windows for Input Buffering"))
class UE5PROJECT_API UANS_PlayerActionWindow : public UAnimNotifyState
{
	GENERATED_BODY()
	
public:
	// 이 노티파이 구간 동안 열릴 Window 태그들
	UPROPERTY(EditAnywhere, Category = "Window", meta = (Categories = "Window"))
	FGameplayTagContainer WindowsToOpen;

	virtual FString GetNotifyName_Implementation() const override;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation, float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

private:
	// 같은 Notify 자산이 같은 Mesh에서 재진입해도 이전 Lease를 먼저 반납한다.
	TMap<TWeakObjectPtr<USkeletalMeshComponent>, TArray<uint64>> ActiveWindowLeases;
};
