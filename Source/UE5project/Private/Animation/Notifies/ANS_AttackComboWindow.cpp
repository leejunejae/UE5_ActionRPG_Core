#include "Animation/Notifies/ANS_AttackComboWindow.h"

#include "Combat/Components/AttackComponent.h"

FString UANS_AttackComboWindow::GetNotifyName_Implementation() const
{
	return TEXT("Attack Combo Window");
}

void UANS_AttackComboWindow::NotifyBegin(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	if (!MeshComp) return;

	UAttackComponent* Attack = MeshComp->GetOwner()
		? MeshComp->GetOwner()->FindComponentByClass<UAttackComponent>() : nullptr;
	if (!Attack) return;

	for (auto It = ActiveLeases.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid()) It.RemoveCurrent();
	}

	const uint64 LeaseId = Attack->AcquireComboInputWindow();
	if (LeaseId != 0)
	{
		ActiveLeases.FindOrAdd(MeshComp).Add(LeaseId);
	}
}

void UANS_AttackComboWindow::NotifyEnd(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	if (!MeshComp) return;

	UAttackComponent* Attack = MeshComp->GetOwner()
		? MeshComp->GetOwner()->FindComponentByClass<UAttackComponent>() : nullptr;
	if (!Attack) return;

	TArray<uint64>* Leases = ActiveLeases.Find(MeshComp);
	if (!Leases || Leases->IsEmpty()) return;

	const uint64 LeaseId = (*Leases)[0];
	Leases->RemoveAt(0);
	if (Leases->IsEmpty()) ActiveLeases.Remove(MeshComp);
	Attack->ReleaseComboInputWindow(LeaseId);
}
