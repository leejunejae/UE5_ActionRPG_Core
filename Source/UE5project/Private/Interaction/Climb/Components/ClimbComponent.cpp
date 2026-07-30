// Fill out your copyright notice in the Description page of Project Settings.


#include "Interaction/Climb/Components/ClimbComponent.h"
#include "Environment/Climbable/Ladder/LadderBase.h"
#include "Interaction/Interfaces/InteractInterface.h"
#include "Characters/Components/CharacterStatusComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Characters/CharacterBase.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/Notifies/ANS_LadderGripTransition.h"
#include "Animation/Interfaces/IAnimInstance.h"
#include "MotionWarpingComponent.h"
#include "DrawDebugHelpers.h"

#include "Utils/CoreLog.h"
#include "Utils/GameplayTagsBase.h"

// Sets default values for this component's properties
UClimbComponent::UClimbComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

}


// Called when the game starts
void UClimbComponent::BeginPlay()
{
	Super::BeginPlay();

	if (ACharacterBase* Character = Cast<ACharacterBase>(GetOwner()))
	{
		if (UCharacterStatusComponent* StatusComponent = Character->GetCharacterStatusComponent())
		{
			StatusComponent->OnDeathStarted.AddUObject(this, &UClimbComponent::HandleOwnerDeathStarted);
		}
	}
}

UCurveVector* UClimbComponent::GetClimbCurve(const FClimbCurveKey& Key) const
{
	if (!LadderClimbProfile) return nullptr;
	return LadderClimbProfile->Curves.FindRef(Key);
}

UAnimMontage* UClimbComponent::GetClimbMontage(EClimbPhase Phase) const
{
	if (!LadderClimbProfile) return nullptr;
	return LadderClimbProfile->Montages.FindRef(Phase);
}

void UClimbComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bEnterMontageActive || bExitMontageActive)
	{
		return;
	}

	if (!bIsClimbing)
	{
		return;
	}

	FVector BodyCurveValue;
	FVector HandCurveValue;
	FVector FootCurveValue;

	AnimTime += DeltaTime;
	if (UCurveVector* BodyCurve = GetClimbCurve(FClimbCurveKey{ LadderStance, ELimbList::Body }))
	{
		BodyCurveValue = BodyCurve->GetVectorValue(AnimTime);
	}

	const bool bUseLadderLocalMovement =
		LadderStance == EClimbPhase::ClimbUp_Right ||
		LadderStance == EClimbPhase::ClimbUp_Left ||
		LadderStance == EClimbPhase::ClimbDown_Right ||
		LadderStance == EClimbPhase::ClimbDown_Left;

	FVector NewLocation;
	if (bUseLadderLocalMovement && IsValid(ClimbObject))
	{
		const FTransform LadderTransform = ClimbObject->GetActorTransform();
		const FVector LocalStartLocation = LadderTransform.InverseTransformPosition(ClimbLocation.Key);
		const FVector LocalTargetLocation = LadderTransform.InverseTransformPosition(ClimbLocation.Value);
		const FVector LocalNewLocation = FMath::Lerp(LocalStartLocation, LocalTargetLocation, BodyCurveValue);
		NewLocation = LadderTransform.TransformPosition(LocalNewLocation);
	}
	else
	{
		NewLocation = FMath::Lerp(ClimbLocation.Key, ClimbLocation.Value, BodyCurveValue);
	}

	ACharacter* Character = Cast<ACharacter>(GetOwner());

	GetOwner()->SetActorLocation(NewLocation);
	Character->GetMesh()->UpdateComponentToWorld();

	switch (LadderStance)
	{
	case EClimbPhase::ClimbUp_Right:
	{
		if (UCurveVector* HandCurve = GetClimbCurve(FClimbCurveKey{ LadderStance, ELimbList::HandR }))
		{
			HandCurveValue = HandCurve->GetVectorValue(AnimTime);
		}

		if (UCurveVector* FootCurve = GetClimbCurve(FClimbCurveKey{ LadderStance, ELimbList::FootL }))
		{
			FootCurveValue = FootCurve->GetVectorValue(AnimTime);
		}

		const FLimbData& HandRData = LimbToGripNode[ELimbList::HandR];
		const FLimbData& FootLData = LimbToGripNode[ELimbList::FootL];
		LimbToGripNode[ELimbList::HandR].LimbLocation = SetBoneIKTargetLadder(HandRData.LimbTargetGripIndex, HandCurveValue, -15.0f, HandRData.PreviousGripIndex);
		LimbToGripNode[ELimbList::FootL].LimbLocation = SetBoneIKTargetLadder(FootLData.LimbTargetGripIndex, FootCurveValue, 15.0f, FootLData.PreviousGripIndex);

		break;
	}
	case EClimbPhase::ClimbUp_Left:
	{
		if (UCurveVector* HandCurve = GetClimbCurve(FClimbCurveKey{ LadderStance, ELimbList::HandL }))
		{
			HandCurveValue = HandCurve->GetVectorValue(AnimTime);
		}

		if (UCurveVector* FootCurve = GetClimbCurve(FClimbCurveKey{ LadderStance, ELimbList::FootR }))
		{
			FootCurveValue = FootCurve->GetVectorValue(AnimTime);
		}

		const FLimbData& HandLData = LimbToGripNode[ELimbList::HandL];
		const FLimbData& FootRData = LimbToGripNode[ELimbList::FootR];
		LimbToGripNode[ELimbList::HandL].LimbLocation = SetBoneIKTargetLadder(HandLData.LimbTargetGripIndex, HandCurveValue, 15.0f, HandLData.PreviousGripIndex);
		LimbToGripNode[ELimbList::FootR].LimbLocation = SetBoneIKTargetLadder(FootRData.LimbTargetGripIndex, FootCurveValue, -15.0f, FootRData.PreviousGripIndex);
		break;
	}
	case EClimbPhase::ClimbDown_Right:
	{
		if (UCurveVector* HandCurve = GetClimbCurve(FClimbCurveKey{ LadderStance, ELimbList::HandR }))
		{
			HandCurveValue = HandCurve->GetVectorValue(AnimTime);
		}

		if (UCurveVector* FootCurve = GetClimbCurve(FClimbCurveKey{ LadderStance, ELimbList::FootL }))
		{
			FootCurveValue = FootCurve->GetVectorValue(AnimTime);
		}

		const FLimbData& HandRData = LimbToGripNode[ELimbList::HandR];
		const FLimbData& FootLData = LimbToGripNode[ELimbList::FootL];
		LimbToGripNode[ELimbList::HandR].LimbLocation = SetBoneIKTargetLadder(HandRData.LimbTargetGripIndex, HandCurveValue, -15.0f, HandRData.PreviousGripIndex);
		LimbToGripNode[ELimbList::FootL].LimbLocation = SetBoneIKTargetLadder(FootLData.LimbTargetGripIndex, FootCurveValue, 15.0f, FootLData.PreviousGripIndex);

		break;
	}
	case EClimbPhase::ClimbDown_Left:
	{
		if (UCurveVector* HandCurve = GetClimbCurve(FClimbCurveKey{ LadderStance, ELimbList::HandL }))
		{
			HandCurveValue = HandCurve->GetVectorValue(AnimTime);
		}

		if (UCurveVector* FootCurve = GetClimbCurve(FClimbCurveKey{ LadderStance, ELimbList::FootR }))
		{
			FootCurveValue = FootCurve->GetVectorValue(AnimTime);
		}

		const FLimbData& HandLData = LimbToGripNode[ELimbList::HandL];
		const FLimbData& FootRData = LimbToGripNode[ELimbList::FootR];
		LimbToGripNode[ELimbList::HandL].LimbLocation = SetBoneIKTargetLadder(HandLData.LimbTargetGripIndex, HandCurveValue, 15.0f, HandLData.PreviousGripIndex);
		LimbToGripNode[ELimbList::FootR].LimbLocation = SetBoneIKTargetLadder(FootRData.LimbTargetGripIndex, FootCurveValue, -15.0f, FootRData.PreviousGripIndex);

		break;
	}
	default:
		return;
	}
}


bool UClimbComponent::RequestEnterLadder(AActor* TargetLadder)
{
	if (IsValid(ClimbObject) || LadderTransitionState != ELadderTransitionState::None)
	{
		return false;
	}

	if (!IsValid(LadderClimbProfile))
	{
		UE_LOG(
			Log_Climb_Ladder,
			Error,
			TEXT("[ClimbComponent] LadderClimbProfile is not configured on '%s'."),
			*GetNameSafe(GetOwner()));
		return false;
	}

	ALadderBase* Ladder = Cast<ALadderBase>(TargetLadder);
	if (!IsValid(Ladder) ||
		!Ladder->GetClass()->ImplementsInterface(UInteractInterface::StaticClass()))
	{
		UE_LOG(Log_Climb_Ladder, Error, TEXT("[ClimbComponent] Invalid ladder target: %s"), *GetNameSafe(TargetLadder));
		return false;
	}

	RegisterClimbObject(Ladder);
	if (GripList1D.IsEmpty())
	{
		UE_LOG(Log_Climb_Ladder, Error, TEXT("[ClimbComponent] Ladder '%s' has no Grip sockets."), *GetNameSafe(TargetLadder));
		DeRegisterClimbObject();
		return false;
	}

	USceneComponent* ClimbPoint = IInteractInterface::Execute_GetEnterInteractLocation(Ladder, GetOwner());
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!IsValid(ClimbPoint) || !IsValid(Character))
	{
		UE_LOG(Log_Climb_Ladder, Error, TEXT("[ClimbComponent] Ladder '%s' is missing a required entry component."), *GetNameSafe(TargetLadder));
		DeRegisterClimbObject();
		return false;
	}

	FVector InitCharacterPosition = CalculateLadderAlignmentLocation(Character);

	const bool bEnterFromBottom = ClimbPoint->ComponentHasTag("Bottom");
	TMap<ELimbList, int32> FinalIdleGripAssignment;
	if (bEnterFromBottom)
	{
		if (!ResolveGripPattern(
				LadderClimbProfile->BottomEnterIdleGripHeightOffsets,
				LadderClimbProfile->BottomEnterIdleReferenceLimb,
				false,
				FinalIdleGripAssignment))
		{
			UE_LOG(
				Log_Climb_Ladder,
				Error,
				TEXT("[ClimbComponent] Ladder '%s' has no valid bottom-entry grip assignment."),
				*GetNameSafe(TargetLadder));
			DeRegisterClimbObject();
			return false;
		}

		const int32 FootRGripIndex = FinalIdleGripAssignment[ELimbList::FootR];
		const int32 FootLGripIndex = FinalIdleGripAssignment[ELimbList::FootL];
		const int32 HandLGripIndex = FinalIdleGripAssignment[ELimbList::HandL];
		const int32 HandRGripIndex = FinalIdleGripAssignment[ELimbList::HandR];

		LimbToGripNode.Add(ELimbList::FootR, FLimbData(FootRGripIndex, SetBoneIKTargetLadder(FootRGripIndex, FVector(), -15.0f)));
		LimbToGripNode.Add(ELimbList::FootL, FLimbData(FootLGripIndex, SetBoneIKTargetLadder(FootLGripIndex, FVector(), 15.0f)));
		LimbToGripNode.Add(ELimbList::HandL, FLimbData(HandLGripIndex, SetBoneIKTargetLadder(HandLGripIndex, FVector(), 15.0f)));
		LimbToGripNode.Add(ELimbList::HandR, FLimbData(HandRGripIndex, SetBoneIKTargetLadder(HandRGripIndex, FVector(), -15.0f)));
		LimbToGripNode.Add(ELimbList::Body, FLimbData(INDEX_NONE, GetOwner()->GetActorLocation()));
		LadderStance = EClimbPhase::Enter_From_Bottom;
	}
	else
	{
		TMap<ELimbList, int32> AuthoredInitialGripAssignment;
		if (!ResolveGripPattern(
				LadderClimbProfile->TopEnterInitialGripHeightOffsets,
				LadderClimbProfile->TopEnterInitialReferenceLimb,
				true,
				AuthoredInitialGripAssignment) ||
			!ResolveGripPattern(
				LadderClimbProfile->TopEnterIdleGripHeightOffsets,
				LadderClimbProfile->TopEnterIdleReferenceLimb,
				true,
				FinalIdleGripAssignment) ||
			!BuildGripRoute(
				GetClimbMontage(EClimbPhase::Enter_From_Top),
				AuthoredInitialGripAssignment,
				FinalIdleGripAssignment))
		{
			UE_LOG(Log_Climb_Ladder, Error, TEXT("[ClimbComponent] Ladder '%s' has invalid top-entry data."), *GetNameSafe(TargetLadder));
			DeRegisterClimbObject();
			return false;
		}

		LimbToGripNode.Add(ELimbList::HandL, FLimbData(
			AuthoredInitialGripAssignment[ELimbList::HandL],
			SetBoneIKTargetLadder(AuthoredInitialGripAssignment[ELimbList::HandL], FVector(), 15.0f)));
		LimbToGripNode.Add(ELimbList::HandR, FLimbData(
			AuthoredInitialGripAssignment[ELimbList::HandR],
			SetBoneIKTargetLadder(AuthoredInitialGripAssignment[ELimbList::HandR], FVector(), -15.0f)));
		LimbToGripNode.Add(ELimbList::FootL, FLimbData(
			AuthoredInitialGripAssignment[ELimbList::FootL],
			SetBoneIKTargetLadder(AuthoredInitialGripAssignment[ELimbList::FootL], FVector(), 15.0f)));
		LimbToGripNode.Add(ELimbList::FootR, FLimbData(
			AuthoredInitialGripAssignment[ELimbList::FootR],
			SetBoneIKTargetLadder(AuthoredInitialGripAssignment[ELimbList::FootR], FVector(), -15.0f)));
		LimbToGripNode.Add(ELimbList::Body, FLimbData(INDEX_NONE, GetOwner()->GetActorLocation()));

		LadderStance = EClimbPhase::Enter_From_Top;
	}

	InitCharacterPosition =
		CalculateBodyTargetLocation(
			FinalIdleGripAssignment,
			InitCharacterPosition);

	ClimbLocation = MakeTuple(GetOwner()->GetActorLocation(), InitCharacterPosition);
	if (!BeginLadderTransition(bEnterFromBottom
		? ELadderTransitionState::EnterBottom
		: ELadderTransitionState::EnterTop))
	{
		UE_LOG(Log_Climb_Ladder, Warning, TEXT("[ClimbComponent] A ladder transition is already active."));
		DeRegisterClimbObject();
		return false;
	}

	Character->GetCapsuleComponent()->IgnoreActorWhenMoving(TargetLadder, true);

	bIsClimbing = true;
	EnterLadderFloat();

	const EClimbPhase EnterPhase = bEnterFromBottom
		? EClimbPhase::Enter_From_Bottom
		: EClimbPhase::Enter_From_Top;
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
		UE_LOG(
			Log_Climb_Ladder,
			Error,
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
	if (!IsValid(Character) || !IsValid(ClimbObject) || LadderTransitionState != ELadderTransitionState::None)
	{
		return false;
	}

	if (bExitTop)
	{
		const USceneComponent* ExitPoint =
			ClimbObject->GetTopExitTarget();
		if (!IsValid(ExitPoint))
		{
			UE_LOG(
				Log_Climb_Ladder,
				Error,
				TEXT("[ClimbComponent] Ladder '%s' has no top-exit target."),
				*GetNameSafe(ClimbObject));
			return false;
		}

		FVector ExitLocation = ExitPoint->GetComponentLocation();
		ExitLocation.Z += Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

		ClimbLocation = MakeTuple(GetOwner()->GetActorLocation(), ExitLocation);
		LadderStance =
			ResolveIdlePhaseFromGripState() == EClimbPhase::Idle_Left
				? EClimbPhase::Exit_From_Top_Left
				: EClimbPhase::Exit_From_Top_Right;

		if (!BuildTopExitGripRoute(LadderStance))
		{
			UE_LOG(
				Log_Climb_Ladder,
				Error,
				TEXT("[ClimbComponent] Ladder '%s' has no valid top-exit grip route for '%s'."),
				*GetNameSafe(ClimbObject),
				*UEnum::GetValueAsString(LadderStance));
			PlannedLimbGripTargets.Empty();
			PlannedGripRouteEndAssignment.Empty();
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
		float Radius = Character->GetCapsuleComponent()->GetScaledCapsuleRadius();
		float HalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

		FCollisionShape DetectShape = FCollisionShape::MakeCapsule(Radius, HalfHeight);

		bool bHit = GetWorld()->SweepSingleByChannel(
			HitResult,
			StartLoc,
			EndLoc,
			FQuat::Identity,
			ECC_GameTraceChannel8,
			DetectShape,
			CollisionParams
		);

		if (!bHit)
		{
			UE_LOG(Log_Climb_Ladder, Warning, TEXT("[ClimbComponent] Failed to find a valid ladder exit location."));
			return false;
		}

		FVector ExitLocation = HitResult.ImpactPoint;
		ExitLocation.Z += Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		ClimbLocation = MakeTuple(GetOwner()->GetActorLocation(), ExitLocation);

		LadderStance =
			ResolveIdlePhaseFromGripState() == EClimbPhase::Idle_Left
				? EClimbPhase::Exit_From_Bottom_Left
				: EClimbPhase::Exit_From_Bottom_Right;
		PlannedLimbGripTargets.Empty();
		PlannedGripRouteEndAssignment.Empty();
	}
	if (!BeginLadderTransition(bExitTop
		? ELadderTransitionState::ExitTop
		: ELadderTransitionState::ExitBottom))
	{
		return false;
	}

	bIsClimbing = true;
	if (!PlayExitMontage(LadderStance))
	{
		UE_LOG(
			Log_Climb_Ladder,
			Error,
			TEXT("[ClimbComponent] Ladder exit was cancelled because montage '%s' could not start."),
			*UEnum::GetValueAsString(LadderStance));
		LadderTransitionState = ELadderTransitionState::None;
		bIsClimbing = false;
		PlannedLimbGripTargets.Empty();
		PlannedGripRouteEndAssignment.Empty();
		ClearTransitionWarpTargets();
		return false;
	}
	SetComponentTickEnabled(false);

	return true;
}

void UClimbComponent::EnterLadderFloat()
{
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		CaptureCharacterState();
		Character->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	}
}

void UClimbComponent::ExitLadderFloat()
{
	CompleteLadderTransition();
}

bool UClimbComponent::BeginLadderTransition(ELadderTransitionState NewTransition)
{
	if (NewTransition == ELadderTransitionState::None ||
		LadderTransitionState != ELadderTransitionState::None)
	{
		return false;
	}

	if (NewTransition == ELadderTransitionState::EnterBottom ||
		NewTransition == ELadderTransitionState::EnterTop)
	{
		CaptureCharacterState();
	}

	LadderTransitionState = NewTransition;
	return true;
}

void UClimbComponent::CompleteLadderTransition()
{
	if (LadderTransitionState == ELadderTransitionState::EnterBottom ||
		LadderTransitionState == ELadderTransitionState::EnterTop)
	{
		LadderTransitionState = ELadderTransitionState::None;
		return;
	}

	if (LadderTransitionState == ELadderTransitionState::ExitBottom ||
		LadderTransitionState == ELadderTransitionState::ExitTop)
	{
		const bool bShouldBroadcastExit =
			!bExitVisualStateReleased;
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
		bSavedOrientRotationToMovement = MovementComponent->bOrientRotationToMovement;
		bHasCharacterStateSnapshot = true;
	}
}

void UClimbComponent::RestoreCharacterState()
{
	if (!bHasCharacterStateSnapshot)
	{
		return;
	}

	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		Character->GetCapsuleComponent()->IgnoreActorWhenMoving(ClimbObject, false);

		UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement();
		MovementComponent->bOrientRotationToMovement = bSavedOrientRotationToMovement;
		MovementComponent->SetMovementMode(
			static_cast<EMovementMode>(SavedMovementMode),
			SavedCustomMovementMode);
	}

	bHasCharacterStateSnapshot = false;
}

void UClimbComponent::ClearLadderSession()
{
	ClearTransitionWarpTargets();
	bEnterMontageActive = false;
	bExitMontageActive = false;
	bExitVisualStateReleased = false;
	LimbToGripNode.Empty();
	ActiveLimbGripTransitions.Empty();
	PlannedLimbGripTargets.Empty();
	PlannedGripRouteEndAssignment.Empty();
	GripList1D.Empty();
	ClimbObject = nullptr;
	bIsClimbing = false;
	AnimTime = 0.0f;
	LadderStance = EClimbPhase::Idle_Right;
	LadderTransitionState = ELadderTransitionState::None;
	SetComponentTickEnabled(false);
}

void UClimbComponent::ResetLadderIKState(bool bRestoreGroundPhase)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	UAnimInstance* AnimInstance = Character && Character->GetMesh()
		? Character->GetMesh()->GetAnimInstance()
		: nullptr;
	if (!IsValid(AnimInstance) ||
		!AnimInstance->GetClass()->ImplementsInterface(UIAnimInstance::StaticClass()))
	{
		return;
	}

	IIAnimInstance::Execute_SetIKPhaseAlpha(
		AnimInstance,
		TAG_IK_Phase_Ladder,
		0.0f);

	if (bRestoreGroundPhase)
	{
		IIAnimInstance::Execute_SetIKPhaseAlpha(
			AnimInstance,
			TAG_IK_Phase_Ground,
			1.0f);
	}

	static constexpr ELimbList LadderLimbs[] =
	{
		ELimbList::HandL,
		ELimbList::HandR,
		ELimbList::FootL,
		ELimbList::FootR
	};

	for (const ELimbList Limb : LadderLimbs)
	{
		IIAnimInstance::Execute_SetIKLayerAlpha(
			AnimInstance,
			TAG_IK_Layer_Ladder_Climb,
			Limb,
			0.0f);
	}
}

void UClimbComponent::ForceDetachFromLadder(bool bBroadcastExit)
{
	const bool bShouldBroadcastExit =
		bBroadcastExit && !bExitVisualStateReleased;
	bool bRestoreGroundPhase = true;
	if (const ACharacterBase* Character = Cast<ACharacterBase>(GetOwner()))
	{
		if (const UCharacterStatusComponent* StatusComponent =
			Character->GetCharacterStatusComponent())
		{
			bRestoreGroundPhase =
				!StatusComponent->GetCurrentState().MatchesTagExact(TAG_State_Dead);
		}
	}

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

bool UClimbComponent::BeginLimbGripTransition(
	ELimbList Limb,
	ELadderGripDirection Direction,
	UCurveVector* TrajectoryCurve)
{
	if (Limb == ELimbList::Body ||
		ActiveLimbGripTransitions.Contains(Limb))
	{
		return false;
	}

	FLimbData* LimbData = LimbToGripNode.Find(Limb);
	if (!LimbData || !GetGripNode(LimbData->LimbTargetGripIndex))
	{
		return false;
	}

	TArray<int32>* PlannedTargets = PlannedLimbGripTargets.Find(Limb);
	if (!PlannedTargets || PlannedTargets->IsEmpty())
	{
		UE_LOG(
			Log_Climb_Ladder,
			Error,
			TEXT("[ClimbComponent] No planned grip target remains for %s."),
			*UEnum::GetValueAsString(Limb));
		return false;
	}

	const int32 StartGripIndex = LimbData->LimbTargetGripIndex;
	const int32 TargetGripIndex = (*PlannedTargets)[0];
	const FGripNode1D* StartGrip = GetGripNode(StartGripIndex);
	const FGripNode1D* TargetGrip = GetGripNode(TargetGripIndex);
	const float DirectionSign =
		Direction == ELadderGripDirection::Up ? 1.0f : -1.0f;
	if (!StartGrip || !TargetGrip ||
		(TargetGrip->LocalPosition.Z - StartGrip->LocalPosition.Z) *
			DirectionSign <= 0.0f)
	{
		UE_LOG(
			Log_Climb_Ladder,
			Error,
			TEXT("[ClimbComponent] Planned target for %s does not move %s."),
			*UEnum::GetValueAsString(Limb),
			Direction == ELadderGripDirection::Up ? TEXT("up") : TEXT("down"));
		return false;
	}
	PlannedTargets->RemoveAt(0);

	FLimbGripTransitionState& Transition =
		ActiveLimbGripTransitions.Add(Limb);
	Transition.StartGripIndex = StartGripIndex;
	Transition.TargetGripIndex = TargetGripIndex;
	Transition.TrajectoryCurve = TrajectoryCurve;

	LimbData->PreviousGripIndex = StartGripIndex;
	LimbData->LimbTargetGripIndex = TargetGripIndex;
	return true;
}

void UClimbComponent::UpdateLimbGripTransition(
	ELimbList Limb,
	float NormalizedTime)
{
	FLimbGripTransitionState* Transition =
		ActiveLimbGripTransitions.Find(Limb);
	FLimbData* LimbData = LimbToGripNode.Find(Limb);
	if (!Transition || !LimbData)
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
		CurveValue = Curve->GetVectorValue(
			FMath::Lerp(MinTime, MaxTime, ClampedTime));
	}

	const float LimbSideOffset =
		Limb == ELimbList::HandL || Limb == ELimbList::FootL
			? 15.0f
			: -15.0f;
	LimbData->LimbLocation = SetBoneIKTargetLadder(
		Transition->TargetGripIndex,
		CurveValue,
		LimbSideOffset,
		Transition->StartGripIndex);
}

void UClimbComponent::CompleteLimbGripTransition(ELimbList Limb)
{
	FLimbData* LimbData = LimbToGripNode.Find(Limb);
	if (!ActiveLimbGripTransitions.Contains(Limb) || !LimbData)
	{
		return;
	}

	UpdateLimbGripTransition(Limb, 1.0f);
	const FLimbGripTransitionState* Transition =
		ActiveLimbGripTransitions.Find(Limb);
	const float LimbSideOffset =
		Limb == ELimbList::HandL || Limb == ELimbList::FootL
			? 15.0f
			: -15.0f;
	LimbData->LimbLocation = SetBoneIKTargetLadder(
		Transition->TargetGripIndex,
		FVector::ZeroVector,
		LimbSideOffset);
	LimbData->PreviousGripIndex = INDEX_NONE;
	ActiveLimbGripTransitions.Remove(Limb);
}

void UClimbComponent::CancelLimbGripTransition(ELimbList Limb)
{
	FLimbGripTransitionState* Transition =
		ActiveLimbGripTransitions.Find(Limb);
	FLimbData* LimbData = LimbToGripNode.Find(Limb);
	if (!Transition || !LimbData)
	{
		return;
	}

	LimbData->LimbTargetGripIndex = Transition->StartGripIndex;
	LimbData->PreviousGripIndex = INDEX_NONE;
	const float LimbSideOffset =
		Limb == ELimbList::HandL || Limb == ELimbList::FootL
			? 15.0f
			: -15.0f;
	LimbData->LimbLocation = SetBoneIKTargetLadder(
		Transition->StartGripIndex,
		FVector::ZeroVector,
		LimbSideOffset);
	ActiveLimbGripTransitions.Remove(Limb);
}

void UClimbComponent::HandleOwnerDeathStarted()
{
	ForceDetachFromLadder(false);
}

bool UClimbComponent::PlayEnterMontage(EClimbPhase EnterPhase)
{
	if (EnterPhase != EClimbPhase::Enter_From_Bottom &&
		EnterPhase != EClimbPhase::Enter_From_Top)
	{
		return false;
	}

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	UAnimInstance* AnimInstance = Character && Character->GetMesh()
		? Character->GetMesh()->GetAnimInstance()
		: nullptr;
	UAnimMontage* Montage = GetClimbMontage(EnterPhase);
	const TCHAR* EntryLabel =
		EnterPhase == EClimbPhase::Enter_From_Bottom ? TEXT("bottom") : TEXT("top");

	if (!IsValid(AnimInstance))
	{
		UE_LOG(
			Log_Climb_Ladder,
			Error,
			TEXT("[ClimbComponent] Cannot play %s-enter montage: AnimInstance is invalid. Character=%s Mesh=%s"),
			EntryLabel,
			*GetNameSafe(Character),
			*GetNameSafe(Character ? Character->GetMesh() : nullptr));
		return false;
	}

	if (!IsValid(Montage))
	{
		UE_LOG(
			Log_Climb_Ladder,
			Error,
			TEXT("[ClimbComponent] Cannot play %s-enter montage: phase '%s' is not configured in DataAsset '%s'."),
			EntryLabel,
			*UEnum::GetValueAsString(EnterPhase),
			*GetNameSafe(LadderClimbProfile));
		return false;
	}

	if (!UpdateEnterWarpTarget(EnterPhase))
	{
		UE_LOG(Log_Climb_Ladder, Error, TEXT("[ClimbComponent] %s-enter warp target could not be configured."), EntryLabel);
		return false;
	}

	AnimInstance->SetRootMotionMode(ERootMotionMode::RootMotionFromMontagesOnly);
	bEnterMontageActive = true;
	const float MontageDuration = AnimInstance->Montage_Play(Montage);
	if (MontageDuration <= 0.0f)
	{
		bEnterMontageActive = false;
		ClearTransitionWarpTargets();
		UE_LOG(Log_Climb_Ladder, Error, TEXT("[ClimbComponent] Failed to play %s-enter montage '%s'."), EntryLabel, *GetNameSafe(Montage));
		return false;
	}

	EnterClimbEndedDelegate.BindUObject(this, &UClimbComponent::OnEnterClimbMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EnterClimbEndedDelegate, Montage);
	return true;
}

bool UClimbComponent::UpdateEnterWarpTarget(EClimbPhase EnterPhase)
{
	ACharacterBase* Character = Cast<ACharacterBase>(GetOwner());
	UMotionWarpingComponent* MotionWarping = Character
		? Character->GetMotionWarpingComponent()
		: nullptr;
	const FName WarpTargetName = IsValid(LadderClimbProfile)
		? LadderClimbProfile->EnterWarpTargetName
		: NAME_None;
	if (!IsValid(MotionWarping) || WarpTargetName.IsNone() || !IsValid(ClimbObject))
	{
		return false;
	}

	FTransform AttachTransform;
	if (EnterPhase == EClimbPhase::Enter_From_Bottom ||
		EnterPhase == EClimbPhase::Enter_From_Top)
	{
		AttachTransform = FTransform(
			CalculateLadderAlignmentRotation(),
			ClimbLocation.Value);
	}
	else
	{
		return false;
	}

	// UE 5.4 Skew Warp interprets the translation target as the character's
	// capsule-bottom (feet) location, while AttachTransform represents
	// the desired actor/capsule-center transform.
	const float CapsuleHalfHeight =
		Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	AttachTransform.SetLocation(
		AttachTransform.GetLocation()
		- Character->GetActorUpVector() * CapsuleHalfHeight);

	MotionWarping->AddOrUpdateWarpTargetFromTransform(
		WarpTargetName,
		AttachTransform);

	const FVector LadderForward =
		ClimbObject->GetActorForwardVector().GetSafeNormal();
	const FVector LadderRight =
		ClimbObject->GetActorRightVector().GetSafeNormal();
	const FVector LadderUp =
		ClimbObject->GetActorUpVector().GetSafeNormal();
	for (const FLadderWarpCheckpoint& Checkpoint :
		LadderClimbProfile->EnterWarpCheckpoints)
	{
		if (Checkpoint.Phase != EnterPhase ||
			Checkpoint.TargetName.IsNone())
		{
			continue;
		}

		if (Checkpoint.TargetName == WarpTargetName)
		{
			UE_LOG(
				Log_Climb_Ladder,
				Warning,
				TEXT("[ClimbComponent] Warp checkpoint '%s' conflicts with the final entry target and was ignored."),
				*Checkpoint.TargetName.ToString());
			continue;
		}

		FTransform CheckpointTransform = AttachTransform;
		const FVector LocalOffset =
			Checkpoint.OffsetFromFinalBody;
		CheckpointTransform.SetLocation(
			AttachTransform.GetLocation() +
			LadderForward * LocalOffset.X +
			LadderRight * LocalOffset.Y +
			LadderUp * LocalOffset.Z);
		CheckpointTransform.SetRotation(
			AttachTransform.GetRotation() *
			Checkpoint.RotationOffset.Quaternion());

		MotionWarping->AddOrUpdateWarpTargetFromTransform(
			Checkpoint.TargetName,
			CheckpointTransform);
	}
	return true;
}

bool UClimbComponent::PlayExitMontage(EClimbPhase ExitPhase)
{
	const bool bTopExit =
		ExitPhase == EClimbPhase::Exit_From_Top_Left ||
		ExitPhase == EClimbPhase::Exit_From_Top_Right;
	const bool bBottomExit =
		ExitPhase == EClimbPhase::Exit_From_Bottom_Left ||
		ExitPhase == EClimbPhase::Exit_From_Bottom_Right;
	if (!bTopExit && !bBottomExit)
	{
		return false;
	}

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	UAnimInstance* AnimInstance = Character && Character->GetMesh()
		? Character->GetMesh()->GetAnimInstance()
		: nullptr;
	UAnimMontage* Montage = GetClimbMontage(ExitPhase);
	if (!IsValid(AnimInstance) || !IsValid(Montage))
	{
		UE_LOG(
			Log_Climb_Ladder,
			Error,
			TEXT("[ClimbComponent] Exit montage for '%s' is not configured in '%s'."),
			*UEnum::GetValueAsString(ExitPhase),
			*GetNameSafe(LadderClimbProfile));
		return false;
	}

	if (!UpdateExitWarpTarget(ExitPhase))
	{
		return false;
	}

	AnimInstance->SetRootMotionMode(
		ERootMotionMode::RootMotionFromMontagesOnly);
	bExitMontageActive = true;
	const float MontageDuration =
		AnimInstance->Montage_Play(Montage);
	if (MontageDuration <= 0.0f)
	{
		bExitMontageActive = false;
		ClearTransitionWarpTargets();
		return false;
	}

	ExitClimbEndedDelegate.BindUObject(
		this,
		&UClimbComponent::OnExitClimbMontageEnded);
	AnimInstance->Montage_SetEndDelegate(
		ExitClimbEndedDelegate,
		Montage);
	ExitClimbBlendingOutDelegate.BindUObject(
		this,
		&UClimbComponent::OnExitClimbMontageBlendingOut);
	AnimInstance->Montage_SetBlendingOutDelegate(
		ExitClimbBlendingOutDelegate,
		Montage);
	return true;
}

bool UClimbComponent::UpdateExitWarpTarget(EClimbPhase ExitPhase)
{
	ACharacterBase* Character = Cast<ACharacterBase>(GetOwner());
	UMotionWarpingComponent* MotionWarping = Character
		? Character->GetMotionWarpingComponent()
		: nullptr;
	if (!IsValid(Character) ||
		!IsValid(MotionWarping) ||
		!IsValid(ClimbObject) ||
		!IsValid(LadderClimbProfile) ||
		LadderClimbProfile->ExitWarpTargetName.IsNone())
	{
		return false;
	}

	const bool bTopExit =
		ExitPhase == EClimbPhase::Exit_From_Top_Left ||
		ExitPhase == EClimbPhase::Exit_From_Top_Right;
	const USceneComponent* ExitPoint = bTopExit
		? ClimbObject->GetTopExitTarget()
		: ClimbObject->GetInitEnterTarget(false);
	if (!IsValid(ExitPoint))
	{
		return false;
	}

	FTransform ExitTransform(
		bTopExit
			? ExitPoint->GetComponentRotation()
			: Character->GetActorRotation(),
		ClimbLocation.Value);
	const float CapsuleHalfHeight =
		Character->GetCapsuleComponent()->
			GetScaledCapsuleHalfHeight();
	ExitTransform.SetLocation(
		ExitTransform.GetLocation() -
		Character->GetActorUpVector() * CapsuleHalfHeight);

	MotionWarping->AddOrUpdateWarpTargetFromTransform(
		LadderClimbProfile->ExitWarpTargetName,
		ExitTransform);

	const FVector LadderForward =
		ClimbObject->GetActorForwardVector().GetSafeNormal();
	const FVector LadderRight =
		ClimbObject->GetActorRightVector().GetSafeNormal();
	const FVector LadderUp =
		ClimbObject->GetActorUpVector().GetSafeNormal();
	for (const FLadderWarpCheckpoint& Checkpoint :
		LadderClimbProfile->ExitWarpCheckpoints)
	{
		if (Checkpoint.Phase != ExitPhase ||
			Checkpoint.TargetName.IsNone() ||
			Checkpoint.TargetName ==
				LadderClimbProfile->ExitWarpTargetName)
		{
			continue;
		}

		const FVector LocalOffset =
			Checkpoint.OffsetFromFinalBody;
		FTransform CheckpointTransform = ExitTransform;
		CheckpointTransform.SetLocation(
			ExitTransform.GetLocation() +
			LadderForward * LocalOffset.X +
			LadderRight * LocalOffset.Y +
			LadderUp * LocalOffset.Z);
		CheckpointTransform.SetRotation(
			ExitTransform.GetRotation() *
			Checkpoint.RotationOffset.Quaternion());
		MotionWarping->AddOrUpdateWarpTargetFromTransform(
			Checkpoint.TargetName,
			CheckpointTransform);
	}

	return true;
}

void UClimbComponent::ClearTransitionWarpTargets()
{
	if (ACharacterBase* Character = Cast<ACharacterBase>(GetOwner()))
	{
		if (UMotionWarpingComponent* MotionWarping = Character->GetMotionWarpingComponent())
		{
			if (IsValid(LadderClimbProfile) && !LadderClimbProfile->EnterWarpTargetName.IsNone())
			{
				MotionWarping->RemoveWarpTarget(LadderClimbProfile->EnterWarpTargetName);
			}
			if (IsValid(LadderClimbProfile))
			{
				for (const FLadderWarpCheckpoint& Checkpoint :
					LadderClimbProfile->EnterWarpCheckpoints)
				{
					if (!Checkpoint.TargetName.IsNone())
					{
						MotionWarping->RemoveWarpTarget(
							Checkpoint.TargetName);
					}
				}
				if (!LadderClimbProfile->ExitWarpTargetName.IsNone())
				{
					MotionWarping->RemoveWarpTarget(
						LadderClimbProfile->ExitWarpTargetName);
				}
				for (const FLadderWarpCheckpoint& Checkpoint :
					LadderClimbProfile->ExitWarpCheckpoints)
				{
					if (!Checkpoint.TargetName.IsNone())
					{
						MotionWarping->RemoveWarpTarget(
							Checkpoint.TargetName);
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
	DrawDebugString(
		GetWorld(),
		ActorLocation + FVector(0.0f, 0.0f, 12.0f),
		TEXT("Capsule Center"),
		nullptr,
		FColor::White,
		Duration,
		false);

	if (IsValid(ClimbObject))
	{
		const FTransform LadderTransform = ClimbObject->GetActorTransform();
		DrawDebugCoordinateSystem(
			GetWorld(),
			LadderTransform.GetLocation(),
			LadderTransform.Rotator(),
			35.0f,
			false,
			Duration,
			0,
			1.5f);
	}

	struct FContactDebugEntry
	{
		ELimbList Limb;
		FName ContactSocket;
		FColor Color;
		const TCHAR* Label;
	};

	static const FContactDebugEntry Entries[] =
	{
		{ ELimbList::FootR, TEXT("Sole_R"), FColor::Red, TEXT("FootR") },
		{ ELimbList::FootL, TEXT("Sole_L"), FColor::Green, TEXT("FootL") },
		{ ELimbList::HandL, TEXT("Palm_L"), FColor::Yellow, TEXT("HandL") },
		{ ELimbList::HandR, TEXT("Palm_R"), FColor::Blue, TEXT("HandR") }
	};

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

		DrawDebugSphere(
			GetWorld(),
			TargetLocation,
			6.0f,
			12,
			Entry.Color,
			false,
			Duration,
			0,
			2.0f);
		DrawDebugSphere(
			GetWorld(),
			ContactLocation,
			4.0f,
			10,
			FColor::White,
			false,
			Duration,
			0,
			1.5f);
		DrawDebugLine(
			GetWorld(),
			ContactLocation,
			TargetLocation,
			Entry.Color,
			false,
			Duration,
			0,
			2.0f);
		DrawDebugString(
			GetWorld(),
			TargetLocation + FVector(0.0f, 0.0f, 8.0f),
			FString::Printf(TEXT("%s %.1fcm"), Entry.Label, ErrorDistance),
			nullptr,
			Entry.Color,
			Duration,
			false);
	}
#endif
}

bool UClimbComponent::ResolveGripPattern(
	const TMap<ELimbList, float>& HeightOffsets,
	ELimbList ReferenceLimb,
	bool bPreferTop,
	TMap<ELimbList, int32>& OutAssignment) const
{
	OutAssignment.Empty();

	static constexpr ELimbList RequiredLimbs[] =
	{
		ELimbList::HandL,
		ELimbList::HandR,
		ELimbList::FootL,
		ELimbList::FootR
	};

	if (!IsValid(LadderClimbProfile) ||
		ReferenceLimb == ELimbList::Body ||
		!HeightOffsets.Contains(ReferenceLimb))
	{
		UE_LOG(
			Log_Climb_Ladder,
			Error,
			TEXT("[ClimbComponent] Grip pattern is missing its reference limb %s (Profile=%s, EntryCount=%d)."),
			*UEnum::GetValueAsString(ReferenceLimb),
			*GetNameSafe(LadderClimbProfile),
			HeightOffsets.Num());
		return false;
	}

	for (const ELimbList Limb : RequiredLimbs)
	{
		if (!HeightOffsets.Contains(Limb))
		{
			UE_LOG(
				Log_Climb_Ladder,
				Error,
				TEXT("[ClimbComponent] Grip pattern in '%s' is missing %s."),
				*GetNameSafe(LadderClimbProfile),
				*UEnum::GetValueAsString(Limb));
			return false;
		}
	}

	const float ReferenceOffset = HeightOffsets[ReferenceLimb];
	const float MatchTolerance =
		FMath::Max(LadderClimbProfile->GripMatchTolerance, 0.0f);
	bool bFoundAssignment = false;
	float BestOccupiedBoundaryHeight = 0.0f;
	float BestScore = TNumericLimits<float>::Max();

	for (int32 ReferenceGripIndex = 0;
		ReferenceGripIndex < GripList1D.Num();
		++ReferenceGripIndex)
	{
		const FGripNode1D* ReferenceGrip =
			GetGripNode(ReferenceGripIndex);
		if (!ReferenceGrip)
		{
			continue;
		}

		TMap<ELimbList, int32> CandidateAssignment;
		float CandidateScore = 0.0f;
		bool bCandidateValid = true;

		for (const ELimbList Limb : RequiredLimbs)
		{
			const float DesiredRelativeHeight =
				HeightOffsets[Limb] - ReferenceOffset;
			int32 BestGripIndex = INDEX_NONE;
			float BestError = TNumericLimits<float>::Max();

			for (int32 GripIndex = 0; GripIndex < GripList1D.Num(); ++GripIndex)
			{
				const FGripNode1D* Grip = GetGripNode(GripIndex);
				if (!Grip)
				{
					continue;
				}

				const float ActualRelativeHeight =
					Grip->LocalPosition.Z -
					ReferenceGrip->LocalPosition.Z;
				const float Error = FMath::Abs(
					ActualRelativeHeight - DesiredRelativeHeight);
				if (Error < BestError)
				{
					BestError = Error;
					BestGripIndex = GripIndex;
				}
			}

			if (BestGripIndex == INDEX_NONE ||
				(BestError > MatchTolerance &&
					!FMath::IsNearlyEqual(
						BestError,
						MatchTolerance,
						KINDA_SMALL_NUMBER)))
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

		for (int32 LeftIndex = 0;
			LeftIndex < UE_ARRAY_COUNT(RequiredLimbs) &&
			bCandidateValid;
			++LeftIndex)
		{
			for (int32 RightIndex = LeftIndex + 1;
				RightIndex < UE_ARRAY_COUNT(RequiredLimbs);
				++RightIndex)
			{
				const ELimbList LeftLimb = RequiredLimbs[LeftIndex];
				const ELimbList RightLimb = RequiredLimbs[RightIndex];
				const float DesiredDifference =
					HeightOffsets[LeftLimb] -
					HeightOffsets[RightLimb];
				const int32 LeftGripIndex =
					CandidateAssignment[LeftLimb];
				const int32 RightGripIndex =
					CandidateAssignment[RightLimb];

				if (LeftGripIndex == RightGripIndex &&
					!FMath::IsNearlyZero(DesiredDifference))
				{
					bCandidateValid = false;
					break;
				}

				const float ActualDifference =
					GripList1D[LeftGripIndex].LocalPosition.Z -
					GripList1D[RightGripIndex].LocalPosition.Z;
				if (!FMath::IsNearlyZero(DesiredDifference) &&
					ActualDifference * DesiredDifference <= 0.0f)
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

		float OccupiedBoundaryHeight = bPreferTop
			? -TNumericLimits<float>::Max()
			: TNumericLimits<float>::Max();
		for (const TPair<ELimbList, int32>& Assignment :
			CandidateAssignment)
		{
			const float GripHeight =
				GripList1D[Assignment.Value].LocalPosition.Z;
			OccupiedBoundaryHeight = bPreferTop
				? FMath::Max(OccupiedBoundaryHeight, GripHeight)
				: FMath::Min(OccupiedBoundaryHeight, GripHeight);
		}

		const float RequiredBoundaryHeight = bPreferTop
			? GripList1D.Last().LocalPosition.Z
			: GripList1D[0].LocalPosition.Z;
		if (!FMath::IsNearlyEqual(
				OccupiedBoundaryHeight,
				RequiredBoundaryHeight))
		{
			continue;
		}

		const bool bMoreSuitableBoundary =
			!bFoundAssignment ||
			(bPreferTop
				? OccupiedBoundaryHeight >
					BestOccupiedBoundaryHeight
				: OccupiedBoundaryHeight <
					BestOccupiedBoundaryHeight);
		const bool bSameBoundaryButBetterFit =
			bFoundAssignment &&
			FMath::IsNearlyEqual(
				OccupiedBoundaryHeight,
				BestOccupiedBoundaryHeight) &&
			CandidateScore < BestScore;

		if (bMoreSuitableBoundary || bSameBoundaryButBetterFit)
		{
			OutAssignment = MoveTemp(CandidateAssignment);
			BestOccupiedBoundaryHeight = OccupiedBoundaryHeight;
			BestScore = CandidateScore;
			bFoundAssignment = true;
		}
	}

	if (!bFoundAssignment)
	{
		FString GripHeights;
		for (const FGripNode1D& Grip : GripList1D)
		{
			GripHeights += FString::Printf(
				TEXT("%.1f "),
				Grip.LocalPosition.Z);
		}

		UE_LOG(
			Log_Climb_Ladder,
			Error,
			TEXT("[ClimbComponent] No grip pattern matched within %.1fcm. Reference=%s Offsets=[HandL %.1f, HandR %.1f, FootL %.1f, FootR %.1f] GripZ=[%s]"),
			MatchTolerance,
			*UEnum::GetValueAsString(ReferenceLimb),
			HeightOffsets[ELimbList::HandL],
			HeightOffsets[ELimbList::HandR],
			HeightOffsets[ELimbList::FootL],
			HeightOffsets[ELimbList::FootR],
			*GripHeights);
	}

	return bFoundAssignment;
}

bool UClimbComponent::BuildGripRoute(
	UAnimMontage* Montage,
	const TMap<ELimbList, int32>& StartAssignment,
	const TMap<ELimbList, int32>& EndAssignment)
{
	PlannedLimbGripTargets.Empty();
	PlannedGripRouteEndAssignment.Empty();

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
		const UANS_LadderGripTransition* GripTransition =
			Cast<UANS_LadderGripTransition>(
				NotifyEvent.NotifyStateClass);
		if (!GripTransition || GripTransition->Limb == ELimbList::Body)
		{
			continue;
		}

		FGripRouteNotify& RouteNotify = RouteNotifies.AddDefaulted_GetRef();
		RouteNotify.TriggerTime = NotifyEvent.GetTriggerTime();
		RouteNotify.Limb = GripTransition->Limb;
		RouteNotify.Direction = GripTransition->Direction;
	}

	RouteNotifies.Sort(
		[](const FGripRouteNotify& Left, const FGripRouteNotify& Right)
		{
			return Left.TriggerTime < Right.TriggerTime;
		});

	TMap<ELimbList, TArray<ELadderGripDirection>> DirectionsByLimb;
	for (const FGripRouteNotify& RouteNotify : RouteNotifies)
	{
		DirectionsByLimb.FindOrAdd(RouteNotify.Limb).Add(
			RouteNotify.Direction);
	}

	for (const TPair<ELimbList, TArray<ELadderGripDirection>>& LimbRoute :
		DirectionsByLimb)
	{
		const int32* InitialGripIndex =
			StartAssignment.Find(LimbRoute.Key);
		const int32* FinalGripIndex = EndAssignment.Find(LimbRoute.Key);
		if (!InitialGripIndex ||
			!FinalGripIndex ||
			!GetGripNode(*InitialGripIndex) ||
			!GetGripNode(*FinalGripIndex) ||
			LimbRoute.Value.IsEmpty())
		{
			return false;
		}

		const float InitialHeight =
			GripList1D[*InitialGripIndex].LocalPosition.Z;
		const float FinalHeight =
			GripList1D[*FinalGripIndex].LocalPosition.Z;
		if (FMath::IsNearlyEqual(InitialHeight, FinalHeight))
		{
			UE_LOG(
				Log_Climb_Ladder,
				Error,
				TEXT("[ClimbComponent] %s has grip transition notifies but its initial and final grips are identical."),
				*UEnum::GetValueAsString(LimbRoute.Key));
			return false;
		}

		const ELadderGripDirection RequiredDirection =
			FinalHeight > InitialHeight
				? ELadderGripDirection::Up
				: ELadderGripDirection::Down;
		for (const ELadderGripDirection Direction : LimbRoute.Value)
		{
			if (Direction != RequiredDirection)
			{
				UE_LOG(
					Log_Climb_Ladder,
					Error,
					TEXT("[ClimbComponent] Grip transition direction for %s does not lead from its authored initial grip to its final idle grip."),
					*UEnum::GetValueAsString(LimbRoute.Key));
				return false;
			}
		}

		TArray<int32> GripPath;
		GripPath.Add(*InitialGripIndex);
		int32 CurrentGripIndex = *InitialGripIndex;
		while (CurrentGripIndex != *FinalGripIndex &&
			GripPath.Num() <= GripList1D.Num())
		{
			CurrentGripIndex = GetNeighborGripIndex(
				CurrentGripIndex,
				RequiredDirection == ELadderGripDirection::Up);
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
			UE_LOG(
				Log_Climb_Ladder,
				Error,
				TEXT("[ClimbComponent] %s has %d transition notifies but only %d grip step(s) between its initial and final targets."),
				*UEnum::GetValueAsString(LimbRoute.Key),
				TransitionCount,
				TotalGripSteps);
			return false;
		}

		TArray<int32>& PlannedTargets =
			PlannedLimbGripTargets.FindOrAdd(LimbRoute.Key);
		int32 PreviousTargetStep = 0;
		for (int32 TransitionIndex = 0;
			TransitionIndex < TransitionCount;
			++TransitionIndex)
		{
			const int32 RemainingTransitions =
				TransitionCount - TransitionIndex - 1;
			const int32 MinimumTargetStep =
				PreviousTargetStep + 1;
			const int32 MaximumTargetStep =
				TotalGripSteps - RemainingTransitions;
			const int32 EvenlyDistributedStep =
				FMath::RoundToInt(
					static_cast<float>(TotalGripSteps) *
					static_cast<float>(TransitionIndex + 1) /
					static_cast<float>(TransitionCount));
			const int32 TargetStep = FMath::Clamp(
				EvenlyDistributedStep,
				MinimumTargetStep,
				MaximumTargetStep);

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

		if (!DirectionsByLimb.Contains(Initial.Key) &&
			Initial.Value != *FinalGripIndex)
		{
			UE_LOG(
				Log_Climb_Ladder,
				Error,
				TEXT("[ClimbComponent] %s has different initial and final grips but no Ladder Grip Transition notify."),
				*UEnum::GetValueAsString(Initial.Key));
			return false;
		}
	}

	PlannedGripRouteEndAssignment = EndAssignment;
	return true;
}

bool UClimbComponent::BuildTopExitGripRoute(EClimbPhase ExitPhase)
{
	if (!IsValid(LadderClimbProfile) ||
		(ExitPhase != EClimbPhase::Exit_From_Top_Left &&
			ExitPhase != EClimbPhase::Exit_From_Top_Right))
	{
		return false;
	}

	TMap<ELimbList, int32> StartAssignment;
	static constexpr ELimbList LadderLimbs[] =
	{
		ELimbList::HandL,
		ELimbList::HandR,
		ELimbList::FootL,
		ELimbList::FootR
	};
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
	if (!ResolveGripPattern(
			LadderClimbProfile->TopEnterInitialGripHeightOffsets,
			LadderClimbProfile->TopEnterInitialReferenceLimb,
			true,
			CanonicalInitialAssignment))
	{
		return false;
	}

	TMap<ELimbList, int32> CanonicalIdleAssignment;
	if (!ResolveGripPattern(
			LadderClimbProfile->TopEnterIdleGripHeightOffsets,
			LadderClimbProfile->TopEnterIdleReferenceLimb,
			true,
			CanonicalIdleAssignment) ||
		!BuildGripRoute(
			GetClimbMontage(EClimbPhase::Enter_From_Top),
			CanonicalInitialAssignment,
			CanonicalIdleAssignment))
	{
		return false;
	}

	const TMap<ELimbList, TArray<int32>> CanonicalEntryTargets =
		PlannedLimbGripTargets;
	UAnimMontage* ExitMontage = GetClimbMontage(ExitPhase);
	if (!IsValid(ExitMontage))
	{
		return false;
	}

	TMap<ELimbList, int32> ExitNotifyCounts;
	for (const FAnimNotifyEvent& NotifyEvent : ExitMontage->Notifies)
	{
		const UANS_LadderGripTransition* GripTransition =
			Cast<UANS_LadderGripTransition>(
				NotifyEvent.NotifyStateClass);
		if (GripTransition &&
			GripTransition->Limb != ELimbList::Body)
		{
			ExitNotifyCounts.FindOrAdd(GripTransition->Limb)++;
		}
	}

	const bool bMirrorRoute =
		ExitPhase == EClimbPhase::Exit_From_Top_Right;
	const auto GetCanonicalLimb =
		[bMirrorRoute](ELimbList ExitLimb)
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
	for (const TPair<ELimbList, int32>& NotifyCount :
		ExitNotifyCounts)
	{
		const ELimbList ExitLimb = NotifyCount.Key;
		const ELimbList CanonicalLimb =
			GetCanonicalLimb(ExitLimb);
		const int32* CanonicalInitialGrip =
			CanonicalInitialAssignment.Find(CanonicalLimb);
		const int32* CanonicalIdleGrip =
			CanonicalIdleAssignment.Find(CanonicalLimb);
		const TArray<int32>* EntryTargets =
			CanonicalEntryTargets.Find(CanonicalLimb);
		const int32* CurrentExitGrip =
			StartAssignment.Find(ExitLimb);
		if (!CanonicalInitialGrip ||
			!CanonicalIdleGrip ||
			!CurrentExitGrip ||
			!EntryTargets)
		{
			return false;
		}

		TArray<int32> EntryPath;
		EntryPath.Reserve(EntryTargets->Num() + 1);
		EntryPath.Add(*CanonicalInitialGrip);
		EntryPath.Append(*EntryTargets);
		if (EntryPath.Last() != *CanonicalIdleGrip ||
			*CurrentExitGrip != *CanonicalIdleGrip ||
			NotifyCount.Value <= 0 ||
			NotifyCount.Value >= EntryPath.Num())
		{
			UE_LOG(
				Log_Climb_Ladder,
				Error,
				TEXT("[ClimbComponent] %s cannot consume %d reversed top-entry grip segment(s) from its current grip."),
				*UEnum::GetValueAsString(ExitLimb),
				NotifyCount.Value);
			return false;
		}

		const int32 ReversedTargetPathIndex =
			EntryPath.Num() - 1 - NotifyCount.Value;
		EndAssignment[ExitLimb] =
			EntryPath[ReversedTargetPathIndex];
	}

	return BuildGripRoute(
		ExitMontage,
		StartAssignment,
		EndAssignment);
}

bool UClimbComponent::ValidatePlannedGripRouteEnd(
	const TMap<ELimbList, int32>& ExpectedAssignment) const
{
	for (const TPair<ELimbList, int32>& Expected : ExpectedAssignment)
	{
		const FLimbData* LimbData = LimbToGripNode.Find(Expected.Key);
		const TArray<int32>* RemainingTargets =
			PlannedLimbGripTargets.Find(Expected.Key);
		if (!LimbData ||
			LimbData->LimbTargetGripIndex != Expected.Value ||
			ActiveLimbGripTransitions.Contains(Expected.Key) ||
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
	const FGripNode1D* HandLGrip = HandLData
		? GetGripNode(HandLData->LimbTargetGripIndex)
		: nullptr;
	const FGripNode1D* HandRGrip = HandRData
		? GetGripNode(HandRData->LimbTargetGripIndex)
		: nullptr;

	if (!HandLGrip || !HandRGrip)
	{
		UE_LOG(
			Log_Climb_Ladder,
			Warning,
			TEXT("[ClimbComponent] Cannot resolve ladder idle side because a hand grip is invalid. Falling back to Idle_Right."));
		return EClimbPhase::Idle_Right;
	}

	return HandLGrip->LocalPosition.Z > HandRGrip->LocalPosition.Z
		? EClimbPhase::Idle_Left
		: EClimbPhase::Idle_Right;
}

bool UClimbComponent::ValidateTopEnterFinalGripAssignment() const
{
	if (!IsValid(LadderClimbProfile))
	{
		return false;
	}

	TMap<ELimbList, int32> ExpectedAssignment;
	if (!ResolveGripPattern(
			LadderClimbProfile->TopEnterIdleGripHeightOffsets,
			LadderClimbProfile->TopEnterIdleReferenceLimb,
			true,
			ExpectedAssignment))
	{
		return false;
	}

	for (const TPair<ELimbList, int32>& Expected : ExpectedAssignment)
	{
		const FLimbData* LimbData = LimbToGripNode.Find(Expected.Key);
		const TArray<int32>* RemainingTargets =
			PlannedLimbGripTargets.Find(Expected.Key);
		if (!LimbData ||
			LimbData->LimbTargetGripIndex != Expected.Value ||
			ActiveLimbGripTransitions.Contains(Expected.Key) ||
			(RemainingTargets && !RemainingTargets->IsEmpty()))
		{
			return false;
		}
	}

	return true;
}

void UClimbComponent::ClimbUpLadder()
{
	if (bIsClimbing)
		return;

	FLimbData& HandLData = LimbToGripNode[ELimbList::HandL];
	FLimbData& HandRData = LimbToGripNode[ELimbList::HandR];
	FLimbData& FootLData = LimbToGripNode[ELimbList::FootL];
	FLimbData& FootRData = LimbToGripNode[ELimbList::FootR];
	const FGripNode1D* HandLGrip = GetGripNode(HandLData.LimbTargetGripIndex);
	const FGripNode1D* HandRGrip = GetGripNode(HandRData.LimbTargetGripIndex);
	if (!HandLGrip || !HandRGrip)
	{
		DeRegisterClimbObject();
		return;
	}

	const float Hand_L_By_LadderAxis = HandLGrip->LocalPosition.Z;
	const float Hand_R_By_LadderAxis = HandRGrip->LocalPosition.Z;

	const bool bClimbRight = Hand_L_By_LadderAxis > Hand_R_By_LadderAxis;

	const FVector CurrentLocation = GetOwner()->GetActorLocation();
	FVector NewTargetLocation = CurrentLocation;

	if (bClimbRight)
	{
		const int32 NewHandRIndex = GetNeighborGripIndex(HandLData.LimbTargetGripIndex, true);
		const int32 NewFootLIndex = GetNeighborGripIndex(FootRData.LimbTargetGripIndex, true);
		if (!GetGripNode(NewHandRIndex))
		{
			RequestExitLadder(true);
			return;
		}
		if (!GetGripNode(NewFootLIndex))
		{
			UE_LOG(Log_Climb_Ladder, Error, TEXT("[ClimbComponent] Invalid FootL Grip while climbing up."));
			return;
		}

		HandRData.PreviousGripIndex = HandRData.LimbTargetGripIndex;
		HandRData.LimbTargetGripIndex = NewHandRIndex;
		FootLData.PreviousGripIndex = FootLData.LimbTargetGripIndex;
		FootLData.LimbTargetGripIndex = NewFootLIndex;

		NewTargetLocation = CalculateBodyTargetLocation(CurrentLocation);

		LadderStance = EClimbPhase::ClimbUp_Right;
	}
	else
	{
		const int32 NewHandLIndex = GetNeighborGripIndex(HandRData.LimbTargetGripIndex, true);
		const int32 NewFootRIndex = GetNeighborGripIndex(FootLData.LimbTargetGripIndex, true);
		if (!GetGripNode(NewHandLIndex))
		{
			RequestExitLadder(true);
			return;
		}
		if (!GetGripNode(NewFootRIndex))
		{
			UE_LOG(Log_Climb_Ladder, Error, TEXT("[ClimbComponent] Invalid FootR Grip while climbing up."));
			return;
		}

		HandLData.PreviousGripIndex = HandLData.LimbTargetGripIndex;
		HandLData.LimbTargetGripIndex = NewHandLIndex;
		FootRData.PreviousGripIndex = FootRData.LimbTargetGripIndex;
		FootRData.LimbTargetGripIndex = NewFootRIndex;

		NewTargetLocation = CalculateBodyTargetLocation(CurrentLocation);

		LadderStance = EClimbPhase::ClimbUp_Left;
	}

	ClimbLocation = MakeTuple(CurrentLocation, NewTargetLocation);

	bIsClimbing = true;
	SetComponentTickEnabled(true);
}

void UClimbComponent::ClimbDownLadder()
{
	if (bIsClimbing)
		return;

	FLimbData& HandLData = LimbToGripNode[ELimbList::HandL];
	FLimbData& HandRData = LimbToGripNode[ELimbList::HandR];
	FLimbData& FootLData = LimbToGripNode[ELimbList::FootL];
	FLimbData& FootRData = LimbToGripNode[ELimbList::FootR];
	const FGripNode1D* HandLGrip = GetGripNode(HandLData.LimbTargetGripIndex);
	const FGripNode1D* HandRGrip = GetGripNode(HandRData.LimbTargetGripIndex);
	if (!HandLGrip || !HandRGrip)
	{
		DeRegisterClimbObject();
		return;
	}

	const float Hand_L_By_LadderAxis = HandLGrip->LocalPosition.Z;
	const float Hand_R_By_LadderAxis = HandRGrip->LocalPosition.Z;

	const bool bClimbRight = Hand_L_By_LadderAxis < Hand_R_By_LadderAxis;

	const FVector CurrentLocation = GetOwner()->GetActorLocation();
	FVector NewTargetLocation = CurrentLocation;

	if (bClimbRight)
	{
		const int32 NewFootLIndex = GetNeighborGripIndex(FootRData.LimbTargetGripIndex, false);
		const int32 NewHandRIndex = GetNeighborGripIndex(HandLData.LimbTargetGripIndex, false);
		if (!GetGripNode(NewFootLIndex))
		{
			RequestExitLadder(false);
			return;
		}
		if (!GetGripNode(NewHandRIndex))
		{
			UE_LOG(Log_Climb_Ladder, Error, TEXT("[ClimbComponent] Invalid HandR Grip while climbing down."));
			return;
		}

		FootLData.PreviousGripIndex = FootLData.LimbTargetGripIndex;
		FootLData.LimbTargetGripIndex = NewFootLIndex;
		HandRData.PreviousGripIndex = HandRData.LimbTargetGripIndex;
		HandRData.LimbTargetGripIndex = NewHandRIndex;

		NewTargetLocation = CalculateBodyTargetLocation(CurrentLocation);

		LadderStance = EClimbPhase::ClimbDown_Right;
	}
	else
	{
		const int32 NewFootRIndex = GetNeighborGripIndex(FootLData.LimbTargetGripIndex, false);
		const int32 NewHandLIndex = GetNeighborGripIndex(HandRData.LimbTargetGripIndex, false);
		if (!GetGripNode(NewFootRIndex))
		{
			RequestExitLadder(false);
			return;
		}
		if (!GetGripNode(NewHandLIndex))
		{
			UE_LOG(Log_Climb_Ladder, Error, TEXT("[ClimbComponent] Invalid HandL Grip while climbing down."));
			return;
		}

		FootRData.PreviousGripIndex = FootRData.LimbTargetGripIndex;
		FootRData.LimbTargetGripIndex = NewFootRIndex;
		HandLData.PreviousGripIndex = HandLData.LimbTargetGripIndex;
		HandLData.LimbTargetGripIndex = NewHandLIndex;

		NewTargetLocation = CalculateBodyTargetLocation(CurrentLocation);

		LadderStance = EClimbPhase::ClimbDown_Left;
	}

	ClimbLocation = MakeTuple(CurrentLocation, NewTargetLocation);

	bIsClimbing = true;
	SetComponentTickEnabled(true);
}

void UClimbComponent::OnEnterClimbMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	const bool bWasBottomEntry =
		LadderTransitionState == ELadderTransitionState::EnterBottom;
	const bool bWasTopEntry =
		LadderTransitionState == ELadderTransitionState::EnterTop;
	bEnterMontageActive = false;
	ClearTransitionWarpTargets();

	if (bInterrupted)
	{
		bool bBroadcastExit = true;
		if (const ACharacterBase* Character = Cast<ACharacterBase>(GetOwner()))
		{
			if (const UCharacterStatusComponent* StatusComponent =
				Character->GetCharacterStatusComponent())
			{
				bBroadcastExit =
					!StatusComponent->GetCurrentState().MatchesTagExact(TAG_State_Dead);
			}
		}

		ForceDetachFromLadder(bBroadcastExit);
		return;
	}

	if (bWasTopEntry && !ValidateTopEnterFinalGripAssignment())
	{
		UE_LOG(
			Log_Climb_Ladder,
			Error,
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
		const float CompletionError = FVector::Distance(
			GetOwner()->GetActorLocation(),
			ClimbLocation.Value);
		const float CompletionTolerance = IsValid(LadderClimbProfile)
			? FMath::Max(
				LadderClimbProfile->TopEnterCompletionTolerance,
				0.0f)
			: 0.0f;
		if (CompletionError > CompletionTolerance)
		{
			UE_LOG(
				Log_Climb_Ladder,
				Error,
				TEXT("[ClimbComponent] Top-entry motion warping ended %.1fcm from the final attach location (Tolerance=%.1fcm). End snap was rejected."),
				CompletionError,
				CompletionTolerance);
			ForceDetachFromLadder(true);
			return;
		}
	}

	GetOwner()->SetActorLocation(ClimbLocation.Value);
	LadderStance = ResolveIdlePhaseFromGripState();
	bIsClimbing = false;
	AnimTime = 0.0f;
	if (bWasBottomEntry)
	{
		DrawBottomEnterContactDebug();
	}
	CompleteLadderTransition();
}

void UClimbComponent::OnExitClimbMontageEnded(
	UAnimMontage* Montage,
	bool bInterrupted)
{
	bExitMontageActive = false;
	ClearTransitionWarpTargets();

	if (bInterrupted)
	{
		ForceDetachFromLadder(true);
		return;
	}

	const bool bWasTopExit =
		LadderTransitionState == ELadderTransitionState::ExitTop;
	if (bWasTopExit &&
		!ValidatePlannedGripRouteEnd(PlannedGripRouteEndAssignment))
	{
		UE_LOG(
			Log_Climb_Ladder,
			Error,
			TEXT("[ClimbComponent] Top-exit montage ended before its planned grip route was completed."));
		ForceDetachFromLadder(true);
		return;
	}

	const float CompletionError = FVector::Distance(
		GetOwner()->GetActorLocation(),
		ClimbLocation.Value);
	const float CompletionTolerance = IsValid(LadderClimbProfile)
		? FMath::Max(
			LadderClimbProfile->ExitCompletionTolerance,
			0.0f)
		: 0.0f;
	if (CompletionError > CompletionTolerance)
	{
		UE_LOG(
			Log_Climb_Ladder,
			Error,
			TEXT("[ClimbComponent] Ladder-exit motion warping ended %.1fcm from its landing target (Tolerance=%.1fcm)."),
			CompletionError,
			CompletionTolerance);
		ForceDetachFromLadder(true);
		return;
	}

	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		Character->GetCharacterMovement()->
			StopMovementImmediately();
	}

	GetOwner()->SetActorLocation(ClimbLocation.Value);
	if (bWasTopExit &&
		IsValid(ClimbObject) &&
		IsValid(ClimbObject->GetTopExitTarget()))
	{
		GetOwner()->SetActorRotation(
			ClimbObject->GetTopExitTarget()->
				GetComponentRotation());
	}

	CompleteLadderTransition();
}

void UClimbComponent::OnExitClimbMontageBlendingOut(
	UAnimMontage* Montage,
	bool bInterrupted)
{
	if (bInterrupted ||
		bExitVisualStateReleased ||
		(LadderTransitionState != ELadderTransitionState::ExitTop &&
			LadderTransitionState != ELadderTransitionState::ExitBottom))
	{
		return;
	}

	ResetLadderIKState(true);
	bExitVisualStateReleased = true;
	OnLadderExit.Broadcast();
}

FVector UClimbComponent::CalculateLadderAlignmentLocation(const ACharacter* Character) const
{
	if (!IsValid(ClimbObject) || !IsValid(Character))
	{
		return GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
	}

	const float CapsuleRadius = Character->GetCapsuleComponent()->GetScaledCapsuleRadius();
	return ClimbObject->GetActorLocation()
		+ ClimbObject->GetActorForwardVector() * (CapsuleRadius + LadderSurfaceClearance);
}

FRotator UClimbComponent::CalculateLadderAlignmentRotation() const
{
	return IsValid(ClimbObject)
		? (-ClimbObject->GetActorForwardVector()).Rotation()
		: GetOwner()->GetActorRotation();
}

FGripNode1D* UClimbComponent::GetGripNode(int32 GripIndex)
{
	return GripList1D.IsValidIndex(GripIndex) ? &GripList1D[GripIndex] : nullptr;
}

const FGripNode1D* UClimbComponent::GetGripNode(int32 GripIndex) const
{
	return GripList1D.IsValidIndex(GripIndex) ? &GripList1D[GripIndex] : nullptr;
}

int32 UClimbComponent::GetNeighborGripIndex(int32 GripIndex, bool bUp, int32 Count) const
{
	while (GripList1D.IsValidIndex(GripIndex) && Count-- > 0)
	{
		GripIndex = bUp
			? GripList1D[GripIndex].NeighborUp.NeighborIndex
			: GripList1D[GripIndex].NeighborDown.NeighborIndex;
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

FVector UClimbComponent::CalculateBodyTargetLocation(
	const FVector& FallbackLocation) const
{
	TMap<ELimbList, int32> CurrentGripAssignment;
	static constexpr ELimbList AnchorLimbs[] =
	{
		ELimbList::HandL,
		ELimbList::HandR,
		ELimbList::FootL,
		ELimbList::FootR
	};

	for (const ELimbList Limb : AnchorLimbs)
	{
		const FLimbData* LimbData = LimbToGripNode.Find(Limb);
		if (!LimbData)
		{
			return FallbackLocation;
		}
		CurrentGripAssignment.Add(Limb, LimbData->LimbTargetGripIndex);
	}

	return CalculateBodyTargetLocation(
		CurrentGripAssignment,
		FallbackLocation);
}

FVector UClimbComponent::CalculateBodyTargetLocation(
	const TMap<ELimbList, int32>& GripAssignment,
	const FVector& FallbackLocation) const
{
	if (!IsValid(ClimbObject) || !IsValid(LadderClimbProfile))
	{
		return FallbackLocation;
	}

	static constexpr ELimbList AnchorLimbs[] =
	{
		ELimbList::HandL,
		ELimbList::HandR,
		ELimbList::FootL,
		ELimbList::FootR
	};

	FVector GripCentroid = FVector::ZeroVector;
	for (const ELimbList Limb : AnchorLimbs)
	{
		const int32* GripIndex = GripAssignment.Find(Limb);
		if (!GripIndex || !GetGripNode(*GripIndex))
		{
			UE_LOG(
				Log_Climb_Ladder,
				Error,
				TEXT("[ClimbComponent] Cannot calculate body anchor because %s has no valid grip."),
				*UEnum::GetValueAsString(Limb));
			return FallbackLocation;
		}

		GripCentroid +=
			GetGripWorldPosition(*GripIndex);
	}
	GripCentroid /= static_cast<float>(UE_ARRAY_COUNT(AnchorLimbs));

	const FVector LadderForward =
		ClimbObject->GetActorForwardVector().GetSafeNormal();
	const FVector LadderRight =
		ClimbObject->GetActorRightVector().GetSafeNormal();
	const FVector LadderUp =
		ClimbObject->GetActorUpVector().GetSafeNormal();

	return GripCentroid +
		LadderForward *
			LadderClimbProfile->BodyAnchorForwardOffset +
		LadderRight *
			LadderClimbProfile->BodyAnchorRightOffset +
		LadderUp *
			LadderClimbProfile->BodyAnchorUpOffset;
}

void UClimbComponent::RegisterClimbObject(ALadderBase* Ladder)
{
	if (!IsValid(Ladder))
	{
		DeRegisterClimbObject();
		return;
	}

	ClimbObject = Ladder;
	GripList1D = ClimbObject->GetGripList1D();
	if (!GripList1D.IsEmpty())
	{
		SetGrip1DRelation(MinGripInterval, MaxGripInterval);

		const USceneComponent* InitBottomTarget = ClimbObject->GetInitEnterTarget(false);
		if (IsValid(InitBottomTarget))
		{
			const float BottomLocalHeight = ClimbObject->GetActorTransform()
				.InverseTransformPosition(InitBottomTarget->GetComponentLocation()).Z;
			SetLowestGrip1D(MinFirstGripHeight, BottomLocalHeight);
		}
	}
}

void UClimbComponent::DeRegisterClimbObject()
{
	ForceDetachFromLadder(false);
}

void UClimbComponent::SetMinFirstGripHeight(float MinValue)
{
	MinFirstGripHeight = MinValue;
}

void UClimbComponent::SetMinGripInterval(float MinInterval)
{
	MinGripInterval = MinInterval;
}

void UClimbComponent::SetMaxGripInterval(float MaxInterval)
{
	MaxGripInterval = MaxInterval;
}

FVector UClimbComponent::GetLimbIKTarget(ELimbList LimbName) const
{
	const FLimbData* LimbData = LimbToGripNode.Find(LimbName);
	if (!LimbData)
	{
		UE_LOG(Log_Climb_Ladder, Warning, TEXT("[ClimbComponent] Bone is not located on the ladder [Bone Name: %s]."), *UEnum::GetValueAsString(LimbName));
		return FVector::ZeroVector;
	}

	return LimbData->LimbLocation;
}

void UClimbComponent::ResetClimbState()
{
	bIsClimbing = false;
	AnimTime = 0.0f;
	LadderStance = ResolveIdlePhaseFromGripState();
	SetComponentTickEnabled(false);

	if (LadderTransitionState == ELadderTransitionState::EnterBottom ||
		LadderTransitionState == ELadderTransitionState::EnterTop)
	{
		CompleteLadderTransition();
	}

	LimbToGripNode[ELimbList::HandR].LimbLocation = SetBoneIKTargetLadder(LimbToGripNode[ELimbList::HandR].LimbTargetGripIndex, FVector(), -15.0f);
	LimbToGripNode[ELimbList::FootL].LimbLocation = SetBoneIKTargetLadder(LimbToGripNode[ELimbList::FootL].LimbTargetGripIndex, FVector(), 15.0f);
	LimbToGripNode[ELimbList::HandL].LimbLocation = SetBoneIKTargetLadder(LimbToGripNode[ELimbList::HandL].LimbTargetGripIndex, FVector(), 15.0f);
	LimbToGripNode[ELimbList::FootR].LimbLocation = SetBoneIKTargetLadder(LimbToGripNode[ELimbList::FootR].LimbTargetGripIndex, FVector(), -15.0f);
}

FVector UClimbComponent::SetBoneIKTargetLadder(int32 TargetGripIndex, const FVector CurveValue, float LimbXDistance, int32 StartGripIndex, float LimbYDistance)
{
	const FGripNode1D* TargetGrip = GetGripNode(TargetGripIndex);
	const FGripNode1D* StartGrip = GetGripNode(StartGripIndex);
	if (!TargetGrip || !IsValid(ClimbObject))
	{
		UE_LOG(Log_Climb_Ladder, Error, TEXT("[ClimbComponent] Cannot calculate ladder IK without a valid ladder and target grip."));
		return FVector::ZeroVector;
	}
	FVector TargetLoc;
	FVector LimbOffset = ClimbObject->GetActorRightVector() * LimbXDistance;

	if (StartGrip)
	{
		TargetLoc = FMath::Lerp(
			GetGripWorldPosition(StartGripIndex),
			GetGripWorldPosition(TargetGripIndex),
			CurveValue.Z) + LimbOffset;
		
		FVector ForwardVector = GetOwner()->GetActorForwardVector();
		FVector ForwardOffset = (ForwardVector * LimbYDistance) * CurveValue.Y;

		FVector RightVector = GetOwner()->GetActorRightVector();
		FVector RightOffset = RightVector * CurveValue.X;

		TargetLoc += ForwardOffset + RightOffset;
	}
	else
	{
		TargetLoc = GetGripWorldPosition(TargetGripIndex) + LimbOffset;
	}

	return TargetLoc;
}

FVector UClimbComponent::SetBoneIKTargetLadder(const FVector TargetLoc, const FVector CurveValue, const FVector StartLoc, float LimbXDistance, float LimbYDistance)
{
	FVector OutLoc;

	OutLoc = FMath::Lerp(StartLoc, TargetLoc, CurveValue.Z);

	FVector ForwardVector = GetOwner()->GetActorForwardVector();
	FVector ForwardOffset = (ForwardVector * LimbYDistance) * CurveValue.Y;

	FVector RightVector = GetOwner()->GetActorRightVector();
	FVector RightOffset = (RightVector * LimbXDistance) * CurveValue.X;

	OutLoc += ForwardOffset + RightOffset;

	return OutLoc;
}

void UClimbComponent::SetGrip1DRelation(float MinInterval, float MaxInterval)
{
	if (!CheckGripListValid())
		return;

	for (int32 i = 0; i < GripList1D.Num(); i++) 
	{
		GripList1D[i].NeighborDown = {};
		GripList1D[i].NeighborUp = {};
		int32 lowerindex = i - 1;
		int32 upperindex = i + 1;

		while (GripList1D.IsValidIndex(lowerindex))
		{
			float DistanceToLowerGrip = FVector::Dist(GetGripWorldPosition(i), GetGripWorldPosition(lowerindex));
			if (DistanceToLowerGrip >= MinInterval && DistanceToLowerGrip <= MaxInterval)
			{
				GripList1D[i].NeighborDown = { lowerindex, DistanceToLowerGrip };
				break;
			}
			else if (DistanceToLowerGrip < MinInterval)
			{
				lowerindex--;
				continue;
			}
			else
			{
				break;
			}
		}

		while (GripList1D.IsValidIndex(upperindex))
		{
			float DistanceToUpperGrip = FVector::Dist(GetGripWorldPosition(i), GetGripWorldPosition(upperindex));
			if (DistanceToUpperGrip >= MinInterval && DistanceToUpperGrip <= MaxInterval)
			{
				GripList1D[i].NeighborUp = { upperindex, DistanceToUpperGrip };
				break;
			}
			else if (DistanceToUpperGrip < MinInterval)
			{
				upperindex++;
				continue;
			}
			else
			{
				break;
			}
		}	
	}
}

bool UClimbComponent::CheckGripListValid()
{
	return GripList1D.IsEmpty() ? false : true;
}

void UClimbComponent::SetLowestGrip1D(float MinHeight, float Comparison)
{
	for (FGripNode1D& GripNode : GripList1D)
	{
		if (GripNode.LocalPosition.Z - Comparison > MinHeight)
		{
			GripNode.Tag.Add(FName("LowestGrip"));
			return;
		}
	}
}

int32 UClimbComponent::GetLowestGrip1DIndex() const
{
	if (GripList1D.IsEmpty())
	{
		return INDEX_NONE;
	}

	for (int32 GripIndex = 0; GripIndex < GripList1D.Num(); ++GripIndex)
	{
		if (GripList1D[GripIndex].Tag.Contains(FName("LowestGrip")))
		{
			return GripIndex;
		}
	}

	return 0;
}

int32 UClimbComponent::GetHighestGrip1DIndex() const
{
	return GripList1D.IsEmpty() ? INDEX_NONE : GripList1D.Num() - 1;
}
