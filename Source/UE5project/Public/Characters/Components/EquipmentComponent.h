// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

// 인터페이스
#include "Characters/Interfaces/EquipmentDataInterface.h"
#include "Combat/Interfaces/AttackSourceInterface.h"
#include "Characters/Interfaces/StatInterface.h"

#include "Items/Weapons/Data/WeaponData.h"

#include "EquipmentComponent.generated.h"

class ACharacter;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnEquipmentMulDel, const EWeaponType);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UE5PROJECT_API UEquipmentComponent : public UActorComponent,
	public IEquipmentDataInterface,
	public IAttackSourceInterface
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
	
	UPROPERTY(VisibleAnywhere, Category = Equipment)
		TObjectPtr<UStaticMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = Equipment)
		TObjectPtr<UStaticMeshComponent> SubEquipMesh;

	UPROPERTY(VisibleAnywhere, Category = Equipment)
		FName WeaponSocket;

	UPROPERTY(VisibleAnywhere, Category = Equipment)
		FName SubEquipSocket;

	const FWeaponSetsInfo* EquipedWeapon;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	FORCEINLINE const FWeaponSetsInfo* GetEquipedWeapon() const { return EquipedWeapon; }
	FORCEINLINE UStaticMeshComponent* GetMainWeaponComponent() const { return WeaponMesh; }
	FORCEINLINE UStaticMeshComponent* GetSubEquipComponent() const { return SubEquipMesh; }

	void EquipWeapon_Implementation(FName WeaponKey) override;
	FVector GetWeaponSocketLocation_Implementation(FName SocketName, bool IsSubWeapon) const;

	FAttackTraceSource GetAttackTraceSource_Implementation(EAttackSourceType AttackSourceType) const;
	FAttackDamageSource GetAttackDamageSource_Implementation() const;

	void SetWeaponSocketName(FName SocketName) { WeaponSocket = SocketName; }
	void SetSubEquipSocketName(FName SocketName) { SubEquipSocket = SocketName; }

	FOnEquipmentMulDel OnWeaponChangedDelegate;
	virtual FOnEquipmentMulDel& OnWeaponSetChanged() override { return OnWeaponChangedDelegate; }
};
