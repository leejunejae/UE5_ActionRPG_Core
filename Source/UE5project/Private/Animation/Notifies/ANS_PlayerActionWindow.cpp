// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/Notifies/ANS_PlayerActionWindow.h"
#include "Characters/Components/CharacterStatusComponent.h"

FString UANS_PlayerActionWindow::GetNotifyName_Implementation() const
{
	if (WindowsToOpen.IsEmpty())
		return TEXT("Action Window (Empty)");

	// 첫 번째 태그 이름을 표시명으로 사용
	TArray<FGameplayTag> Tags;
	WindowsToOpen.GetGameplayTagArray(Tags);

	FString Name = TEXT("Window: ");
	for (int32 i = 0; i < Tags.Num(); ++i)
	{
		if (i > 0) Name += TEXT(", ");
		Name += Tags[i].GetTagName().ToString();
	}
	return Name;
}

void UANS_PlayerActionWindow::NotifyBegin(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp) return;

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner) return;

	UCharacterStatusComponent* Status = Owner->FindComponentByClass<UCharacterStatusComponent>();
	if (!Status) return;

	TArray<FGameplayTag> Tags;
	WindowsToOpen.GetGameplayTagArray(Tags);

	for (const FGameplayTag& Tag : Tags)
	{
		Status->OpenWindow(Tag);
	}
}

void UANS_PlayerActionWindow::NotifyEnd(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!MeshComp) return;

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner) return;

	UCharacterStatusComponent* Status = Owner->FindComponentByClass<UCharacterStatusComponent>();
	if (!Status) return;

	UAnimInstance* AnimInstance = MeshComp->GetAnimInstance();
	if (!AnimInstance || !AnimInstance->Montage_IsPlaying(Cast<UAnimMontage>(Animation)))
		return;

	TArray<FGameplayTag> Tags;
	WindowsToOpen.GetGameplayTagArray(Tags);

	for (const FGameplayTag& Tag : Tags)
	{
		Status->CloseWindow(Tag);
	}
}
