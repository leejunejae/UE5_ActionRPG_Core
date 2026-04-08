// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Components/CharacterStatusComponent.h"
#include "GameFramework/Character.h" 
#include "GameFramework/CharacterMovementComponent.h"

#include "Combat/Components/HitReactionComponent.h"
#include "Characters/Components/StatComponent.h"

// Sets default values for this component's properties
UCharacterStatusComponent::UCharacterStatusComponent()
{

	// ...
}


// Called when the game starts
void UCharacterStatusComponent::BeginPlay()
{
	Super::BeginPlay();

	CachedCharacter = Cast<ACharacter>(GetOwner());

	if (UHitReactionComponent* HitComp = CachedCharacter->FindComponentByClass<UHitReactionComponent>())
	{
		HitComp->HitEndDelegate.AddUObject(this, &UCharacterStatusComponent::SetGroundStance_Native, EGroundStance::Normal);
	}
	// ...

	CurrentStateTag = FGameplayTag::RequestGameplayTag(TEXT("State.Normal"));
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

	if (!WindowRules || !CurrentStateTag.IsValid())
		return;

	if (const FGameplayTagContainer* Defaults = WindowRules->DefaultWindowsByState.Find(CurrentStateTag))
	{
		for (const FGameplayTag& W : *Defaults)
			OpenWindows.Add(W);
	}
}

void UCharacterStatusComponent::ApplyCloseOnActionBegin(const FGameplayTag& ActionTag)
{
	if (!WindowRules || !ActionTag.IsValid())
		return;

	if (const FGameplayTagContainer* CloseSet = WindowRules->CloseOnActionBegin.Find(ActionTag))
	{
		for (const FGameplayTag& W : *CloseSet)
			OpenWindows.Remove(W);
	}
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

void UCharacterStatusComponent::SwitchAction(const FGameplayTag& NewActionTag)
{
	if (!NewActionTag.IsValid()) return;

	// 교체 철학: 행동이 바뀌면 이전 행동에서 열어둔 Window는 신뢰 불가
	// => 현재 State 기본값으로 완전 리셋 후, 새 행동 시작 닫기 적용
	ResetWindowsToStateDefaults();

	CurrentActionTag = NewActionTag;
	ApplyCloseOnActionBegin(NewActionTag);

	// 전환 직후 버퍼도 한 번 소비 시도
	TryConsumeBufferedActions();
}

void UCharacterStatusComponent::OpenWindow(const FGameplayTag& WindowTag)
{
	if (!WindowTag.IsValid()) return;

	OpenWindows.Add(WindowTag);

	// 윈도우가 열리는 순간 버퍼 소비
	TryConsumeBufferedActions();
}

void UCharacterStatusComponent::CloseWindow(const FGameplayTag& WindowTag)
{
	if (!WindowTag.IsValid()) return;
	OpenWindows.Remove(WindowTag);
}

bool UCharacterStatusComponent::CanTryAction(const FGameplayTag& ActionTag) const
{
	if (!ActionTag.IsValid()) return true;

	// Dead 상태면 전부 불가(원하면 Rules로만 통제해도 됨)
	if (CurrentStateTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(TEXT("State.Dead"))))
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
	if (!ActionTag.IsValid()) return false;

	// 가능하면 즉시 교체 실행
	if (CanTryAction(ActionTag))
	{
		SwitchAction(ActionTag);
		return true;
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

void UCharacterStatusComponent::TryConsumeBufferedActions()
{
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

	SwitchAction(Chosen);
}

bool UCharacterStatusComponent::IsInAir() const
{
	if (const ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		return Character->GetCharacterMovement()->IsFalling();
	}
	return false;
}

void UCharacterStatusComponent::ExecuteDeath()
{
	bIsDead = true;
	OnDeath.Broadcast();
}

bool UCharacterStatusComponent::CanTransitionGroundStance(EGroundStance DestStance, EGroundStance TargetStance)
{

	return false;
}
