// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ControllerBase.generated.h"

/**
 * 
 */

class UPlayerHUDWidget;

UCLASS()
class UE5PROJECT_API AControllerBase : public APlayerController
{
	GENERATED_BODY()

public:
	/** HUD에 띄울 위젯 클래스 (블루프린트에서 WBP_PlayerHUD로 지정) */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UPlayerHUDWidget> PlayerHUDClass;

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;

private:
	void InitializeFullScreenUI();
	void CreatePlayerHUD();
	void BindHUDToPawn();
	void SetupForPlayerPawn();

	void BindToPlayerDeath();
	void UnbindFromPlayerDeath();
	void HandlePlayerDeathFinalized();

	UPROPERTY()
	TObjectPtr<UPlayerHUDWidget> PlayerHUDWidget;

#pragma region Respawn
public:
	/** 플레이어 부활 요청 (GameOverWidget에서 호출) */
	UFUNCTION(BlueprintCallable, Category = "Player")
	void RespawnPlayer();

private:
	void HandlePlayerRespawnFinalized();
#pragma endregion Respawn
};
