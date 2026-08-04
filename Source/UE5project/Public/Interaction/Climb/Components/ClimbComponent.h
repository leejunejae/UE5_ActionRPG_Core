// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Characters/Data/BaseCharacterHeader.h"
#include "Interaction/Climb/Data/ClimbHeader.h"
#include "Characters/Data/IKData.h"
#include "Interaction/Climb/Data/LadderClimbDataAsset.h"
#include "ClimbComponent.generated.h"

class ALadderBase;
class UAnimSequence;

USTRUCT(BlueprintType)
struct FLimbData
{
	GENERATED_BODY()

public:
	int32 LimbTargetGripIndex = INDEX_NONE;
	int32 PreviousGripIndex = INDEX_NONE;
	FVector LimbLocation;

public:
	FLimbData() {}
	FLimbData(int32 InLimbTargetGripIndex, FVector InLimbLocation, int32 InPreviousGripIndex = INDEX_NONE)
		: LimbTargetGripIndex(InLimbTargetGripIndex)
		, PreviousGripIndex(InPreviousGripIndex)
		, LimbLocation(InLimbLocation)
	{}
};

DECLARE_MULTICAST_DELEGATE(FMultiDelegate);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UE5PROJECT_API UClimbComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UClimbComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#pragma region Owner Data
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Climb|Ladder")
	TObjectPtr<ULadderClimbDataAsset> LadderClimbProfile;

	/** Extra gap between the character capsule and the ladder origin plane. */
	UPROPERTY(EditAnywhere, Category = "Setting", meta = (ClampMin = "0.0"))
		float LadderSurfaceClearance = 10.0f;

	UPROPERTY(EditAnywhere, Category = "Debug|Ladder Contact")
		bool bDrawBottomEnterContactDebug = false;

	UPROPERTY(EditAnywhere, Category = "Debug|Ladder Contact", meta = (ClampMin = "0.0"))
		float BottomEnterContactDebugDuration = 10.0f;

protected:
	const FLadderRepeatedStepDefinition* GetRepeatedStepDefinition(EClimbPhase Phase) const;
	UAnimMontage* GetClimbMontage(EClimbPhase Phase) const;
#pragma endregion Owner Data

#pragma region Climbable Object
public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void RegisterClimbObject(ALadderBase* Ladder);
	void DeRegisterClimbObject();

#pragma endregion Climbable Object

#pragma region Grip And FootHold
protected:
	TArray<FGripNode1D> GripList1D;

	TMap<ELimbList, FLimbData> LimbToGripNode;
	TTuple<FVector, FVector> ClimbLocation;

public:
	bool RequestEnterLadder(AActor* TargetLadder);
	bool RequestExitLadder(bool bExitTop);

	void ForceDetachFromLadder(bool bBroadcastExit = false);
	bool BeginLimbGripTransition(
		ELimbList Limb,
		ELadderGripDirection Direction,
		UCurveVector* TrajectoryCurve);
	void UpdateLimbGripTransition(ELimbList Limb, float NormalizedTime);
	void CompleteLimbGripTransition(ELimbList Limb);
	void CancelLimbGripTransition(ELimbList Limb);

	void SetGrip1DRelation(float MinInterval, float MaxInterval);
	FVector GetLimbIKTarget(ELimbList LimbName) const;
	FORCEINLINE EClimbPhase GetLadderStance() const { return LadderStance; }
	/// <summary>
	/// Getter Function For Find Grip about various rule
	/// </summary>

#pragma region Setting Value
private:
	float MinGripInterval = 0.0f;
	float MaxGripInterval = TNumericLimits<float>::Max();

	FOnMontageEnded EnterClimbEndedDelegate;
	FOnMontageEnded ExitClimbEndedDelegate;
	FOnMontageBlendingOutStarted ExitClimbBlendingOutDelegate;

private:
	void OnEnterClimbMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void OnExitClimbMontageBlendingOut(
		UAnimMontage* Montage,
		bool bInterrupted);
	void OnExitClimbMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void PrepareCharacterForLadderTransition();
	FVector CalculateLadderAlignmentLocation(const ACharacter* Character) const;
	FRotator CalculateLadderAlignmentRotation() const;
	bool BeginLadderTransition(ELadderActionState NewState);
	void CompleteExitTransition();
	void CaptureCharacterState();
	void RestoreCharacterState();
	void ClearLadderSession();
	void ResetLadderIKState(bool bRestoreGroundPhase);
	void StopActiveTransitionMontage();
	void HandleOwnerDeathStarted();

	UFUNCTION()
	void HandleClimbObjectDestroyed(AActor* DestroyedActor);
	bool PlayEnterMontage(EClimbPhase EnterPhase);
	bool PlayExitMontage(EClimbPhase ExitPhase);
	bool UpdateEnterWarpTarget(EClimbPhase EnterPhase);
	bool UpdateExitWarpTarget(EClimbPhase ExitPhase);
	void ClearTransitionWarpTargets();
	void DrawBottomEnterContactDebug() const;
	bool ResolveGripPattern(
		const TMap<ELimbList, float>& HeightOffsets,
		ELimbList ReferenceLimb,
		bool bPreferTop,
		TMap<ELimbList, int32>& OutAssignment) const;
	bool BuildGripRoute(
		UAnimMontage* Montage,
		const TMap<ELimbList, int32>& StartAssignment,
		const TMap<ELimbList, int32>& EndAssignment);
	bool BuildTopExitGripRoute(EClimbPhase ExitPhase);
	bool ValidatePlannedGripRouteEnd(
		const TMap<ELimbList, int32>& ExpectedAssignment) const;
	EClimbPhase ResolveIdlePhaseFromGripState() const;
	bool ValidateTopEnterFinalGripAssignment() const;
	FGripNode1D* GetGripNode(int32 GripIndex);
	const FGripNode1D* GetGripNode(int32 GripIndex) const;
	int32 GetNeighborGripIndex(int32 GripIndex, bool bUp, int32 Count = 1) const;
	FVector GetGripWorldPosition(int32 GripIndex) const;
	FVector CalculateBodyTargetLocation(const FVector& FallbackLocation) const;
	FVector CalculateBodyTargetLocation(
		const TMap<ELimbList, int32>& GripAssignment,
		const FVector& FallbackLocation) const;
	bool MoveCharacterAlongClimbPath(const FVector& TargetLocation);
	bool ResolveRepeatedStepLimbs(
		EClimbPhase Phase,
		ELimbList& OutMovingHand,
		ELimbList& OutMovingFoot) const;
	bool CanStartRepeatedClimb() const;
	bool StartRepeatedClimbStep(bool bUp);
	void FinishActiveRepeatedStep();
	void TickRepeatedStep(float DeltaTime);
	void TickRepeatedStepRecovery(float DeltaTime);

/// <summary>
/// Setter Function For Setting Value
/// </summary>
public:
	void SetMinGripInterval(float MinInterval);
	void SetMaxGripInterval(float MaxInterval);

#pragma endregion Setting Value

#pragma region Ladder Climbing
public:	
	void ClimbUpLadder();
	void ClimbDownLadder();
	void ClearRepeatedClimbInput();
	FORCEINLINE ELadderActionState GetLadderActionState() const { return LadderActionState; }
	FORCEINLINE float GetRepeatedStepProgress() const { return RepeatedStepRuntime.Progress; }
	FORCEINLINE float GetRepeatedStepExplicitTime() const { return RepeatedStepRuntime.ExplicitTime; }
	UAnimSequence* GetLadderIdleAnimation() const;
	FORCEINLINE UAnimSequence* GetActiveRepeatedStepAnimation() const { return RepeatedStepRuntime.Animation.Get(); }

	FMultiDelegate OnLadderExit;

private:
	UPROPERTY(VisibleAnyWhere, Category = "ClimbState")
	EClimbPhase LadderStance = EClimbPhase::Idle_Right;

	UPROPERTY(VisibleAnyWhere, Category = "ClimbState")
	TObjectPtr<ALadderBase> ClimbObject;

	UPROPERTY(VisibleAnywhere, Category = "ClimbState")
	ELadderActionState LadderActionState = ELadderActionState::Detached;

	bool bHasCharacterStateSnapshot = false;
	uint8 SavedMovementMode = 0;
	uint8 SavedCustomMovementMode = 0;
	bool bSavedOrientRotationToMovement = false;

	struct FLimbGripTransitionState
	{
		int32 StartGripIndex = INDEX_NONE;
		int32 TargetGripIndex = INDEX_NONE;
		TWeakObjectPtr<UCurveVector> TrajectoryCurve;
	};

	struct FTransitionRuntime
	{
		TMap<ELimbList, FLimbGripTransitionState> ActiveGripTransitions;
		TMap<ELimbList, TArray<int32>> PlannedGripTargets;
		TMap<ELimbList, int32> PlannedRouteEndAssignment;
		bool bVisualStateReleased = false;

		void Reset()
		{
			ActiveGripTransitions.Empty();
			PlannedGripTargets.Empty();
			PlannedRouteEndAssignment.Empty();
			bVisualStateReleased = false;
		}
	};

	struct FActiveRepeatedStep
	{
		EClimbPhase Phase = EClimbPhase::Idle_Right;
		ELimbList MovingHand = ELimbList::Body;
		ELimbList MovingFoot = ELimbList::Body;
		int32 HandStartGrip = INDEX_NONE;
		int32 HandTargetGrip = INDEX_NONE;
		int32 FootStartGrip = INDEX_NONE;
		int32 FootTargetGrip = INDEX_NONE;
		FVector BodyStart = FVector::ZeroVector;
		FVector BodyTarget = FVector::ZeroVector;
		float ElapsedTime = 0.0f;
		float Duration = 0.0f;
	};

	struct FRepeatedStepRuntime
	{
		TOptional<FActiveRepeatedStep> ActiveStep;
		TWeakObjectPtr<UAnimSequence> Animation;
		float Progress = 0.0f;
		float ExplicitTime = 0.0f;
		float RecoveryElapsed = 0.0f;
		int8 InputDirection = 0;

		void Reset()
		{
			ActiveStep.Reset();
			Animation.Reset();
			Progress = 0.0f;
			ExplicitTime = 0.0f;
			RecoveryElapsed = 0.0f;
			InputDirection = 0;
		}
	};

	FTransitionRuntime TransitionRuntime;
	FRepeatedStepRuntime RepeatedStepRuntime;

	FVector SetBoneIKTargetLadder(int32 TargetGripIndex, const FVector CurveValue, float LimbXDistance = 0.0f, int32 StartGripIndex = INDEX_NONE);
#pragma endregion Ladder Climbing
};
