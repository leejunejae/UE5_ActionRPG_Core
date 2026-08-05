// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interaction/Climb/Data/ClimbHeader.h"
#include "Characters/Data/IKData.h"
#include "ClimbComponent.generated.h"

class ALadderBase;
class UAnimMontage;
class UAnimSequence;
class UCurveVector;
class ULadderClimbDataAsset;
struct FLadderRepeatedStepDefinition;

struct FLimbData
{
	int32 LimbTargetGripIndex = INDEX_NONE;
	FVector LimbLocation;

	FLimbData() = default;
	FLimbData(int32 InLimbTargetGripIndex, FVector InLimbLocation)
	    : LimbTargetGripIndex(InLimbTargetGripIndex), LimbLocation(InLimbLocation)
	{
	}
};

DECLARE_MULTICAST_DELEGATE(FOnLadderExitDelegate);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
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

	/** Draws the resolved capsule and limb contact targets after bottom entry. */
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
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

#pragma endregion Climbable Object

#pragma region Grip And FootHold
protected:
	TArray<FGripNode1D> GripList1D;

	TMap<ELimbList, FLimbData> LimbToGripNode;
	FVector TransitionTargetLocation = FVector::ZeroVector;

public:
	bool RequestEnterLadder(AActor* TargetLadder);
	bool RequestExitLadder(bool bExitTop);

	bool BeginLimbGripTransition(ELimbList Limb, ELadderGripDirection Direction, UCurveVector* TrajectoryCurve,
	                             UObject* TransitionSource);
	void UpdateLimbGripTransition(ELimbList Limb, float NormalizedTime, const UObject* TransitionSource);
	void CompleteLimbGripTransition(ELimbList Limb, const UObject* TransitionSource);

	FVector GetLimbIKTarget(ELimbList LimbName) const;
	FORCEINLINE EClimbPhase GetLadderStance() const { return LadderStance; }
	/// <summary>
	/// Getter Function For Find Grip about various rule
	/// </summary>

#pragma region Setting Value
private:
	UPROPERTY(EditDefaultsOnly, Category = "Climb|Grip", meta = (ClampMin = "0.0", Units = "cm"))
	float MaxGripInterval = 60.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Climb|Grip", meta = (ClampMin = "0.0", Units = "cm"))
	float DuplicateGripHeightTolerance = 1.0f;

private:
	bool RegisterClimbObject(ALadderBase* Ladder);
	void DeRegisterClimbObject();
	void ForceDetachFromLadder(bool bBroadcastExit = false);
	bool BuildGripNeighborRelations();
	void OnEnterClimbMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void OnExitClimbMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);
	void OnExitClimbMontageEnded(UAnimMontage* Montage, bool bInterrupted);
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
	bool ResolveGripPattern(const TMap<ELimbList, float>& HeightOffsets, ELimbList ReferenceLimb, bool bPreferTop,
	                        TMap<ELimbList, int32>& OutAssignment) const;
	bool BuildGripRoute(UAnimMontage* Montage, const TMap<ELimbList, int32>& StartAssignment,
	                    const TMap<ELimbList, int32>& EndAssignment);
	bool BuildTopExitGripRoute(EClimbPhase ExitPhase);
	bool ValidatePlannedGripRouteEnd(const TMap<ELimbList, int32>& ExpectedAssignment) const;
	EClimbPhase ResolveIdlePhaseFromGripState() const;
	const FGripNode1D* GetGripNode(int32 GripIndex) const;
	int32 GetNeighborGripIndex(int32 GripIndex, bool bUp, int32 Count = 1) const;
	FVector GetGripWorldPosition(int32 GripIndex) const;
	bool TryCalculateBodyTargetLocation(const TMap<ELimbList, int32>& GripAssignment, FVector& OutTargetLocation) const;
	bool MoveCharacterAlongClimbPath(const FVector& TargetLocation);
	bool ResolveRepeatedStepLimbs(EClimbPhase Phase, ELimbList& OutMovingHand, ELimbList& OutMovingFoot) const;
	bool CanStartRepeatedClimb() const;
	bool StartRepeatedClimbStep(bool bUp);
	void FinishActiveRepeatedStep();
	void TickRepeatedStep(float DeltaTime);
	void TickRepeatedStepRecovery(float DeltaTime);

#pragma endregion Setting Value

#pragma region Ladder Climbing
public:
	void ClimbUpLadder();
	void ClimbDownLadder();
	void ClearRepeatedClimbInput();
	FORCEINLINE ELadderActionState GetLadderActionState() const { return LadderActionState; }
	FORCEINLINE float GetRepeatedStepExplicitTime() const { return RepeatedStepRuntime.ExplicitTime; }
	UAnimSequence* GetLadderIdleAnimation() const;
	FORCEINLINE UAnimSequence* GetActiveRepeatedStepAnimation() const { return RepeatedStepRuntime.Animation.Get(); }

	FOnLadderExitDelegate OnLadderExit;

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

	struct FLimbGripTransitionState
	{
		int32 StartGripIndex = INDEX_NONE;
		int32 TargetGripIndex = INDEX_NONE;
		TWeakObjectPtr<UCurveVector> TrajectoryCurve;
		TWeakObjectPtr<UObject> Source;
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
		float ExplicitTime = 0.0f;
		float RecoveryElapsed = 0.0f;
		int8 InputDirection = 0;

		void Reset()
		{
			ActiveStep.Reset();
			Animation.Reset();
			ExplicitTime = 0.0f;
			RecoveryElapsed = 0.0f;
			InputDirection = 0;
		}
	};

	FTransitionRuntime TransitionRuntime;
	FRepeatedStepRuntime RepeatedStepRuntime;

	FVector SetBoneIKTargetLadder(int32 TargetGripIndex, const FVector CurveValue, float LimbXDistance = 0.0f,
	                              int32 StartGripIndex = INDEX_NONE);
#pragma endregion Ladder Climbing
};
