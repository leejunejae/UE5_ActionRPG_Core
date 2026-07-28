#include "Animation/Notifies/ANS_LadderGripTransition.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimNotifyLibrary.h"
#include "Characters/CharacterBase.h"
#include "Interaction/Climb/Components/ClimbComponent.h"
#include "Utils/CoreLog.h"

namespace
{
	UClimbComponent* GetClimbComponent(const USkeletalMeshComponent* MeshComp)
	{
		const ACharacterBase* Character =
			MeshComp ? Cast<ACharacterBase>(MeshComp->GetOwner()) : nullptr;
		return Character ? Character->GetClimbComponent() : nullptr;
	}
}

void UANS_LadderGripTransition::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(
		MeshComp,
		Animation,
		TotalDuration,
		EventReference);

	UClimbComponent* ClimbComponent = GetClimbComponent(MeshComp);
	if (!ClimbComponent)
	{
		return;
	}

	if (!ClimbComponent->BeginLimbGripTransition(Limb,Direction,TrajectoryCurve))
	{
		UE_LOG(Log_Climb_Ladder, Error, TEXT("[LadderGripTransition] Failed to begin %s transition."), *UEnum::GetValueAsString(Limb));
	}
}

void UANS_LadderGripTransition::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	UClimbComponent* ClimbComponent = GetClimbComponent(MeshComp);
	const FAnimNotifyEvent* Notify = EventReference.GetNotify();
	if (!ClimbComponent || !Notify)
	{
		return;
	}

	const float StartTime = Notify->GetTriggerTime();
	const float Duration = FMath::Max(Notify->GetDuration(), KINDA_SMALL_NUMBER);
	float NormalizedTime = 0.0f;

	if (const UAnimMontage* Montage = Cast<UAnimMontage>(Animation))
	{
		UAnimInstance* AnimInstance = MeshComp ? MeshComp->GetAnimInstance() : nullptr;
		if (!AnimInstance)
		{
			return;
		}

		NormalizedTime = FMath::Clamp((AnimInstance->Montage_GetPosition(Montage) - StartTime) /Duration,0.0f,1.0f);
	}
	else
	{
		NormalizedTime = UAnimNotifyLibrary::GetCurrentAnimationNotifyStateTimeRatio(EventReference);
	}

	ClimbComponent->UpdateLimbGripTransition(Limb, NormalizedTime);
}

void UANS_LadderGripTransition::NotifyEnd(USkeletalMeshComponent* MeshComp,UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (UClimbComponent* ClimbComponent = GetClimbComponent(MeshComp))
	{
		ClimbComponent->CompleteLimbGripTransition(Limb);
	}
}
