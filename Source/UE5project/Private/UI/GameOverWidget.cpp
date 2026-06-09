// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/GameOverWidget.h"
#include "Characters/Player/Controller/ControllerBase.h"
#include "Core/Subsystems/GameInstanceSystem/UIManagerSubsystem.h"
#include "Components/Button.h"

void UGameOverWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (RestartButton)
    {
        RestartButton->OnClicked.AddDynamic(this, &UGameOverWidget::OnRestartClicked);
    }
    if (MainMenuButton)
    {
        MainMenuButton->OnClicked.AddDynamic(this, &UGameOverWidget::OnMainMenuClicked);
    }

    if (FadeInSequence)
    {
        FadeInFinishedDelegate.BindDynamic(this, &UGameOverWidget::OnFadeInFinished);
        BindToAnimationFinished(FadeInSequence, FadeInFinishedDelegate);
    }
}

void UGameOverWidget::NativeDestruct()
{
    // 진행 중인 애니메이션이 있으면 정지
    if (FadeInSequence && IsAnimationPlaying(FadeInSequence))
    {
        StopAnimation(FadeInSequence);
    }
    Super::NativeDestruct();
}

void UGameOverWidget::OnRestartClicked()
{
    // Step 5에서 진짜 부활 로직 추가
    // 일단은 단순히 InGame 상태로 복귀
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UUIManagerSubsystem* UIMgr = GI->GetSubsystem<UUIManagerSubsystem>())
        {
            UIMgr->SetScreenState(EGameScreenState::InGame);
        }
    }

    if (APlayerController* PC = GetOwningPlayer())
    {
        if (AControllerBase* ControllerBase = Cast<AControllerBase>(PC))
        {
            ControllerBase->RespawnPlayer();
        }
    }
}

void UGameOverWidget::OnMainMenuClicked()
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UUIManagerSubsystem* UIMgr = GI->GetSubsystem<UUIManagerSubsystem>())
        {
            UIMgr->TravelToStartupMap();
        }
    }
}

void UGameOverWidget::ShowWidget()
{
    Super::ShowWidget();

    // 버튼 비활성화 (애니메이션 끝나면 활성화됨)
    if (RestartButton) RestartButton->SetIsEnabled(false);
    if (MainMenuButton) MainMenuButton->SetIsEnabled(false);

    // 페이드인 시작
    if (FadeInSequence)
    {
        PlayAnimation(FadeInSequence);
    }
}

void UGameOverWidget::HideWidget()
{
    Super::HideWidget();

    // 다음 표시를 위해 애니메이션 정지 + 버튼 상태 리셋
    if (FadeInSequence && IsAnimationPlaying(FadeInSequence))
    {
        StopAnimation(FadeInSequence);
    }
}

void UGameOverWidget::OnFadeInFinished()
{
    // 페이드인 끝나면 버튼 활성화
    if (RestartButton) RestartButton->SetIsEnabled(true);
    if (MainMenuButton) MainMenuButton->SetIsEnabled(true);
}