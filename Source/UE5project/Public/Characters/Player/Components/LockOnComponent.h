// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LockOnComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnLockOnTargetChanged, AActor*);

class USphereComponent;
class APlayerController;

USTRUCT()
struct FLockOnCandidateScore
{
	GENERATED_BODY()

	UPROPERTY() TWeakObjectPtr<AActor> Target;
	UPROPERTY() float Score = -FLT_MAX;
	UPROPERTY() FVector2D ScreenPos = FVector2D::ZeroVector;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UE5PROJECT_API ULockOnComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	ULockOnComponent();

	bool IsLockedOn() const { return LockedOnTarget.IsValid(); }
	AActor* GetCurrentTarget() const { return LockedOnTarget.Get(); }

	/** 토글: 켜져있으면 해제, 꺼져있으면 최고의 타겟을 잡음 */
	bool ToggleLockOn();

	/** 좌/우 스위치: true=Right, false=Left */
	bool SwitchTarget(bool bRight);

	/** 틱에서 유지/갱신 */
	void TickLockOn(float DeltaTime);

	/** 락온 강제 해제 */
	void ClearLockOn();

public:
	FOnLockOnTargetChanged OnLockOnTargetChanged;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	// ---- Candidate management ----
	UPROPERTY() TObjectPtr<USphereComponent> CandidateSphere = nullptr;

	UPROPERTY() TSet<TWeakObjectPtr<AActor>> Candidates;

	UFUNCTION()
		void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
			UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
			bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
		void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
			UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	bool IsValidCandidate(AActor* Actor) const;

	// ---- Scoring / selection ----
	bool GetScreenPos(AActor* Target, FVector2D& OutScreen) const;
	float ComputeScore(AActor* Target, const FVector2D& ScreenPos) const;
	TWeakObjectPtr<AActor> FindBestTarget() const;

	TWeakObjectPtr<AActor> FindSwitchTarget(bool bRight) const;

	// ---- Validity / 유지 ----
	bool IsTargetStillValid(AActor* Target) const;
	bool HasLineOfSight(AActor* Target) const;

	/** LockedOnTarget을 변경하면서 자동으로 델리게이트 브로드캐스트 */
	void SetLockedOnTarget(AActor* NewTarget);

private:

	UPROPERTY(EditAnywhere, Category = "Range")
		float LockOnRadius = 2000.0f;

	UPROPERTY(EditAnywhere, Category = "Range")
		float BreakRadius = 2400.f;

	UPROPERTY(EditAnywhere, Category = "Screen")
		float MaxScreenRadiusNorm = 0.55f;

	UPROPERTY(EditAnywhere, Category = "LOS")
		float LOSGraceTime = 0.8f;

	UPROPERTY(EditAnywhere, Category = "Screen")
		float OffscreenGraceTime = 1.2f;

	// 스코어 가중치(엘든링 느낌: ScreenCenter 비중 크게)
	UPROPERTY(EditAnywhere, Category = "Score")
		float W_ScreenCenter = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Score")
		float W_Distance = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Score")
		float W_Angle = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Score")
		float LOSBonus = 0.15f; // LOS 있으면 점수 살짝 가산

	UPROPERTY(VisibleAnywhere, Category = "Target")
		TWeakObjectPtr<AActor> LockedOnTarget;

	float TimeSinceLOSLost = 0.f;
	float TimeSinceOffscreen = 0.f;
		
};
