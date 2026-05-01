// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NavigationSystem.h"
//#include "NavigationInvokerComponent.h"

// 인터페이스
#include "Combat/Interfaces/HitReactionInterface.h"
#include "Characters/Interfaces/DeathInterface.h"
#include "Combat/Interfaces/AttackSourceInterface.h"

// 구조체, 자료형
#include "Characters/Enemies/Data/EnemyData.h"
#include "Combat/Data/CombatData.h"

#include "Characters/CharacterBase.h"
#include "EnemyBase.generated.h"

class UCharacterStatComponent;
class UAttackComponent;
class UHitReactionComponent;
class UCharacterStatusComponent;
class UEnemyBaseAnimInstance;
class AEnemyBaseAIController;

DECLARE_DELEGATE(FOnSingleDelegate);

UCLASS()
class UE5PROJECT_API AEnemyBase : public ACharacterBase,
	public IDeathInterface,
	public IAttackSourceInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AEnemyBase(const FObjectInitializer& ObjectInitializer);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void PostInitializeComponents() override;

#pragma region Status
public:
	FOnDeathDelegate OnDeath;
	virtual FOnDeathDelegate& GetOnDeathDelegate() override { return OnDeath; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
		float GuardNegation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
		float Aggressiveness;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
		int32 Phase;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
		bool PoiseBroken;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
		bool StanceBroken;

#pragma endregion Status

#pragma region Equip
	UPROPERTY(Transient) // 런타임 생성물이니 Save/Serialize 필요 없으면 Transient 권장
		TObjectPtr<UStaticMeshComponent> MainWeapon = nullptr;

	UPROPERTY(Transient) // 런타임 생성물이니 Save/Serialize 필요 없으면 Transient 권장
		TObjectPtr<UStaticMeshComponent> SubEquip = nullptr;

	UStaticMeshComponent* GetMainWeaponMesh() const override { return MainWeapon; }

	virtual FAttackTraceSource GetAttackTraceSource(EAttackSourceType AttackSourceType) const override;
	virtual FAttackDamageSource GetAttackDamageSource() const override;

	//void CreateAndAttachEquip(UStaticMesh* MeshAsset, FName SocketName, FTransform Equip, FName CompName);
#pragma endregion Equip

//private:
	//UPROPERTY(EditAnywhere)
		//class UNavigationInvokerComponent* NavigationInvokerComponent;

/* 이 적이 사용할 데이터*/
#pragma region Character_Data
public:
	// 데이터 로드에 사용할 식별자
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BaseData")
		FName EnemyID;

	void SetEnemyID(const FName ID) { EnemyID = ID; }
	bool ApplyEnemyInfo(const FEnemyInfo* Info);
	bool ApplyEnemyStats(const FEnemyStats* Stat);

#pragma endregion Character_Data

#pragma region Stat
protected:
	UPROPERTY(VisibleAnywhere, Category = Stat)
		TObjectPtr<UCharacterStatComponent> StatComponent;

public:
	FORCEINLINE UCharacterStatComponent* GetStatComponent() const { return StatComponent; }

#pragma endregion Stat


#pragma region HitReaction
public:
	void OnHit_Implementation(const FAttackRequest& AttackInfo) override;
	
#pragma endregion HitReaction

#pragma region AI
protected:
	virtual void PossessedBy(AController* NewController) override;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Route")
		FGuid DefaultRouteGuid;

public:
	void SetDefaultRouteGuid(const FGuid& InGuid) { DefaultRouteGuid = InGuid; }
	FGuid GetDefaultRouteGuid() const { return DefaultRouteGuid; }
#pragma endregion

#pragma region Animation
protected:
	UPROPERTY(VisibleAnywhere, Category = Animation)
		TObjectPtr<UEnemyBaseAnimInstance> CharacterBaseAnim;

	UPROPERTY(VisibleAnywhere, Category = "Animation")
		TMap<FGameplayTag, FAnimDataSet> AnimProfiles;

	UPROPERTY(VisibleAnywhere, Category = "Animation")
		FGameplayTag CurrentWeaponTag;
#pragma endregion Animation
};
