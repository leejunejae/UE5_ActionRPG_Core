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

DECLARE_MULTICAST_DELEGATE_OneParam(FOnWeaponChanged, const EWeaponType);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnArmorChanged, const EArmorSlot);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UE5PROJECT_API UEquipmentComponent : public UActorComponent,
	public IEquipmentDataInterface
{
	GENERATED_BODY()
	
public:
	UEquipmentComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

#pragma region Weapon
private:
	UPROPERTY(VisibleAnywhere, Category = "Equipment|Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Equipment|Weapon")
	TObjectPtr<UStaticMeshComponent> SubEquipMesh;

	UPROPERTY(VisibleAnywhere, Category = "Equipment|Weapon")
	FName WeaponSocket;

	UPROPERTY(VisibleAnywhere, Category = "Equipment|Weapon")
	FName SubEquipSocket;

	const FWeaponSetsInfo* EquipedWeapon = nullptr;
	FName EquipedWeaponKey = NAME_None;   // 키 캐싱

private:
	FAttackDamageSource CalculateAttackDamageSource(float StrengthBonus, float DexterityBonus, float AffinityBonus) const;

public:
	FORCEINLINE const FWeaponSetsInfo* GetEquipedWeapon() const { return EquipedWeapon; }
	FORCEINLINE FName GetEquipedWeaponKey() const { return EquipedWeaponKey; }

	FORCEINLINE UStaticMeshComponent* GetMainWeaponComponent() const { return WeaponMesh; }
	FORCEINLINE UStaticMeshComponent* GetSubEquipComponent() const { return SubEquipMesh; }

	virtual void EquipWeapon_Implementation(FName WeaponKey) override;
	FVector GetWeaponSocketLocation_Implementation(FName SocketName, bool IsSubWeapon) const;

	FAttackTraceSource GetAttackTraceSource(EAttackSourceType AttackSourceType) const;
	FAttackDamageSource GetAttackDamageSource() const;

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	FAttackDamageSource PreviewAttackDamageSource(float OverrideStrengthBonus, float OverrideDexterityBonus, float OverrideAffinityBonus) const;

	void SetWeaponSocketName(FName SocketName) { WeaponSocket = SocketName; }
	void SetSubEquipSocketName(FName SocketName) { SubEquipSocket = SocketName; }

	FOnWeaponChanged OnWeaponChangedDelegate;
	virtual FOnWeaponChanged& OnWeaponSetChanged() override { return OnWeaponChangedDelegate; }
#pragma endregion Weapon

#pragma region Armor
private:
	// ArmorMeshes   : 슬롯 → SkeletalMeshComponent
	// EquipedArmors : 슬롯 → 장착된 FArmorPieceInfo (없으면 nullptr)
	UPROPERTY(VisibleAnywhere, Category = "Equipment|Armor")
	TMap<EArmorSlot, TObjectPtr<USkeletalMeshComponent>> ArmorMeshes;
	TMap<EArmorSlot, const FArmorPieceInfo*> EquipedArmors;
	TMap<EArmorSlot, FName> EquipedArmorKeys;   // 키 캐싱

	void InitArmorMeshComponents(ACharacter* Character);

	// 방어구 슬롯 변경 후 호출 — 방어력·저항력 재계산 (무게는 RecalcEquipLoad가 별도 처리)
	void RecalcArmorStats();

public:
	// ArmorKey로 DataTable에서 FArmorPieceInfo를 조회하고,
	// ArmorDefinition 에셋의 ArmorSlot 값으로 슬롯을 자동 판단하여 장착
	void EquipArmor(FName ArmorKey);

	// 특정 슬롯을 명시적으로 해제
	void UnequipArmor(EArmorSlot Slot);

	FORCEINLINE const FArmorPieceInfo* GetEquipedArmor(EArmorSlot Slot) const
	{
		const FArmorPieceInfo* const* Found = EquipedArmors.Find(Slot);
		return Found ? *Found : nullptr;
	}

	FORCEINLINE FName GetEquipedArmorKey(EArmorSlot Slot) const
	{
		const FName* Found = EquipedArmorKeys.Find(Slot);
		return Found ? *Found : NAME_None;
	}

	FORCEINLINE USkeletalMeshComponent* GetArmorMeshComponent(EArmorSlot Slot) const
	{
		const TObjectPtr<USkeletalMeshComponent>* Found = ArmorMeshes.Find(Slot);
		return Found ? Found->Get() : nullptr;
	}

	FOnArmorChanged OnArmorChangedDelegate;
#pragma endregion Armor

#pragma region Shared
private:
	TScriptInterface<IStatInterface> CachedStat;

	// 무기/방어구 중 무엇이 바뀌든 호출 — 전체 장비 무게 재계산 후 StatComponent에 반영
	void RecalcEquipLoad();
#pragma endregion
};
