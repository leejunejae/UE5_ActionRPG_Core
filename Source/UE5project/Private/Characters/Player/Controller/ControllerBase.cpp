// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Player/Controller/ControllerBase.h"
#include "Core/Subsystems/GameInstanceSystem/UIManagerSubsystem.h"
#include "UI/PlayerHUDWidget.h"
#include "Characters/Player/PlayerBase.h"
#include "Characters/Player/Components/PlayerStatComponent.h"

void AControllerBase::BeginPlay()
{
    Super::BeginPlay();

    // 로컬 플레이어용에만 위젯 생성 (서버에선 안 함)
    if (!IsLocalPlayerController()) return;

    InitializeFullScreenUI();
    CreatePlayerHUD();

    // 이미 Possess된 Pawn이 있으면 바로 연결
    // (BeginPlay 전에 OnPossess가 끝났을 수 있음)
    if (APawn* CurrentPawn = GetPawn())
    {
        BindHUDToPawn(CurrentPawn);
    }
}

void AControllerBase::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    // BeginPlay 이후에 새 Pawn으로 갈아탈 경우 대응
    if (PlayerHUDWidget && InPawn)
    {
        BindHUDToPawn(InPawn);
    }
}

void AControllerBase::InitializeFullScreenUI()
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UUIManagerSubsystem* UIMgr = GI->GetSubsystem<UUIManagerSubsystem>())
        {
            UIMgr->CreateFullScreenWidgets(this);
        }
    }
}

void AControllerBase::CreatePlayerHUD()
{
    if (!PlayerHUDClass) return;
    if (PlayerHUDWidget) return;  // 중복 생성 방지

    PlayerHUDWidget = CreateWidget<UPlayerHUDWidget>(this, PlayerHUDClass);
    if (PlayerHUDWidget)
    {
        PlayerHUDWidget->AddToViewport();
        // GameHUDWidget이 알아서 InGame 상태일 때만 보이게 처리함
    }
}

void AControllerBase::BindHUDToPawn(APawn* InPawn)
{
    if (!PlayerHUDWidget) return;
    if (!InPawn) return;

    APlayerBase* PlayerChar = Cast<APlayerBase>(InPawn);
    if (!PlayerChar) return;

    if (UPlayerStatComponent* StatComp = PlayerChar->GetStatComponent())
    {
        PlayerHUDWidget->BindToStatComponent(StatComp);
    }
}