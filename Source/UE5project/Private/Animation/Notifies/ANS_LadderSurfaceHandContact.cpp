#include "Animation/Notifies/ANS_LadderSurfaceHandContact.h"

#include "Characters/CharacterBase.h"
#include "Interaction/Climb/Components/ClimbComponent.h"
#include "Utils/CoreLog.h"

namespace
{
UClimbComponent* GetSurfaceHandContactClimbComponent(const USkeletalMeshComponent* MeshComp)
{
	const ACharacterBase* Character = MeshComp ? Cast<ACharacterBase>(MeshComp->GetOwner()) : nullptr;
	return Character ? Character->GetClimbComponent() : nullptr;
}
}

void UANS_LadderSurfaceHandContact::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	                                             float TotalDuration,
	                                             const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	UClimbComponent* ClimbComponent = GetSurfaceHandContactClimbComponent(MeshComp);
	if (!ClimbComponent)
	{
		return;
	}

	if (!ClimbComponent->BeginTopSurfaceHandContact(Hand, TraceUpDistance, TraceDownDistance, SurfaceOffset,
	                                                TraceChannel, this, bDrawDebug))
	{
		UE_LOG(Log_Climb_Ladder, Warning, TEXT("[LadderSurfaceHandContact] Failed to resolve %s contact."),
		       *UEnum::GetValueAsString(Hand));
	}
}

void UANS_LadderSurfaceHandContact::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	                                           const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	if (UClimbComponent* ClimbComponent = GetSurfaceHandContactClimbComponent(MeshComp))
	{
		ClimbComponent->EndTopSurfaceHandContact(Hand, this);
	}
}
