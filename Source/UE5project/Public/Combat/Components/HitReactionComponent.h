// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

// 엔진 헤더
#include "CoreMinimal.h"
#include "Combat/Data/HitReactionData.h"
#include "Combat/Data/DataAsset/HitReactionDataAsset.h"
#include "Components/ActorComponent.h"
#include "Engine/DataAsset.h"

#include "HitReactionComponent.generated.h"

DECLARE_MULTICAST_DELEGATE(FMultiDelegate);

class ICharacterStatusInterface;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UE5PROJECT_API UHitReactionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UHitReactionComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	void InitializeComponentLogic();

	void SetHitReactionDA(UHitReactionDataAsset* HitReactionDA);
	bool ExecuteHitResponse(const FHitReactionRequest& ReactionData);
	bool PlayReaction(const FHitReactionInfo& HitReaction, const FName SectionName = NAME_None);
	void CancelHitReaction(EActionExitReason ExitReason = EActionExitReason::Interrupted,
		bool bStopMontage = true);

	FHitResolution ResolveHit(const FAttackRequest& AttackRequest) const;
	float CalculateHitAngle(const FVector HitPoint);
	float CalculateAttackAngle(const FAttackRequest& AttackRequest) const;
	void BeginParryActiveWindow();
	void EndParryActiveWindow();
	void ResetParryActiveWindow();
	bool IsParryActiveWindowOpen() const { return ParryActiveWindowCount > 0; }
	bool CanUpgradeActiveReaction(ECombatReaction IncomingReaction) const;
	bool TransitionHitAirToRecovery(FName RecoverySection = TEXT("Recovery"));

	void OnHitReactionEnded(UAnimMontage* Montage, bool bInterrupted);
	bool IsHitReactionActive() const { return bHitReactionActive; }
	bool IsHitAirReactionActive() const
	{
		return bHitReactionActive && ActiveReaction == ECombatReaction::HitAir;
	}
	
	FMultiDelegate HitEndDelegate;
	FMultiDelegate HitStartDelegate;
	
private:
	UPROPERTY(VisibleAnywhere, Meta = (AllowPrivateAccess = true))
		UHitReactionDataAsset* HitReactionDataAsset;

	FHitReactionInfo CurHitReaction = FHitReactionInfo();

	void FinishHitReaction(bool bStopMontage, bool bBroadcastEnd,
		EActionExitReason ExitReason = EActionExitReason::Completed);
	void ClearHitReactActionIfCurrent() const;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveHitMontage;

	bool bHitReactionActive = false;
	bool bFinishingHitReaction = false;
	int32 ParryActiveWindowCount = 0;
	ECombatReaction ActiveReaction = ECombatReaction::None;
};
