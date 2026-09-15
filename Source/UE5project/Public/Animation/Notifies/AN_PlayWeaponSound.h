#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Items/Weapons/Data/WeaponAudioData.h"
#include "AN_PlayWeaponSound.generated.h"

/** 장착 무기 데이터에서 태그로 선택한 단발 사운드를 애니메이션 타이밍에 재생한다. */
UCLASS(DisplayName = "Play Weapon Sound")
class UE5PROJECT_API UAN_PlayWeaponSound : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Sound",
		meta = (Categories = "Audio.Weapon"))
	FGameplayTag WeaponSoundTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Sound")
	bool bSubWeapon = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Sound", meta = (ClampMin = "0.0"))
	float VolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Sound", meta = (ClampMin = "0.01"))
	float PitchMultiplier = 1.0f;
};
