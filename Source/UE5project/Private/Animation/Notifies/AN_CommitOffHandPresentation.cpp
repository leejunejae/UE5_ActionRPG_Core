#include "Animation/Notifies/AN_CommitOffHandPresentation.h"

#include "Characters/Player/PlayerBase.h"
#include "Components/SkeletalMeshComponent.h"

void UAN_CommitOffHandPresentation::Notify(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	if (APlayerBase* Player = MeshComp ? Cast<APlayerBase>(MeshComp->GetOwner()) : nullptr)
	{
		Player->CommitOffHandPresentationTransition(TargetState);
	}
}

FString UAN_CommitOffHandPresentation::GetNotifyName_Implementation() const
{
	return TargetState == EWeaponPresentationState::Drawn
		? TEXT("Commit Off Hand: Drawn")
		: TEXT("Commit Off Hand: Holstered");
}
