// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ActionWindowRules.generated.h"

/** 현재 Action이 진행되는 동안 실패한 입력 요청을 어떻게 처리할지 정의한다. */
USTRUCT(BlueprintType)
struct FActionInputBufferPolicy
{
    GENERATED_BODY()

    /** 현재 Action 때문에 실행할 수 없는 다른 Action 입력을 버퍼에 저장한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input Buffer")
        bool bAllowBufferWhileActive = true;

    /** 이 Action이 시작될 때 이미 쌓여 있던 모든 입력 버퍼를 폐기한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input Buffer")
        bool bClearExistingBufferOnBegin = false;
};

UCLASS()
class UE5PROJECT_API UActionWindowRules : public UDataAsset
{
	GENERATED_BODY()
	
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Windows")
        TMap<FGameplayTag, FGameplayTagContainer> DefaultWindowsByState;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Windows")
        TMap<FGameplayTag, FGameplayTagContainer> CloseOnActionBegin;

    /** 항목이 없는 Action은 기본 정책(true/false)을 사용하여 기존 동작을 유지한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input Buffer")
        TMap<FGameplayTag, FActionInputBufferPolicy> InputBufferPolicies;
};
