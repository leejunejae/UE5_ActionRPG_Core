// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetInterface.h"

/* WidgetMenu�� ����Ʈ�� ���̵����� �� ���Ǵ� �Լ� */
void UWidgetInterface::SetUp()
{
	// ����Ʈ�� �ش� �޴��� ���̵��� ��.
	this->AddToViewport();

	FInputModeUIOnly InputModeData;
	this->SetIsFocusable(true);
	InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

	UWorld* World = GetWorld();
	if (!ensure(World != nullptr)) return;

	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (!ensure(PlayerController != nullptr)) return;

	PlayerController->SetInputMode(InputModeData);
	PlayerController->SetShowMouseCursor(true);
}

/* WidgetMenu�� ����Ʈ���� ������ �� ����Ǵ� �Լ� */
void UWidgetInterface::OffWidget()
{
	this->RemoveFromParent();

	UWorld* World = GetWorld();
	if (!ensure(World != nullptr)) return;

	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (!ensure(PlayerController != nullptr)) return;

	FInputModeGameOnly InputModeData;

	// GameOnly���� InputMode ����
	PlayerController->SetInputMode(InputModeData);
	PlayerController->SetShowMouseCursor(false);
}