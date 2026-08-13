// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimTypes.h"
#include "Components/ActorComponent.h"
#include "RideComponent.generated.h"

class ARide;
class APlayerBase;
class APlayerController;
class ACameraActor;
class USpringArmComponent;
class UAnimMontage;
class UCurveFloat;
class URideProfileDataAsset;

UENUM(BlueprintType)
enum class ERideActionState : uint8
{
	Detached,
	Mounting,
	Riding,
	DismountingNormal,
	DismountingMoving,
	Recovering,
	ForceDetaching,
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UE5PROJECT_API URideComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	URideComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

public:	
	ARide* GetCurrentRide() const { return CurrentRide; }
	ERideActionState GetRideActionState() const { return RideActionState; }
	bool IsRideTransitionAnimationActive() const { return IsValid(ActiveRideTransitionMontage); }
	const URideProfileDataAsset* GetRideProfile() const { return RideProfile; }

	bool RequestSpawnRide();
	bool RequestDismount(FVector InitVelocity);
	void HandleRideInputStarted();
	void HandleRideInputCompleted();
	void HandlePlayerLanded();
	void HandlePlayerGroundAnimationReady();
	void ForceDetachFromRide(bool bDestroyRide = true);

	float GetRideSpeed() const;
	float GetRideDirection() const;

private:
	bool BeginRideSession(ARide* NewRide, const FVector& InitialVelocity);
	bool CanStartMount() const;
	void ClearMountAction();
	bool PlayRideTransitionAnimation(bool bMounting);
	void StopRideTransitionAnimation();
	void OnMountMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void OnDismountMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);
	void OnDismountMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void HandleMountEnd();
	void HandleDismountEnd();
	void CompleteRideSession(ARide* Ride, bool bDestroyRide, bool bRestoreGroundState,
		ERideActionState FinalState = ERideActionState::Detached);
	void AttemptForcedDetach();
	void FinishForcedDetach();
	void HandleForcedDetachFailure();
	void AbortForcedDetachForEndPlay();
	void CapturePlayerState();
	void RestorePlayerState(bool bRestoreGroundState);
	void RegisterRide(ARide* NewRide);
	void UnregisterRide(ARide* Ride);
	bool TryResolveGroundedDismountTransform(const FTransform& Anchor, const ARide* Ride,
		FTransform& OutTransform) const;
	bool IsSafeDismountTransform(const FTransform& Candidate, const ARide* Ride) const;
	bool TransferControlToRide(ARide* Ride, const FVector& InitialVelocity);
	bool TransferControlToPlayer(ARide* Ride, bool bRestoreRideOnFailure = true);
	bool LockCurrentCamera(APlayerController* Controller);
	void BlendFromLockedCamera(APlayerController* Controller, AActor* NewViewTarget);
	void UpdateTransitionCamera(float DeltaTime);
	void ReleaseTransitionCamera();
	void RefreshTransitionTick();
	USpringArmComponent* GetCameraSpringArm(AActor* ViewTarget) const;
	void RefreshCameraRig(AActor* ViewTarget) const;
	void ClearTransitionTimers();
	void StartTransitionWatchdog();
	void OnTransitionTimeout();

	UFUNCTION()
	void HandleRideDestroyed(AActor* DestroyedActor);

	void BeginRideCollision();
	void EndRideCollision(ARide* Ride);
	void UpdateMountTransition();
	void UpdateNormalDismountTransition();
	bool TryEvaluateTransitionCurve(const UCurveFloat* Curve, float& OutValue) const;
	void RestoreTransitionRootMotionMode();
	void ResetRideIKState(bool bRestoreGroundPhase);

	UPROPERTY(Transient)
	TObjectPtr<APlayerBase> Player = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ride|Profile", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<URideProfileDataAsset> RideProfile = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ARide> CurrentRide = nullptr;

	ERideActionState RideActionState = ERideActionState::Detached;

	FTimerHandle TransitionWatchdogHandle;
	FTimerHandle ForcedDetachRetryTimerHandle;
	FVector TransitionStartTargetOffset = FVector::ZeroVector;
	FVector TransitionTargetOffset = FVector::ZeroVector;
	float TransitionStartArmLength = 0.0f;
	float TransitionTargetArmLength = 0.0f;
	float TransitionCameraElapsed = 0.0f;
	bool bTransitionCameraLagEnabled = false;
	bool bTransitionCameraRotationLagEnabled = false;
	FTransform MountStartTransform;
	FTransform NormalDismountStartTransform;
	FTransform NormalDismountTargetTransform;
	float MountHorizontalAlpha = 0.0f;
	float MountVerticalAlpha = 0.0f;
	float DismountHorizontalAlpha = 0.0f;
	float DismountVerticalAlpha = 0.0f;
	double TransitionAnimationStartTime = 0.0;
	float TransitionAnimationPlayRate = 1.0f;

	bool bHasPlayerStateSnapshot = false;
	TEnumAsByte<EMovementMode> SavedMovementMode = MOVE_Walking;
	uint8 SavedCustomMovementMode = 0;
	FName SavedCollisionProfile;
	TEnumAsByte<ECollisionEnabled::Type> SavedCollisionEnabled = ECollisionEnabled::QueryAndPhysics;
	FCollisionResponseContainer SavedCollisionResponses;
	TEnumAsByte<ECollisionChannel> SavedCollisionObjectType = ECC_Pawn;
	bool bSavedSkipJumpStart = false;
	bool bSavedAutoManageCameraTarget = true;
	TEnumAsByte<ERootMotionMode::Type> SavedRootMotionMode = ERootMotionMode::RootMotionFromMontagesOnly;
	bool bRideInputPressed = false;
	bool bRecoveryLandingSatisfied = true;
	bool bDismountVisualStateReleased = false;
	bool bForcedDetachDestroyRide = false;
	bool bForcedDetachRestoreGroundState = true;
	bool bForcedDetachCompletionStarted = false;
	int32 ForcedDetachAttemptCount = 0;
	static constexpr int32 MaxForcedDetachAttempts = 4;
	static constexpr float ForcedDetachRetryInterval = 0.05f;

	UPROPERTY(Transient)
	TObjectPtr<APlayerController> SessionController = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> TransitionCamera = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<APlayerController> TransitionController = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AActor> TransitionViewTarget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USpringArmComponent> TransitionSpringArm = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveRideTransitionMontage = nullptr;
};
