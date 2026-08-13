#include "Characters/Components/RideComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/Interfaces/IAnimInstance.h"
#include "Characters/Player/Components/PlayerStatusComponent.h"
#include "Characters/Player/PlayerBase.h"
#include "Characters/Player/PlayerBaseAnimInstance.h"
#include "Characters/Player/PlayerRide.h"
#include "Characters/Rideable/Ride.h"
#include "Characters/Rideable/RideProfileDataAsset.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Curves/CurveFloat.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"
#include "Utils/CoreLog.h"
#include "Utils/GameplayTagsBase.h"

URideComponent::URideComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void URideComponent::BeginPlay()
{
	Super::BeginPlay();
	Player = Cast<APlayerBase>(GetOwner());
}

void URideComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (RideActionState != ERideActionState::Detached || IsValid(CurrentRide))
	{
		ForceDetachFromRide(true);
		if (RideActionState == ERideActionState::ForceDetaching)
		{
			AbortForcedDetachForEndPlay();
		}
	}
	ReleaseTransitionCamera();
	Player = nullptr;
	Super::EndPlay(EndPlayReason);
}

void URideComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (RideActionState == ERideActionState::Mounting)
	{
		UpdateMountTransition();
	}
	else if (RideActionState == ERideActionState::DismountingNormal)
	{
		UpdateNormalDismountTransition();
	}

	if (IsValid(TransitionSpringArm))
	{
		UpdateTransitionCamera(DeltaTime);
	}
}

bool URideComponent::RequestSpawnRide()
{
	if (!CanStartMount())
	{
		return false;
	}

	UPlayerStatusComponent* Status = Player->GetCharacterStatusComponent();
	Status->SwitchAction(TAG_Action_Mount);

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APlayerRide* SpawnedRide = GetWorld()->SpawnActor<APlayerRide>(
		Player->GetRideClass(),
		Player->GetActorTransform(),
		SpawnParameters);
	if (!IsValid(SpawnedRide))
	{
		UE_LOG(Log_RideSpawn, Warning, TEXT("[RideComponent] Failed to spawn a ride for '%s'."), *GetNameSafe(Player));
		ClearMountAction();
		return false;
	}

	if (!BeginRideSession(SpawnedRide, Player->GetVelocity()))
	{
		SpawnedRide->Destroy();
		ClearMountAction();
		return false;
	}

	return true;
}

bool URideComponent::CanStartMount() const
{
	if (!IsValid(Player) || !Player->GetRideClass() || !IsValid(GetRideProfile()) ||
		RideActionState != ERideActionState::Detached)
	{
		return false;
	}

	const UPlayerStatusComponent* Status = Player->GetCharacterStatusComponent();
	const UCharacterMovementComponent* Movement = Player->GetCharacterMovement();
	if (!IsValid(Status) || Status->IsDead() ||
		!Status->GetCurrentState().MatchesTagExact(TAG_State_Ground) ||
		!IsValid(Movement) || !Movement->IsMovingOnGround() ||
		!Status->CanTryAction(TAG_Action_Mount))
	{
		return false;
	}

	// A mounted, climbing, or otherwise attached character is not a valid
	// origin for a new ride even if an external system left its state as Ground.
	return Player->GetAttachParentActor() == nullptr;
}

void URideComponent::ClearMountAction()
{
	if (!IsValid(Player))
	{
		return;
	}

	if (UPlayerStatusComponent* Status = Player->GetCharacterStatusComponent())
	{
		if (Status->GetCurrentAction().MatchesTagExact(TAG_Action_Mount))
		{
			Status->ClearAction();
		}
	}
}

void URideComponent::HandleRideInputStarted()
{
	if (bRideInputPressed)
	{
		return;
	}
	bRideInputPressed = true;

	if (RideActionState == ERideActionState::Detached)
	{
		RequestSpawnRide();
	}
	else if (RideActionState == ERideActionState::Riding && IsValid(CurrentRide))
	{
		RequestDismount(CurrentRide->GetVelocity());
	}
}

void URideComponent::HandleRideInputCompleted()
{
	bRideInputPressed = false;
}

void URideComponent::HandlePlayerLanded()
{
	if (RideActionState == ERideActionState::Recovering)
	{
		bRecoveryLandingSatisfied = true;
	}
}

void URideComponent::HandlePlayerGroundAnimationReady()
{
	if (RideActionState != ERideActionState::Recovering || !IsValid(Player))
	{
		return;
	}

	const UCharacterMovementComponent* Movement = Player->GetCharacterMovement();
	const UPlayerStatusComponent* Status = Player->GetCharacterStatusComponent();
	if (bRecoveryLandingSatisfied && IsValid(Movement) && Movement->IsMovingOnGround() && IsValid(Status) &&
		Status->GetCurrentState().MatchesTagExact(TAG_State_Ground))
	{
		RideActionState = ERideActionState::Detached;
		bRecoveryLandingSatisfied = true;
	}
}

bool URideComponent::BeginRideSession(ARide* NewRide, const FVector& InitialVelocity)
{
	if (!IsValid(Player) || !IsValid(NewRide) || RideActionState != ERideActionState::Detached)
	{
		return false;
	}

	CapturePlayerState();
	RegisterRide(NewRide);
	NewRide->ApplyRideProfile(GetRideProfile());
	BeginRideCollision();
	NewRide->Mount(Player, InitialVelocity);

	if (!TransferControlToRide(NewRide, InitialVelocity))
	{
		UE_LOG(Log_RideSpawn, Error, TEXT("[RideComponent] Mount cancelled because control could not transfer to '%s'."),
		       *GetNameSafe(NewRide));
		CompleteRideSession(NewRide, false, true);
		return false;
	}

	Player->GetCharacterMovement()->DisableMovement();
	// Transition poses are owned exclusively by the full-body montage. Keep the
	// underlying ride state machine on locomotion so it does not play the same
	// mount sequence a second time.
	RideActionState = ERideActionState::Mounting;
	Player->GetCharacterStatusComponent()->SetState(TAG_State_Ride);
	MountStartTransform = Player->GetActorTransform();
	MountHorizontalAlpha = 0.0f;
	MountVerticalAlpha = 0.0f;

	RefreshTransitionTick();
	if (!PlayRideTransitionAnimation(true))
	{
		ForceDetachFromRide(true);
		return false;
	}
	StartTransitionWatchdog();
	return true;
}

bool URideComponent::RequestDismount(FVector InitVelocity)
{
	if (!IsValid(Player) || !IsValid(CurrentRide) || RideActionState != ERideActionState::Riding)
	{
		return false;
	}

	ARide* Ride = CurrentRide;
	const URideProfileDataAsset* Profile = GetRideProfile();
	if (!IsValid(Profile))
	{
		return false;
	}
	const bool bMovingDismount = InitVelocity.SizeSquared2D() >
		FMath::Square(Profile->MovingDismountSpeedThreshold);

	if (!bMovingDismount)
	{
		FTransform Candidate;
		if (!TryResolveGroundedDismountTransform(Ride->GetDismountTransform(), Ride, Candidate))
		{
			UE_LOG(Log_RideSpawn, Warning,
			       TEXT("[RideComponent] Dismount rejected because no walkable ground was found beside '%s'."),
			       *GetNameSafe(Ride));
			return false;
		}
		if (!IsSafeDismountTransform(Candidate, Ride))
		{
			UE_LOG(Log_RideSpawn, Warning,
			       TEXT("[RideComponent] Dismount rejected because the target beside '%s' is blocked."),
			       *GetNameSafe(Ride));
			return false;
		}
		NormalDismountTargetTransform = Candidate;
	}

	if (!TransferControlToPlayer(Ride))
	{
		return false;
	}
	Ride->NotifyDismountStarted(bMovingDismount);

	if (bMovingDismount)
	{
		RideActionState = ERideActionState::DismountingMoving;
		Player->DetachFromActor(FDetachmentTransformRules(EDetachmentRule::KeepWorld, false));
		Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		Player->SetSkipJumpStart(true);

		FVector DismountVelocity = InitVelocity * Profile->MovingDismountVelocityScale;
		DismountVelocity.Z = Profile->MovingDismountVerticalVelocity;
		Player->LaunchCharacter(DismountVelocity, true, true);

		CompleteRideSession(Ride, false, true, ERideActionState::Recovering);
		if (IsValid(Ride))
		{
			Ride->FinishDismount();
		}
		return true;
	}

	RideActionState = ERideActionState::DismountingNormal;
	Player->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Player->GetCharacterMovement()->StopMovementImmediately();
	Player->GetCharacterMovement()->DisableMovement();
	Player->SetSkipJumpStart(false);
	// The dismount montage owns the transition pose and completion delegate.
	// Do not enter the legacy AnimBP dismount state at the same time.
	NormalDismountStartTransform = Player->GetActorTransform();
	DismountHorizontalAlpha = 0.0f;
	DismountVerticalAlpha = 0.0f;
	bDismountVisualStateReleased = false;
	RefreshTransitionTick();
	if (!PlayRideTransitionAnimation(false))
	{
		ForceDetachFromRide(true);
		return false;
	}
	StartTransitionWatchdog();
	return true;
}

void URideComponent::HandleMountEnd()
{
	if (!IsValid(Player) || !IsValid(CurrentRide) || RideActionState != ERideActionState::Mounting)
	{
		return;
	}

	ClearTransitionTimers();
	RestoreTransitionRootMotionMode();
	const FTransform MountTransform = CurrentRide->GetMountTransform();
	Player->SetActorLocationAndRotation(MountTransform.GetLocation(), MountTransform.GetRotation().Rotator());
	CurrentRide->AttachRider();
	Player->GetCapsuleComponent()->SetCollisionEnabled(SavedCollisionEnabled);
	RideActionState = ERideActionState::Riding;
	RefreshTransitionTick();
	ClearMountAction();
}

bool URideComponent::PlayRideTransitionAnimation(bool bMounting)
{
	if (!IsValid(Player) || !IsValid(Player->GetMesh()))
	{
		return false;
	}

	UAnimInstance* AnimInstance = Player->GetMesh()->GetAnimInstance();
	const URideProfileDataAsset* Profile = GetRideProfile();
	if (!IsValid(AnimInstance) || !IsValid(Profile))
	{
		return false;
	}

	UAnimMontage* ConfiguredMontage = bMounting ? Profile->MountMontage : Profile->DismountMontage;
	if (!IsValid(ConfiguredMontage))
	{
		UE_LOG(Log_RideSpawn, Error, TEXT("[RideComponent] %s montage is not configured in RideProfile."),
			bMounting ? TEXT("Mount") : TEXT("Dismount"));
		return false;
	}
	StopRideTransitionAnimation();
	// Transition curves exclusively own capsule movement. The montage still
	// evaluates its pose and curves, but its authored root motion is not applied
	// a second time by CharacterMovement.
	AnimInstance->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
	ActiveRideTransitionMontage = ConfiguredMontage;
	const float PlayedLength = AnimInstance->Montage_Play(ConfiguredMontage);
	if (PlayedLength <= 0.0f)
	{
		UE_LOG(Log_RideSpawn, Error, TEXT("[RideComponent] Failed to play the configured %s montage."),
			bMounting ? TEXT("mount") : TEXT("dismount"));
		ActiveRideTransitionMontage = nullptr;
		RestoreTransitionRootMotionMode();
		return false;
	}
	TransitionAnimationStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	TransitionAnimationPlayRate = FMath::Max(
		FMath::Abs(AnimInstance->Montage_GetPlayRate(ConfiguredMontage)),
		UE_SMALL_NUMBER);

	FOnMontageEnded EndDelegate;
	if (bMounting)
	{
		EndDelegate.BindUObject(this, &URideComponent::OnMountMontageEnded);
	}
	else
	{
		EndDelegate.BindUObject(this, &URideComponent::OnDismountMontageEnded);
		FOnMontageBlendingOutStarted BlendingOutDelegate;
		BlendingOutDelegate.BindUObject(this, &URideComponent::OnDismountMontageBlendingOut);
		AnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, ActiveRideTransitionMontage);
	}
	AnimInstance->Montage_SetEndDelegate(EndDelegate, ActiveRideTransitionMontage);
	return true;
}

void URideComponent::StopRideTransitionAnimation()
{
	if (!IsValid(ActiveRideTransitionMontage) || !IsValid(Player) || !IsValid(Player->GetMesh()))
	{
		ActiveRideTransitionMontage = nullptr;
		return;
	}

	if (UAnimInstance* AnimInstance = Player->GetMesh()->GetAnimInstance())
	{
		FOnMontageEnded EmptyDelegate;
		FOnMontageBlendingOutStarted EmptyBlendingOutDelegate;
		AnimInstance->Montage_SetEndDelegate(EmptyDelegate, ActiveRideTransitionMontage);
		AnimInstance->Montage_SetBlendingOutDelegate(EmptyBlendingOutDelegate, ActiveRideTransitionMontage);
		if (AnimInstance->Montage_IsPlaying(ActiveRideTransitionMontage))
		{
			AnimInstance->Montage_Stop(0.1f, ActiveRideTransitionMontage);
		}
	}
	ActiveRideTransitionMontage = nullptr;
}

void URideComponent::OnMountMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != ActiveRideTransitionMontage)
	{
		return;
	}
	ActiveRideTransitionMontage = nullptr;
	if (RideActionState != ERideActionState::Mounting)
	{
		return;
	}
	if (bInterrupted)
	{
		ForceDetachFromRide(true);
		return;
	}
	HandleMountEnd();
}

void URideComponent::OnDismountMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	if (bInterrupted || bDismountVisualStateReleased || Montage != ActiveRideTransitionMontage ||
		RideActionState != ERideActionState::DismountingNormal || !IsValid(Player))
	{
		return;
	}

	if (UPlayerStatusComponent* Status = Player->GetCharacterStatusComponent();
		IsValid(Status) && !Status->IsDead())
	{
		ResetRideIKState(true);
		Status->SetState(TAG_State_Ground);
		bDismountVisualStateReleased = true;
	}
}

void URideComponent::OnDismountMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != ActiveRideTransitionMontage)
	{
		return;
	}
	ActiveRideTransitionMontage = nullptr;
	if (RideActionState != ERideActionState::DismountingNormal)
	{
		return;
	}
	if (bInterrupted)
	{
		ForceDetachFromRide(true);
		return;
	}
	HandleDismountEnd();
}

void URideComponent::HandleDismountEnd()
{
	if (!IsValid(Player) || RideActionState != ERideActionState::DismountingNormal)
	{
		return;
	}

	ARide* Ride = CurrentRide;
	ClearTransitionTimers();
	RestoreTransitionRootMotionMode();
	Player->SetActorLocationAndRotation(
		NormalDismountTargetTransform.GetLocation(),
		NormalDismountTargetTransform.GetRotation().Rotator());
	Player->DetachFromActor(FDetachmentTransformRules(EDetachmentRule::KeepWorld, false));

	CompleteRideSession(Ride, false, true, ERideActionState::Recovering);
	if (IsValid(Ride))
	{
		Ride->FinishDismount();
	}
}

void URideComponent::ForceDetachFromRide(bool bDestroyRide)
{
	if (RideActionState == ERideActionState::Detached && !IsValid(CurrentRide))
	{
		ClearTransitionTimers();
		RestoreTransitionRootMotionMode();
		return;
	}

	if (RideActionState == ERideActionState::ForceDetaching)
	{
		bForcedDetachDestroyRide |= bDestroyRide;
		return;
	}

	RideActionState = ERideActionState::ForceDetaching;
	RefreshTransitionTick();
	bForcedDetachDestroyRide = bDestroyRide;
	bForcedDetachRestoreGroundState = !(IsValid(Player) && Player->GetCharacterStatusComponent() &&
		Player->GetCharacterStatusComponent()->IsDead());
	bForcedDetachCompletionStarted = false;
	ForcedDetachAttemptCount = 0;
	ClearTransitionTimers();
	StopRideTransitionAnimation();
	AttemptForcedDetach();
}

void URideComponent::AttemptForcedDetach()
{
	if (RideActionState != ERideActionState::ForceDetaching || bForcedDetachCompletionStarted)
	{
		return;
	}

	++ForcedDetachAttemptCount;
	if (TransferControlToPlayer(CurrentRide, false))
	{
		FinishForcedDetach();
		return;
	}

	if (ForcedDetachAttemptCount >= MaxForcedDetachAttempts || !GetWorld())
	{
		HandleForcedDetachFailure();
		return;
	}

	GetWorld()->GetTimerManager().SetTimer(
		ForcedDetachRetryTimerHandle,
		this,
		&URideComponent::AttemptForcedDetach,
		ForcedDetachRetryInterval,
		false);
}

void URideComponent::FinishForcedDetach()
{
	if (RideActionState != ERideActionState::ForceDetaching || bForcedDetachCompletionStarted)
	{
		return;
	}
	bForcedDetachCompletionStarted = true;
	ARide* Ride = CurrentRide;
	if (IsValid(Player))
	{
		Player->DetachFromActor(FDetachmentTransformRules(EDetachmentRule::KeepWorld, false));
	}
	CompleteRideSession(Ride, bForcedDetachDestroyRide, bForcedDetachRestoreGroundState);
}

void URideComponent::HandleForcedDetachFailure()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(ForcedDetachRetryTimerHandle);
	}
	APlayerController* Controller = SessionController;
	if (!IsValid(Controller) && IsValid(CurrentRide))
	{
		Controller = Cast<APlayerController>(CurrentRide->GetController());
	}
	if (IsValid(Controller))
	{
		if (Controller->GetPawn() == CurrentRide)
		{
			Controller->UnPossess();
		}
		if (IsValid(Player))
		{
			Controller->SetViewTarget(Player);
		}
	}
	ReleaseTransitionCamera();
	UE_LOG(Log_RideSpawn, Error,
		TEXT("[RideComponent] Forced detach could not return possession to '%s' after %d attempts; ride session is preserved and the ride will not be destroyed."),
		*GetNameSafe(Player), ForcedDetachAttemptCount);
}

void URideComponent::AbortForcedDetachForEndPlay()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(ForcedDetachRetryTimerHandle);
	}
	APlayerController* Controller = SessionController;
	if (IsValid(Controller) && Controller->GetPawn() == CurrentRide)
	{
		Controller->UnPossess();
		if (IsValid(Player))
		{
			Controller->SetViewTarget(Player);
		}
	}
	ARide* Ride = CurrentRide;
	if (IsValid(Player))
	{
		Player->DetachFromActor(FDetachmentTransformRules(EDetachmentRule::KeepWorld, false));
	}
	// Component teardown cannot own an asynchronous retry. Restore all local
	// state, but deliberately leave the ride alive when possession was not proven.
	CompleteRideSession(Ride, false, bForcedDetachRestoreGroundState);
}

void URideComponent::CompleteRideSession(ARide* Ride, bool bDestroyRide, bool bRestoreGroundState,
	ERideActionState FinalState)
{
	ClearTransitionTimers();
	StopRideTransitionAnimation();
	RestoreTransitionRootMotionMode();
	ClearMountAction();
	bRideInputPressed = false;
	if (FinalState == ERideActionState::Recovering)
	{
		bRecoveryLandingSatisfied = RideActionState != ERideActionState::DismountingMoving;
	}
	const bool bContinueRideForward = RideActionState == ERideActionState::DismountingMoving && !bDestroyRide;
	if (IsValid(Ride))
	{
		Ride->ReleaseRider(bContinueRideForward);
	}

	EndRideCollision(Ride);
	UnregisterRide(Ride);
	CurrentRide = nullptr;
	RestorePlayerState(bRestoreGroundState);
	RideActionState = FinalState;
	bDismountVisualStateReleased = false;
	bForcedDetachCompletionStarted = false;
	ForcedDetachAttemptCount = 0;
	RefreshTransitionTick();

	if (bDestroyRide && IsValid(Ride))
	{
		Ride->Destroy();
	}
}

void URideComponent::CapturePlayerState()
{
	if (bHasPlayerStateSnapshot || !IsValid(Player))
	{
		return;
	}

	UCharacterMovementComponent* Movement = Player->GetCharacterMovement();
	SavedMovementMode = Movement->MovementMode;
	SavedCustomMovementMode = Movement->CustomMovementMode;
	SavedCollisionProfile = Player->GetCapsuleComponent()->GetCollisionProfileName();
	SavedCollisionEnabled = Player->GetCapsuleComponent()->GetCollisionEnabled();
	SavedCollisionResponses = Player->GetCapsuleComponent()->GetCollisionResponseToChannels();
	SavedCollisionObjectType = Player->GetCapsuleComponent()->GetCollisionObjectType();
	bSavedSkipJumpStart = Player->ShouldSkipJumpStart();
	if (const UAnimInstance* AnimInstance = Player->GetMesh() ? Player->GetMesh()->GetAnimInstance() : nullptr)
	{
		// UE 5.4 exposes a setter but no public getter for RootMotionMode. Read the
		// reflected property so the transition can restore the actual pre-ride mode.
		if (const FByteProperty* RootMotionModeProperty =
			FindFProperty<FByteProperty>(AnimInstance->GetClass(), TEXT("RootMotionMode")))
		{
			SavedRootMotionMode = static_cast<ERootMotionMode::Type>(
				RootMotionModeProperty->GetPropertyValue_InContainer(AnimInstance));
		}
	}
	SessionController = Cast<APlayerController>(Player->GetController());
	if (IsValid(SessionController))
	{
		bSavedAutoManageCameraTarget = SessionController->bAutoManageActiveCameraTarget;
	}
	bHasPlayerStateSnapshot = true;
}

void URideComponent::RestorePlayerState(bool bRestoreGroundState)
{
	if (!bHasPlayerStateSnapshot || !IsValid(Player))
	{
		bHasPlayerStateSnapshot = false;
		SessionController = nullptr;
		return;
	}

	const bool bPreserveMovingDismount = RideActionState == ERideActionState::DismountingMoving;
	Player->GetCapsuleComponent()->SetCollisionProfileName(SavedCollisionProfile);
	Player->GetCapsuleComponent()->SetCollisionObjectType(SavedCollisionObjectType);
	Player->GetCapsuleComponent()->SetCollisionResponseToChannels(SavedCollisionResponses);
	Player->GetCapsuleComponent()->SetCollisionEnabled(SavedCollisionEnabled);
	Player->SetSkipJumpStart(bPreserveMovingDismount ? true : (bRestoreGroundState ? bSavedSkipJumpStart : false));
	if (bRestoreGroundState)
	{
		if (!bPreserveMovingDismount)
		{
			Player->GetCharacterMovement()->SetMovementMode(SavedMovementMode, SavedCustomMovementMode);
		}
		if (UPlayerStatusComponent* Status = Player->GetCharacterStatusComponent())
		{
			Status->SetState(TAG_State_Ground);
		}
	}
	ResetRideIKState(bRestoreGroundState);

	if (IsValid(SessionController))
	{
		SessionController->bAutoManageActiveCameraTarget = bSavedAutoManageCameraTarget;
	}
	bHasPlayerStateSnapshot = false;
	SessionController = nullptr;
}

void URideComponent::RegisterRide(ARide* NewRide)
{
	CurrentRide = NewRide;
	if (IsValid(CurrentRide))
	{
		CurrentRide->OnDestroyed.RemoveDynamic(this, &URideComponent::HandleRideDestroyed);
		CurrentRide->OnDestroyed.AddDynamic(this, &URideComponent::HandleRideDestroyed);
	}
}

void URideComponent::UnregisterRide(ARide* Ride)
{
	if (IsValid(Ride))
	{
		Ride->OnDestroyed.RemoveDynamic(this, &URideComponent::HandleRideDestroyed);
	}
}

void URideComponent::HandleRideDestroyed(AActor* DestroyedActor)
{
	if (DestroyedActor == CurrentRide)
	{
		ForceDetachFromRide(false);
	}
}

bool URideComponent::IsSafeDismountTransform(const FTransform& Candidate, const ARide* Ride) const
{
	if (!IsValid(Player) || !GetWorld())
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RideDismount), false, Player);
	if (IsValid(Ride))
	{
		QueryParams.AddIgnoredActor(Ride);
	}

	const UCapsuleComponent* Capsule = Player->GetCapsuleComponent();
	const URideProfileDataAsset* Profile = GetRideProfile();
	if (!IsValid(Capsule) || !IsValid(Profile))
	{
		return false;
	}
	const float Tolerance = FMath::Max(Profile->DismountOverlapInflationTolerance, 0.0f);
	const FCollisionShape Shape = FCollisionShape::MakeCapsule(
		FMath::Max(Capsule->GetScaledCapsuleRadius() - Tolerance, 1.0f),
		FMath::Max(Capsule->GetScaledCapsuleHalfHeight() - Tolerance, 1.0f));
	const FVector Location = Candidate.GetLocation();
	const FCollisionResponseParams ResponseParams(SavedCollisionResponses);
	return !GetWorld()->OverlapBlockingTestByChannel(
		Location,
		Candidate.GetRotation(),
		SavedCollisionObjectType,
		Shape,
		QueryParams,
		ResponseParams);
}

bool URideComponent::TryResolveGroundedDismountTransform(
	const FTransform& Anchor, const ARide* Ride, FTransform& OutTransform) const
{
	if (!IsValid(Player) || !GetWorld())
	{
		return false;
	}

	const URideProfileDataAsset* Profile = GetRideProfile();
	const UCapsuleComponent* Capsule = Player->GetCapsuleComponent();
	const UCharacterMovementComponent* Movement = Player->GetCharacterMovement();
	if (!IsValid(Profile) || !IsValid(Capsule) || !IsValid(Movement))
	{
		return false;
	}

	const float Radius = Capsule->GetScaledCapsuleRadius();
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(Radius, HalfHeight);
	const FVector AnchorLocation = Anchor.GetLocation();
	const FVector SweepStart = AnchorLocation + FVector::UpVector * Profile->DismountGroundTraceUpDistance;
	const FVector SweepEnd = AnchorLocation - FVector::UpVector * Profile->DismountGroundTraceDownDistance;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RideDismountGround), false, Player);
	if (IsValid(Ride))
	{
		QueryParams.AddIgnoredActor(Ride);
	}

	FHitResult GroundHit;
	const FCollisionResponseParams ResponseParams(SavedCollisionResponses);
	if (!GetWorld()->SweepSingleByChannel(
		GroundHit,
		SweepStart,
		SweepEnd,
		FQuat::Identity,
		SavedCollisionObjectType,
		CapsuleShape,
		QueryParams,
		ResponseParams) ||
		!GroundHit.bBlockingHit || !Movement->IsWalkable(GroundHit))
	{
		return false;
	}

	// FHitResult::Location is the swept capsule center at first contact. Use it
	// directly instead of reconstructing a center from one floor impact point;
	// this accounts for capsule curvature on stairs, slopes, and step edges.
	FVector GroundedLocation = GroundHit.Location;
	GroundedLocation.Z += FMath::Max(Profile->DismountGroundClearance, 0.0f);
	FRotator GroundedRotation = Anchor.GetRotation().Rotator();
	GroundedRotation.Pitch = 0.0f;
	GroundedRotation.Roll = 0.0f;
	OutTransform = FTransform(GroundedRotation, GroundedLocation, FVector::OneVector);
	return true;
}

bool URideComponent::TransferControlToRide(ARide* Ride, const FVector& InitialVelocity)
{
	if (!IsValid(Player) || !IsValid(Ride))
	{
		return false;
	}

	APlayerController* Controller = SessionController;
	if (!IsValid(Controller))
	{
		return false;
	}

	const FRotator SourceControlRotation = Controller->GetControlRotation();
	Controller->bAutoManageActiveCameraTarget = false;
	LockCurrentCamera(Controller);
	Controller->Possess(Ride);
	if (Controller->GetPawn() != Ride)
	{
		Controller->Possess(Player);
		Controller->SetViewTarget(Player);
		ReleaseTransitionCamera();
		Controller->bAutoManageActiveCameraTarget = bSavedAutoManageCameraTarget;
		return false;
	}

	Controller->SetControlRotation(SourceControlRotation);
	Ride->RefreshRideCameraComponents();
	BlendFromLockedCamera(Controller, Ride);
	Ride->GetCharacterMovement()->Velocity = InitialVelocity;
	return true;
}

bool URideComponent::TransferControlToPlayer(ARide* Ride, bool bRestoreRideOnFailure)
{
	if (!IsValid(Player))
	{
		return false;
	}

	APlayerController* Controller = SessionController;
	if (!IsValid(Controller))
	{
		Controller = IsValid(Ride) ? Cast<APlayerController>(Ride->GetController()) : nullptr;
	}
	if (!IsValid(Controller))
	{
		return false;
	}

	FRotator ControlRotation = Controller->GetControlRotation();
	Controller->bAutoManageActiveCameraTarget = false;
	LockCurrentCamera(Controller);
	Controller->Possess(Player);
	if (Controller->GetPawn() != Player)
	{
		UE_LOG(Log_RideSpawn, Error, TEXT("[RideComponent] Failed to return possession to '%s'."), *GetNameSafe(Player));
		if (bRestoreRideOnFailure && IsValid(Ride))
		{
			Controller->Possess(Ride);
			Controller->SetViewTarget(Ride);
		}
		else
		{
			Controller->UnPossess();
			Controller->SetViewTarget(Player);
		}
		ReleaseTransitionCamera();
		return false;
	}

	Controller->SetControlRotation(ControlRotation);
	Player->RefreshPlayerCameraComponents();
	BlendFromLockedCamera(Controller, Player);
	return true;
}

bool URideComponent::LockCurrentCamera(APlayerController* Controller)
{
	if (!IsValid(Controller) || !GetWorld())
	{
		return false;
	}

	ReleaseTransitionCamera();

	FVector ViewLocation;
	FRotator ViewRotation;
	AActor* SourceViewTarget = Controller->GetViewTarget();
	if (USpringArmComponent* SourceSpringArm = GetCameraSpringArm(SourceViewTarget))
	{
		TransitionStartArmLength = SourceSpringArm->TargetArmLength;
	}
	Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.ObjectFlags |= RF_Transient;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	TransitionCamera = GetWorld()->SpawnActor<ACameraActor>(ViewLocation, ViewRotation, SpawnParameters);
	if (!IsValid(TransitionCamera))
	{
		return false;
	}

	if (Controller->PlayerCameraManager && TransitionCamera->GetCameraComponent())
	{
		TransitionCamera->GetCameraComponent()->SetFieldOfView(Controller->PlayerCameraManager->GetFOVAngle());
		TransitionCamera->GetCameraComponent()->SetConstraintAspectRatio(false);
	}
	Controller->SetViewTarget(TransitionCamera);
	return true;
}

void URideComponent::BlendFromLockedCamera(APlayerController* Controller, AActor* NewViewTarget)
{
	const URideProfileDataAsset* Profile = GetRideProfile();
	if (!IsValid(Controller) || !IsValid(NewViewTarget))
	{
		ReleaseTransitionCamera();
		return;
	}

	if (!IsValid(Profile) || !IsValid(TransitionCamera) || Profile->CameraBlendDuration <= 0.0f)
	{
		Controller->SetViewTarget(NewViewTarget);
		ReleaseTransitionCamera();
		return;
	}

	TransitionSpringArm = GetCameraSpringArm(NewViewTarget);
	if (!IsValid(TransitionSpringArm))
	{
		Controller->SetViewTarget(NewViewTarget);
		ReleaseTransitionCamera();
		return;
	}

	TransitionController = Controller;
	TransitionViewTarget = NewViewTarget;
	TransitionCameraElapsed = 0.0f;
	TransitionTargetArmLength = TransitionSpringArm->TargetArmLength;
	TransitionTargetOffset = TransitionSpringArm->TargetOffset;
	bTransitionCameraLagEnabled = TransitionSpringArm->bEnableCameraLag;
	bTransitionCameraRotationLagEnabled = TransitionSpringArm->bEnableCameraRotationLag;
	TransitionSpringArm->bEnableCameraLag = false;
	TransitionSpringArm->bEnableCameraRotationLag = false;
	TransitionSpringArm->TargetArmLength = TransitionStartArmLength;
	RefreshCameraRig(NewViewTarget);

	const FVector CameraCorrection = TransitionCamera->GetActorLocation() -
		(Cast<APlayerBase>(NewViewTarget)
			? CastChecked<APlayerBase>(NewViewTarget)->GetCameraTransform().GetLocation()
			: CastChecked<ARide>(NewViewTarget)->GetCameraTransform().GetLocation());
	TransitionStartTargetOffset = TransitionTargetOffset + CameraCorrection;
	TransitionSpringArm->TargetOffset = TransitionStartTargetOffset;
	RefreshCameraRig(NewViewTarget);
	Controller->SetViewTarget(NewViewTarget);
	TransitionCamera->Destroy();
	TransitionCamera = nullptr;
	RefreshTransitionTick();
}

void URideComponent::UpdateTransitionCamera(float DeltaTime)
{
	if (!IsValid(TransitionController) || !IsValid(TransitionViewTarget) || !IsValid(TransitionSpringArm))
	{
		ReleaseTransitionCamera();
		return;
	}

	TransitionCameraElapsed += DeltaTime;
	const URideProfileDataAsset* Profile = GetRideProfile();
	if (!IsValid(Profile))
	{
		ReleaseTransitionCamera();
		return;
	}
	const float LinearAlpha = FMath::Clamp(TransitionCameraElapsed / Profile->CameraBlendDuration, 0.0f, 1.0f);
	const float BlendAlpha = FMath::SmoothStep(0.0f, 1.0f, LinearAlpha);
	TransitionSpringArm->TargetArmLength = FMath::Lerp(
		TransitionStartArmLength, TransitionTargetArmLength, BlendAlpha);
	TransitionSpringArm->TargetOffset = FMath::Lerp(
		TransitionStartTargetOffset, TransitionTargetOffset, BlendAlpha);
	RefreshCameraRig(TransitionViewTarget);

	if (LinearAlpha >= 1.0f)
	{
		ReleaseTransitionCamera();
	}
}

USpringArmComponent* URideComponent::GetCameraSpringArm(AActor* ViewTarget) const
{
	if (APlayerBase* TargetPlayer = Cast<APlayerBase>(ViewTarget))
	{
		return TargetPlayer->GetRideSpringArmComponent();
	}
	if (ARide* TargetRide = Cast<ARide>(ViewTarget))
	{
		return TargetRide->GetRideSpringArmComponent();
	}
	return nullptr;
}

void URideComponent::RefreshCameraRig(AActor* ViewTarget) const
{
	if (USpringArmComponent* TargetSpringArm = GetCameraSpringArm(ViewTarget))
	{
		// UpdateComponentToWorld alone does not recalculate the spring arm socket.
		// Force its camera calculation before exposing the new view target so an
		// old one-frame socket location can never reach the player camera manager.
		TargetSpringArm->TickComponent(0.0f, LEVELTICK_All, nullptr);
	}

	if (APlayerBase* TargetPlayer = Cast<APlayerBase>(ViewTarget))
	{
		TargetPlayer->RefreshPlayerCameraComponents();
	}
	else if (ARide* TargetRide = Cast<ARide>(ViewTarget))
	{
		TargetRide->RefreshRideCameraComponents();
	}
}

void URideComponent::ReleaseTransitionCamera()
{
	if (IsValid(TransitionCamera))
	{
		TransitionCamera->Destroy();
	}
	TransitionCamera = nullptr;
	if (IsValid(TransitionSpringArm))
	{
		TransitionSpringArm->TargetArmLength = TransitionTargetArmLength;
		TransitionSpringArm->TargetOffset = TransitionTargetOffset;
		TransitionSpringArm->bEnableCameraLag = bTransitionCameraLagEnabled;
		TransitionSpringArm->bEnableCameraRotationLag = bTransitionCameraRotationLagEnabled;
		RefreshCameraRig(TransitionViewTarget);
	}
	TransitionSpringArm = nullptr;
	TransitionController = nullptr;
	TransitionViewTarget = nullptr;
	TransitionCameraElapsed = 0.0f;
	RefreshTransitionTick();
}

void URideComponent::RefreshTransitionTick()
{
	const bool bNeedsTransitionTick =
		RideActionState == ERideActionState::Mounting ||
		RideActionState == ERideActionState::DismountingNormal ||
		IsValid(TransitionSpringArm);
	SetComponentTickEnabled(bNeedsTransitionTick);
}

void URideComponent::ClearTransitionTimers()
{
	if (!GetWorld())
	{
		return;
	}
	FTimerManager& Timers = GetWorld()->GetTimerManager();
	Timers.ClearTimer(TransitionWatchdogHandle);
	Timers.ClearTimer(ForcedDetachRetryTimerHandle);
}

void URideComponent::StartTransitionWatchdog()
{
	const URideProfileDataAsset* Profile = GetRideProfile();
	if (!GetWorld() || !IsValid(Profile))
	{
		return;
	}
	GetWorld()->GetTimerManager().SetTimer(
		TransitionWatchdogHandle,
		this,
		&URideComponent::OnTransitionTimeout,
		FMath::Max(Profile->TransitionTimeout, 0.1f),
		false);
}

void URideComponent::OnTransitionTimeout()
{
	if (RideActionState == ERideActionState::Mounting || RideActionState == ERideActionState::DismountingNormal)
	{
		UE_LOG(Log_RideSpawn, Error, TEXT("[RideComponent] Ride transition timed out in state %d."),
		       static_cast<int32>(RideActionState));
		ForceDetachFromRide(true);
	}
}

void URideComponent::BeginRideCollision()
{
	if (!IsValid(Player))
	{
		return;
	}
	Player->GetCapsuleComponent()->SetCollisionProfileName(TEXT("Character_Riding"));
	Player->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (IsValid(CurrentRide))
	{
		Player->GetCapsuleComponent()->IgnoreActorWhenMoving(CurrentRide, true);
		CurrentRide->GetCapsuleComponent()->IgnoreActorWhenMoving(Player, true);
	}
}

void URideComponent::EndRideCollision(ARide* Ride)
{
	if (!IsValid(Player))
	{
		return;
	}
	if (IsValid(Ride))
	{
		Player->GetCapsuleComponent()->IgnoreActorWhenMoving(Ride, false);
		Ride->GetCapsuleComponent()->IgnoreActorWhenMoving(Player, false);
	}
	if (bHasPlayerStateSnapshot)
	{
		Player->GetCapsuleComponent()->SetCollisionProfileName(SavedCollisionProfile);
		Player->GetCapsuleComponent()->SetCollisionObjectType(SavedCollisionObjectType);
		Player->GetCapsuleComponent()->SetCollisionResponseToChannels(SavedCollisionResponses);
		Player->GetCapsuleComponent()->SetCollisionEnabled(SavedCollisionEnabled);
	}
}

void URideComponent::UpdateMountTransition()
{
	if (!IsValid(Player) || !IsValid(CurrentRide) || RideActionState != ERideActionState::Mounting)
	{
		ForceDetachFromRide(true);
		return;
	}

	const URideProfileDataAsset* Profile = GetRideProfile();
	float HorizontalCurveValue = 0.0f;
	float VerticalCurveValue = 0.0f;
	if (!IsValid(Profile) ||
		!TryEvaluateTransitionCurve(Profile->MountHorizontalCurve, HorizontalCurveValue) ||
		!TryEvaluateTransitionCurve(Profile->MountVerticalCurve, VerticalCurveValue))
	{
		ForceDetachFromRide(true);
		return;
	}

	MountHorizontalAlpha = FMath::Max(MountHorizontalAlpha,
		FMath::Clamp(HorizontalCurveValue, 0.0f, 1.0f));
	// Preserve the authored vertical arc. Values above 1 move the rider above
	// the target, and a later return to 1 settles exactly onto the saddle.
	MountVerticalAlpha = VerticalCurveValue;
	const FTransform Target = CurrentRide->GetMountTransform();
	FVector Location = FMath::Lerp(MountStartTransform.GetLocation(), Target.GetLocation(), MountHorizontalAlpha);
	Location.Z = FMath::Lerp(MountStartTransform.GetLocation().Z, Target.GetLocation().Z, MountVerticalAlpha);
	const FQuat Rotation = FQuat::Slerp(MountStartTransform.GetRotation(), Target.GetRotation(), MountHorizontalAlpha);
	Player->SetActorLocationAndRotation(Location, Rotation.Rotator());
}

void URideComponent::UpdateNormalDismountTransition()
{
	if (!IsValid(Player) || RideActionState != ERideActionState::DismountingNormal)
	{
		ForceDetachFromRide(true);
		return;
	}

	const URideProfileDataAsset* Profile = GetRideProfile();
	float HorizontalCurveValue = 0.0f;
	float VerticalCurveValue = 0.0f;
	if (!IsValid(Profile) ||
		!TryEvaluateTransitionCurve(Profile->DismountHorizontalCurve, HorizontalCurveValue) ||
		!TryEvaluateTransitionCurve(Profile->DismountVerticalCurve, VerticalCurveValue))
	{
		ForceDetachFromRide(true);
		return;
	}

	DismountHorizontalAlpha = FMath::Max(DismountHorizontalAlpha,
		FMath::Clamp(HorizontalCurveValue, 0.0f, 1.0f));
	// Unlike horizontal progress, vertical motion may intentionally overshoot
	// and descend, so consume the authored curve without clamping or latching.
	DismountVerticalAlpha = VerticalCurveValue;
	FVector Location = FMath::Lerp(
		NormalDismountStartTransform.GetLocation(), NormalDismountTargetTransform.GetLocation(), DismountHorizontalAlpha);
	Location.Z = FMath::Lerp(
		NormalDismountStartTransform.GetLocation().Z, NormalDismountTargetTransform.GetLocation().Z, DismountVerticalAlpha);
	const FQuat Rotation = FQuat::Slerp(
		NormalDismountStartTransform.GetRotation(), NormalDismountTargetTransform.GetRotation(), DismountHorizontalAlpha);
	Player->SetActorLocationAndRotation(Location, Rotation.Rotator());
}

bool URideComponent::TryEvaluateTransitionCurve(const UCurveFloat* Curve, float& OutValue) const
{
	OutValue = 0.0f;
	if (!GetWorld() || !IsValid(Curve) || !IsValid(ActiveRideTransitionMontage) ||
		!IsValid(Player) || !IsValid(Player->GetMesh()))
	{
		return false;
	}

	const float MontageLength = ActiveRideTransitionMontage->GetPlayLength();
	if (MontageLength <= UE_SMALL_NUMBER)
	{
		return false;
	}

	const float NormalizedTime = FMath::Clamp(
		(static_cast<float>(GetWorld()->GetTimeSeconds() - TransitionAnimationStartTime) *
			TransitionAnimationPlayRate) / MontageLength,
		0.0f,
		1.0f);
	OutValue = Curve->GetFloatValue(NormalizedTime);
	return true;
}

void URideComponent::RestoreTransitionRootMotionMode()
{
	if (IsValid(Player) && IsValid(Player->GetMesh()))
	{
		if (UAnimInstance* AnimInstance = Player->GetMesh()->GetAnimInstance())
		{
			AnimInstance->SetRootMotionMode(SavedRootMotionMode);
		}
	}
}

void URideComponent::ResetRideIKState(bool bRestoreGroundPhase)
{
	UAnimInstance* AnimInstance = Player && Player->GetMesh() ? Player->GetMesh()->GetAnimInstance() : nullptr;
	if (!IsValid(AnimInstance) || !AnimInstance->GetClass()->ImplementsInterface(UIAnimInstance::StaticClass()))
	{
		return;
	}
	IIAnimInstance::Execute_SetIKPhaseAlpha(AnimInstance, TAG_IK_Phase_Ride, 0.0f);
	if (bRestoreGroundPhase)
	{
		IIAnimInstance::Execute_SetIKPhaseAlpha(AnimInstance, TAG_IK_Phase_Ground, 1.0f);
	}
}

float URideComponent::GetRideSpeed() const
{
	return IsValid(CurrentRide) ? CurrentRide->GetRideSpeed() : 0.0f;
}

float URideComponent::GetRideDirection() const
{
	return IsValid(CurrentRide) ? CurrentRide->GetRideDirection() : 0.0f;
}
