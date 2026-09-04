#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_AttackComboWindow.generated.h"

/** Opens the attack component's combo-only input window. */
UCLASS(DisplayName = "Attack Combo Window",
	meta = (ToolTip = "Allows an existing attack context to advance to its next combo index"))
class UE5PROJECT_API UANS_AttackComboWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation, float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

private:
	TMap<TWeakObjectPtr<USkeletalMeshComponent>, TArray<uint64>> ActiveLeases;
};
