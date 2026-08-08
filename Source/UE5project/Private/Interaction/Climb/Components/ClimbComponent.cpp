// Fill out your copyright notice in the Description page of Project Settings.

#include "Interaction/Climb/Components/ClimbComponent.h"
#include "Environment/Climbable/Ladder/LadderBase.h"
#include "Interaction/Climb/Data/LadderClimbDataAsset.h"
#include "Interaction/Interfaces/InteractInterface.h"
#include "Characters/Components/CharacterStatusComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Characters/CharacterBase.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/Notifies/ANS_LadderGripTransition.h"
#include "Animation/Interfaces/IAnimInstance.h"
#include "Curves/CurveVector.h"
#include "MotionWarpingComponent.h"
#include "DrawDebugHelpers.h"

#include "Utils/CoreLog.h"
#include "Utils/GameplayTagsBase.h"

namespace
{
bool IsLadderEnterPhase(EClimbPhase Phase)
{
	return Phase == EClimbPhase::Enter_From_Bottom || Phase == EClimbPhase::Enter_From_Top;
}

bool IsLadderTopExitPhase(EClimbPhase Phase)
{
	return Phase == EClimbPhase::Exit_From_Top_Left || Phase == EClimbPhase::Exit_From_Top_Right;
}

bool IsLadderBottomExitPhase(EClimbPhase Phase)
{
	return Phase == EClimbPhase::Exit_From_Bottom_Left || Phase == EClimbPhase::Exit_From_Bottom_Right;
}

bool IsLadderExitPhase(EClimbPhase Phase)
{
	return IsLadderTopExitPhase(Phase) || IsLadderBottomExitPhase(Phase);
}
}

// Sets default values for this component's properties
UClimbComponent::UClimbComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these
	// features off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

// Called when the game starts
void UClimbComponent::BeginPlay()
{
	Super::BeginPlay();

	if (ACharacterBase* Character = Cast<ACharacterBase>(GetOwner()))
	{
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			Mesh->AddTickPrerequisiteComponent(this);
		}

		if (UCharacterStatusComponent* StatusComponent = Character->GetCharacterStatusComponent())
		{
			StatusComponent->OnDeathStarted.AddUObject(this, &UClimbComponent::HandleOwnerDeathStarted);
		}
	}
}

void UClimbComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ACharacterBase* Character = Cast<ACharacterBase>(GetOwner()))
	{
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			Mesh->RemoveTickPrerequisiteComponent(this);
		}

		if (UCharacterStatusComponent* StatusComponent = Character->GetCharacterStatusComponent())
		{
			StatusComponent->OnDeathStarted.RemoveAll(this);
		}
	}

	ForceDetachFromLadder(false);
	Super::EndPlay(EndPlayReason);
}

const FLadderRepeatedStepDefinition* UClimbComponent::GetRepeatedStepDefinition(EClimbPhase Phase) const
{
	return LadderClimbProfile ? LadderClimbProfile->RepeatedSteps.Find(Phase) : nullptr;
}

UAnimMontage* UClimbComponent::GetClimbMontage(EClimbPhase Phase) const
{
	if (!LadderClimbProfile)
		return nullptr;
	return LadderClimbProfile->Montages.FindRef(Phase);
}

UAnimSequence* UClimbComponent::GetLadderIdleAnimation() const
{
	if (!LadderClimbProfile)
		return nullptr;
	return ResolveIdlePhaseFromGripState() == EClimbPhase::Idle_Left ? LadderClimbProfile->IdleLeftAnimation.Get()
	                                                                 : LadderClimbProfile->IdleRightAnimation.Get();
}

void UClimbComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (LadderActionState == ELadderActionState::ClimbingStep)
	{
		TickRepeatedStep(DeltaTime);
		return;
	}

	if (LadderActionState == ELadderActionState::Recovering)
	{
		TickRepeatedStepRecovery(DeltaTime);
	}
}

void UClimbComponent::TickRepeatedStep(float DeltaTime)
{
	if (!RepeatedStepRuntime.ActiveStep.IsSet() || !IsValid(ClimbObject))
	{
		ForceDetachFromLadder(true);
		return;
	}

	FActiveRepeatedStep& ActiveStep = RepeatedStepRuntime.ActiveStep.GetValue();
	const FLadderRepeatedStepDefinition* Step = GetRepeatedStepDefinition(ActiveStep.Phase);
	if (!Step || ActiveStep.Duration <= 0.0f || !IsValid(Step->BodyCurve.Get()) || !IsValid(Step->HandCurve.Get()) ||
	    !IsValid(Step->FootCurve.Get()))
	{
		ForceDetachFromLadder(true);
		return;
	}

	ActiveStep.ElapsedTime = FMath::Min(ActiveStep.ElapsedTime + DeltaTime, ActiveStep.Duration);
	const float Progress = FMath::Clamp(ActiveStep.ElapsedTime / ActiveStep.Duration, 0.0f, 1.0f);
	RepeatedStepRuntime.ExplicitTime = Progress * Step->Animation->GetPlayLength();

	const FVector BodyCurveValue = Step->BodyCurve->GetVectorValue(Progress);
	const FVector HandCurveValue = Step->HandCurve->GetVectorValue(Progress);
	const FVector FootCurveValue = Step->FootCurve->GetVectorValue(Progress);
	const FTransform LadderTransform = ClimbObject->GetActorTransform();
	const FVector LocalStart = LadderTransform.InverseTransformPosition(ActiveStep.BodyStart);
	const FVector LocalTarget = LadderTransform.InverseTransformPosition(ActiveStep.BodyTarget);
	const FVector NewLocation = LadderTransform.TransformPosition(FMath::Lerp(LocalStart, LocalTarget, BodyCurveValue));
	if (!MoveCharacterAlongClimbPath(NewLocation))
	{
		ForceDetachFromLadder(true);
		return;
	}

	FLimbData* HandData = LimbToGripNode.Find(ActiveStep.MovingHand);
	FLimbData* FootData = LimbToGripNode.Find(ActiveStep.MovingFoot);
	if (!HandData || !FootData)
	{
		ForceDetachFromLadder(true);
		return;
	}

	const float HandSideOffset = ActiveStep.MovingHand == ELimbList::HandL ? 15.0f : -15.0f;
	const float FootSideOffset = ActiveStep.MovingFoot == ELimbList::FootL ? 15.0f : -15.0f;
	HandData->LimbLocation = SetBoneIKTargetLadder(ActiveStep.HandTargetGrip, HandCurveValue, HandSideOffset,
	                                               ActiveStep.HandStartGrip);
	FootData->LimbLocation = SetBoneIKTargetLadder(ActiveStep.FootTargetGrip, FootCurveValue, FootSideOffset,
	                                               ActiveStep.FootStartGrip);

	if (Progress >= 1.0f)
	{
		FinishActiveRepeatedStep();
	}
}

void UClimbComponent::TickRepeatedStepRecovery(float DeltaTime)
{
	RepeatedStepRuntime.RecoveryElapsed += DeltaTime;
	const float RecoveryDuration = IsValid(LadderClimbProfile)
	                                   ? FMath::Max(LadderClimbProfile->RepeatedStepRecoveryDuration, 0.0f)
	                                   : 0.0f;
	if (RepeatedStepRuntime.RecoveryElapsed < RecoveryDuration)
	{
		return;
	}

	LadderActionState = ELadderActionState::Idle;
	RepeatedStepRuntime.RecoveryElapsed = 0.0f;
	RepeatedStepRuntime.Animation.Reset();
	RepeatedStepRuntime.ExplicitTime = 0.0f;
	SetComponentTickEnabled(false);
	if (RepeatedStepRuntime.InputDirection != 0)
	{
		StartRepeatedClimbStep(RepeatedStepRuntime.InputDirection > 0);
	}
}

bool UClimbComponent::MoveCharacterAlongClimbPath(const FVector& TargetLocation)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	UCharacterMovementComponent* MovementComponent = Character ? Character->GetCharacterMovement() : nullptr;
	UCapsuleComponent* CapsuleComponent = Character ? Character->GetCapsuleComponent() : nullptr;
	if (!IsValid(Character) || !IsValid(MovementComponent) || !IsValid(CapsuleComponent))
	{
		UE_LOG(
		    Log_Climb_Ladder, Error,
		    TEXT("[ClimbComponent] Cannot move along the ladder without a valid CharacterMovement updated component."));
		return false;
	}

	const FVector Delta = TargetLocation - CapsuleComponent->GetComponentLocation();
	if (Delta.IsNearlyZero())
	{
		return true;
	}

	FHitResult Hit;
	MovementComponent->MoveUpdatedComponent(Delta, CapsuleComponent->GetComponentQuat(), true, &Hit,
	                                        ETeleportType::None);

	if (Hit.IsValidBlockingHit())
	{
		UE_LOG(
		    Log_Climb_Ladder, Warning,
		    TEXT(
		        "[ClimbComponent] Repeated ladder movement was blocked by '%s'. The ladder session will be cancelled."),
		    *GetNameSafe(Hit.GetActor()));
		return false;
	}

	return true;
}

bool UClimbComponent::RequestEnterLadder(AActor* TargetLadder)
{
	if (IsValid(ClimbObject) || LadderActionState != ELadderActionState::Detached)
	{
		return false;
	}

	if (!IsValid(LadderClimbProfile))
	{
		UE_LOG(Log_Climb_Ladder, Error, TEXT("[ClimbComponent] LadderClimbProfile is not configured on '%s'."),
		       *GetNameSafe(GetOwner()));
		return false;
	}

	ALadderBase* Ladder = Cast<ALadderBase>(TargetLadder);
	if (!IsValid(Ladder) || !Ladder->GetClass()->ImplementsInterface(UInteractInterface::StaticClass()))
	{
		UE_LOG(Log_Climb_Ladder, Error, TEXT("[ClimbComponent] Invalid ladder target: %s"), *GetNameSafe(TargetLadder));
		return false;
	}

	if (!RegisterClimbObject(Ladder))
	{
		UE_LOG(Log_Climb_Ladder, Error, TEXT("[ClimbComponent] Ladder '%s' has invalid Grip data."),
		       *GetNameSafe(TargetLadder));
		return false;
	}

	USceneComponent* ClimbPoint = IInteractInterface::Execute_GetEnterInteractLocation(Ladder, GetOwner());
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!IsValid(ClimbPoint) || !IsValid(Character))
	{
		UE_LOG(Log_Climb_Ladder, Error, TEXT("[ClimbComponent] Ladder '%s' is missing a required entry component."),
		       *GetNameSafe(TargetLadder));
		DeRegisterClimbObject();
		return false;
	}

	const bool bEnterFromBottom = ClimbPoint->ComponentHasTag("Bottom");
	TMap<ELimbList, int32> FinalIdleGripAssignment;
	if (bEnterFromBottom)
	{
		if (!ResolveGripPattern(LadderClimbProfile->BottomEnterIdleGripHeightOffsets,
		                        LadderClimbProfile->BottomEnterIdleReferenceLimb, false, FinalIdleGripAssignment))
		{
			UE_LOG(Log_Climb_Ladder, Error,
			       TEXT("[ClimbComponent] Ladder '%s' has no valid bottom-entry grip assignment."),
			       *GetNameSafe(TargetLadder));
			DeRegisterClimbObject();
			return false;
		}

		const int32 FootRGripIndex = FinalIdleGripAssignment[ELimbList::FootR];
		const int32 FootLGripIndex = FinalIdleGripAssignment[ELimbList::FootL];
		const int32 HandLGripIndex = FinalIdleGripAssignment[ELimbList::HandL];
		const int32 HandRGripIndex = FinalIdleGripAssignment[ELimbList::HandR];

		LimbToGripNode.Add(ELimbList::FootR,
		                   FLimbData(FootRGripIndex, SetBoneIKTargetLadder(FootRGripIndex, FVector(), -15.0f)));
		LimbToGripNode.Add(ELimbList::FootL,
		                   FLimbData(FootLGripIndex, SetBoneIKTargetLadder(FootLGripIndex, FVector(), 15.0f)));
		LimbToGripNode.Add(ELimbList::HandL,
		                   FLimbData(HandLGripIndex, SetBoneIKTargetLadder(HandLGripIndex, FVector(), 15.0f)));
		LimbToGripNode.Add(ELimbList::HandR,
		                   FLimbData(HandRGripIndex, SetBoneIKTargetLadder(HandRGripIndex, FVector(), -15.0f)));
		LadderStance = EClimbPhase::Enter_From_Bottom;
	}
	else
	{
		TMap<ELimbList, int32> AuthoredInitialGripAssignment;
		if (!ResolveGripPattern(LadderClimbProfile->TopEnterInitialGripHeightOffsets,
		                        LadderClimbProfile->TopEnterInitialReferenceLimb, true,
		                        AuthoredInitialGripAssignment) ||
		    !ResolveGripPattern(LadderClimbProfile->TopEnterIdleGripHeightOffsets,
		                        LadderClimbProfile->TopEnterIdleReferenceLimb, true, FinalIdleGripAssignment) ||
		    !BuildGripRoute(GetClimbMontage(EClimbPhase::Enter_From_Top), AuthoredInitialGripAssignment,
		                    FinalIdleGripAssignment))
		{
			UE_LOG(Log_Climb_Ladder, Error, TEXT("[ClimbComponent] Ladder '%s' has invalid top-entry data."),
			       *GetNameSafe(TargetLadder));
			DeRegisterClimbObject();
			return false;
		}

		LimbToGripNode.Add(ELimbList::HandL,
		                   FLimbData(AuthoredInitialGripAssignment[ELimbList::HandL],
		                             SetBoneIKTargetLadder(AuthoredInitialGripAssignment[ELimbList::HandL], FVector(),
		                                                   15.0f)));
		LimbToGripNode.Add(ELimbList::HandR,
		                   FLimbData(AuthoredInitialGripAssignment[ELimbList::HandR],
		                             SetBoneIKTargetLadder(AuthoredInitialGripAssignment[ELimbList::HandR], FVector(),
		                                                   -15.0f)));
		LimbToGripNode.Add(ELimbList::FootL,
		                   FLimbData(AuthoredInitialGripAssignment[ELimbList::FootL],
		                             SetBoneIKTargetLadder(AuthoredInitialGripAssignment[ELimbList::FootL], FVector(),
		                                                   15.0f)));
		LimbToGripNode.Add(ELimbList::FootR,
		                   FLimbData(AuthoredInitialGripAssignment[ELimbList::FootR],
		                             SetBoneIKTargetLadder(AuthoredInitialGripAssignment[ELimbList::FootR], FVector(),
		                                                   -15.0f)));

		LadderStance = EClimbPhase::Enter_From_Top;
	}

	FVector BodyTargetLocation = FVector::ZeroVector;
	if (!TryCalculateBodyTargetLocation(FinalIdleGripAssignment, BodyTargetLocation))
	{
		DeRegisterClimbObject();
		return false;
	}

	TransitionTargetLocation = BodyTargetLocation;
	if (!BeginLadderTransition(ELadderActionState::Entering))
	{
		UE_LOG(Log_Climb_Ladder, Warning, TEXT("[ClimbComponent] A ladder transition is already active."));
		DeRegisterClimbObject();
		return false;
	}

	Character->GetCapsuleComponent()->IgnoreActorWhenMoving(TargetLadder, true);
	Character->GetCharacterMovement()->SetMovementMode(MOVE_Flying);

	const EClimbPhase EnterPhase = bEnterFromBottom ? EClimbPhase::Enter_From_Bottom : EClimbPhase::Enter_From_Top;
	if (!bEnterFromBottom)
	{
		// ClimbTopLocation normally performs this rotation during interaction
		// movement. Enforce the same authored start facing immediately before
		// the montage so a stale Blueprint/component rotation cannot make
		// motion warping turn the character through the entry animation.
		Character->SetActorRotation(CalculateLadderAlignmentRotation());
	}
	if (!PlayEnterMontage(EnterPhase))
	{
		UE_LOG(Log_Climb_Ladder, Error,
		       TEXT("[ClimbComponent] Ladder entry was cancelled because the %s montage could not start."),
		       bEnterFromBottom ? TEXT("bottom") : TEXT("top"));
		ForceDetachFromLadder(false);
		return false;
	}

	SetComponentTickEnabled(false);
	return true;
}

bool UClimbComponent::RequestExitLadder(bool bExitTop)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!IsValid(Character) || !IsValid(ClimbObject) || LadderActionState != ELadderActionState::Idle)
	{
		return false;
	}
	const EClimbPhase IdlePhase = ResolveIdlePhaseFromGripState();

	if (bExitTop)
	{
		const USceneComponent* ExitPoint = ClimbObject->GetTopExitTarget();
		if (!IsValid(ExitPoint))
		{
			UE_LOG(Log_Climb_Ladder, Error, TEXT("[ClimbComponent] Ladder '%s' has no top-exit target."),
			       *GetNameSafe(ClimbObject));
			return false;
		}

		FVector ExitLocation = ExitPoint->GetComponentLocation();
		ExitLocation.Z += Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

		TransitionTargetLocation = ExitLocation;
		LadderStance = IdlePhase == EClimbPhase::Idle_Left ? EClimbPhase::Exit_From_Top_Left
		                                                   : EClimbPhase::Exit_From_Top_Right;

		if (!BuildTopExitGripRoute(LadderStance))
		{
			UE_LOG(Log_Climb_Ladder, Error,
			       TEXT("[ClimbComponent] Ladder '%s' has no valid top-exit grip route for '%s'."),
			       *GetNameSafe(ClimbObject), *UEnum::GetValueAsString(LadderStance));
			TransitionRuntime.PlannedGripTargets.Empty();
			TransitionRuntime.PlannedRouteEndAssignment.Empty();
			LadderStance = IdlePhase;
			return false;
		}
	}
	else
	{
		FVector StartLoc = GetOwner()->GetActorLocation();
		FVector EndLoc = StartLoc;
		EndLoc.Z -= Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 2.0f;

		FHitResult HitResult;
		FCollisionQueryParams CollisionParams;
		CollisionParams.AddIgnoredActor(GetOwner());
		CollisionParams.AddIgnoredActor(ClimbObject);
		float Radius = Character->GetCapsuleComponent()->GetScaledCapsuleRadius();
		float HalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

		FCollisionShape DetectShape = FCollisionShape::MakeCapsule(Radius, HalfHeight);

		bool bHit = GetWorld()->SweepSingleByChannel(HitResult, StartLoc, EndLoc, FQuat::Identity,
		                                             ECC_GameTraceChannel8, DetectShape, CollisionParams);

		if (!bHit)
		{
			UE_LOG(Log_Climb_Ladder, Warning, TEXT("[ClimbComponent] Failed to find a valid ladder exit location."));
			return false;
		}

		TransitionTargetLocation = HitResult.Location;

		LadderStance = IdlePhase == EClimbPhase::Idle_Left ? EClimbPhase::Exit_From_Bottom_Left
		                                                   : EClimbPhase::Exit_From_Bottom_Right;
		TransitionRuntime.PlannedGripTargets.Empty();
		TransitionRuntime.PlannedRouteEndAssignment.Empty();
	}
	if (!BeginLadderTransition(ELadderActionState::Exiting))
	{
		LadderStance = IdlePhase;
		TransitionRuntime.PlannedGripTargets.Empty();
		TransitionRuntime.PlannedRouteEndAssignment.Empty();
		return false;
	}

	if (!PlayExitMontage(LadderStance))
	{
		UE_LOG(Log_Climb_Ladder, Error,
		       TEXT("[ClimbComponent] Ladder exit was cancelled because montage '%s' could not start."),
		       *UEnum::GetValueAsString(LadderStance));
		LadderActionState = ELadderActionState::Idle;
		LadderStance = IdlePhase;
		TransitionRuntime.PlannedGripTargets.Empty();
		TransitionRuntime.PlannedRouteEndAssignment.Empty();
		ClearTransitionWarpTargets();
		return false;
	}
	SetComponentTickEnabled(false);

	return true;
}

bool UClimbComponent::BeginLadderTransition(ELadderActionState NewState)
{
	if ((NewState != ELadderActionState::Entering && NewState != ELadderActionState::Exiting) ||
	    (NewState == ELadderActionState::Entering ? LadderActionState != ELadderActionState::Detached
	                                              : LadderActionState != ELadderActionState::Idle))
	{
		return false;
	}

	if (NewState == ELadderActionState::Entering)
	{
		CaptureCharacterState();
	}

	LadderActionState = NewState;
	return true;
}

void UClimbComponent::CompleteExitTransition()
{
	if (LadderActionState == ELadderActionState::Exiting)
	{
		const bool bShouldBroadcastExit = !TransitionRuntime.bVisualStateReleased;
		RestoreCharacterState();
		ClearLadderSession();
		if (bShouldBroadcastExit)
		{
			OnLadderExit.Broadcast();
		}
	}
}

void UClimbComponent::CaptureCharacterState()
{
	if (bHasCharacterStateSnapshot)
	{
		return;
	}

	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement();
		SavedMovementMode = static_cast<uint8>(MovementComponent->MovementMode);
		SavedCustomMovementMode = MovementComponent->CustomMovementMode;
		bHasCharacterStateSnapshot = true;
	}
}

void UClimbComponent::RestoreCharacterState()
{
	RestoreTopTransitionCollision();

	if (!bHasCharacterStateSnapshot)
	{
		return;
	}

	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		Character->GetCapsuleComponent()->IgnoreActorWhenMoving(ClimbObject, false);

		UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement();
		MovementComponent->SetMovementMode(static_cast<EMovementMode>(SavedMovementMode), SavedCustomMovementMode);
	}

	bHasCharacterStateSnapshot = false;
}

bool UClimbComponent::BeginTopTransitionCollision()
{
	if (bTopTransitionCollisionActive)
	{
		return true;
	}

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	UCapsuleComponent* Capsule = Character ? Character->GetCapsuleComponent() : nullptr;
	if (!IsValid(Capsule))
	{
		return false;
	}

	SavedCapsuleCollisionEnabled = Capsule->GetCollisionEnabled();
	Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	bTopTransitionCollisionActive = true;
	return true;
}

void UClimbComponent::RestoreTopTransitionCollision()
{
	if (!bTopTransitionCollisionActive)
	{
		return;
	}

	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
		{
			Capsule->SetCollisionEnabled(SavedCapsuleCollisionEnabled);
		}
	}

	bTopTransitionCollisionActive = false;
	SavedCapsuleCollisionEnabled = ECollisionEnabled::QueryAndPhysics;
}

void UClimbComponent::ClearLadderSession()
{
	RestoreTopTransitionCollision();

	if (ClimbObject)
	{
		ClimbObject->OnDestroyed.RemoveDynamic(this, &UClimbComponent::HandleClimbObjectDestroyed);
	}

	ClearTransitionWarpTargets();
	LimbToGripNode.Empty();
	ActiveSurfaceHandContacts.Empty();
	GripList1D.Empty();
	ClimbObject = nullptr;
	TransitionTargetLocation = FVector::ZeroVector;
	TransitionRuntime.Reset();
	RepeatedStepRuntime.Reset();
	LadderStance = EClimbPhase::Idle_Right;
	LadderActionState = ELadderActionState::Detached;
	SetComponentTickEnabled(false);
}

void UClimbComponent::ResetLadderIKState(bool bRestoreGroundPhase)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	UAnimInstance* AnimInstance = Character && Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!IsValid(AnimInstance) || !AnimInstance->GetClass()->ImplementsInterface(UIAnimInstance::StaticClass()))
	{
		return;
	}

	IIAnimInstance::Execute_SetIKPhaseAlpha(AnimInstance, TAG_IK_Phase_Ladder, 0.0f);

	if (bRestoreGroundPhase)
	{
		IIAnimInstance::Execute_SetIKPhaseAlpha(AnimInstance, TAG_IK_Phase_Ground, 1.0f);
	}

	static constexpr ELimbList LadderLimbs[] = {ELimbList::HandL, ELimbList::HandR, ELimbList::FootL, ELimbList::FootR};

	for (const ELimbList Limb : LadderLimbs)
	{
		IIAnimInstance::Execute_SetIKLayerAlpha(AnimInstance, TAG_IK_Layer_Ladder_Climb, Limb, 0.0f);
	}
}

void UClimbComponent::StopActiveTransitionMontage()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	UAnimInstance* AnimInstance = Character && Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!IsValid(AnimInstance))
	{
		return;
	}

	UAnimMontage* TransitionMontage = nullptr;
	FOnMontageEnded EmptyEndedDelegate;
	FOnMontageBlendingOutStarted EmptyBlendingOutDelegate;
	if (LadderActionState == ELadderActionState::Entering)
	{
		const EClimbPhase EnterPhase = LadderStance == EClimbPhase::Enter_From_Top ? EClimbPhase::Enter_From_Top
		                                                                           : EClimbPhase::Enter_From_Bottom;
		TransitionMontage = GetClimbMontage(EnterPhase);
		if (IsValid(TransitionMontage))
		{
			AnimInstance->Montage_SetEndDelegate(EmptyEndedDelegate, TransitionMontage);
		}
	}
	else if (LadderActionState == ELadderActionState::Exiting)
	{
		TransitionMontage = GetClimbMontage(LadderStance);
		if (IsValid(TransitionMontage))
		{
			AnimInstance->Montage_SetEndDelegate(EmptyEndedDelegate, TransitionMontage);
			AnimInstance->Montage_SetBlendingOutDelegate(EmptyBlendingOutDelegate, TransitionMontage);
		}
	}

	if (IsValid(TransitionMontage) && AnimInstance->Montage_IsPlaying(TransitionMontage))
	{
		AnimInstance->Montage_Stop(0.1f, TransitionMontage);
	}
}

void UClimbComponent::ForceDetachFromLadder(bool bBroadcastExit)
{
	const bool bShouldBroadcastExit = bBroadcastExit && !TransitionRuntime.bVisualStateReleased;
	bool bRestoreGroundPhase = true;
	if (const ACharacterBase* Character = Cast<ACharacterBase>(GetOwner()))
	{
		if (const UCharacterStatusComponent* StatusComponent = Character->GetCharacterStatusComponent())
		{
			bRestoreGroundPhase = !StatusComponent->GetCurrentState().MatchesTagExact(TAG_State_Dead);
		}
	}

	StopActiveTransitionMontage();
	ResetLadderIKState(bRestoreGroundPhase);

	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		Character->GetCharacterMovement()->StopMovementImmediately();
	}

	RestoreCharacterState();
	ClearLadderSession();

	if (bShouldBroadcastExit)
	{
		OnLadderExit.Broadcast();
	}
}

bool UClimbComponent::BeginLimbGripTransition(ELimbList Limb, ELadderGripDirection Direction,
                                              UCurveVector* TrajectoryCurve, UObject* TransitionSource)
{
	if ((LadderActionState != ELadderActionState::Entering && LadderActionState != ELadderActionState::Exiting) ||
	    !IsValid(TransitionSource) || Limb == ELimbList::Body || TransitionRuntime.ActiveGripTransitions.Contains(Limb))
	{
		return false;
	}

	FLimbData* LimbData = LimbToGripNode.Find(Limb);
	if (!LimbData || !GetGripNode(LimbData->LimbTargetGripIndex))
	{
		return false;
	}

	TArray<int32>* PlannedTargets = TransitionRuntime.PlannedGripTargets.Find(Limb);
	if (!PlannedTargets || PlannedTargets->IsEmpty())
	{
		UE_LOG(Log_Climb_Ladder, Error, TEXT("[ClimbComponent] No planned grip target remains for %s."),
		       *UEnum::GetValueAsString(Limb));
		return false;
	}

	const int32 StartGripIndex = LimbData->LimbTargetGripIndex;
	const int32 TargetGripIndex = (*PlannedTargets)[0];
	const FGripNode1D* StartGrip = GetGripNode(StartGripIndex);
	const FGripNode1D* TargetGrip = GetGripNode(TargetGripIndex);
	const float DirectionSign = Direction == ELadderGripDirection::Up ? 1.0f : -1.0f;
	if (!StartGrip || !TargetGrip ||
		(TargetGrip->ClimbCoordinate - StartGrip->ClimbCoordinate) * DirectionSign <= 0.0f)
	{
		UE_LOG(Log_Climb_Ladder, Error, TEXT("[ClimbComponent] Planned target for %s does not move %s."),
		       *UEnum::GetValueAsString(Limb), Direction == ELadderGripDirection::Up ? TEXT("up") : TEXT("down"));
		return false;
	}
	PlannedTargets->RemoveAt(0);

	FLimbGripTransitionState& Transition = TransitionRuntime.ActiveGripTransitions.Add(Limb);
	Transition.StartGripIndex = StartGripIndex;
	Transition.TargetGripIndex = TargetGripIndex;
	Transition.TrajectoryCurve = TrajectoryCurve;
	Transition.Source = TransitionSource;

	LimbData->LimbTargetGripIndex = TargetGripIndex;
	return true;
}

void UClimbComponent::UpdateLimbGripTransition(ELimbList Limb, float NormalizedTime, const UObject* TransitionSource)
{
	FLimbGripTransitionState* Transition = TransitionRuntime.ActiveGripTransitions.Find(Limb);
	FLimbData* LimbData = LimbToGripNode.Find(Limb);
	if (!Transition || !LimbData || Transition->Source.Get() != TransitionSource)
	{
		return;
	}

	const float ClampedTime = FMath::Clamp(NormalizedTime, 0.0f, 1.0f);
	FVector CurveValue(0.0f, 0.0f, ClampedTime);
	if (UCurveVector* Curve = Transition->TrajectoryCurve.Get())
	{
		float MinTime = 0.0f;
		float MaxTime = 1.0f;
		Curve->FloatCurves[2].GetTimeRange(MinTime, MaxTime);
		CurveValue = Curve->GetVectorValue(FMath::Lerp(MinTime, MaxTime, ClampedTime));
	}

	const float LimbSideOffset = Limb == ELimbList::HandL || Limb == ELimbList::FootL ? 15.0f : -15.0f;
	LimbData->LimbLocation = SetBoneIKTargetLadder(Transition->TargetGripIndex, CurveValue, LimbSideOffset,
	                                               Transition->StartGripIndex);
}

void UClimbComponent::CompleteLimbGripTransition(ELimbList Limb, const UObject* TransitionSource)
{
	FLimbData* LimbData = LimbToGripNode.Find(Limb);
	const FLimbGripTransitionState* Transition = TransitionRuntime.ActiveGripTransitions.Find(Limb);
	if (!Transition || !LimbData || Transition->Source.Get() != TransitionSource)
	{
		return;
	}

	UpdateLimbGripTransition(Limb, 1.0f, TransitionSource);
	const float LimbSideOffset = Limb == ELimbList::HandL || Limb == ELimbList::FootL ? 15.0f : -15.0f;
	LimbData->LimbLocation = SetBoneIKTargetLadder(Transition->TargetGripIndex, FVector::ZeroVector, LimbSideOffset);
	TransitionRuntime.ActiveGripTransitions.Remove(Limb);
}

bool UClimbComponent::BeginTopSurfaceHandContact(ELimbList Limb, float TraceUpDistance, float TraceDownDistance,
	                                              float SurfaceOffset, ECollisionChannel TraceChannel,
	                                              UObject* ContactSource, bool bDrawDebug)
{
	const bool bTopTransition = LadderStance == EClimbPhase::Enter_From_Top || IsLadderTopExitPhase(LadderStance);
	if (!bTopTransition || (LadderActionState != ELadderActionState::Entering &&
	                      LadderActionState != ELadderActionState::Exiting) ||
	    (Limb != ELimbList::HandL && Limb != ELimbList::HandR) || !IsValid(ContactSource) || !GetWorld())
	{
		return false;
	}

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
	const FName PalmSocket = Limb == ELimbList::HandL ? TEXT("Palm_L") : TEXT("Palm_R");
	if (!IsValid(Mesh) || !Mesh->DoesSocketExist(PalmSocket))
	{
		return false;
	}

	const FVector PalmLocation = Mesh->GetSocketLocation(PalmSocket);
	const FVector TraceStart = PalmLocation + FVector::UpVector * FMath::Max(TraceUpDistance, 0.0f);
	const FVector TraceEnd = PalmLocation - FVector::UpVector * FMath::Max(TraceDownDistance, 0.0f);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LadderSurfaceHandContact), false, GetOwner());
	QueryParams.AddIgnoredActor(ClimbObject);

	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, TraceChannel, QueryParams);
	if (bDrawDebug)
	{
		DrawDebugLine(GetWorld(), TraceStart, TraceEnd, bHit ? FColor::Green : FColor::Red, false, 5.0f, 0, 2.0f);
		if (bHit)
		{
			DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 7.0f, 12, FColor::Yellow, false, 5.0f, 0, 2.0f);
			DrawDebugDirectionalArrow(GetWorld(), Hit.ImpactPoint, Hit.ImpactPoint + Hit.ImpactNormal * 25.0f,
			                          8.0f, FColor::Cyan, false, 5.0f, 0, 2.0f);
		}
	}
	if (!bHit)
	{
		UE_LOG(Log_Climb_Ladder, Warning, TEXT("[ClimbComponent] No platform surface was found for %s."),
		       *UEnum::GetValueAsString(Limb));
		return false;
	}

	FSurfaceHandContactState& Contact = ActiveSurfaceHandContacts.FindOrAdd(Limb);
	Contact.Location = Hit.ImpactPoint + Hit.ImpactNormal * SurfaceOffset;
	Contact.SurfaceNormal = Hit.ImpactNormal;
	Contact.Source = ContactSource;
	return true;
}

void UClimbComponent::EndTopSurfaceHandContact(ELimbList Limb, const UObject* ContactSource)
{
	const FSurfaceHandContactState* Contact = ActiveSurfaceHandContacts.Find(Limb);
	if (Contact && Contact->Source.Get() == ContactSource)
	{
		ActiveSurfaceHandContacts.Remove(Limb);
	}
}

void UClimbComponent::HandleOwnerDeathStarted()
{
	ForceDetachFromLadder(false);
}

void UClimbComponent::HandleClimbObjectDestroyed(AActor* DestroyedActor)
{
	if (DestroyedActor == ClimbObject)
	{
		ForceDetachFromLadder(true);
	}
}

bool UClimbComponent::PlayEnterMontage(EClimbPhase EnterPhase)
{
	if (!IsLadderEnterPhase(EnterPhase))
	{
		return false;
	}

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	UAnimInstance* AnimInstance = Character && Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	UAnimMontage* Montage = GetClimbMontage(EnterPhase);
	const TCHAR* EntryLabel = EnterPhase == EClimbPhase::Enter_From_Bottom ? TEXT("bottom") : TEXT("top");

	if (!IsValid(AnimInstance))
	{
		UE_LOG(Log_Climb_Ladder, Error,
		       TEXT("[ClimbComponent] Cannot play %s-enter montage: AnimInstance is invalid. Character=%s Mesh=%s"),
		       EntryLabel, *GetNameSafe(Character), *GetNameSafe(Character ? Character->GetMesh() : nullptr));
		return false;
	}

	if (!IsValid(Montage))
	{
		UE_LOG(Log_Climb_Ladder, Error,
		       TEXT("[ClimbComponent] Cannot play %s-enter montage: phase '%s' is not configured in DataAsset '%s'."),
		       EntryLabel, *UEnum::GetValueAsString(EnterPhase), *GetNameSafe(LadderClimbProfile));
		return false;
	}

	if (EnterPhase == EClimbPhase::Enter_From_Top && !BeginTopTransitionCollision())
	{
		UE_LOG(Log_Climb_Ladder, Error, TEXT("[ClimbComponent] Failed to disable top-entry capsule collision."));
		return false;
	}
	if (!UpdateTransitionWarpTargets(EnterPhase))
	{
		RestoreTopTransitionCollision();
		UE_LOG(Log_Climb_Ladder, Error, TEXT("[ClimbComponent] %s-enter warp target could not be configured."),
		       EntryLabel);
		return false;
	}
	AnimInstance->SetRootMotionMode(ERootMotionMode::RootMotionFromMontagesOnly);
	const float MontageDuration = AnimInstance->Montage_Play(Montage);
	if (MontageDuration <= 0.0f)
	{
		RestoreTopTransitionCollision();
		ClearTransitionWarpTargets();
		UE_LOG(Log_Climb_Ladder, Error, TEXT("[ClimbComponent] Failed to play %s-enter montage '%s'."), EntryLabel,
		       *GetNameSafe(Montage));
		return false;
	}

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &UClimbComponent::OnEnterClimbMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, Montage);
	return true;
}

bool UClimbComponent::PlayExitMontage(EClimbPhase ExitPhase)
{
	if (!IsLadderExitPhase(ExitPhase))
	{
		return false;
	}

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	UAnimInstance* AnimInstance = Character && Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	UAnimMontage* Montage = GetClimbMontage(ExitPhase);
	if (!IsValid(AnimInstance) || !IsValid(Montage))
	{
		UE_LOG(Log_Climb_Ladder, Error, TEXT("[ClimbComponent] Exit montage for '%s' is not configured in '%s'."),
		       *UEnum::GetValueAsString(ExitPhase), *GetNameSafe(LadderClimbProfile));
		return false;
	}

	if (IsLadderTopExitPhase(ExitPhase) && !BeginTopTransitionCollision())
	{
		UE_LOG(Log_Climb_Ladder, Error, TEXT("[ClimbComponent] Failed to disable top-exit capsule collision."));
		return false;
	}
	if (!UpdateTransitionWarpTargets(ExitPhase))
	{
		RestoreTopTransitionCollision();
		return false;
	}

	AnimInstance->SetRootMotionMode(ERootMotionMode::RootMotionFromMontagesOnly);
	const float MontageDuration = AnimInstance->Montage_Play(Montage);
	if (MontageDuration <= 0.0f)
	{
		RestoreTopTransitionCollision();
		ClearTransitionWarpTargets();
		return false;
	}

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &UClimbComponent::OnExitClimbMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, Montage);
	FOnMontageBlendingOutStarted BlendingOutDelegate;
	BlendingOutDelegate.BindUObject(this, &UClimbComponent::OnExitClimbMontageBlendingOut);
	AnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, Montage);
	return true;
}

bool UClimbComponent::UpdateTransitionWarpTargets(EClimbPhase Phase)
{
	ACharacterBase* Character = Cast<ACharacterBase>(GetOwner());
	UMotionWarpingComponent* MotionWarping = Character ? Character->GetMotionWarpingComponent() : nullptr;
	if (!IsValid(Character) || !IsValid(MotionWarping) || !IsValid(ClimbObject) || !IsValid(LadderClimbProfile) ||
		(!IsLadderEnterPhase(Phase) && !IsLadderExitPhase(Phase)))
	{
		return false;
	}

	const bool bTopExit = IsLadderTopExitPhase(Phase);
	const USceneComponent* TopExitPoint = bTopExit ? ClimbObject->GetTopExitTarget() : nullptr;
	if (bTopExit && !IsValid(TopExitPoint))
	{
		return false;
	}

	const FRotator TransitionRotation = IsLadderEnterPhase(Phase)
		? CalculateLadderAlignmentRotation()
		: (bTopExit ? TopExitPoint->GetComponentRotation() : Character->GetActorRotation());
	FTransform TransitionTransform(TransitionRotation, TransitionTargetLocation);
	const float CapsuleHalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	TransitionTransform.SetLocation(
		TransitionTransform.GetLocation() - Character->GetActorUpVector() * CapsuleHalfHeight);

	const FVector LadderForward = ClimbObject->GetLadderForwardVector().GetSafeNormal();
	const FVector LadderRight = ClimbObject->GetLadderRightVector().GetSafeNormal();
	const FVector LadderUp = ClimbObject->GetLadderUpVector().GetSafeNormal();
	const FVector LadderTopEndpoint = ClimbObject->GetLadderTopEndpointWorld();
	const FLadderPhaseWarpTargets* PhaseWarpTargets = LadderClimbProfile->WarpTargetsByPhase.Find(Phase);
	if (!PhaseWarpTargets || PhaseWarpTargets->Targets.IsEmpty())
	{
		UE_LOG(Log_Climb_Ladder, Error, TEXT("[ClimbComponent] Phase '%s' has no warp target definitions."),
		       *UEnum::GetValueAsString(Phase));
		return false;
	}

	bool bConfiguredTransitionTarget = false;
	TSet<FName> ConfiguredNames;

#if ENABLE_DRAW_DEBUG
	if (bDrawTransitionWarpDebug && GetWorld())
	{
		const float Duration = TransitionWarpDebugDuration;
		const FVector TransitionAnchor = TransitionTransform.GetLocation();
		DrawDebugSphere(GetWorld(), TransitionAnchor, 10.0f, 16, FColor::Cyan, false, Duration, 0, 2.0f);
		DrawDebugString(GetWorld(), TransitionAnchor + LadderUp * 15.0f, TEXT("TransitionTarget Anchor"), nullptr,
		                FColor::Cyan, Duration, false);
		DrawDebugSphere(GetWorld(), LadderTopEndpoint, 10.0f, 16, FColor::Magenta, false, Duration, 0, 2.0f);
		DrawDebugString(GetWorld(), LadderTopEndpoint + LadderUp * 15.0f, TEXT("LadderTopEndpoint Anchor"), nullptr,
		                FColor::Magenta, Duration, false);
	}
#endif

	for (const FLadderWarpTargetDefinition& Definition : PhaseWarpTargets->Targets)
	{
		if (Definition.TargetName.IsNone())
		{
			continue;
		}

		if (ConfiguredNames.Contains(Definition.TargetName))
		{
			UE_LOG(Log_Climb_Ladder, Error, TEXT("[ClimbComponent] Duplicate warp target '%s' for phase '%s'."),
			       *Definition.TargetName.ToString(), *UEnum::GetValueAsString(Phase));
			return false;
		}

		FTransform TargetTransform = TransitionTransform;
		if (Definition.Anchor == ELadderWarpTargetAnchor::LadderTopEndpoint)
		{
			TargetTransform.SetLocation(LadderTopEndpoint);
		}
		else
		{
			bConfiguredTransitionTarget = true;
		}

		TargetTransform.SetLocation(TargetTransform.GetLocation() + LadderForward * Definition.Offset.X +
		                            LadderRight * Definition.Offset.Y + LadderUp * Definition.Offset.Z);
		TargetTransform.SetRotation(TargetTransform.GetRotation() * Definition.RotationOffset.Quaternion());
		MotionWarping->AddOrUpdateWarpTargetFromTransform(Definition.TargetName, TargetTransform);
		ConfiguredNames.Add(Definition.TargetName);

#if ENABLE_DRAW_DEBUG
		if (bDrawTransitionWarpDebug && GetWorld())
		{
			const FColor TargetColor = Definition.Anchor == ELadderWarpTargetAnchor::TransitionTarget
				                           ? FColor::Green
				                           : FColor::Orange;
			const FVector TargetLocation = TargetTransform.GetLocation();
			DrawDebugSphere(GetWorld(), TargetLocation, 7.0f, 12, TargetColor, false, TransitionWarpDebugDuration, 0,
			                2.0f);
			DrawDebugCoordinateSystem(GetWorld(), TargetLocation, TargetTransform.Rotator(), 25.0f, false,
			                          TransitionWarpDebugDuration, 0, 1.5f);
			DrawDebugString(GetWorld(), TargetLocation + LadderUp * 10.0f, Definition.TargetName.ToString(), nullptr,
			                TargetColor, TransitionWarpDebugDuration, false);
		}
#endif
	}

	if (ConfiguredNames.IsEmpty() || !bConfiguredTransitionTarget)
	{
		UE_LOG(Log_Climb_Ladder, Error,
		       TEXT("[ClimbComponent] Phase '%s' requires at least one TransitionTarget warp definition."),
		       *UEnum::GetValueAsString(Phase));
		return false;
	}
	return true;
}

void UClimbComponent::ClearTransitionWarpTargets()
{
	if (ACharacterBase* Character = Cast<ACharacterBase>(GetOwner()))
	{
		if (UMotionWarpingComponent* MotionWarping = Character->GetMotionWarpingComponent())
		{
			if (IsValid(LadderClimbProfile))
			{
				for (const TPair<EClimbPhase, FLadderPhaseWarpTargets>& PhaseTargets :
				     LadderClimbProfile->WarpTargetsByPhase)
				{
					for (const FLadderWarpTargetDefinition& Definition : PhaseTargets.Value.Targets)
					{
						if (!Definition.TargetName.IsNone())
						{
							MotionWarping->RemoveWarpTarget(Definition.TargetName);
						}
					}
				}
			}
		}
	}
}

void UClimbComponent::DrawBottomEnterContactDebug() const
{
#if ENABLE_DRAW_DEBUG
	if (!bDrawBottomEnterContactDebug || !GetWorld())
	{
		return;
	}

	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
	if (!IsValid(Character) || !IsValid(Mesh))
	{
		return;
	}

	const float Duration = BottomEnterContactDebugDuration;
	const FVector ActorLocation = Character->GetActorLocation();
	DrawDebugSphere(GetWorld(), ActorLocation, 8.0f, 12, FColor::White, false, Duration, 0, 1.5f);
	DrawDebugString(GetWorld(), ActorLocation + FVector(0.0f, 0.0f, 12.0f), TEXT("Capsule Center"), nullptr,
	                FColor::White, Duration, false);

	if (IsValid(ClimbObject))
	{
		const FTransform LadderTransform = ClimbObject->GetActorTransform();
		DrawDebugCoordinateSystem(GetWorld(), LadderTransform.GetLocation(), LadderTransform.Rotator(), 35.0f, false,
		                          Duration, 0, 1.5f);
	}

	struct FContactDebugEntry
	{
		ELimbList Limb;
		FName ContactSocket;
		FColor Color;
		const TCHAR* Label;
	};

	static const FContactDebugEntry Entries[] = {{ELimbList::FootR, TEXT("Sole_R"), FColor::Red, TEXT("FootR")},
	                                             {ELimbList::FootL, TEXT("Sole_L"), FColor::Green, TEXT("FootL")},
	                                             {ELimbList::HandL, TEXT("Palm_L"), FColor::Yellow, TEXT("HandL")},
	                                             {ELimbList::HandR, TEXT("Palm_R"), FColor::Blue, TEXT("HandR")}};

	for (const FContactDebugEntry& Entry : Entries)
	{
		const FLimbData* LimbData = LimbToGripNode.Find(Entry.Limb);
		if (!LimbData || !Mesh->DoesSocketExist(Entry.ContactSocket))
		{
			continue;
		}

		const FVector TargetLocation = LimbData->LimbLocation;
		const FVector ContactLocation = Mesh->GetSocketLocation(Entry.ContactSocket);
		const float ErrorDistance = FVector::Distance(ContactLocation, TargetLocation);

		DrawDebugSphere(GetWorld(), TargetLocation, 6.0f, 12, Entry.Color, false, Duration, 0, 2.0f);
		DrawDebugSphere(GetWorld(), ContactLocation, 4.0f, 10, FColor::White, false, Duration, 0, 1.5f);
		DrawDebugLine(GetWorld(), ContactLocation, TargetLocation, Entry.Color, false, Duration, 0, 2.0f);
		DrawDebugString(GetWorld(), TargetLocation + FVector(0.0f, 0.0f, 8.0f),
		                FString::Printf(TEXT("%s %.1fcm"), Entry.Label, ErrorDistance), nullptr, Entry.Color, Duration,
		                false);
	}
#endif
}

bool UClimbComponent::ResolveGripPattern(const TMap<ELimbList, float>& HeightOffsets, ELimbList ReferenceLimb,
                                         bool bPreferTop, TMap<ELimbList, int32>& OutAssignment) const
{
	OutAssignment.Empty();

	static constexpr ELimbList RequiredLimbs[] = {ELimbList::HandL, ELimbList::HandR, ELimbList::FootL,
	                                              ELimbList::FootR};

	if (!IsValid(LadderClimbProfile) || ReferenceLimb == ELimbList::Body || !HeightOffsets.Contains(ReferenceLimb))
	{
		UE_LOG(Log_Climb_Ladder, Error,
		       TEXT("[ClimbComponent] Grip pattern is missing its reference limb %s (Profile=%s, EntryCount=%d)."),
		       *UEnum::GetValueAsString(ReferenceLimb), *GetNameSafe(LadderClimbProfile), HeightOffsets.Num());
		return false;
	}

	for (const ELimbList Limb : RequiredLimbs)
	{
		if (!HeightOffsets.Contains(Limb))
		{
			UE_LOG(Log_Climb_Ladder, Error, TEXT("[ClimbComponent] Grip pattern in '%s' is missing %s."),
			       *GetNameSafe(LadderClimbProfile), *UEnum::GetValueAsString(Limb));
			return false;
		}
	}

	const float ReferenceOffset = HeightOffsets[ReferenceLimb];
	const float MatchTolerance = FMath::Max(LadderClimbProfile->GripMatchTolerance, 0.0f);
	bool bFoundAssignment = false;
	float BestScore = TNumericLimits<float>::Max();

	for (int32 ReferenceGripIndex = 0; ReferenceGripIndex < GripList1D.Num(); ++ReferenceGripIndex)
	{
		const FGripNode1D* ReferenceGrip = GetGripNode(ReferenceGripIndex);
		if (!ReferenceGrip)
		{
			continue;
		}

		TMap<ELimbList, int32> CandidateAssignment;
		float CandidateScore = 0.0f;
		bool bCandidateValid = true;

		for (const ELimbList Limb : RequiredLimbs)
		{
			const float DesiredRelativeHeight = HeightOffsets[Limb] - ReferenceOffset;
			int32 BestGripIndex = INDEX_NONE;
			float BestError = TNumericLimits<float>::Max();

			for (int32 GripIndex = 0; GripIndex < GripList1D.Num(); ++GripIndex)
			{
				const FGripNode1D* Grip = GetGripNode(GripIndex);
				if (!Grip)
				{
					continue;
				}

				const float ActualRelativeHeight = Grip->ClimbCoordinate - ReferenceGrip->ClimbCoordinate;
				const float Error = FMath::Abs(ActualRelativeHeight - DesiredRelativeHeight);
				if (Error < BestError)
				{
					BestError = Error;
					BestGripIndex = GripIndex;
				}
			}

			if (BestGripIndex == INDEX_NONE ||
			    (BestError > MatchTolerance && !FMath::IsNearlyEqual(BestError, MatchTolerance, KINDA_SMALL_NUMBER)))
			{
				bCandidateValid = false;
				break;
			}

			CandidateAssignment.Add(Limb, BestGripIndex);
			CandidateScore += BestError * BestError;
		}

		if (!bCandidateValid)
		{
			continue;
		}

		for (int32 LeftIndex = 0; LeftIndex < UE_ARRAY_COUNT(RequiredLimbs) && bCandidateValid; ++LeftIndex)
		{
			for (int32 RightIndex = LeftIndex + 1; RightIndex < UE_ARRAY_COUNT(RequiredLimbs); ++RightIndex)
			{
				const ELimbList LeftLimb = RequiredLimbs[LeftIndex];
				const ELimbList RightLimb = RequiredLimbs[RightIndex];
				const float DesiredDifference = HeightOffsets[LeftLimb] - HeightOffsets[RightLimb];
				const int32 LeftGripIndex = CandidateAssignment[LeftLimb];
				const int32 RightGripIndex = CandidateAssignment[RightLimb];

				if (LeftGripIndex == RightGripIndex && !FMath::IsNearlyZero(DesiredDifference))
				{
					bCandidateValid = false;
					break;
				}

				const float ActualDifference = GripList1D[LeftGripIndex].ClimbCoordinate -
				                               GripList1D[RightGripIndex].ClimbCoordinate;
				if (!FMath::IsNearlyZero(DesiredDifference) && ActualDifference * DesiredDifference <= 0.0f)
				{
					bCandidateValid = false;
					break;
				}
			}
		}

		if (!bCandidateValid)
		{
			continue;
		}

		float OccupiedBoundaryHeight = bPreferTop ? -TNumericLimits<float>::Max() : TNumericLimits<float>::Max();
		for (const TPair<ELimbList, int32>& Assignment : CandidateAssignment)
		{
			const float GripHeight = GripList1D[Assignment.Value].ClimbCoordinate;
			OccupiedBoundaryHeight = bPreferTop ? FMath::Max(OccupiedBoundaryHeight, GripHeight)
			                                    : FMath::Min(OccupiedBoundaryHeight, GripHeight);
		}

		const float RequiredBoundaryHeight = bPreferTop ? GripList1D.Last().ClimbCoordinate
		                                                : GripList1D[0].ClimbCoordinate;
		if (!FMath::IsNearlyEqual(OccupiedBoundaryHeight, RequiredBoundaryHeight))
		{
			continue;
		}

		if (!bFoundAssignment || CandidateScore < BestScore)
		{
			OutAssignment = MoveTemp(CandidateAssignment);
			BestScore = CandidateScore;
			bFoundAssignment = true;
		}
	}

	if (!bFoundAssignment)
	{
		FString GripHeights;
		for (const FGripNode1D& Grip : GripList1D)
		{
			GripHeights += FString::Printf(TEXT("%.1f "), Grip.ClimbCoordinate);
		}

		UE_LOG(Log_Climb_Ladder, Error,
		       TEXT("[ClimbComponent] No grip pattern matched within %.1fcm. Reference=%s Offsets=[HandL %.1f, HandR "
		            "%.1f, FootL %.1f, FootR %.1f] GripZ=[%s]"),
		       MatchTolerance, *UEnum::GetValueAsString(ReferenceLimb), HeightOffsets[ELimbList::HandL],
		       HeightOffsets[ELimbList::HandR], HeightOffsets[ELimbList::FootL], HeightOffsets[ELimbList::FootR],
		       *GripHeights);
	}

	return bFoundAssignment;
}

bool UClimbComponent::BuildGripRoute(UAnimMontage* Montage, const TMap<ELimbList, int32>& StartAssignment,
                                     const TMap<ELimbList, int32>& EndAssignment)
{
	TransitionRuntime.PlannedGripTargets.Empty();
	TransitionRuntime.PlannedRouteEndAssignment.Empty();

	if (!IsValid(Montage))
	{
		return false;
	}

	struct FGripRouteNotify
	{
		float TriggerTime = 0.0f;
		ELimbList Limb = ELimbList::Body;
		ELadderGripDirection Direction = ELadderGripDirection::Down;
	};

	TArray<FGripRouteNotify> RouteNotifies;
	for (const FAnimNotifyEvent& NotifyEvent : Montage->Notifies)
	{
		const UANS_LadderGripTransition* GripTransition = Cast<UANS_LadderGripTransition>(NotifyEvent.NotifyStateClass);
		if (!GripTransition || GripTransition->Limb == ELimbList::Body)
		{
			continue;
		}

		FGripRouteNotify& RouteNotify = RouteNotifies.AddDefaulted_GetRef();
		RouteNotify.TriggerTime = NotifyEvent.GetTriggerTime();
		RouteNotify.Limb = GripTransition->Limb;
		RouteNotify.Direction = GripTransition->Direction;
	}

	RouteNotifies.Sort([](const FGripRouteNotify& Left, const FGripRouteNotify& Right)
	                   { return Left.TriggerTime < Right.TriggerTime; });

	TMap<ELimbList, TArray<ELadderGripDirection>> DirectionsByLimb;
	for (const FGripRouteNotify& RouteNotify : RouteNotifies)
	{
		DirectionsByLimb.FindOrAdd(RouteNotify.Limb).Add(RouteNotify.Direction);
	}

	for (const TPair<ELimbList, TArray<ELadderGripDirection>>& LimbRoute : DirectionsByLimb)
	{
		const int32* InitialGripIndex = StartAssignment.Find(LimbRoute.Key);
		const int32* FinalGripIndex = EndAssignment.Find(LimbRoute.Key);
		if (!InitialGripIndex || !FinalGripIndex || !GetGripNode(*InitialGripIndex) || !GetGripNode(*FinalGripIndex) ||
		    LimbRoute.Value.IsEmpty())
		{
			return false;
		}

		const float InitialHeight = GripList1D[*InitialGripIndex].ClimbCoordinate;
		const float FinalHeight = GripList1D[*FinalGripIndex].ClimbCoordinate;
		if (FMath::IsNearlyEqual(InitialHeight, FinalHeight))
		{
			UE_LOG(
			    Log_Climb_Ladder, Error,
			    TEXT("[ClimbComponent] %s has grip transition notifies but its initial and final grips are identical."),
			    *UEnum::GetValueAsString(LimbRoute.Key));
			return false;
		}

		const ELadderGripDirection RequiredDirection = FinalHeight > InitialHeight ? ELadderGripDirection::Up
		                                                                           : ELadderGripDirection::Down;
		for (const ELadderGripDirection Direction : LimbRoute.Value)
		{
			if (Direction != RequiredDirection)
			{
				UE_LOG(Log_Climb_Ladder, Error,
				       TEXT("[ClimbComponent] Grip transition direction for %s does not lead from its authored initial "
				            "grip to its final idle grip."),
				       *UEnum::GetValueAsString(LimbRoute.Key));
				return false;
			}
		}

		TArray<int32> GripPath;
		GripPath.Add(*InitialGripIndex);
		int32 CurrentGripIndex = *InitialGripIndex;
		while (CurrentGripIndex != *FinalGripIndex && GripPath.Num() <= GripList1D.Num())
		{
			CurrentGripIndex = GetNeighborGripIndex(CurrentGripIndex, RequiredDirection == ELadderGripDirection::Up);
			if (!GetGripNode(CurrentGripIndex))
			{
				return false;
			}
			GripPath.Add(CurrentGripIndex);
		}

		if (GripPath.Last() != *FinalGripIndex)
		{
			return false;
		}

		const int32 TransitionCount = LimbRoute.Value.Num();
		const int32 TotalGripSteps = GripPath.Num() - 1;
		if (TotalGripSteps < TransitionCount)
		{
			UE_LOG(Log_Climb_Ladder, Error,
			       TEXT("[ClimbComponent] %s has %d transition notifies but only %d grip step(s) between its initial "
			            "and final targets."),
			       *UEnum::GetValueAsString(LimbRoute.Key), TransitionCount, TotalGripSteps);
			return false;
		}

		TArray<int32>& PlannedTargets = TransitionRuntime.PlannedGripTargets.FindOrAdd(LimbRoute.Key);
		int32 PreviousTargetStep = 0;
		for (int32 TransitionIndex = 0; TransitionIndex < TransitionCount; ++TransitionIndex)
		{
			const int32 RemainingTransitions = TransitionCount - TransitionIndex - 1;
			const int32 MinimumTargetStep = PreviousTargetStep + 1;
			const int32 MaximumTargetStep = TotalGripSteps - RemainingTransitions;
			const int32 EvenlyDistributedStep = FMath::RoundToInt(static_cast<float>(TotalGripSteps) *
			                                                      static_cast<float>(TransitionIndex + 1) /
			                                                      static_cast<float>(TransitionCount));
			const int32 TargetStep = FMath::Clamp(EvenlyDistributedStep, MinimumTargetStep, MaximumTargetStep);

			PlannedTargets.Add(GripPath[TargetStep]);
			PreviousTargetStep = TargetStep;
		}
	}

	for (const TPair<ELimbList, int32>& Initial : StartAssignment)
	{
		const int32* FinalGripIndex = EndAssignment.Find(Initial.Key);
		if (!FinalGripIndex)
		{
			return false;
		}

		if (!DirectionsByLimb.Contains(Initial.Key) && Initial.Value != *FinalGripIndex)
		{
			UE_LOG(
			    Log_Climb_Ladder, Error,
			    TEXT("[ClimbComponent] %s has different initial and final grips but no Ladder Grip Transition notify."),
			    *UEnum::GetValueAsString(Initial.Key));
			return false;
		}
	}

	TransitionRuntime.PlannedRouteEndAssignment = EndAssignment;
	return true;
}

bool UClimbComponent::BuildTopExitGripRoute(EClimbPhase ExitPhase)
{
	if (!IsValid(LadderClimbProfile) || !IsLadderTopExitPhase(ExitPhase))
	{
		return false;
	}

	TMap<ELimbList, int32> StartAssignment;
	static constexpr ELimbList LadderLimbs[] = {ELimbList::HandL, ELimbList::HandR, ELimbList::FootL, ELimbList::FootR};
	for (const ELimbList Limb : LadderLimbs)
	{
		const FLimbData* LimbData = LimbToGripNode.Find(Limb);
		if (!LimbData || !GetGripNode(LimbData->LimbTargetGripIndex))
		{
			return false;
		}
		StartAssignment.Add(Limb, LimbData->LimbTargetGripIndex);
	}

	TMap<ELimbList, int32> CanonicalInitialAssignment;
	if (!ResolveGripPattern(LadderClimbProfile->TopEnterInitialGripHeightOffsets,
	                        LadderClimbProfile->TopEnterInitialReferenceLimb, true, CanonicalInitialAssignment))
	{
		return false;
	}

	TMap<ELimbList, int32> CanonicalIdleAssignment;
	if (!ResolveGripPattern(LadderClimbProfile->TopEnterIdleGripHeightOffsets,
	                        LadderClimbProfile->TopEnterIdleReferenceLimb, true, CanonicalIdleAssignment) ||
	    !BuildGripRoute(GetClimbMontage(EClimbPhase::Enter_From_Top), CanonicalInitialAssignment,
	                    CanonicalIdleAssignment))
	{
		return false;
	}

	const TMap<ELimbList, TArray<int32>> CanonicalEntryTargets = TransitionRuntime.PlannedGripTargets;
	UAnimMontage* ExitMontage = GetClimbMontage(ExitPhase);
	if (!IsValid(ExitMontage))
	{
		return false;
	}

	TMap<ELimbList, int32> ExitNotifyCounts;
	for (const FAnimNotifyEvent& NotifyEvent : ExitMontage->Notifies)
	{
		const UANS_LadderGripTransition* GripTransition = Cast<UANS_LadderGripTransition>(NotifyEvent.NotifyStateClass);
		if (GripTransition && GripTransition->Limb != ELimbList::Body)
		{
			ExitNotifyCounts.FindOrAdd(GripTransition->Limb)++;
		}
	}

	const bool bMirrorRoute = ExitPhase == EClimbPhase::Exit_From_Top_Right;
	const auto GetCanonicalLimb = [bMirrorRoute](ELimbList ExitLimb)
	{
		if (!bMirrorRoute)
		{
			return ExitLimb;
		}

		switch (ExitLimb)
		{
		case ELimbList::HandL:
			return ELimbList::HandR;
		case ELimbList::HandR:
			return ELimbList::HandL;
		case ELimbList::FootL:
			return ELimbList::FootR;
		case ELimbList::FootR:
			return ELimbList::FootL;
		default:
			return ExitLimb;
		}
	};

	// An exit montage consumes only as many reversed entry segments as it has
	// grip-transition notifies. It does not force every limb all the way back
	// to the entry animation's first contact.
	TMap<ELimbList, int32> EndAssignment = StartAssignment;
	for (const TPair<ELimbList, int32>& NotifyCount : ExitNotifyCounts)
	{
		const ELimbList ExitLimb = NotifyCount.Key;
		const ELimbList CanonicalLimb = GetCanonicalLimb(ExitLimb);
		const int32* CanonicalInitialGrip = CanonicalInitialAssignment.Find(CanonicalLimb);
		const int32* CanonicalIdleGrip = CanonicalIdleAssignment.Find(CanonicalLimb);
		const TArray<int32>* EntryTargets = CanonicalEntryTargets.Find(CanonicalLimb);
		const int32* CurrentExitGrip = StartAssignment.Find(ExitLimb);
		if (!CanonicalInitialGrip || !CanonicalIdleGrip || !CurrentExitGrip || !EntryTargets)
		{
			return false;
		}

		TArray<int32> EntryPath;
		EntryPath.Reserve(EntryTargets->Num() + 1);
		EntryPath.Add(*CanonicalInitialGrip);
		EntryPath.Append(*EntryTargets);
		if (EntryPath.Last() != *CanonicalIdleGrip || *CurrentExitGrip != *CanonicalIdleGrip ||
		    NotifyCount.Value <= 0 || NotifyCount.Value >= EntryPath.Num())
		{
			UE_LOG(
			    Log_Climb_Ladder, Error,
			    TEXT("[ClimbComponent] %s cannot consume %d reversed top-entry grip segment(s) from its current grip."),
			    *UEnum::GetValueAsString(ExitLimb), NotifyCount.Value);
			return false;
		}

		const int32 ReversedTargetPathIndex = EntryPath.Num() - 1 - NotifyCount.Value;
		EndAssignment[ExitLimb] = EntryPath[ReversedTargetPathIndex];
	}

	return BuildGripRoute(ExitMontage, StartAssignment, EndAssignment);
}

bool UClimbComponent::ValidatePlannedGripRouteEnd(const TMap<ELimbList, int32>& ExpectedAssignment) const
{
	for (const TPair<ELimbList, int32>& Expected : ExpectedAssignment)
	{
		const FLimbData* LimbData = LimbToGripNode.Find(Expected.Key);
		const TArray<int32>* RemainingTargets = TransitionRuntime.PlannedGripTargets.Find(Expected.Key);
		if (!LimbData || LimbData->LimbTargetGripIndex != Expected.Value ||
		    TransitionRuntime.ActiveGripTransitions.Contains(Expected.Key) ||
		    (RemainingTargets && !RemainingTargets->IsEmpty()))
		{
			return false;
		}
	}

	return true;
}

EClimbPhase UClimbComponent::ResolveIdlePhaseFromGripState() const
{
	const FLimbData* HandLData = LimbToGripNode.Find(ELimbList::HandL);
	const FLimbData* HandRData = LimbToGripNode.Find(ELimbList::HandR);
	const FGripNode1D* HandLGrip = HandLData ? GetGripNode(HandLData->LimbTargetGripIndex) : nullptr;
	const FGripNode1D* HandRGrip = HandRData ? GetGripNode(HandRData->LimbTargetGripIndex) : nullptr;

	if (!HandLGrip || !HandRGrip)
	{
		UE_LOG(Log_Climb_Ladder, Warning,
		       TEXT("[ClimbComponent] Cannot resolve ladder idle side because a hand grip is invalid. Falling back to "
		            "Idle_Right."));
		return EClimbPhase::Idle_Right;
	}

	return HandLGrip->ClimbCoordinate > HandRGrip->ClimbCoordinate ? EClimbPhase::Idle_Left
	                                                            : EClimbPhase::Idle_Right;
}

bool UClimbComponent::ResolveRepeatedStepLimbs(EClimbPhase Phase, ELimbList& OutMovingHand,
                                               ELimbList& OutMovingFoot) const
{
	if (const FLadderRepeatedStepDefinition* Step = GetRepeatedStepDefinition(Phase))
	{
		OutMovingHand = Step->MovingHand == ELadderSide::Left ? ELimbList::HandL : ELimbList::HandR;
		OutMovingFoot = Step->MovingFoot == ELadderSide::Left ? ELimbList::FootL : ELimbList::FootR;
		return true;
	}

	return false;
}

bool UClimbComponent::StartRepeatedClimbStep(bool bUp)
{
	if (!CanStartRepeatedClimb())
	{
		return false;
	}

	FLimbData* HandLData = LimbToGripNode.Find(ELimbList::HandL);
	FLimbData* HandRData = LimbToGripNode.Find(ELimbList::HandR);
	if (!HandLData || !HandRData)
	{
		return false;
	}

	const FGripNode1D* HandLGrip = GetGripNode(HandLData->LimbTargetGripIndex);
	const FGripNode1D* HandRGrip = GetGripNode(HandRData->LimbTargetGripIndex);
	if (!HandLGrip || !HandRGrip)
	{
		DeRegisterClimbObject();
		return false;
	}

	const bool bMoveLeftHand = bUp ? HandLGrip->ClimbCoordinate <= HandRGrip->ClimbCoordinate
	                               : HandLGrip->ClimbCoordinate >= HandRGrip->ClimbCoordinate;
	const ELimbList DesiredMovingHand = bMoveLeftHand ? ELimbList::HandL : ELimbList::HandR;

	const EClimbPhase CandidatePhases[2] = {bUp ? EClimbPhase::ClimbUp_Right : EClimbPhase::ClimbDown_Right,
	                                        bUp ? EClimbPhase::ClimbUp_Left : EClimbPhase::ClimbDown_Left};

	EClimbPhase SelectedPhase = EClimbPhase::Idle_Right;
	ELimbList MovingHand = ELimbList::Body;
	ELimbList MovingFoot = ELimbList::Body;
	bool bFoundStep = false;
	for (const EClimbPhase CandidatePhase : CandidatePhases)
	{
		ELimbList CandidateHand = ELimbList::Body;
		ELimbList CandidateFoot = ELimbList::Body;
		if (ResolveRepeatedStepLimbs(CandidatePhase, CandidateHand, CandidateFoot) &&
		    CandidateHand == DesiredMovingHand)
		{
			SelectedPhase = CandidatePhase;
			MovingHand = CandidateHand;
			MovingFoot = CandidateFoot;
			bFoundStep = true;
			break;
		}
	}

	if (!bFoundStep)
	{
		UE_LOG(Log_Climb_Ladder, Error, TEXT("[ClimbComponent] No %s repeated step is configured for moving %s."),
		       bUp ? TEXT("up") : TEXT("down"), *UEnum::GetValueAsString(DesiredMovingHand));
		return false;
	}

	const FLadderRepeatedStepDefinition* SelectedStep = GetRepeatedStepDefinition(SelectedPhase);
	if (!SelectedStep || !IsValid(SelectedStep->Animation.Get()) || SelectedStep->PlayRate <= 0.0f ||
	    !IsValid(SelectedStep->BodyCurve.Get()) || !IsValid(SelectedStep->HandCurve.Get()) ||
	    !IsValid(SelectedStep->FootCurve.Get()))
	{
		UE_LOG(Log_Climb_Ladder, Error,
		       TEXT("[ClimbComponent] Repeated step '%s' is missing its animation, play rate, or movement curves."),
		       *UEnum::GetValueAsString(SelectedPhase));
		return false;
	}

	const ELimbList StationaryHand = MovingHand == ELimbList::HandL ? ELimbList::HandR : ELimbList::HandL;
	const ELimbList StationaryFoot = MovingFoot == ELimbList::FootL ? ELimbList::FootR : ELimbList::FootL;
	FLimbData* MovingHandData = LimbToGripNode.Find(MovingHand);
	FLimbData* MovingFootData = LimbToGripNode.Find(MovingFoot);
	const FLimbData* StationaryHandData = LimbToGripNode.Find(StationaryHand);
	const FLimbData* StationaryFootData = LimbToGripNode.Find(StationaryFoot);
	if (!MovingHandData || !MovingFootData || !StationaryHandData || !StationaryFootData)
	{
		return false;
	}

	const int32 NewHandIndex = GetNeighborGripIndex(StationaryHandData->LimbTargetGripIndex, bUp);
	const int32 NewFootIndex = GetNeighborGripIndex(StationaryFootData->LimbTargetGripIndex, bUp);
	const bool bHandGripValid = GetGripNode(NewHandIndex) != nullptr;
	const bool bFootGripValid = GetGripNode(NewFootIndex) != nullptr;
	if ((bUp && !bHandGripValid) || (!bUp && !bFootGripValid))
	{
		RequestExitLadder(bUp);
		return false;
	}
	if (!bHandGripValid || !bFootGripValid)
	{
		UE_LOG(Log_Climb_Ladder, Error,
		       TEXT("[ClimbComponent] Invalid moving-limb grip while climbing %s (Hand=%s, Foot=%s)."),
		       bUp ? TEXT("up") : TEXT("down"), *UEnum::GetValueAsString(MovingHand),
		       *UEnum::GetValueAsString(MovingFoot));
		return false;
	}

	const FVector CurrentLocation = GetOwner()->GetActorLocation();
	TMap<ELimbList, int32> TargetGripAssignment;
	for (const TPair<ELimbList, FLimbData>& LimbPair : LimbToGripNode)
	{
		TargetGripAssignment.Add(LimbPair.Key, LimbPair.Value.LimbTargetGripIndex);
	}
	TargetGripAssignment.FindOrAdd(MovingHand) = NewHandIndex;
	TargetGripAssignment.FindOrAdd(MovingFoot) = NewFootIndex;
	FVector TargetBodyLocation = FVector::ZeroVector;
	if (!TryCalculateBodyTargetLocation(TargetGripAssignment, TargetBodyLocation))
	{
		return false;
	}

	FActiveRepeatedStep NewStep;
	NewStep.Phase = SelectedPhase;
	NewStep.MovingHand = MovingHand;
	NewStep.MovingFoot = MovingFoot;
	NewStep.HandStartGrip = MovingHandData->LimbTargetGripIndex;
	NewStep.HandTargetGrip = NewHandIndex;
	NewStep.FootStartGrip = MovingFootData->LimbTargetGripIndex;
	NewStep.FootTargetGrip = NewFootIndex;
	NewStep.BodyStart = CurrentLocation;
	NewStep.BodyTarget = TargetBodyLocation;
	NewStep.Duration = SelectedStep->Animation->GetPlayLength() / SelectedStep->PlayRate;
	if (NewStep.Duration <= 0.0f)
	{
		return false;
	}

	RepeatedStepRuntime.ActiveStep = NewStep;
	RepeatedStepRuntime.Animation = SelectedStep->Animation.Get();
	LadderStance = SelectedPhase;
	LadderActionState = ELadderActionState::ClimbingStep;
	RepeatedStepRuntime.ExplicitTime = 0.0f;
	SetComponentTickEnabled(true);
	return true;
}

bool UClimbComponent::CanStartRepeatedClimb() const
{
	return IsValid(ClimbObject) && LadderActionState == ELadderActionState::Idle;
}

void UClimbComponent::ClimbUpLadder()
{
	RepeatedStepRuntime.InputDirection = 1;
	StartRepeatedClimbStep(true);
}

void UClimbComponent::ClimbDownLadder()
{
	RepeatedStepRuntime.InputDirection = -1;
	StartRepeatedClimbStep(false);
}

void UClimbComponent::ClearRepeatedClimbInput()
{
	RepeatedStepRuntime.InputDirection = 0;
}

void UClimbComponent::OnEnterClimbMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (LadderActionState != ELadderActionState::Entering || !IsLadderEnterPhase(LadderStance) ||
	    Montage != GetClimbMontage(LadderStance))
	{
		UE_LOG(Log_Climb_Ladder, Warning,
		       TEXT("[ClimbComponent] Ignored an entry montage callback that does not match the active ladder state."));
		return;
	}

	const bool bWasBottomEntry = LadderStance == EClimbPhase::Enter_From_Bottom;
	const bool bWasTopEntry = LadderStance == EClimbPhase::Enter_From_Top;

	if (bInterrupted)
	{
		bool bBroadcastExit = true;
		if (const ACharacterBase* Character = Cast<ACharacterBase>(GetOwner()))
		{
			if (const UCharacterStatusComponent* StatusComponent = Character->GetCharacterStatusComponent())
			{
				bBroadcastExit = !StatusComponent->GetCurrentState().MatchesTagExact(TAG_State_Dead);
			}
		}

		ForceDetachFromLadder(bBroadcastExit);
		return;
	}

	if (bWasTopEntry && !ValidatePlannedGripRouteEnd(TransitionRuntime.PlannedRouteEndAssignment))
	{
		UE_LOG(Log_Climb_Ladder, Error,
		       TEXT("[ClimbComponent] Top-entry montage ended before all limbs reached their configured final grips."));
		ForceDetachFromLadder(true);
		return;
	}

	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		Character->GetCharacterMovement()->StopMovementImmediately();
	}

	if (bWasTopEntry)
	{
		const float CompletionError = FVector::Distance(GetOwner()->GetActorLocation(), TransitionTargetLocation);
		const float CompletionTolerance = IsValid(LadderClimbProfile)
		                                      ? FMath::Max(LadderClimbProfile->TopEnterCompletionTolerance, 0.0f)
		                                      : 0.0f;
		if (CompletionError > CompletionTolerance)
		{
			UE_LOG(Log_Climb_Ladder, Error,
			       TEXT("[ClimbComponent] Top-entry motion warping ended %.1fcm from the final attach location "
			            "(Tolerance=%.1fcm). End snap was rejected."),
			       CompletionError, CompletionTolerance);
			ForceDetachFromLadder(true);
			return;
		}
	}

	ClearTransitionWarpTargets();
	GetOwner()->SetActorLocation(TransitionTargetLocation);
	if (bWasTopEntry)
	{
		RestoreTopTransitionCollision();
	}
	LadderStance = ResolveIdlePhaseFromGripState();
	LadderActionState = ELadderActionState::Recovering;
	RepeatedStepRuntime.ExplicitTime = 0.0f;
	RepeatedStepRuntime.RecoveryElapsed = 0.0f;
	if (bWasBottomEntry)
	{
		DrawBottomEnterContactDebug();
	}
	SetComponentTickEnabled(true);
}

void UClimbComponent::OnExitClimbMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (LadderActionState != ELadderActionState::Exiting || !IsLadderExitPhase(LadderStance) ||
	    Montage != GetClimbMontage(LadderStance))
	{
		UE_LOG(Log_Climb_Ladder, Warning,
		       TEXT("[ClimbComponent] Ignored an exit montage callback that does not match the active ladder state."));
		return;
	}

	if (bInterrupted)
	{
		ForceDetachFromLadder(true);
		return;
	}

	const bool bWasTopExit = IsLadderTopExitPhase(LadderStance);
	if (bWasTopExit && !ValidatePlannedGripRouteEnd(TransitionRuntime.PlannedRouteEndAssignment))
	{
		UE_LOG(Log_Climb_Ladder, Error,
		       TEXT("[ClimbComponent] Top-exit montage ended before its planned grip route was completed."));
		ForceDetachFromLadder(true);
		return;
	}

	const float CompletionError = FVector::Distance(GetOwner()->GetActorLocation(), TransitionTargetLocation);
	const float CompletionTolerance = IsValid(LadderClimbProfile)
	                                      ? FMath::Max(LadderClimbProfile->ExitCompletionTolerance, 0.0f)
	                                      : 0.0f;
	if (CompletionError > CompletionTolerance)
	{
		UE_LOG(
		    Log_Climb_Ladder, Error,
		    TEXT(
		        "[ClimbComponent] Ladder-exit motion warping ended %.1fcm from its landing target (Tolerance=%.1fcm)."),
		    CompletionError, CompletionTolerance);
		ForceDetachFromLadder(true);
		return;
	}

	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		Character->GetCharacterMovement()->StopMovementImmediately();
	}

	GetOwner()->SetActorLocation(TransitionTargetLocation);
	if (bWasTopExit)
	{
		RestoreTopTransitionCollision();
	}
	if (bWasTopExit && IsValid(ClimbObject) && IsValid(ClimbObject->GetTopExitTarget()))
	{
		GetOwner()->SetActorRotation(ClimbObject->GetTopExitTarget()->GetComponentRotation());
	}

	CompleteExitTransition();
}

void UClimbComponent::OnExitClimbMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	if (bInterrupted || TransitionRuntime.bVisualStateReleased || LadderActionState != ELadderActionState::Exiting ||
	    !IsLadderExitPhase(LadderStance) || Montage != GetClimbMontage(LadderStance))
	{
		return;
	}

	ResetLadderIKState(true);
	TransitionRuntime.bVisualStateReleased = true;
	OnLadderExit.Broadcast();
}

FRotator UClimbComponent::CalculateLadderAlignmentRotation() const
{
	if (!IsValid(ClimbObject))
	{
		return GetOwner()->GetActorRotation();
	}

	FVector FacingDirection = -ClimbObject->GetLadderForwardVector();
	FacingDirection.Z = 0.0f;
	return FacingDirection.IsNearlyZero() ? GetOwner()->GetActorRotation() : FacingDirection.Rotation();
}

const FGripNode1D* UClimbComponent::GetGripNode(int32 GripIndex) const
{
	return GripList1D.IsValidIndex(GripIndex) ? &GripList1D[GripIndex] : nullptr;
}

int32 UClimbComponent::GetNeighborGripIndex(int32 GripIndex, bool bUp, int32 Count) const
{
	while (GripList1D.IsValidIndex(GripIndex) && Count-- > 0)
	{
		GripIndex = bUp ? GripList1D[GripIndex].NeighborUpIndex : GripList1D[GripIndex].NeighborDownIndex;
	}

	return GripList1D.IsValidIndex(GripIndex) ? GripIndex : INDEX_NONE;
}

FVector UClimbComponent::GetGripWorldPosition(int32 GripIndex) const
{
	const FGripNode1D* GripNode = GetGripNode(GripIndex);
	return GripNode && IsValid(ClimbObject)
	           ? ClimbObject->GetActorTransform().TransformPosition(GripNode->LocalPosition)
	           : FVector::ZeroVector;
}

bool UClimbComponent::TryCalculateBodyTargetLocation(const TMap<ELimbList, int32>& GripAssignment,
                                                     FVector& OutTargetLocation) const
{
	if (!IsValid(ClimbObject) || !IsValid(LadderClimbProfile))
	{
		return false;
	}

	static constexpr ELimbList AnchorLimbs[] = {ELimbList::HandL, ELimbList::HandR, ELimbList::FootL, ELimbList::FootR};

	FVector GripCentroid = FVector::ZeroVector;
	for (const ELimbList Limb : AnchorLimbs)
	{
		const int32* GripIndex = GripAssignment.Find(Limb);
		if (!GripIndex || !GetGripNode(*GripIndex))
		{
			UE_LOG(Log_Climb_Ladder, Error,
			       TEXT("[ClimbComponent] Cannot calculate body anchor because %s has no valid grip."),
			       *UEnum::GetValueAsString(Limb));
			return false;
		}

		GripCentroid += GetGripWorldPosition(*GripIndex);
	}
	GripCentroid /= static_cast<float>(UE_ARRAY_COUNT(AnchorLimbs));

	const FVector LadderForward = ClimbObject->GetLadderForwardVector().GetSafeNormal();
	const FVector LadderRight = ClimbObject->GetLadderRightVector().GetSafeNormal();
	const FVector LadderUp = ClimbObject->GetLadderUpVector().GetSafeNormal();

	OutTargetLocation = GripCentroid + LadderForward * LadderClimbProfile->BodyAnchorForwardOffset +
	                    LadderRight * LadderClimbProfile->BodyAnchorRightOffset +
	                    LadderUp * LadderClimbProfile->BodyAnchorUpOffset;
	return true;
}

bool UClimbComponent::RegisterClimbObject(ALadderBase* Ladder)
{
	if (!IsValid(Ladder))
	{
		DeRegisterClimbObject();
		return false;
	}

	ClimbObject = Ladder;
	ClimbObject->OnDestroyed.RemoveDynamic(this, &UClimbComponent::HandleClimbObjectDestroyed);
	ClimbObject->OnDestroyed.AddDynamic(this, &UClimbComponent::HandleClimbObjectDestroyed);
	GripList1D = ClimbObject->GetGripList1D();
	GripList1D.Sort([](const FGripNode1D& Left, const FGripNode1D& Right)
	                { return Left.ClimbCoordinate < Right.ClimbCoordinate; });
	if (GripList1D.IsEmpty() || !BuildGripNeighborRelations())
	{
		DeRegisterClimbObject();
		return false;
	}
	return true;
}

void UClimbComponent::DeRegisterClimbObject()
{
	if (bHasCharacterStateSnapshot || LadderActionState != ELadderActionState::Detached)
	{
		ForceDetachFromLadder(false);
		return;
	}

	ClearLadderSession();
}

FVector UClimbComponent::GetLimbIKTarget(ELimbList LimbName) const
{
	if (const FSurfaceHandContactState* SurfaceContact = ActiveSurfaceHandContacts.Find(LimbName))
	{
		return SurfaceContact->Location;
	}

	const FLimbData* LimbData = LimbToGripNode.Find(LimbName);
	if (!LimbData)
	{
		UE_LOG(Log_Climb_Ladder, Warning, TEXT("[ClimbComponent] Bone is not located on the ladder [Bone Name: %s]."),
		       *UEnum::GetValueAsString(LimbName));
		return FVector::ZeroVector;
	}

	return LimbData->LimbLocation;
}

void UClimbComponent::FinishActiveRepeatedStep()
{
	if (!RepeatedStepRuntime.ActiveStep.IsSet() || LadderActionState != ELadderActionState::ClimbingStep)
	{
		return;
	}

	const FActiveRepeatedStep CompletedStep = RepeatedStepRuntime.ActiveStep.GetValue();
	const float CompletionError = FVector::Distance(GetOwner()->GetActorLocation(), CompletedStep.BodyTarget);
	const float CompletionTolerance = IsValid(LadderClimbProfile)
	                                      ? FMath::Max(LadderClimbProfile->RepeatedClimbCompletionTolerance, 0.0f)
	                                      : 0.0f;
	if (CompletionError > CompletionTolerance)
	{
		UE_LOG(Log_Climb_Ladder, Error,
		       TEXT("[ClimbComponent] Repeated climb ended %.1fcm from its body target (Tolerance=%.1fcm)."),
		       CompletionError, CompletionTolerance);
		ForceDetachFromLadder(true);
		return;
	}

	if (!MoveCharacterAlongClimbPath(CompletedStep.BodyTarget))
	{
		ForceDetachFromLadder(true);
		return;
	}

	FLimbData* HandData = LimbToGripNode.Find(CompletedStep.MovingHand);
	FLimbData* FootData = LimbToGripNode.Find(CompletedStep.MovingFoot);
	if (!HandData || !FootData)
	{
		ForceDetachFromLadder(true);
		return;
	}

	HandData->LimbTargetGripIndex = CompletedStep.HandTargetGrip;
	FootData->LimbTargetGripIndex = CompletedStep.FootTargetGrip;
	RepeatedStepRuntime.ActiveStep.Reset();
	LadderStance = ResolveIdlePhaseFromGripState();
	LadderActionState = ELadderActionState::Recovering;
	RepeatedStepRuntime.RecoveryElapsed = 0.0f;

	LimbToGripNode[ELimbList::HandR].LimbLocation =
	    SetBoneIKTargetLadder(LimbToGripNode[ELimbList::HandR].LimbTargetGripIndex, FVector(), -15.0f);
	LimbToGripNode[ELimbList::FootL].LimbLocation =
	    SetBoneIKTargetLadder(LimbToGripNode[ELimbList::FootL].LimbTargetGripIndex, FVector(), 15.0f);
	LimbToGripNode[ELimbList::HandL].LimbLocation =
	    SetBoneIKTargetLadder(LimbToGripNode[ELimbList::HandL].LimbTargetGripIndex, FVector(), 15.0f);
	LimbToGripNode[ELimbList::FootR].LimbLocation =
	    SetBoneIKTargetLadder(LimbToGripNode[ELimbList::FootR].LimbTargetGripIndex, FVector(), -15.0f);
}

FVector UClimbComponent::SetBoneIKTargetLadder(int32 TargetGripIndex, const FVector CurveValue, float LimbXDistance,
                                               int32 StartGripIndex)
{
	const FGripNode1D* TargetGrip = GetGripNode(TargetGripIndex);
	const FGripNode1D* StartGrip = GetGripNode(StartGripIndex);
	if (!TargetGrip || !IsValid(ClimbObject))
	{
		UE_LOG(Log_Climb_Ladder, Error,
		       TEXT("[ClimbComponent] Cannot calculate ladder IK without a valid ladder and target grip."));
		return FVector::ZeroVector;
	}
	FVector TargetLoc;
	const FVector LadderForward = ClimbObject->GetLadderForwardVector().GetSafeNormal();
	const FVector LadderRight = ClimbObject->GetLadderRightVector().GetSafeNormal();
	const FVector LimbOffset = LadderRight * LimbXDistance;

	if (StartGrip)
	{
		TargetLoc = FMath::Lerp(GetGripWorldPosition(StartGripIndex), GetGripWorldPosition(TargetGripIndex),
		                        CurveValue.Z) +
		            LimbOffset;

		const FVector ForwardOffset = LadderForward * CurveValue.Y;
		const FVector RightOffset = LadderRight * CurveValue.X;

		TargetLoc += ForwardOffset + RightOffset;
	}
	else
	{
		TargetLoc = GetGripWorldPosition(TargetGripIndex) + LimbOffset;
	}

	return TargetLoc;
}

bool UClimbComponent::BuildGripNeighborRelations()
{
	if (GripList1D.IsEmpty())
		return false;
	if (MaxGripInterval <= 0.0f)
	{
		UE_LOG(Log_Climb_Ladder, Error, TEXT("[ClimbComponent] MaxGripInterval must be greater than zero."));
		return false;
	}
	bool bRelationsValid = true;

	for (FGripNode1D& Grip : GripList1D)
	{
		Grip.NeighborDownIndex = INDEX_NONE;
		Grip.NeighborUpIndex = INDEX_NONE;
	}

	for (int32 LowerIndex = 0; LowerIndex + 1 < GripList1D.Num(); ++LowerIndex)
	{
		const int32 UpperIndex = LowerIndex + 1;
		const float UpDistance = GripList1D[UpperIndex].ClimbCoordinate - GripList1D[LowerIndex].ClimbCoordinate;
		if (UpDistance <= DuplicateGripHeightTolerance)
		{
			bRelationsValid = false;
			UE_LOG(Log_Climb_Ladder, Error,
			       TEXT("[ClimbComponent] Grips %d and %d overlap on the ladder Up axis (Distance=%.2fcm)."),
			       LowerIndex, UpperIndex, UpDistance);
			continue;
		}
		if (UpDistance > MaxGripInterval)
		{
			bRelationsValid = false;
			UE_LOG(Log_Climb_Ladder, Error,
			       TEXT("[ClimbComponent] Grips %d and %d are too far apart (Distance=%.1fcm, Max=%.1fcm)."),
			       LowerIndex, UpperIndex, UpDistance, MaxGripInterval);
			continue;
		}

		GripList1D[LowerIndex].NeighborUpIndex = UpperIndex;
		GripList1D[UpperIndex].NeighborDownIndex = LowerIndex;
	}
	return bRelationsValid;
}
