// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Player/Controller/ControllerBase.h"
#include "Core/Subsystems/GameInstanceSystem/UIManagerSubsystem.h"
#include "UI/PlayerHUDWidget.h"
#include "Characters/Player/PlayerBase.h"
#include "Characters/Player/Components/PlayerStatComponent.h"
#include "Characters/Player/Components/PlayerStatusComponent.h"

#include "UI/GameMenuWidget.h"
#include "Characters/Player/Controller/InGameMenuInputConfigDataAsset.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

void AControllerBase::BeginPlay()
{
    Super::BeginPlay();

    // 로컬 플레이어용에만 위젯 생성 (서버에선 안 함)
    if (!IsLocalPlayerController()) return;

    InitializeFullScreenUI();
    CreateGameMenu();
    SetupForPlayerPawn();
}

void AControllerBase::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    // BeginPlay 이후에 새 Pawn으로 갈아탈 경우 대응
    SetupForPlayerPawn();
}

void AControllerBase::OnUnPossess()
{
    UnbindFromPlayerDeath();

    Super::OnUnPossess();
}

void AControllerBase::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (!InGameMenuInputConfig) return;

    // Enhanced Input 서브시스템에 메뉴 IMC 등록
    // Priority 1 — PlayerBase의 게임플레이 IMC(Priority 0)보다 높게 설정해
    // 메뉴 키가 게임플레이 입력보다 먼저 처리되도록 함
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        if (InGameMenuInputConfig->MenuContext)
        {
            Subsystem->AddMappingContext(InGameMenuInputConfig->MenuContext, 1);
        }
    }

    UEnhancedInputComponent* EIC = CastChecked<UEnhancedInputComponent>(InputComponent);
    if (!EIC) return;

    if (InGameMenuInputConfig->IA_OpenStatus)
        EIC->BindAction(InGameMenuInputConfig->IA_OpenStatus, ETriggerEvent::Started, this, &AControllerBase::Input_OpenStatus);
    if (InGameMenuInputConfig->IA_OpenEquipment)
        EIC->BindAction(InGameMenuInputConfig->IA_OpenEquipment, ETriggerEvent::Started, this, &AControllerBase::Input_OpenEquipment);
    if (InGameMenuInputConfig->IA_OpenInventory)
        EIC->BindAction(InGameMenuInputConfig->IA_OpenInventory, ETriggerEvent::Started, this, &AControllerBase::Input_OpenInventory);
    if (InGameMenuInputConfig->IA_OpenOptions)
        EIC->BindAction(InGameMenuInputConfig->IA_OpenOptions, ETriggerEvent::Started, this, &AControllerBase::Input_OpenOptions);
    if (InGameMenuInputConfig->IA_OpenSkills)
        EIC->BindAction(InGameMenuInputConfig->IA_OpenSkills, ETriggerEvent::Started, this, &AControllerBase::Input_OpenSkills);
    if (InGameMenuInputConfig->IA_OpenMap)
        EIC->BindAction(InGameMenuInputConfig->IA_OpenMap, ETriggerEvent::Started, this, &AControllerBase::Input_OpenMap);
}

void AControllerBase::CreateGameMenu()
{
    if (!GameMenuClass || GameMenuWidget) return;

    GameMenuWidget = CreateWidget<UGameMenuWidget>(this, GameMenuClass);
    if (GameMenuWidget)
    {
        GameMenuWidget->AddToViewport(5); // HUD(ZOrder 0)보다 위
    }
}

void AControllerBase::ToggleMenuTab(EGameMenuTab Tab)
{
    if (!GameMenuWidget) return;

    if (!GameMenuWidget->IsOpen())
    {
        GameMenuWidget->OpenToTab(Tab);
    }
    else if (GameMenuWidget->GetActiveTab() == Tab)
    {
        GameMenuWidget->CloseMenu();
    }
    else
    {
        // 이미 열려있고 다른 탭 → 탭만 전환 (닫았다 열지 않음)
        GameMenuWidget->OpenToTab(Tab);
    }
}

void AControllerBase::Input_OpenStatus() { ToggleMenuTab(EGameMenuTab::Status); }
void AControllerBase::Input_OpenEquipment() { ToggleMenuTab(EGameMenuTab::Equipment); }
void AControllerBase::Input_OpenInventory() { ToggleMenuTab(EGameMenuTab::Inventory); }
void AControllerBase::Input_OpenSkills() { ToggleMenuTab(EGameMenuTab::Skills); }
void AControllerBase::Input_OpenMap() { ToggleMenuTab(EGameMenuTab::Map); }
void AControllerBase::Input_OpenOptions() { ToggleMenuTab(EGameMenuTab::Options); }

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