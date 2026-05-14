// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"

#include "Characters/Data/StatusData.h"
#include "Characters/Interfaces/CharacterStatusInterface.h"
#include "Characters/Player/ActionWindowRules.h"

#include "CharacterStatusComponent.generated.h"

class ACharacter;
class UCharacterMovementComponent;

DECLARE_MULTICAST_DELEGATE(FOnMultiDelegate);
DECLARE_DELEGATE_OneParam(FOnSingleTagDelegate, const FGameplayTag& /*ActionTag*/);

USTRUCT(BlueprintType)
struct FBufferedAction
{
	GENERATED_BODY()

	UPROPERTY() FGameplayTag ActionTag;
	UPROPERTY() float ExpireTime = 0.f;
	UPROPERTY() int32 Priority = 0;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UE5PROJECT_API UCharacterStatusComponent : public UActorComponent
	, public ICharacterStatusInterface
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCharacterStatusComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Windows")
		TObjectPtr<UActionWindowRules> WindowRules = nullptr;

	// 버퍼 유효 시간(엘든링 느낌: 0.15~0.25)
	UPROPERTY(EditAnywhere, Category = "Buffer")
		float BufferDuration = 0.18f;

	// ---- State ----
	void SetState(const FGameplayTag& NewStateTag);
	FGameplayTag GetState() const { return CurrentStateTag; }

	// ---- Action (교체 철학) ----
	void SwitchAction(const FGameplayTag& NewActionTag); // 행동 교체(이전 행동 윈도우 무효)
	FGameplayTag GetCurrentAction() const { return CurrentActionTag; }
	void ClearAction(); // 행동 종료시 설정 초기화

	// ---- Window control (Notify에서 호출) ----
	void OpenWindow(const FGameplayTag& WindowTag);
	void CloseWindow(const FGameplayTag& WindowTag);

	// ---- Input request ----
	bool RequestAction(const FGameplayTag& ActionTag, int32 Priority = 0);
	bool CanTryAction(const FGameplayTag& ActionTag) const;


	// ---- 버퍼 소비 시 실제 행동 실행을 위한 델리게이트 ----
	FOnSingleTagDelegate OnActionConsumed;
private:
	// 현재 상태/행동 태그
	UPROPERTY(VisibleAnywhere, Category = "State") FGameplayTag CurrentStateTag;  // State.Normal 등
	UPROPERTY(VisibleAnywhere, Category = "State") FGameplayTag CurrentActionTag; // Action.Attack 등(없어도 됨)

	// 현재 열려있는 Window.* 집합
	UPROPERTY(VisibleAnywhere, Category = "State")
		TSet<FGameplayTag> OpenWindows;

	// 입력 버퍼
	TArray<FBufferedAction> BufferedActions;

private:
	static FGameplayTag ToWindowTag(const FGameplayTag& ActionTag);

	void ResetWindowsToStateDefaults();
	void ApplyCloseOnActionBegin(const FGameplayTag& ActionTag);

	void PruneExpiredBufferedActions();
	void TryConsumeBufferedActions();

public:
	FORCEINLINE bool IsDead() const { return bIsDead; }

	bool IsInAir() const;

	void ExecuteDeath();

	FOnMultiDelegate OnDeath;

protected:
	TWeakObjectPtr<ACharacter> CachedCharacter;

	bool bIsDead = false;

#pragma region Ground
public:
	FORCEINLINE EGroundStance GetGroundStance_Native() const { return GroundStance; }
	FORCEINLINE void SetGroundStance_Native(EGroundStance NewStance) { GroundStance = NewStance; }

	EGroundStance GetGroundStance_Implementation() const { return GroundStance; }
	void SetGroundStance_Implementation(EGroundStance NewStance) { GroundStance = NewStance; }

	bool CanTransitionGroundStance(EGroundStance DestStance, EGroundStance TargetStance);
private:
	UPROPERTY(VisibleAnywhere, Category = "Stance")
	EGroundStance GroundStance;
#pragma endregion Ground
};
