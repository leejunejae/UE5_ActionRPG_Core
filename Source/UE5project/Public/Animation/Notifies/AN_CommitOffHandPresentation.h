#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Items/Weapons/Data/WeaponData.h"
#include "AN_CommitOffHandPresentation.generated.h"

UCLASS(DisplayName = "Commit Off Hand Presentation")
class UE5PROJECT_API UAN_CommitOffHandPresentation : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Off Hand")
	EWeaponPresentationState TargetState = EWeaponPresentationState::Drawn;
};
