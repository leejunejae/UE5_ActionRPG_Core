#include "Animation/Notifies/ANS_CriticalExecutionWindow.h"

#include "Characters/Enemies/EnemyBase.h"

FString UANS_CriticalExecutionWindow::GetNotifyName_Implementation() const
{
	return TEXT("Critical Execution Window");
}

void UANS_CriticalExecutionWindow::NotifyBegin(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	if (!MeshComp) return;

	if (AEnemyBase* Enemy = Cast<AEnemyBase>(MeshComp->GetOwner()))
	{
		Enemy->BeginCriticalExecutionWindow();
	}
}

void UANS_CriticalExecutionWindow::NotifyEnd(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	if (!MeshComp) return;

	if (AEnemyBase* Enemy = Cast<AEnemyBase>(MeshComp->GetOwner()))
	{
		Enemy->EndCriticalExecutionWindow();
	}
}
