#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Characters/Data/IKData.h"
#include "Interaction/Climb/Data/ClimbHeader.h"
#include "ANS_LadderGripTransition.generated.h"

class UCurveVector;

UCLASS(meta = (DisplayName = "Ladder Grip Transition"))
class UE5PROJECT_API UANS_LadderGripTransition : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Ladder Grip")
	ELimbList Limb = ELimbList::FootL;

	UPROPERTY(EditAnywhere, Category = "Ladder Grip")
	ELadderGripDirection Direction = ELadderGripDirection::Down;

	UPROPERTY(EditAnywhere, Category = "Ladder Grip")
	TObjectPtr<UCurveVector> TrajectoryCurve;

	virtual void NotifyBegin(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyTick(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float FrameDeltaTime,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
