// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

// 인터페이스
#include "Characters/Interfaces/EquipmentDataInterface.h"
#include "Characters/Interfaces/StatInterface.h"

#include "Items/Weapons/Data/WeaponData.h"
#include "Items/Armor/Data/ArmorData.h"
#include "Combat/Data/AttackData.h"

#include "EquipmentComponent.generated.h"

class ACharacter;
class UArmorDataAsset;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnEquipmentMulDel, const EWeaponType);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UE5PROJECT_API UEquipmentComponent : public UActorComponent,
	public IEquipmentDataInterface
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UEquipmentComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	TScriptInterface<IStatInterface> CachedStat;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	
/* ============================================================
*  무기
* ============================================================ */
private:
	UPROPERTY(VisibleAnywhere, Category = "Equipment|Weapon")
		TObjectPtr<UStaticMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Equipment|Weapon")
		TObjectPtr<UStaticMeshComponent> SubEquipMesh;

	UPROPERTY(VisibleAnywhere, Category = "Equipment|Weapon")
		FName WeaponSocket;

	UPROPERTY(VisibleAnywhere, Category = "Equipment|Weapon")
		FName SubEquipSocket;

	const FWeaponSetsInfo* EquipedWeapon;

public:
	FORCEINLINE const FWeaponSetsInfo* GetEquipedWeapon() const { return EquipedWeapon; }
	FORCEINLINE UStaticMeshComponent* GetMainWeaponComponent() const { return WeaponMesh; }
	FORCEINLINE UStaticMeshComponent* GetSubEquipComponent() const { return SubEquipMesh; }

	void EquipWeapon_Implementation(FName WeaponKey) override;
	FVector GetWeaponSocketLocation_Implementation(FName SocketName, bool IsSubWeapon) const;

	FAttackTraceSource GetAttackTraceSource(EAttackSourceType AttackSourceType) const;
	FAttackDamageSource GetAttackDamageSource() const;

	void SetWeaponSocketName(FName SocketName) { WeaponSocket = SocketName; }
	void SetSubEquipSocketName(FName SocketName) { SubEquipSocket = SocketName; }

	FOnEquipmentMulDel OnWeaponChangedDelegate;
	virtual FOnEquipmentMulDel& OnWeaponSetChanged() override { return OnWeaponChangedDelegate; }


/* ============================================================
 *  방어구
 *  ArmorMeshes[i]    : (uint8)EArmorSlot 인덱스의 SkeletalMeshComponent
 *  EquipedArmors[i]  : 해당 슬롯에 장착된 FArmorPieceInfo 포인터 (없으면 nullptr)
 * ============================================================ */
private:
	UPROPERTY(VisibleAnywhere, Category = "Equipment|Armor")
	TMap<EArmorSlot, TObjectPtr<USkeletalMeshComponent>> ArmorMeshes;

	TMap<EArmorSlot, const FArmorPieceInfo*> EquipedArmors;

	void InitArmorMeshComponents(ACharacter* Character);

	/** 장착 슬롯 변경 후 호출 — StatComponent의 방어력·저항력·장비 하중 재계산 */
	void RecalcArmorStats();

public:
	void EquipArmor(FName ArmorKey);

	/**
	 * 특정 슬롯을 명시적으로 해제.
	 */
	void UnequipArmor(EArmorSlot Slot);

	/** 슬롯에 장착된 방어구 정보 반환 (없으면 nullptr) */
	FORCEINLINE const FArmorPieceInfo* GetEquipedArmor(EArmorSlot Slot) const
	{
		return EquipedArmors.Contains(Slot) ? EquipedArmors[Slot] : nullptr;
	}

	/** 슬롯의 SkeletalMeshComponent 반환 */
	FORCEINLINE USkeletalMeshComponent* GetArmorMeshComponent(EArmorSlot Slot) const
	{
		const TObjectPtr<USkeletalMeshComponent>* Found = ArmorMeshes.Find(Slot);
		return Found ? Found->Get() : nullptr;
	}
};
