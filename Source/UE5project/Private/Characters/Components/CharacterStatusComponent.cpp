// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/Components/CharacterStatusComponent.h"
#include "GameFramework/Character.h" 
#include "GameFramework/CharacterMovementComponent.h"

#include "Combat/Components/HitReactionComponent.h"
#include "Characters/Components/StatComponent.h"

// 유틸리티
#include "Utils/CoreLog.h"
#include "Utils/GameplayTagsBase.h"

UCharacterStatusComponent::UCharacterStatusComponent()
{
}

void UCharacterStatusComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentStateTag = FGameplayTag::RequestGameplayTag(TEXT("State.Ground"));
	CurrentActionTag = FGameplayTag(); // 비어있음

	ResetWindowsToStateDefaults();
}

FGameplayTag UCharacterStatusComponent::ToWindowTag(const FGameplayTag& ActionTag)
{
	const FString S = ActionTag.GetTagName().ToString(); // Action.Roll
	return FGameplayTag::RequestGameplayTag(FName(S.Replace(TEXT("Action."), TEXT("Window."))));
}

void UCharacterStatusComponent::ResetWindowsToStateDefaults()
{
	OpenWindows.Reset();
	BaseWindows.Reset();
	WindowRefCounts.Reset();
	ActiveWindowLeases.Reset();

	if (!WindowRules || !CurrentStateTag.IsValid())
		return;

	if (const FGameplayTagContainer* Defaults = WindowRules->DefaultWindowsByState.Find(CurrentStateTag))
	{
		for (const FGameplayTag& W : *Defaults)
		{
			BaseWindows.Add(W);
			OpenWindows.Add(W);
		}
	}
}

void UCharacterStatusComponent::ApplyCloseOnActionBegin(const FGameplayTag& ActionTag)
{
	if (!WindowRules || !ActionTag.IsValid())
		return;

	if (const FGameplayTagContainer* CloseSet = WindowRules->CloseOnActionBegin.Find(ActionTag))
	{
		for (const FGameplayTag& W : *CloseSet)
		{
			BaseWindows.Remove(W);
			OpenWindows.Remove(W);
		}
	}
}

void UCharacterStatusComponent::SetWindowRules(UActionWindowRules* NewWindowRules)
{
	WindowRules = NewWindowRules;
	ResetWindowsToStateDefaults();
}

void UCharacterStatusComponent::SetState(const FGameplayTag& NewStateTag)
{
	if (!NewStateTag.IsValid()) return;

	CurrentStateTag = NewStateTag;

	// 상태가 바뀌면 기본 허용 Window 세트가 바뀌므로 리셋
	ResetWindowsToStateDefaults();

	// 상태 전환 직후 버퍼 소비 시도(예: 가드 해제 순간 버퍼된 공격이 나가게)
	TryConsumeBufferedActions();
}

void UCharacterStatusComponent::SwitchAction(const FGameplayTag& NewActionTag, EActionExitReason ExitReason)
{
	if (!NewActionTag.IsValid()) return;

	const FGameplayTag PreviousAction = CurrentActionTag;
	if (PreviousAction.IsValid() && !PreviousAction.MatchesTagExact(NewActionTag))
	{
		OnActionTransition.ExecuteIfBound(PreviousAction, NewActionTag, ExitReason);
	}

	// 교체 철학: 행동이 바뀌면 이전 행동에서 열어둔 Window는 신뢰 불가
	// => 현재 State 기본값으로 완전 리셋 후, 새 행동 시작 닫기 적용
	ResetWindowsToStateDefaults();

	CurrentActionTag = NewActionTag;
	ApplyCloseOnActionBegin(NewActionTag);
}

void UCharacterStatusComponent::ClearAction()
{
	CurrentActionTag = FGameplayTag();
	ResetWindowsToStateDefaults();
}

uint64 UCharacterStatusComponent::AcquireWindows(const FGameplayTagContainer& WindowTags)
{
	if (WindowTags.IsEmpty()) return 0;

	const uint64 LeaseId = NextWindowLeaseId++;
	if (NextWindowLeaseId == 0)
	{
		NextWindowLeaseId = 1;
	}
	ActiveWindowLeases.Add(LeaseId, WindowTags);

	for (const FGameplayTag& WindowTag : WindowTags)
	{
		if (!WindowTag.IsValid()) continue;
		++WindowRefCounts.FindOrAdd(WindowTag);
		OpenWindows.Add(WindowTag);
	}

	// 윈도우가 열리는 순간 버퍼 소비
	TryConsumeBufferedActions();
	return LeaseId;
}

void UCharacterStatusComponent::ReleaseWindows(uint64 LeaseId)
{
	FGameplayTagContainer WindowTags;
	if (LeaseId == 0 || !ActiveWindowLeases.RemoveAndCopyValue(LeaseId, WindowTags))
	{
		return;
	}

	for (const FGameplayTag& WindowTag : WindowTags)
	{
		int32* RefCount = WindowRefCounts.Find(WindowTag);
		if (!RefCount) continue;

		--(*RefCount);
		if (*RefCount <= 0)
		{
			WindowRefCounts.Remove(WindowTag);
			if (!BaseWindows.Contains(WindowTag))
			{
				OpenWindows.Remove(WindowTag);
			}
		}
	}
}

bool UCharacterStatusComponent::CanTryAction(const FGameplayTag& ActionTag) const
{
	if (!ActionTag.IsValid()) return true;

	if (CurrentStateTag.MatchesTagExact(TAG_State_Dead))
		return false;

	const FGameplayTag Window = ToWindowTag(ActionTag);
	return OpenWindows.Contains(Window);
}

void UCharacterStatusComponent::PruneExpiredBufferedActions()
{
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	for (int32 i = BufferedActions.Num() - 1; i >= 0; --i)
	{
		if (BufferedActions[i].ExpireTime <= Now)
			BufferedActions.RemoveAtSwap(i);
	}
}

bool UCharacterStatusComponent::RequestAction(const FGameplayTag& ActionTag, int32 Priority)
{
	if (!ActionTag.IsValid())
	{
		UE_LOG(Log_Character_Player_Input, Error, TEXT("[CharacterStatusComponent] ActionTag Invalid"));
		return false;
	}

	// 가능하면 즉시 교체 실행
	if (CanTryAction(ActionTag))
	{
		SwitchAction(ActionTag);
		return true;
	}

	// 같은 태그가 이미 버퍼에 있으면 갱신만
	for (FBufferedAction& Existing : BufferedActions)
	{
		if (Existing.ActionTag == ActionTag)
		{
			const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
			Existing.ExpireTime = Now + BufferDuration;
			Existing.Priority = FMath::Max(Existing.Priority, Priority);
			return false;
		}
	}

	// 불가능하면 버퍼에 저장
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	FBufferedAction BA;
	BA.ActionTag = ActionTag;
	BA.Priority = Priority;
	BA.ExpireTime = Now + BufferDuration;

	BufferedActions.Add(BA);
	return false;
}

void UCharacterStatusComponent::RemoveBufferedAction(const FGameplayTag& ActionTag)
{
	if (!ActionTag.IsValid()) return;
	BufferedActions.RemoveAll([&ActionTag](const FBufferedAction& BufferedAction)
	{
		return BufferedAction.ActionTag.MatchesTagExact(ActionTag);
	});
}

void UCharacterStatusComponent::TryConsumeBufferedActions()
{
	if (bConsumingBufferedAction)
	{
		return;
	}
	TGuardValue<bool> ConsumingGuard(bConsumingBufferedAction, true);

	PruneExpiredBufferedActions();
	if (BufferedActions.Num() == 0) return;

	// 가능한 버퍼 중 우선순위 가장 높은 것 1개 소비
	int32 BestIdx = INDEX_NONE;
	int32 BestPriority = INT32_MIN;

	for (int32 i = 0; i < BufferedActions.Num(); ++i)
	{
		const FBufferedAction& BA = BufferedActions[i];
		if (!CanTryAction(BA.ActionTag))
			continue;

		if (BA.Priority > BestPriority)
		{
			BestPriority = BA.Priority;
			BestIdx = i;
		}
	}

	if (BestIdx == INDEX_NONE)
		return;

	const FGameplayTag Chosen = BufferedActions[BestIdx].ActionTag;
	BufferedActions.RemoveAtSwap(BestIdx);

	// SwitchAction은 버퍼를 재귀적으로 소비하지 않는다.
	// 현재 요청을 완전히 실행한 뒤 다음 Window 변경에서 다음 버퍼를 평가한다.
	SwitchAction(Chosen);
	OnActionConsumed.ExecuteIfBound(Chosen);
}

bool UCharacterStatusComponent::IsInAir() const
{
	if (const ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		return Character->GetCharacterMovement()->IsFalling();
	}
	return false;
}

void UCharacterStatusComponent::EnterDeath()
{
	if (IsDead()) return; // 중복 진입 방지

	// 사망 직전 State 보존 (모션 선택 + 뒷정리 분기용)
	PrevStateBeforeDeath = CurrentStateTag;

	// 진행 중 액션의 소유 리소스를 먼저 정리한 뒤 상태 태그와 버퍼를 비운다.
	if (CurrentActionTag.IsValid())
	{
		OnActionTransition.ExecuteIfBound(
			CurrentActionTag, FGameplayTag(), EActionExitReason::Death);
	}

	// 진행 중 액션/입력 버퍼 정리
	CurrentActionTag = FGameplayTag();
	BufferedActions.Reset();
	OpenWindows.Reset();

	// State.Dead 전이 → ResetWindowsToStateDefaults가 윈도우 전부 닫음
	SetState(TAG_State_Dead);

	OnDeathStarted.Broadcast();
}

void UCharacterStatusComponent::FinalizeDeath()
{
	if (!IsDead() || bDeathFinalized) return; // Dead 상태에서 1회만
	bDeathFinalized = true;

	OnDeathFinalized.Broadcast();
}

void UCharacterStatusComponent::EnterRespawn()
{
	if (!IsDead()) return;  // Dead 상태에서만 부활 가능

	// 사망 플래그 초기화
	bDeathFinalized = false;

	// 이전 State로 복귀 (저장된 PrevState 또는 기본값 Ground)
	FGameplayTag TargetState = TAG_State_Ground;

	SetState(TargetState);

	OnRespawnStarted.Broadcast();
}

void UCharacterStatusComponent::FinalizeRespawn()
{
	OnRespawnFinalized.Broadcast();
}

bool UCharacterStatusComponent::IsDead() const
{
	return CurrentStateTag.MatchesTagExact(TAG_State_Dead);
}
