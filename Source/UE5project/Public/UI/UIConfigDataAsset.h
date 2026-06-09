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

	/** 시작 화면 레벨 (캐릭터 없는 빈 레벨) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	TSoftObjectPtr<UWorld> StartupMap;

	/** "게임 시작" 시 진입할 기본 게임플레이 레벨 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	TSoftObjectPtr<UWorld> DefaultGameMap;
};
