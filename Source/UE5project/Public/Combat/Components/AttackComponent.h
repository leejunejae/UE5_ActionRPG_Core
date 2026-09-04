// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataAsset.h"
#include "Combat/Data/AttackData.h"
#include "AttackComponent.generated.h"

class UPBEHAnimInstance;
class UHitReactionInterface;
class UNiagaraSystem;
class UNiagaraComponent;
class IAttackSourceInterface;
struct FBoneTransformSegment;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnMultiOneParamDelegate, bool);
DECLARE_DELEGATE_OneParam(FOnComboAttackRequested, FName);

UENUM(BlueprintType)
enum class EAttackSessionState : uint8
{
	Idle,
	Preparing,
	Active
};

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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	virtual const FBaseAttackData* ExecuteAttack(FName AttackName, float Playrate = 1.0f);
	const FBaseAttackData* GetNextComboAttackData(FName AttackName) const;
	bool TryHandleComboInput(FName AttackName, float BufferDuration);
	void ClearBufferedComboInput();
	uint64 AcquireComboInputWindow();
	void ReleaseComboInputWindow(uint64 LeaseId);
	const FBaseAttackData* BeginChargeAttack(FName AttackName, float Playrate = 1.0f);
	const FBaseAttackData* CommitChargeAttack(const FAttackModifiers& CommittedModifiers);
	bool EndChargeAttack();
	const FBaseAttackData* GetNextAttackData(FName AttackName) const;
	virtual bool PlayAnimation(const FAttackContext& AttackInfo, int32 Index, float Playrate = 1.0f);
	virtual void ExecuteAttackTrace(float StartTime, float EndTime, bool bDrawDebug = false);
	void CancelAttack(EActionExitReason ExitReason = EActionExitReason::Interrupted,
		bool bStopMontage = true);

	void BeginAttackTrace(FGameplayTag Profile, const UAnimSequence* AnimKey, FName WindowName, float StartTime);
	void TickAttackTrace(float DeltaTime, bool bDrawDebug);
	void EndAttackTrace(float EndTime, bool bDrawDebug = false);

	void InitAttackContextSet(const FAttackContextSet* InContextSet){CurAttackContextSet = InContextSet;}

	FORCEINLINE float GetLastTraceTime() { return LastTraceTime; }
	FORCEINLINE bool IsAttackActive() const { return AttackSessionState == EAttackSessionState::Active; }
	FORCEINLINE bool IsChargePreparing() const { return AttackSessionState == EAttackSessionState::Preparing; }
	FORCEINLINE bool IsAttackTraceActive() const { return bAttackTraceActive; }
	FName GetActiveMontageSection() const;
	bool IsActiveMontageSection(FName SectionName) const;

	FOnMultiOneParamDelegate OnAttackFinished;
	FOnComboAttackRequested OnComboAttackRequested;

protected:
	TSet<AActor*> HitActorListCurrentAttack;
	const FBoneTransformSegment* CurrentSeg = nullptr;

	const FAttackContextSet* CurAttackContextSet = nullptr;

	UPROPERTY(VisibleAnywhere, Category = Attack)
		FAttackContext CurAttackContext;

	UPROPERTY(VisibleAnywhere, Category = Attack)
		int32 ComboIndex = 0;

	float LastTraceTime = 0.0f;
	FTransform PreviousTraceRootWorldTransform = FTransform::Identity;
	bool bHasPreviousTraceRootWorldTransform = false;

private:
	bool ResolveNextAttack(FName AttackName, FAttackContext& OutContext, int32& OutIndex) const;
	bool PlayChargePreparation(const FAttackContext& AttackInfo, int32 Index, float Playrate);
	void FinishAttackSession(bool bInterrupted, bool bStopMontage,
		EActionExitReason ExitReason = EActionExitReason::Completed);
	void ResetAttackTrace();
	void ResetComboInputState(bool bClearBufferedInput);
	void ConsumeBufferedComboInput();

	UPROPERTY(VisibleAnywhere, Category = "Attack|Runtime")
	EAttackSessionState AttackSessionState = EAttackSessionState::Idle;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveAttackMontage;

	FAttackModifiers ActiveAttackModifiers;
	TSet<uint64> ActiveComboWindowLeases;
	uint64 NextComboWindowLeaseId = 1;
	FName BufferedComboAttackName = NAME_None;
	float BufferedComboExpireTime = 0.0f;

	bool bAttackTraceActive = false;
	bool bFinishingAttackSession = false;
	uint64 AttackSessionId = 0;
};
