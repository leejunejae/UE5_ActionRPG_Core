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
DECLARE_MULTICAST_DELEGATE_OneParam(FOnParryDelegate, FAttackRequest);

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

public:	
	// Called every frame
	void InitializeComponentLogic();

	void SetHitReactionDA(UHitReactionDataAsset* HitReactionDA);
	void ExecuteHitResponse(const FHitReactionRequest ReactionData);
	void PlayReaction(const FHitReactionInfo HitReaction, const FName SectionName = NAME_None);

	virtual EHitResponse EvaluateHitResponse(const FAttackRequest& AttackRequest);
	float CalculateHitAngle(const FVector HitPoint);

	void OnHitReactionEnded(UAnimMontage* Montage, bool bInterrupted);
	
	FMultiDelegate HitEndDelegate;
	FMultiDelegate HitStartDelegate;
	FOnParryDelegate ParryDelegate;
	
private:
	UPROPERTY(VisibleAnywhere, Meta = (AllowPrivateAccess = true))
		UHitReactionDataAsset* HitReactionDataAsset;

	FHitReactionInfo CurHitReaction = FHitReactionInfo();

	FOnMontageEnded HitMontageEndDelegate;
	FOnMontageBlendingOutStarted HitMontageBlendingOutDelegate;
};
