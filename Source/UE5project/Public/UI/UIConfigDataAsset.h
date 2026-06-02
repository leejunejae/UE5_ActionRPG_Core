// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UIConfigDataAsset.generated.h"

class UFullScreenWidget;
/**
 * 
 */
UCLASS(BlueprintType)
class UE5PROJECT_API UUIConfigDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	/** 게임 시작 시 생성할 전체 화면 위젯 클래스들 (시작 화면, 로딩 화면 등) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TArray<TSubclassOf<UFullScreenWidget>> FullScreenWidgetClasses;
};
