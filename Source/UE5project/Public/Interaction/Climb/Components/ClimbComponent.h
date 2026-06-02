// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Characters/Data/BaseCharacterHeader.h"
#include "Interaction/Climb/Data/ClimbHeader.h"
#include "Characters/Data/IKData.h"
#include "Interaction/Climb/Interfaces/ClimbInterface.h"
#include "Interaction/Climb/Data/LadderClimbDataAsset.h"
#include "ClimbComponent.generated.h"

class ICharacterStatusInterface;

USTRUCT(BlueprintType)
struct FLimbData
{
	GENERATED_BODY()

public:
	FGripNode1D* LimbTargetGrip;
	FVector LimbLocation;

public:
	FLimbData() {}
	FLimbData(FGripNode1D* InLimbTargetGrip, FVector InLimbLocation)
		: LimbTargetGrip(InLimbTargetGrip)
		, LimbLocation(InLimbLocation)
	{}
};

DECLARE_MULTICAST_DELEGATE(FMultiDelegate);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UE5PROJECT_API UClimbComponent : public UActorComponent,
	public IClimbInterface
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
	UPROPERTY(VisibleAnywhere, Meta = (AllowPrivateAccess = true))
		const class ULadderClimbDataAsset* ClimbCurveDA;

	UPROPERTY(EditAnywhere, Category = "Curve")
		TObjectPtr<UCurveFloat> EnterRotatorCurve;
	
	UPROPERTY(VisibleAnywhere, Category = "Anim")
		TObjectPtr<UAnimMontage> EnterLadderMontage;

	UPROPERTY(EditAnywhere, Category = "Setting")
		bool HasEnterPhase = true;

protected:
	UCurveVector* GetClimbCurve(const FClimbCurveKey& Key) const;
	UAnimMontage* GetClimbMontage(EClimbPhase Phase) const;
#pragma endregion Owner Data

#pragma region Climbable Object
public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void RegisterClimbObject(AActor* RegistObject);
	void DeRegisterClimbObject();
	AActor* GetClimbObject();

#pragma endregion Climbable Object

#pragma region Grip And FootHold
protected:
	TArray<FGripNode1D> GripList1D;
	TArray<FGripNode2D> GripList2D;

	TMap<ELimbList, FLimbData> LimbToGripNode;
	TTuple<FVector, FVector> ClimbLocation;

	FVector BodyLocation;

	bool bIsClimbing;

public:
	bool RequestEnterLadder(AActor* TargetLadder);
	bool RequestExitLadder(bool bExitTop);

	void EnterLadderFloat();
	void ExitLadderFloat();

	void SetGrip1DRelation(float MinInterval, float MaxInterval);
	bool CheckGripListValid();
	FGripNode1D* GetLimbPlaceGrip(ELimbList LimbName);
	FVector GetLimbIKTarget(ELimbList LimbName);
	FORCEINLINE EClimbPhase GetLadderStance() const { return LadderStance; }
	/// <summary>
	/// Getter Function For Find Grip about various rule
	/// </summary>

	FGripNode1D* GetLowestGrip1D();
	FGripNode1D* GetHighestGrip1D();

	void SetLowestGrip1D(float MinHeight = 0.0f, float Comparision = 0.0f);

#pragma region Setting Value
private:
	float MinFirstGripHeight = 0.0f;
	float MinGripInterval = 0.0f;
	float MaxGripInterval = TNumericLimits<float>::Max();

	FTimerHandle LadderBlendCheckTimer;

	FOnMontageEnded EnterClimbEndedDelegate;

private:
	void OnEnterClimbMontageEnded(UAnimMontage* Montage, bool bInterrupted);

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
	FORCEINLINE EClimbPhase GetLadderStance_Native() const { return LadderStance; }
	virtual EClimbPhase GetLadderStance_Implementation() const { return LadderStance; }
	void ClimbUpLadder();
	void ClimbDownLadder();
	void ResetClimbState();

	FMultiDelegate OnLadderExit;

private:
	UPROPERTY(VisibleAnyWhere, Category = "ClimbState")
	EClimbPhase LadderStance;

	UPROPERTY(VisibleAnyWhere, Category = "ClimbState")
	AActor* ClimbObject;

	UPROPERTY(VisibleAnyWhere, Category = "ClimbState")
	float AnimTime;

	FVector SetBoneIKTargetLadder(const FGripNode1D* TargetGrip, const FVector CurveValue, const float LimbXDistance = 0.0f, const FGripNode1D* StartGrip = nullptr, const float LimbYDistance = -15.0f, bool IsDebug = false);
	FVector SetBoneIKTargetLadder(const FVector TargetLoc, const FVector CurveValue, const FVector StartLoc = FVector(), const float LimbXDistance = 0.0f, const float LimbYDistance = -15.0f, bool IsDebug = false);
#pragma endregion Ladder Climbing
};
