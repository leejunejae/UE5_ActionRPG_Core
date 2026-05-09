// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataAsset.h"

#include "Combat/Data/DataAsset/PlayerAttackDataAsset.h"
#include "AttackComponent.generated.h"

class UPBEHAnimInstance;
class UHitReactionInterface;
class UNiagaraSystem;
class UNiagaraComponent;
class IAttackSourceInterface;
struct FBoneTransformSegment;

DECLARE_MULTICAST_DELEGATE(FOnMultiDelegate);

USTRUCT(BlueprintType)
struct FBoneTransformSample
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float Time;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FTransform BoneTransform;

	FBoneTransformSample()
		: Time(0.f), BoneTransform(FTransform::Identity)
	{
	}

	FBoneTransformSample(float InTime, const FTransform& InTransform)
		: Time(InTime), BoneTransform(InTransform) {}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UE5PROJECT_API UAttackComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAttackComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	virtual void ExecuteAttack(FName AttackName, float Playrate = 1.0f);
	virtual void PlayAnimation(FAttackContext AttackInfo, int32 index, float Playrate = 1.0f);
	virtual void ExecuteAttackTrace(float StartTime, float EndTime, bool bDrawDebug = false);

	void BeginAttackTrace(FGameplayTag Profile, const UAnimSequence* AnimKey, FName WindowName, float StartTime);
	void TickAttackTrace(float DeltaTime, bool bDrawDebug);
	void EndAttackTrace(float EndTime, bool bDrawDebug = false);

	void InitAttackContextSet(const FAttackContextSet* InContextSet){CurAttackContextSet = InContextSet;}

	FORCEINLINE float GetLastTraceTime() { return LastTraceTime; }

	FOnMultiDelegate OnAttackFinished;

protected:
	TScriptInterface<IAttackSourceInterface> AttackSourceInterface;
	FOnMontageEnded OnMontageEndedDelegate;
	TSet<AActor*> HitActorListCurrentAttack;
	const FBoneTransformSegment* CurrentSeg = nullptr;

	const FAttackContextSet* CurAttackContextSet = nullptr;

	UPROPERTY(VisibleAnywhere, Category = Attack)
		FAttackContext CurAttackContext;

	UPROPERTY(VisibleAnywhere, Category = Attack)
		int32 ComboIndex = 0;

	float LastTraceTime = 0.0f;
};
