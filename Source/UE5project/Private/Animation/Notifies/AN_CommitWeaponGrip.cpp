#include "Animation/Notifies/AN_CommitWeaponGrip.h"

#include "Characters/Player/PlayerBase.h"
#include "Components/SkeletalMeshComponent.h"

FString UAN_CommitWeaponGrip::GetNotifyName_Implementation() const
{
	return TargetGripMode == EWeaponGripMode::TwoHanded
		? TEXT("Commit Grip: Two Handed") : TEXT("Commit Grip: One Handed");
}

void UAN_CommitWeaponGrip::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	if (APlayerBase* Player = MeshComp ? Cast<APlayerBase>(MeshComp->GetOwner()) : nullptr)
	{
		Player->CommitGripModeTransition(TargetGripMode);
	}
}
