// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"

#include "Characters/Data/StatusData.h"
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
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCharacterStatusComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

#pragma region State and Action Window
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Windows")
		TObjectPtr<UActionWindowRules> WindowRules = nullptr;

	// 버퍼 유효 시간
	UPROPERTY(EditAnywhere, Category = "Buffer")
		float BufferDuration = 0.18f;

	// ---- State ----
	void SetState(const FGameplayTag& NewStateTag);
	FGameplayTag GetCurrentState() const { return CurrentStateTag; }

	// ---- Action ----
	void SwitchAction(const FGameplayTag& NewActionTag); // 행동 교체(이전 행동 윈도우 무효)
	FGameplayTag GetCurrentAction() const { return CurrentActionTag; }
	void ClearAction(); // 행동 종료시 설정 초기화
	bool IsWindowOpen(const FGameplayTag& WindowTag) const { return WindowTag.IsValid() && OpenWindows.Contains(WindowTag);} // 윈도우 확인

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

#pragma endregion State and Action Window
public:
	bool IsInAir() const;

#pragma region Death
	// ---- Death ----
	void EnterDeath();      // 사망 진입: 이전 State 캡처 → State.Dead 전이 → OnDeathStarted
	void FinalizeDeath();   // 사망 모션 종료(노티파이): OnDeathFinalized
	bool IsDead() const;    // State.Dead (flat, exact)
	FGameplayTag GetPreviousStateBeforeDeath() const { return PrevStateBeforeDeath; }

	FOnMultiDelegate OnDeathStarted;    // 진입: 모션 재생 / 입력 차단 / 이전 State 정리
	FOnMultiDelegate OnDeathFinalized;  // 종료: 래그돌 / GameOver 트리거

	// ---- Respawn ----
	void EnterRespawn();      // 부활 진입: State 복귀 → OnRespawnStarted
	void FinalizeRespawn();   // 부활 종료(연출 끝): OnRespawnFinalized

	FOnMultiDelegate OnRespawnStarted;    // 진입: 파티클 시작 등
	FOnMultiDelegate OnRespawnFinalized;  // 종료: 입력 활성화, UI 복귀

private:
	FGameplayTag PrevStateBeforeDeath;
	bool bDeathFinalized = false;
#pragma endregion Death
};
