#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Items/Weapons/Data/WeaponData.h"
#include "AN_CommitWeaponGrip.generated.h"

/** 그립 전환 몽타주에서 손의 접촉이 바뀌는 정확한 시점에 런타임 파지 상태를 적용한다. */
UCLASS(DisplayName = "Commit Weapon Grip")
class UE5PROJECT_API UAN_CommitWeaponGrip : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Grip")
	EWeaponGripMode TargetGripMode = EWeaponGripMode::TwoHanded;
};
