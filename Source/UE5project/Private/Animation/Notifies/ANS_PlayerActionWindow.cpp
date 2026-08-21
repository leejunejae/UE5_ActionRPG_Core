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

	for (auto It = ActiveWindowLeases.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}

	const uint64 LeaseId = Status->AcquireWindows(WindowsToOpen);
	if (LeaseId != 0)
	{
		ActiveWindowLeases.FindOrAdd(MeshComp).Add(LeaseId);
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

	TArray<uint64>* Leases = ActiveWindowLeases.Find(MeshComp);
	if (!Leases || Leases->IsEmpty())
	{
		return;
	}

	// 같은 Notify가 재생 중 다시 시작된 경우 Begin 순서대로 종료된다.
	const uint64 LeaseId = (*Leases)[0];
	Leases->RemoveAt(0);
	if (Leases->IsEmpty())
	{
		ActiveWindowLeases.Remove(MeshComp);
	}
	Status->ReleaseWindows(LeaseId);
}
