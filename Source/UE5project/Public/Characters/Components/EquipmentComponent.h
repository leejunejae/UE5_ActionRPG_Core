// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

// 인터페이스
#include "Characters/Interfaces/EquipmentDataInterface.h"
#include "Characters/Interfaces/WeaponRuntimeSourceInterface.h"

#include "Items/Weapons/Data/WeaponData.h"
#include "Items/Armor/Data/ArmorData.h"
#include "Combat/Data/AttackData.h"

#include "EquipmentComponent.generated.h"

class ACharacter;
class UArmorDataAsset;
class UNiagaraSystem;
class UMaterialInterface;
class USoundBase;
class UWeaponDataAsset;
class UWeaponEquipmentDataAsset;
class UOffHandWeaponDataAsset;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnWeaponChanged, FGameplayTag);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnArmorChanged, const EArmorSlot);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UE5PROJECT_API UEquipmentComponent : public UActorComponent,
	public IEquipmentDataInterface,
	public IWeaponRuntimeSourceInterface
{
	GENERATED_BODY()
	
public:
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
	const FOffHandWeaponSetsInfo* EquippedOffHandWeapon = nullptr;
	FName EquippedOffHandWeaponKey = NAME_None;

	/** 장착 중에는 soft reference로 로드한 정의 에셋을 강하게 유지한다. */
	UPROPERTY(Transient)
	TObjectPtr<UWeaponDataAsset> EquippedWeaponDefinition = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UOffHandWeaponDataAsset> EquippedOffHandWeaponDefinition = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Equipment|Weapon")
	EWeaponGripMode CurrentGripMode = EWeaponGripMode::OneHanded;

	UPROPERTY(VisibleAnywhere, Category = "Equipment|Weapon")
	EWeaponPresentationState OffHandPresentationState = EWeaponPresentationState::Holstered;

	UPROPERTY(VisibleAnywhere, Category = "Equipment|Weapon")
	FGameplayTag CurrentCombatStyle;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> MainWeaponTrailSystem = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> SubWeaponTrailSystem = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> MainWeaponTrailMaterial = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> SubWeaponTrailMaterial = nullptr;

	UPROPERTY(Transient)
	TMap<FGameplayTag, FLoadedWeaponSoundSet> CachedWeaponSounds;

	UPROPERTY(Transient)
	TMap<FGameplayTag, FLoadedWeaponSoundSet> CachedOffHandWeaponSounds;

private:
	void GetCurrentAttackBonuses(float& OutStrengthBonus, float& OutDexterityBonus, float& OutAffinityBonus) const;
	void CacheWeaponSounds(const UWeaponEquipmentDataAsset* WeaponDefinition,
		TMap<FGameplayTag, FLoadedWeaponSoundSet>& OutSounds);
	void RefreshOffHandPresentation();
	void RefreshCombatStyle();
	FGameplayTag ResolveCombatStyle() const;
	FGameplayTag ResolveCombatStyleForState(EWeaponGripMode GripMode,
		EWeaponPresentationState PresentationState) const;

public:
	FORCEINLINE const FWeaponSetsInfo* GetEquipedWeapon() const { return EquipedWeapon; }
	FORCEINLINE FName GetEquipedWeaponKey() const { return EquipedWeaponKey; }
	FORCEINLINE const FOffHandWeaponSetsInfo* GetEquippedOffHandWeapon() const { return EquippedOffHandWeapon; }
	FORCEINLINE FName GetEquippedOffHandWeaponKey() const { return EquippedOffHandWeaponKey; }
	FORCEINLINE EWeaponGripMode GetCurrentGripMode() const { return CurrentGripMode; }
	FORCEINLINE EWeaponPresentationState GetOffHandPresentationState() const { return OffHandPresentationState; }
	bool IsOffHandActive() const;
	bool CanSetOffHandPresentationState(EWeaponPresentationState NewState) const;
	bool CanSetWeaponUseState(EWeaponGripMode NewGripMode,
		EWeaponPresentationState NewPresentationState) const;
	FORCEINLINE FGameplayTag GetCurrentCombatStyle() const { return CurrentCombatStyle; }
	FGameplayTag ResolveCombatStyleForGripMode(EWeaponGripMode GripMode) const;
	FGameplayTag ResolveCombatStyleForWeaponState(EWeaponGripMode GripMode,
		EWeaponPresentationState PresentationState) const;

	FORCEINLINE UStaticMeshComponent* GetMainWeaponComponent() const { return WeaponMesh; }
	FORCEINLINE UStaticMeshComponent* GetSubEquipComponent() const { return SubEquipMesh; }

	virtual void EquipWeapon_Implementation(FName WeaponKey) override;
	UFUNCTION(BlueprintCallable, Category = "Equipment|Weapon")
	void UnequipWeapon();
	UFUNCTION(BlueprintCallable, Category = "Equipment|Weapon")
	void EquipOffHandWeapon(FName WeaponKey);

	UFUNCTION(BlueprintCallable, Category = "Equipment|Weapon")
	void UnequipOffHandWeapon();

	/** 장착 슬롯은 유지한 채 보조무기를 손과 보관 소켓 사이에서 전환한다. */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Weapon")
	bool SetOffHandPresentationState(EWeaponPresentationState NewState);
	UFUNCTION(BlueprintCallable, Category = "Equipment|Weapon")
	bool ToggleOffHandPresentation();
	UFUNCTION(BlueprintCallable, Category = "Equipment|Weapon")
	bool SetWeaponUseState(EWeaponGripMode NewGripMode,
		EWeaponPresentationState NewPresentationState);

	/** 파지 상태만 바꾸며 독립 보조 슬롯은 유지하고 활성/보관 표현만 갱신한다. */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Weapon")
	bool SetGripMode(EWeaponGripMode NewGripMode);
	UFUNCTION(BlueprintPure, Category = "Equipment|Weapon")
	bool CanSetGripMode(EWeaponGripMode NewGripMode) const;
	virtual FVector GetWeaponSocketLocation_Implementation(FName SocketName, bool IsSubWeapon) const override;
	virtual UNiagaraSystem* GetWeaponTrailSystem_Implementation(bool IsSubWeapon) const override;
	virtual UMaterialInterface* GetWeaponTrailMaterial_Implementation(bool IsSubWeapon) const override;
	virtual FName GetWeaponTrailStartSocket_Implementation(bool IsSubWeapon) const override;
	virtual FName GetWeaponTrailEndSocket_Implementation(bool IsSubWeapon) const override;
	virtual USoundBase* GetWeaponSound_Implementation(FGameplayTag WeaponSoundTag, bool IsSubWeapon) const override;

	FAttackTraceSource GetAttackTraceSource(EAttackSourceType AttackSourceType) const;
	FAttackDamageSource GetAttackDamageSource(EAttackSourceType AttackSourceType = EAttackSourceType::MainHand) const;

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	FAttackDamageSource PreviewAttackDamageSource(float OverrideStrengthBonus, float OverrideDexterityBonus, float OverrideAffinityBonus) const;

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	FWeaponRequirementBreakdown GetEquippedWeaponRequirementBreakdown() const;

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
	// 무기/방어구 중 무엇이 바뀌든 호출 — 전체 장비 무게 재계산 후 StatComponent에 반영
	void RecalcEquipLoad();
#pragma endregion
};
