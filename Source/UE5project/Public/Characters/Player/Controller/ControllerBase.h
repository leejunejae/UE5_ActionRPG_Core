// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Core/Subsystems/GameInstanceSystem/UIManagerSubsystem.h"
#include "ControllerBase.generated.h"

/**
 * 
 */

class UGameMenuWidget;
class UInGameMenuInputConfigDataAsset;
class UPlayerHUDWidget;
class ULevelStreaming;
class ACharacterPreviewActor;

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
	virtual void OnUnPossess() override;

#pragma region Screen
private:
	void InitializeFullScreenUI();

	void BindToPlayerDeath();
	void UnbindFromPlayerDeath();
	void HandlePlayerDeathFinalized();
#pragma endregion Screen

#pragma region HUD
private:
	void CreatePlayerHUD();
	void BindHUDToPawn();
	void SetupForPlayerPawn();

	UPROPERTY()
	TObjectPtr<UPlayerHUDWidget> PlayerHUDWidget;
#pragma endregion HUD

#pragma region InGame Menu
public:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UGameMenuWidget> GameMenuClass;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInGameMenuInputConfigDataAsset> InGameMenuInputConfig;

protected:
	virtual void SetupInputComponent() override;

private:
	void CreateGameMenu();
	void ToggleMenuTab(EGameMenuTab Tab);
	void DisablePlayerGameplayInput();
	void EnablePlayerGameplayInput();

	void Input_OpenStatus();
	void Input_OpenEquipment();
	void Input_OpenInventory();
	void Input_OpenSkills();
	void Input_OpenMap();

	UPROPERTY()
	TObjectPtr<UGameMenuWidget> GameMenuWidget;
#pragma endregion InGame Menu

#pragma region Respawn
public:
	/** 플레이어 부활 요청 (GameOverWidget에서 호출) */
	UFUNCTION(BlueprintCallable, Category = "Player")
	void RespawnPlayer();

private:
	void HandlePlayerRespawnFinalized();
#pragma endregion Respawn
};
