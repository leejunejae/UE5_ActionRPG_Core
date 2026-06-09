// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Player/Controller/ControllerBase.h"
#include "Core/Subsystems/GameInstanceSystem/UIManagerSubsystem.h"
#include "UI/PlayerHUDWidget.h"
#include "Characters/Player/PlayerBase.h"
#include "Characters/Player/Components/PlayerStatComponent.h"
#include "Characters/Player/Components/PlayerStatusComponent.h"

void AControllerBase::BeginPlay()
{
    Super::BeginPlay();

    // 로컬 플레이어용에만 위젯 생성 (서버에선 안 함)
    if (!IsLocalPlayerController()) return;

    InitializeFullScreenUI();
    SetupForPlayerPawn();
}

void AControllerBase::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    // BeginPlay 이후에 새 Pawn으로 갈아탈 경우 대응
    SetupForPlayerPawn();
}

void AControllerBase::BindToPlayerDeath()
{
    APlayerBase* PlayerChar = Cast<APlayerBase>(GetPawn());
    if (!PlayerChar) return;

    if (UPlayerStatusComponent* StatusComp = PlayerChar->GetCharacterStatusComponent())
    {
        StatusComp->OnDeathFinalized.RemoveAll(this);
        StatusComp->OnDeathFinalized.AddUObject(this, &AControllerBase::HandlePlayerDeathFinalized);
    }
}

void AControllerBase::UnbindFromPlayerDeath()
{
    APlayerBase* PlayerChar = Cast<APlayerBase>(GetPawn());
    if (!PlayerChar) return;

    if (UPlayerStatusComponent* StatusComp = PlayerChar->GetCharacterStatusComponent())
    {
        StatusComp->OnDeathFinalized.RemoveAll(this);
    }
}

void AControllerBase::HandlePlayerDeathFinalized()
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UUIManagerSubsystem* UIMgr = GI->GetSubsystem<UUIManagerSubsystem>())
        {
            UIMgr->SetScreenState(EGameScreenState::GameOver);
        }
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

void AControllerBase::BindHUDToPawn()
{
    if (!PlayerHUDWidget) return;

    APlayerBase* PlayerChar = Cast<APlayerBase>(GetPawn());
    if (!PlayerChar) return;

    if (UPlayerStatComponent* StatComp = PlayerChar->GetStatComponent())
    {
        PlayerHUDWidget->BindToStatComponent(StatComp);
    }
}

void AControllerBase::SetupForPlayerPawn()
{
    // PlayerBase가 아니면(StartupMap 등) HUD를 만들지 않음
    if (!Cast<APlayerBase>(GetPawn())) return;

    CreatePlayerHUD();
    BindHUDToPawn();
    BindToPlayerDeath();
}

void AControllerBase::RespawnPlayer()
{
    APlayerBase* PlayerChar = Cast<APlayerBase>(GetPawn());
    if (!PlayerChar) return;

    UCharacterStatusComponent* StatusComp = PlayerChar->GetCharacterStatusComponent();
    if (!StatusComp) return;

    // 부활 완료 시 UI를 InGame으로 돌리기 위해 OnRespawnFinalized 구독
    // (1회만 듣고 해제할 거라 람다 + Handle 패턴은 안 씀, 단순 AddUObject)
    StatusComp->OnRespawnFinalized.AddUObject(
        this, &AControllerBase::HandlePlayerRespawnFinalized);

    // 부활 진입
    StatusComp->EnterRespawn();
}

void AControllerBase::HandlePlayerRespawnFinalized()
{
    // UI를 InGame으로 복귀
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UUIManagerSubsystem* UIMgr = GI->GetSubsystem<UUIManagerSubsystem>())
        {
            UIMgr->SetScreenState(EGameScreenState::InGame);
        }
    }

    // 중복 구독 방지를 위해 해제
    APlayerBase* PlayerChar = Cast<APlayerBase>(GetPawn());
    if (PlayerChar)
    {
        if (UCharacterStatusComponent* StatusComp = PlayerChar->GetCharacterStatusComponent())
        {
            StatusComp->OnRespawnFinalized.RemoveAll(this);
        }
    }
}