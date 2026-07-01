// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Items/Armor/Data/ArmorData.h"
#include "Items/Weapons/Data/WeaponData.h"
#include "CharacterPreviewActor.generated.h"

class UEquipmentComponent;
class APlayerBase;
class USceneCaptureComponent2D;

UCLASS()
class UE5PROJECT_API ACharacterPreviewActor : public AActor
{
	GENERATED_BODY()
	
public:
	ACharacterPreviewActor();

	/** 캡처 패널이 보이는 동안만 true — StatusTabWidget이 탭 전환 시 호출 */
	void SetCaptureActive(bool bActive);

	/** 플레이어 EquipmentComponent 델리게이트에 영구 바인딩 (자가 등록 직후 1회 호출됨) */
	void BindToPlayer(APlayerBase* Player);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Preview")
	TObjectPtr<USkeletalMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, Category = "Preview")
	TObjectPtr<UEquipmentComponent> EquipmentComponent;

	UPROPERTY(VisibleAnywhere, Category = "Preview")
	TObjectPtr<USceneCaptureComponent2D> CaptureComponent;

	UPROPERTY(EditAnywhere, Category = "Preview")
	TObjectPtr<UAnimationAsset> IdleAnimAsset;

	TWeakObjectPtr<APlayerBase> BoundPlayer;
	FDelegateHandle WeaponChangedHandle;
	FDelegateHandle ArmorChangedHandle;

	void PlayIdleAnimation();
	void SyncEquipmentFrom(APlayerBase* SourcePlayer);
	void HandleWeaponChanged(EWeaponType WeaponType);
	void HandleArmorChanged(EArmorSlot Slot);
};
