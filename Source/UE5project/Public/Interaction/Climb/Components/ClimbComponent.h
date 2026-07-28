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
	UCurveVector* GetClimbCurve(const FClimbCurveKey& Key) const;
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

	bool bIsClimbing;

public:
	bool RequestEnterLadder(AActor* TargetLadder);
	bool RequestExitLadder(bool bExitTop);

	void EnterLadderFloat();
	void ExitLadderFloat();
	void ForceDetachFromLadder(bool bBroadcastExit = false);
	bool BeginLimbGripTransition(
		ELimbList Limb,
		ELadderGripDirection Direction,
		UCurveVector* TrajectoryCurve);
	void UpdateLimbGripTransition(ELimbList Limb, float NormalizedTime);
	void CompleteLimbGripTransition(ELimbList Limb);
	void CancelLimbGripTransition(ELimbList Limb);

	void SetGrip1DRelation(float MinInterval, float MaxInterval);
	bool CheckGripListValid();
	FVector GetLimbIKTarget(ELimbList LimbName) const;
	FORCEINLINE EClimbPhase GetLadderStance() const { return LadderStance; }
	/// <summary>
	/// Getter Function For Find Grip about various rule
	/// </summary>

	int32 GetLowestGrip1DIndex() const;
	int32 GetHighestGrip1DIndex() const;

	void SetLowestGrip1D(float MinHeight = 0.0f, float Comparision = 0.0f);

#pragma region Setting Value
private:
	float MinFirstGripHeight = 0.0f;
	float MinGripInterval = 0.0f;
	float MaxGripInterval = TNumericLimits<float>::Max();

	FOnMontageEnded EnterClimbEndedDelegate;

private:
	void OnEnterClimbMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	FVector CalculateLadderAlignmentLocation(const ACharacter* Character) const;
	FRotator CalculateLadderAlignmentRotation() const;
	bool BeginLadderTransition(ELadderTransitionState NewTransition);
	void CompleteLadderTransition();
	void CaptureCharacterState();
	void RestoreCharacterState();
	void ClearLadderSession();
	void ResetLadderIKState(bool bRestoreGroundPhase);
	void HandleOwnerDeathStarted();
	bool PlayEnterMontage(EClimbPhase EnterPhase);
	bool UpdateEnterWarpTarget(EClimbPhase EnterPhase);
	void ClearTransitionWarpTargets();
	void DrawBottomEnterContactDebug() const;
	bool ResolveGripPattern(
		const TMap<ELimbList, float>& HeightOffsets,
		ELimbList ReferenceLimb,
		bool bPreferTop,
		TMap<ELimbList, int32>& OutAssignment) const;
	bool BuildTopEnterGripRoute(
		const TMap<ELimbList, int32>& InitialAssignment,
		const TMap<ELimbList, int32>& FinalAssignment,
		TMap<ELimbList, int32>& OutValidatedInitialAssignment);
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

/// <summary>
/// Setter Function For Setting Value
/// </summary>
public:
	void SetMinFirstGripHeight(float MinValue);
	void SetMinGripInterval(float MinInterval);
	void SetMaxGripInterval(float MaxInterval);

#pragma endregion Setting Value

#pragma region Ladder Climbing
public:	
	void ClimbUpLadder();
	void ClimbDownLadder();
	void ResetClimbState();

	FMultiDelegate OnLadderExit;

private:
	UPROPERTY(VisibleAnyWhere, Category = "ClimbState")
	EClimbPhase LadderStance = EClimbPhase::Idle_Right;

	UPROPERTY(VisibleAnyWhere, Category = "ClimbState")
	TObjectPtr<ALadderBase> ClimbObject;

	UPROPERTY(VisibleAnyWhere, Category = "ClimbState")
	float AnimTime;

	UPROPERTY(VisibleAnywhere, Category = "ClimbState")
	ELadderTransitionState LadderTransitionState = ELadderTransitionState::None;

	bool bHasCharacterStateSnapshot = false;
	bool bEnterMontageActive = false;
	uint8 SavedMovementMode = 0;
	uint8 SavedCustomMovementMode = 0;
	bool bSavedOrientRotationToMovement = false;

	struct FLimbGripTransitionState
	{
		int32 StartGripIndex = INDEX_NONE;
		int32 TargetGripIndex = INDEX_NONE;
		TWeakObjectPtr<UCurveVector> TrajectoryCurve;
	};

	TMap<ELimbList, FLimbGripTransitionState> ActiveLimbGripTransitions;
	TMap<ELimbList, TArray<int32>> PlannedLimbGripTargets;

	FVector SetBoneIKTargetLadder(int32 TargetGripIndex, const FVector CurveValue, float LimbXDistance = 0.0f, int32 StartGripIndex = INDEX_NONE, float LimbYDistance = -15.0f);
	FVector SetBoneIKTargetLadder(const FVector TargetLoc, const FVector CurveValue, const FVector StartLoc = FVector(), float LimbXDistance = 0.0f, float LimbYDistance = -15.0f);
#pragma endregion Ladder Climbing
};
