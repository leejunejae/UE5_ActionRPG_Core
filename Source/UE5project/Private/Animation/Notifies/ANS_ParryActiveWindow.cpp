#include "Animation/Notifies/ANS_ParryActiveWindow.h"

#include "Combat/Components/HitReactionComponent.h"

FString UANS_ParryActiveWindow::GetNotifyName_Implementation() const
{
	return TEXT("Parry Active Window");
}

void UANS_ParryActiveWindow::NotifyBegin(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	if (!MeshComp) return;

	if (AActor* Owner = MeshComp->GetOwner())
	{
		if (UHitReactionComponent* HitReaction = Owner->FindComponentByClass<UHitReactionComponent>())
		{
			HitReaction->BeginParryActiveWindow();
		}
	}
}

void UANS_ParryActiveWindow::NotifyEnd(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	if (!MeshComp) return;

	if (AActor* Owner = MeshComp->GetOwner())
	{
		if (UHitReactionComponent* HitReaction = Owner->FindComponentByClass<UHitReactionComponent>())
		{
			HitReaction->EndParryActiveWindow();
		}
	}
}
