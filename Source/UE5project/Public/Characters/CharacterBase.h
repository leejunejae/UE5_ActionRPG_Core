// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

// 기본 헤더
#include "CoreMinimal.h"

#include "Interaction/Climb/Data/ClimbHeader.h"
#include "Characters/Data/StatusData.h"
#include "GameplayTags.h"

#include "Combat/Interfaces/HitReactionInterface.h"
#include "GenericTeamAgentInterface.h"

#include "GameFramework/Character.h"
#include "CharacterBase.generated.h"


class UAttackComponent;
class UHitReactionComponent;
class UCharacterStatusComponent;
class UStatComponent;
class UClimbComponent;

class ARide;

UCLASS()
class UE5PROJECT_API ACharacterBase : public ACharacter,
	public IHitReactionInterface,
	public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ACharacterBase(const FObjectInitializer& ObjectInitializer);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void PostInitializeComponents() override;

#pragma region Affiliation
protected:
	UPROPERTY(VisibleAnywhere, Category = "Affiliation")
		uint8 TeamID;

public:
	virtual FGenericTeamId GetGenericTeamId() const override { return FGenericTeamId(TeamID); }
#pragma endregion Affiliation

#pragma region State & Stance
protected:
	ELocomotionGait CurLocomotionGait = ELocomotionGait::Jog;
	TMap<ELocomotionGait, FGaitSetting> GaitData;

public:
	FORCEINLINE ELocomotionGait GetCurLocomotionGait() const { return CurLocomotionGait; }
	void SetCurLocomotionGait(ELocomotionGait NewGait);

#pragma endregion State & Stance

#pragma region Attack
protected:
	UPROPERTY(VisibleAnywhere, Category = Attack)
		TObjectPtr<UAttackComponent> AttackComponent;

public:
	FORCEINLINE UAttackComponent* GetAttackComponent() const { return AttackComponent; }

#pragma endregion Attack


#pragma region HitReaction
protected:
	UPROPERTY(VisibleAnywhere, Category = HitReaction)
		TObjectPtr<UHitReactionComponent> HitReactionComponent;

public:
	//virtual void OnHit_Implementation(const FAttackRequest& AttackInfo) override;

	FORCEINLINE UHitReactionComponent* GetHitReactionComponent() const { return HitReactionComponent; }

#pragma endregion HitReaction


#pragma region Status
protected:
	UPROPERTY(VisibleAnywhere, Category = Status)
		TObjectPtr<UCharacterStatusComponent> CharacterStatusComponent;

public:
	FORCEINLINE UCharacterStatusComponent* GetCharacterStatusComponent() const { return CharacterStatusComponent; }
#pragma endregion Status

#pragma region Stat
protected:
	UPROPERTY(VisibleAnywhere, Category = Stat)
	TObjectPtr<UStatComponent> StatComponent;

public:
	FORCEINLINE UStatComponent* GetStatComponent() const { return StatComponent; }
#pragma endregion Stat

#pragma region Ladder
protected:
	UPROPERTY(VisibleAnywhere, Category = Climb)
		TObjectPtr<UClimbComponent> ClimbComponent;

	UPROPERTY(EditAnywhere)
		float MinGripInterval = 15.0f;
	UPROPERTY(EditAnywhere)
		float MaxGripInterval = 60.0f;
	UPROPERTY(EditAnywhere)
		float MinFirstGripHeight = 0.0f;

public:
	FORCEINLINE UClimbComponent* GetClimbComponent() const { return ClimbComponent; }
#pragma endregion Ladder

#pragma region Animation
public:
	FORCEINLINE FGameplayTag GetCharacterProfileTag() const { return CurrentProfileTag; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Animation")
		FGameplayTag CurrentProfileTag;

#pragma endregion Animation

#pragma region Equip
public:
	virtual UStaticMeshComponent* GetMainWeaponMesh() const { return nullptr; }

#pragma endregion Equip

#pragma region Ride
protected:
	ARide* Ride;

public:
	ARide* GetCurrentRide();
#pragma endregion Ride

#pragma region Death
protected:
	virtual void HandleDeathStarted();    // 진입: 입력 차단 + 이전 State 정리
	virtual void HandleDeathFinalized();

	virtual void HandleRespawnStarted();
	virtual void HandleRespawnFinalized();
#pragma endregion Death
};