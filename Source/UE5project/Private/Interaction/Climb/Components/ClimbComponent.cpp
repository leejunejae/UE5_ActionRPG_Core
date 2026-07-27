// Fill out your copyright notice in the Description page of Project Settings.


#include "Interaction/Climb/Components/ClimbComponent.h"
#include "Environment/Climbable/Ladder/LadderBase.h"
#include "Interaction/Interfaces/InteractInterface.h"
#include "Characters/Components/CharacterStatusComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Characters/CharacterBase.h"
#include "Animation/AnimInstance.h"
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

	static ConstructorHelpers::FObjectFinder<UCurveFloat> EnterRotatorCurve_Asset(TEXT("/Game/04_Animations/Player/Ladder/Ladder/Ladder_Curve/Player_Ladder_Enter_Top_Rotator.Player_Ladder_Enter_Top_Rotator"));
	if (EnterRotatorCurve_Asset.Succeeded())
	{
		EnterRotatorCurve = EnterRotatorCurve_Asset.Object;
	}
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

	if (bBottomEnterMontageActive)
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
		LadderStance == EClimbPhase::ClimbUp_OneStep ||
		LadderStance == EClimbPhase::ClimbDown_Right ||
		LadderStance == EClimbPhase::ClimbDown_Left ||
		LadderStance == EClimbPhase::ClimbDown_OneStep;

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
	case EClimbPhase::Enter_From_Bottom:
	{

		break;
	}
	case EClimbPhase::Enter_From_Top:
	{
		const USceneComponent* EnterTopPoint = ClimbObject->GetInitEnterTarget(true);
		if (!IsValid(EnterTopPoint) || !EnterRotatorCurve)
		{
			UE_LOG(Log_Climb_Ladder, Error, TEXT("[ClimbComponent] Top entry rotation data is invalid for '%s'."), *GetNameSafe(ClimbObject));
			break;
		}
		const FRotator StartRotator = EnterTopPoint->GetComponentRotation();
		const FRotator TargetRotator = CalculateLadderAlignmentRotation();
		const float StartYaw = StartRotator.Yaw > 0.0f ? FMath::Fmod(StartRotator.Yaw, 360.0f) : FMath::Fmod(StartRotator.Yaw, 360.0f) + 360.0f;
		const float TargetYaw = TargetRotator.Yaw > 0.0f ? FMath::Fmod(TargetRotator.Yaw, 360.0f) : FMath::Fmod(TargetRotator.Yaw, 360.0f) + 360.0f;
		float EnterRotation = EnterRotatorCurve->GetFloatValue(AnimTime);

		const float NewRotatorYaw = FMath::Lerp(StartYaw, TargetYaw, EnterRotation);
		const FRotator NewRotator = FRotator(GetOwner()->GetActorRotation().Pitch, NewRotatorYaw, GetOwner()->GetActorRotation().Roll);
		GetOwner()->SetActorRotation(NewRotator);

		const USceneComponent* InitLeftHandPoint = ClimbObject->GetTopEnterHandTarget(false);
		const USceneComponent* InitRightHandPoint = ClimbObject->GetTopEnterHandTarget(true);

		FVector HandRTarget = GetGripWorldPosition(LimbToGripNode[ELimbList::HandR].LimbTargetGripIndex);
		HandRTarget += ClimbObject->GetActorRightVector() * -15.0f;

		FVector HandLTarget = GetGripWorldPosition(LimbToGripNode[ELimbList::HandL].LimbTargetGripIndex);
		HandLTarget += ClimbObject->GetActorRightVector() * 15.0f;

		const int32 FootLTargetGripIndex = LimbToGripNode[ELimbList::FootL].LimbTargetGripIndex;
		const FGripNode1D* FootLTargetGrip = GetGripNode(FootLTargetGripIndex);

		if (UCurveVector* HandCurve = GetClimbCurve(FClimbCurveKey{ LadderStance, ELimbList::HandR }))
		{
			HandCurveValue = HandCurve->GetVectorValue(AnimTime);
		}

		if (UCurveVector* FootCurve = GetClimbCurve(FClimbCurveKey{ LadderStance, ELimbList::FootL }))
		{
			FootCurveValue = FootCurve->GetVectorValue(AnimTime);
		}

		FVector HandLCurveValue;

		if (UCurveVector* HandLCurve = GetClimbCurve(FClimbCurveKey{ LadderStance, ELimbList::HandR }))
		{
			HandLCurveValue = HandLCurve->GetVectorValue(AnimTime);
		}

		LimbToGripNode[ELimbList::HandR].LimbLocation = SetBoneIKTargetLadder(HandRTarget, HandCurveValue, InitRightHandPoint->GetComponentLocation());
		LimbToGripNode[ELimbList::HandL].LimbLocation = SetBoneIKTargetLadder(HandLTarget, HandLCurveValue, InitLeftHandPoint->GetComponentLocation());
		LimbToGripNode[ELimbList::FootL].LimbLocation = SetBoneIKTargetLadder(
			FootLTargetGripIndex, FootCurveValue, 15.0f, LimbToGripNode[ELimbList::FootL].PreviousGripIndex);


		if (!GetGripNode(LimbToGripNode[ELimbList::FootL].PreviousGripIndex))
			UE_LOG(Log_Anim_IK_Climb, Log, TEXT("[ClimbComponent] Prev nullptr"));

		break;
	}
	case EClimbPhase::Exit_From_Bottom_Right:
	case EClimbPhase::Exit_From_Bottom_Left:
	{

		break;
	}
	case EClimbPhase::Exit_From_Top_Right:
	case EClimbPhase::Exit_From_Top_Left:
	{

		break;
	}
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

	//ResetClimbState();

	FVector InitCharacterPosition = CalculateLadderAlignmentLocation(Character);

	const bool bEnterFromBottom = ClimbPoint->ComponentHasTag("Bottom");
	if (bEnterFromBottom)
	{
		TMap<ELimbList, int32> GripAssignment;
		if (!ResolveBottomEnterGripAssignment(GripAssignment))
		{
			UE_LOG(
				Log_Climb_Ladder,
				Error,
				TEXT("[ClimbComponent] Ladder '%s' has no valid bottom-entry grip assignment."),
				*GetNameSafe(TargetLadder));
			DeRegisterClimbObject();
			return false;
		}

		const int32 FootRGripIndex = GripAssignment[ELimbList::FootR];
		const int32 FootLGripIndex = GripAssignment[ELimbList::FootL];
		const int32 HandLGripIndex = GripAssignment[ELimbList::HandL];
		const int32 HandRGripIndex = GripAssignment[ELimbList::HandR];

		LimbToGripNode.Add(ELimbList::FootR, FLimbData(FootRGripIndex, SetBoneIKTargetLadder(FootRGripIndex, FVector(), -15.0f)));
		LimbToGripNode.Add(ELimbList::FootL, FLimbData(FootLGripIndex, SetBoneIKTargetLadder(FootLGripIndex, FVector(), 15.0f)));
		LimbToGripNode.Add(ELimbList::HandL, FLimbData(HandLGripIndex, SetBoneIKTargetLadder(HandLGripIndex, FVector(), 15.0f)));
		LimbToGripNode.Add(ELimbList::HandR, FLimbData(HandRGripIndex, SetBoneIKTargetLadder(HandRGripIndex, FVector(), -15.0f)));
		LimbToGripNode.Add(ELimbList::Body, FLimbData(INDEX_NONE, GetOwner()->GetActorLocation()));
		LadderStance = EClimbPhase::Enter_From_Bottom;
	}
	else
	{
		const USceneComponent* InitLeftHandPoint = Ladder->GetTopEnterHandTarget(false);
		const USceneComponent* InitRightHandPoint = Ladder->GetTopEnterHandTarget(true);
		const int32 HandLGripIndex = GetHighestGrip1DIndex();
		const int32 HandRGripIndex = GetNeighborGripIndex(HandLGripIndex, false);
		const int32 FootRGripIndex = GetNeighborGripIndex(HandRGripIndex, false);
		const int32 FootLGripIndex = GetNeighborGripIndex(FootRGripIndex, false);
		if (!IsValid(InitLeftHandPoint) || !IsValid(InitRightHandPoint) ||
			!GetGripNode(HandLGripIndex) || !GetGripNode(HandRGripIndex) ||
			!GetGripNode(FootRGripIndex) || !GetGripNode(FootLGripIndex))
		{
			UE_LOG(Log_Climb_Ladder, Error, TEXT("[ClimbComponent] Ladder '%s' has invalid top-entry data."), *GetNameSafe(TargetLadder));
			DeRegisterClimbObject();
			return false;
		}

		LimbToGripNode.Add(ELimbList::HandL, FLimbData(HandLGripIndex, InitLeftHandPoint->GetComponentLocation()));
		LimbToGripNode.Add(ELimbList::HandR, FLimbData(HandRGripIndex, InitRightHandPoint->GetComponentLocation()));
		LimbToGripNode.Add(ELimbList::FootR, FLimbData(FootRGripIndex, SetBoneIKTargetLadder(FootRGripIndex, FVector(), -15.0f)));
		LimbToGripNode.Add(ELimbList::FootL, FLimbData(FootLGripIndex, SetBoneIKTargetLadder(FootLGripIndex, FVector(), 15.0f, HandRGripIndex), HandRGripIndex));
		LimbToGripNode.Add(ELimbList::Body, FLimbData(INDEX_NONE, GetOwner()->GetActorLocation()));

		LadderStance = EClimbPhase::Enter_From_Top;
	}

	if (bEnterFromBottom)
	{
		InitCharacterPosition = CalculateBottomAttachTransform(Ladder).GetLocation();
	}
	else
	{
		InitCharacterPosition = CalculateBodyTargetLocation(
			LimbToGripNode[ELimbList::FootL].LimbTargetGripIndex,
			LimbToGripNode[ELimbList::HandL].LimbTargetGripIndex,
			InitCharacterPosition);
	}

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

	if (bEnterFromBottom)
	{
		if (!PlayBottomEnterMontage())
		{
			UE_LOG(
				Log_Climb_Ladder,
				Error,
				TEXT("[ClimbComponent] Bottom ladder entry was cancelled because its transition montage could not start."));
			ForceDetachFromLadder(false);
			return false;
		}

		SetComponentTickEnabled(false);
	}
	else
	{
		SetComponentTickEnabled(true);
	}

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
		const USceneComponent* ExitPoint = ClimbObject->GetInitEnterTarget(true);
		FVector ExitLocation = ExitPoint->GetComponentLocation();
		ExitLocation.Z += Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

		ClimbLocation = MakeTuple(GetOwner()->GetActorLocation(), ExitLocation);

		LadderStance = EClimbPhase::Exit_From_Top_Left;
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

		FVector TraceVec = EndLoc - StartLoc;
		FVector Center = StartLoc + TraceVec * 0.5f;
		float DebugHalfHeight = FVector::Dist(StartLoc, EndLoc) * 0.5f;
		FQuat CapsuleRot = FRotationMatrix::MakeFromZ(TraceVec).ToQuat();
		FColor DrawColor = bHit ? FColor::Green : FColor::Red;
		float DebugLifeTime = 5.0f;

		DrawDebugCapsule(
			GetWorld(),
			Center,
			DebugHalfHeight,
			20.0f,
			CapsuleRot,
			DrawColor,
			false,
			DebugLifeTime
		);

		if (!bHit)
		{
			UE_LOG(Log_Climb_Ladder, Warning, TEXT("[ClimbComponent] Failed to find a valid ladder exit location."));
			return false;
		}

		FVector ExitLocation = HitResult.ImpactPoint;
		ExitLocation.Z += Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		ClimbLocation = MakeTuple(GetOwner()->GetActorLocation(), ExitLocation);

		LadderStance = EClimbPhase::Exit_From_Bottom_Left;
	}
	if (!BeginLadderTransition(bExitTop
		? ELadderTransitionState::ExitTop
		: ELadderTransitionState::ExitBottom))
	{
		return false;
	}

	bIsClimbing = true;
	SetComponentTickEnabled(true);

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
		RestoreCharacterState();
		ClearLadderSession();
		OnLadderExit.Broadcast();
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
	bBottomEnterMontageActive = false;
	LimbToGripNode.Empty();
	GripList1D.Empty();
	ClimbObject = nullptr;
	bIsClimbing = false;
	AnimTime = 0.0f;
	LadderStance = EClimbPhase::Idle;
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

	if (bBroadcastExit)
	{
		OnLadderExit.Broadcast();
	}
}

void UClimbComponent::HandleOwnerDeathStarted()
{
	ForceDetachFromLadder(false);
}

bool UClimbComponent::PlayBottomEnterMontage()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	UAnimInstance* AnimInstance = Character && Character->GetMesh()
		? Character->GetMesh()->GetAnimInstance()
		: nullptr;
	UAnimMontage* Montage = GetClimbMontage(EClimbPhase::Enter_From_Bottom);

	if (!IsValid(AnimInstance))
	{
		UE_LOG(
			Log_Climb_Ladder,
			Error,
			TEXT("[ClimbComponent] Cannot play bottom-enter montage: AnimInstance is invalid. Character=%s Mesh=%s"),
			*GetNameSafe(Character),
			*GetNameSafe(Character ? Character->GetMesh() : nullptr));
		return false;
	}

	if (!IsValid(Montage))
	{
		UE_LOG(
			Log_Climb_Ladder,
			Error,
			TEXT("[ClimbComponent] Cannot play bottom-enter montage: Enter_From_Bottom is not configured in DataAsset '%s'."),
			*GetNameSafe(LadderClimbProfile));
		return false;
	}

	if (!UpdateBottomEnterWarpTarget())
	{
		UE_LOG(Log_Climb_Ladder, Error, TEXT("[ClimbComponent] Bottom-enter warp target could not be configured."));
		return false;
	}

	AnimInstance->SetRootMotionMode(ERootMotionMode::RootMotionFromMontagesOnly);
	bBottomEnterMontageActive = true;
	const float MontageDuration = AnimInstance->Montage_Play(Montage);
	if (MontageDuration <= 0.0f)
	{
		bBottomEnterMontageActive = false;
		ClearTransitionWarpTargets();
		UE_LOG(Log_Climb_Ladder, Error, TEXT("[ClimbComponent] Failed to play bottom-enter montage '%s'."), *GetNameSafe(Montage));
		return false;
	}

	EnterClimbEndedDelegate.BindUObject(this, &UClimbComponent::OnEnterClimbMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EnterClimbEndedDelegate, Montage);
	return true;
}

bool UClimbComponent::UpdateBottomEnterWarpTarget()
{
	ACharacterBase* Character = Cast<ACharacterBase>(GetOwner());
	UMotionWarpingComponent* MotionWarping = Character
		? Character->GetMotionWarpingComponent()
		: nullptr;
	const FName WarpTargetName = IsValid(LadderClimbProfile)
		? LadderClimbProfile->BottomEnterWarpTargetName
		: NAME_None;
	if (!IsValid(MotionWarping) || WarpTargetName.IsNone())
	{
		return false;
	}

	FTransform BottomAttachTransform = CalculateBottomAttachTransform(ClimbObject);

	// UE 5.4 Skew Warp interprets the translation target as the character's
	// capsule-bottom (feet) location, while BottomAttachTransform represents
	// the desired actor/capsule-center transform.
	const float CapsuleHalfHeight =
		Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	BottomAttachTransform.SetLocation(
		BottomAttachTransform.GetLocation()
		- Character->GetActorUpVector() * CapsuleHalfHeight);

	MotionWarping->AddOrUpdateWarpTargetFromTransform(
		WarpTargetName,
		BottomAttachTransform);
	return true;
}

void UClimbComponent::ClearTransitionWarpTargets()
{
	if (ACharacterBase* Character = Cast<ACharacterBase>(GetOwner()))
	{
		if (UMotionWarpingComponent* MotionWarping = Character->GetMotionWarpingComponent())
		{
			if (IsValid(LadderClimbProfile) && !LadderClimbProfile->BottomEnterWarpTargetName.IsNone())
			{
				MotionWarping->RemoveWarpTarget(LadderClimbProfile->BottomEnterWarpTargetName);
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

bool UClimbComponent::ResolveBottomEnterGripAssignment(
	TMap<ELimbList, int32>& OutAssignment) const
{
	OutAssignment.Empty();

	static const TArray<ELimbList> DefaultOrder =
	{
		ELimbList::FootR,
		ELimbList::FootL,
		ELimbList::HandL,
		ELimbList::HandR
	};

	const TArray<ELimbList>& GripOrder =
		LadderClimbProfile && LadderClimbProfile->BottomEnterGripOrder.Num() == 4
			? LadderClimbProfile->BottomEnterGripOrder
			: DefaultOrder;

	TSet<ELimbList> UniqueLimbs;
	for (const ELimbList Limb : GripOrder)
	{
		if (Limb == ELimbList::Body || UniqueLimbs.Contains(Limb))
		{
			UE_LOG(
				Log_Climb_Ladder,
				Error,
				TEXT("[ClimbComponent] BottomEnterGripOrder must contain each hand and foot exactly once."));
			return false;
		}
		UniqueLimbs.Add(Limb);
	}

	int32 GripIndex = GetLowestGrip1DIndex();
	for (const ELimbList Limb : GripOrder)
	{
		if (!GetGripNode(GripIndex))
		{
			return false;
		}

		OutAssignment.Add(Limb, GripIndex);
		GripIndex = GetNeighborGripIndex(GripIndex, true);
	}

	return
		OutAssignment.Contains(ELimbList::FootR) &&
		OutAssignment.Contains(ELimbList::FootL) &&
		OutAssignment.Contains(ELimbList::HandL) &&
		OutAssignment.Contains(ELimbList::HandR);
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

		NewTargetLocation = CalculateBodyTargetLocation(
			FootRData.LimbTargetGripIndex, HandRData.LimbTargetGripIndex, CurrentLocation);

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

		NewTargetLocation = CalculateBodyTargetLocation(
			FootLData.LimbTargetGripIndex, HandLData.LimbTargetGripIndex, CurrentLocation);

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

		NewTargetLocation = CalculateBodyTargetLocation(
			FootLData.LimbTargetGripIndex, HandLData.LimbTargetGripIndex, CurrentLocation);

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

		NewTargetLocation = CalculateBodyTargetLocation(
			FootRData.LimbTargetGripIndex, HandRData.LimbTargetGripIndex, CurrentLocation);

		LadderStance = EClimbPhase::ClimbDown_Left;
	}

	ClimbLocation = MakeTuple(CurrentLocation, NewTargetLocation);

	bIsClimbing = true;
	SetComponentTickEnabled(true);
}

void UClimbComponent::OnEnterClimbMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	bBottomEnterMontageActive = false;
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

	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		Character->GetCharacterMovement()->StopMovementImmediately();
	}

	GetOwner()->SetActorLocation(ClimbLocation.Value);
	LadderStance = EClimbPhase::Idle;
	bIsClimbing = false;
	AnimTime = 0.0f;
	DrawBottomEnterContactDebug();
	CompleteLadderTransition();
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

FTransform UClimbComponent::CalculateBottomAttachTransform(ALadderBase* Ladder) const
{
	if (!IsValid(Ladder) ||
		!IsValid(LadderClimbProfile))
	{
		return FTransform::Identity;
	}

	FTransform AttachTransform = Ladder->GetBottomAttachBaseTransform();
	const FVector ProfileOffset = LadderClimbProfile->BottomEnterBodyOffset;
	const FVector AttachLocation =
		AttachTransform.GetLocation() +
		Ladder->GetActorForwardVector().GetSafeNormal() * ProfileOffset.X +
		Ladder->GetActorRightVector().GetSafeNormal() * ProfileOffset.Y +
		Ladder->GetActorUpVector().GetSafeNormal() * ProfileOffset.Z;

	AttachTransform.SetLocation(AttachLocation);
	return AttachTransform;
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
	int32 FootGripIndex,
	int32 HandGripIndex,
	const FVector& CurrentLocation) const
{
	if (!IsValid(ClimbObject) || !GetGripNode(FootGripIndex) || !GetGripNode(HandGripIndex))
	{
		return CurrentLocation;
	}

	const FVector FootPosition = GetGripWorldPosition(FootGripIndex);
	const FVector HandPosition = GetGripWorldPosition(HandGripIndex);
	const FVector LadderLocation = ClimbObject->GetActorLocation();
	const FVector LadderUpVector = ClimbObject->GetActorUpVector().GetSafeNormal();
	const float BodyAnchorUpOffset =
		IsValid(LadderClimbProfile) ? LadderClimbProfile->BodyAnchorUpOffset : 3.0f;
	const float TargetAxisPosition = FVector::DotProduct(
		(FootPosition + HandPosition) * 0.5f - LadderLocation,
		LadderUpVector) + BodyAnchorUpOffset;
	const float CurrentAxisPosition = FVector::DotProduct(
		CurrentLocation - LadderLocation,
		LadderUpVector);

	return CurrentLocation + LadderUpVector * (TargetAxisPosition - CurrentAxisPosition);
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
	LadderStance = EClimbPhase::Idle;
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
