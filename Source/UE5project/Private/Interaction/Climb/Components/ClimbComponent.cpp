// Fill out your copyright notice in the Description page of Project Settings.


#include "Interaction/Climb/Components/ClimbComponent.h"
#include "Environment/Climbable/Interfaces/ClimbObjectInterface.h"
#include "Interaction/Climb/Interfaces/LadderInterface.h"
#include "Interaction/Interfaces/InteractInterface.h"
#include "Characters/Components/CharacterStatusComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Characters/CharacterBase.h"

#include "Utils/CoreLog.h"
#include "Utils/GameplayTagsBase.h"

// Sets default values for this component's properties
UClimbComponent::UClimbComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	static ConstructorHelpers::FObjectFinder<ULadderClimbDataAsset> LadderClimbDA_Asset(TEXT("/Game/04_Animations/Player/Ladder/Ladder/Ladder_Curve/ClimbCurveSet.ClimbCurveSet"));
	if (LadderClimbDA_Asset.Succeeded())
	{
		ClimbCurveDA = LadderClimbDA_Asset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UCurveFloat> EnterRotatorCurve_Asset(TEXT("/Game/04_Animations/Player/Ladder/Ladder/Ladder_Curve/Player_Ladder_Enter_Top_Rotator.Player_Ladder_Enter_Top_Rotator"));
	if (EnterRotatorCurve_Asset.Succeeded())
	{
		EnterRotatorCurve = EnterRotatorCurve_Asset.Object;
	}
	// ...
}


// Called when the game starts
void UClimbComponent::BeginPlay()
{
	Super::BeginPlay();
	// ..
}

UCurveVector* UClimbComponent::GetClimbCurve(const FClimbCurveKey& Key) const
{
	if (!ClimbCurveDA) return nullptr;
	return ClimbCurveDA->Curves.FindRef(Key);
}

UAnimMontage* UClimbComponent::GetClimbMontage(EClimbPhase Phase) const
{
	if (!ClimbCurveDA) return nullptr;
	return ClimbCurveDA->Montages.FindRef(Phase);
}

void UClimbComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsClimbing) return;

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
		const USceneComponent* EnterTopPoint = ILadderInterface::Execute_GetInitEnterTarget(ClimbObject, true);
		if (!IsValid(EnterTopPoint) || !EnterRotatorCurve)
		{
			UE_LOG(LogTemp, Error, TEXT("[ClimbComponent] Top entry rotation data is invalid for '%s'."), *GetNameSafe(ClimbObject));
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

		const USceneComponent* InitLeftHandPoint = ILadderInterface::Execute_GetTopEnterHandTarget(ClimbObject, false);
		const USceneComponent* InitRightHandPoint = ILadderInterface::Execute_GetTopEnterHandTarget(ClimbObject, true);

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
			//UE_LOG(Log_Anim_IK_Climb, Log, TEXT("[AnimTime : %f], [CurveValue : %f]"), AnimTime, FootCurveValue.Z);
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

	//UE_LOG(LogTemp, Warning, TEXT("[AnimTime : %f], [CurveValue : %f]"), AnimTime, BodyCurveValue.Z);
}


bool UClimbComponent::RequestEnterLadder(AActor* TargetLadder)
{
	if (!IsValid(TargetLadder) ||
		!TargetLadder->GetClass()->ImplementsInterface(UClimbObjectInterface::StaticClass()) ||
		!TargetLadder->GetClass()->ImplementsInterface(ULadderInterface::StaticClass()) ||
		!TargetLadder->GetClass()->ImplementsInterface(UInteractInterface::StaticClass()))
	{
		UE_LOG(LogTemp, Error, TEXT("[ClimbComponent] Invalid ladder target: %s"), *GetNameSafe(TargetLadder));
		return false;
	}

	RegisterClimbObject(TargetLadder);
	if (GripList1D.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[ClimbComponent] Ladder '%s' has no Grip sockets."), *GetNameSafe(TargetLadder));
		DeRegisterClimbObject();
		return false;
	}

	USceneComponent* ClimbPoint = IInteractInterface::Execute_GetEnterInteractLocation(TargetLadder, GetOwner());
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!IsValid(ClimbPoint) || !IsValid(Character))
	{
		UE_LOG(LogTemp, Error, TEXT("[ClimbComponent] Ladder '%s' is missing a required entry component."), *GetNameSafe(TargetLadder));
		DeRegisterClimbObject();
		return false;
	}

	//ResetClimbState();

	FVector InitCharacterPosition = CalculateLadderAlignmentLocation(Character);

	if (ClimbPoint->ComponentHasTag("Bottom"))
	{
		const int32 FootLGripIndex = GetLowestGrip1DIndex();
		const int32 FootRGripIndex = GetNeighborGripIndex(FootLGripIndex, true);
		const int32 HandRGripIndex = GetNeighborGripIndex(FootRGripIndex, true);
		const int32 HandLGripIndex = GetNeighborGripIndex(HandRGripIndex, true);
		if (!GetGripNode(FootLGripIndex) || !GetGripNode(FootRGripIndex) ||
			!GetGripNode(HandRGripIndex) || !GetGripNode(HandLGripIndex))
		{
			UE_LOG(LogTemp, Error, TEXT("[ClimbComponent] Ladder '%s' has no valid four-grip chain from the bottom."), *GetNameSafe(TargetLadder));
			DeRegisterClimbObject();
			return false;
		}

		LimbToGripNode.Add(ELimbList::FootL, FLimbData(FootLGripIndex, SetBoneIKTargetLadder(FootLGripIndex, FVector(), 15.0f)));
		LimbToGripNode.Add(ELimbList::FootR, FLimbData(FootRGripIndex, SetBoneIKTargetLadder(FootRGripIndex, FVector(), -15.0f)));
		LimbToGripNode.Add(ELimbList::HandR, FLimbData(HandRGripIndex, SetBoneIKTargetLadder(HandRGripIndex, FVector(), -15.0f)));
		LimbToGripNode.Add(ELimbList::HandL, FLimbData(HandLGripIndex, SetBoneIKTargetLadder(HandLGripIndex, FVector(), 15.0f)));
		LimbToGripNode.Add(ELimbList::Body, FLimbData(INDEX_NONE, GetOwner()->GetActorLocation()));
		LadderStance = EClimbPhase::Enter_From_Bottom;
	}
	else
	{
		//GetOwner()->GetCapsuleComponent()->SetCapsuleHalfHeight(50.0f);
		const USceneComponent* InitLeftHandPoint = ILadderInterface::Execute_GetTopEnterHandTarget(TargetLadder, false);
		const USceneComponent* InitRightHandPoint = ILadderInterface::Execute_GetTopEnterHandTarget(TargetLadder, true);
		const int32 HandLGripIndex = GetHighestGrip1DIndex();
		const int32 HandRGripIndex = GetNeighborGripIndex(HandLGripIndex, false);
		const int32 FootRGripIndex = GetNeighborGripIndex(HandRGripIndex, false);
		const int32 FootLGripIndex = GetNeighborGripIndex(FootRGripIndex, false);
		if (!IsValid(InitLeftHandPoint) || !IsValid(InitRightHandPoint) ||
			!GetGripNode(HandLGripIndex) || !GetGripNode(HandRGripIndex) ||
			!GetGripNode(FootRGripIndex) || !GetGripNode(FootLGripIndex))
		{
			UE_LOG(LogTemp, Error, TEXT("[ClimbComponent] Ladder '%s' has invalid top-entry data."), *GetNameSafe(TargetLadder));
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

	InitCharacterPosition = CalculateBodyTargetLocation(
		LimbToGripNode[ELimbList::FootL].LimbTargetGripIndex,
		LimbToGripNode[ELimbList::HandL].LimbTargetGripIndex,
		InitCharacterPosition);

	ClimbLocation = MakeTuple(GetOwner()->GetActorLocation(), InitCharacterPosition);
	Character->GetCapsuleComponent()->IgnoreActorWhenMoving(TargetLadder, true);

	SetComponentTickEnabled(true);
	bIsClimbing = true;

	EnterLadderFloat();

	return true;
}

bool UClimbComponent::RequestExitLadder(bool bExitTop)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());

	if (bExitTop)
	{
		const USceneComponent* ExitPoint = ILadderInterface::Execute_GetInitEnterTarget(ClimbObject, true);
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
			UE_LOG(LogTemp, Warning, TEXT("Failed To Find Exit Location"));
			return false;
		}

		FVector ExitLocation = HitResult.ImpactPoint;
		ExitLocation.Z += Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		ClimbLocation = MakeTuple(GetOwner()->GetActorLocation(), ExitLocation);

		LadderStance = EClimbPhase::Exit_From_Bottom_Left;
	}
	bIsClimbing = true;
	SetComponentTickEnabled(true);

	return true;
}

void UClimbComponent::EnterLadderFloat()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	Character->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	Character->GetCapsuleComponent()->SetCapsuleHalfHeight(60.0f);
}

void UClimbComponent::ExitLadderFloat()
{
	if (ACharacterBase* Character = Cast<ACharacterBase>(GetOwner()))
	{
		Character->GetCapsuleComponent()->IgnoreActorWhenMoving(ClimbObject, false);
		Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		Character->GetCapsuleComponent()->SetCapsuleHalfHeight(90.0f);
	}

	LimbToGripNode.Empty();
	GripList1D.Empty();
	ClimbObject = nullptr;
	bIsClimbing = false;
	AnimTime = 0.0f;
	LadderStance = EClimbPhase::Idle;
	SetComponentTickEnabled(false);

	OnLadderExit.Broadcast();
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

	bool bClimbRight = Hand_L_By_LadderAxis > Hand_R_By_LadderAxis;

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
			UE_LOG(LogTemp, Error, TEXT("[ClimbComponent] Invalid FootL Grip while climbing up."));
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
			UE_LOG(LogTemp, Error, TEXT("[ClimbComponent] Invalid FootR Grip while climbing up."));
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

	bool bClimbRight = Hand_L_By_LadderAxis < Hand_R_By_LadderAxis;

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
			UE_LOG(LogTemp, Error, TEXT("[ClimbComponent] Invalid HandR Grip while climbing down."));
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
			UE_LOG(LogTemp, Error, TEXT("[ClimbComponent] Invalid HandL Grip while climbing down."));
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
	GetOwner()->SetActorLocation(ClimbLocation.Value);
	LadderStance = EClimbPhase::Idle;
	//GetOwner()->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
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
	const float TargetAxisPosition = FVector::DotProduct(
		(FootPosition + HandPosition) * 0.5f - LadderLocation,
		LadderUpVector) + FVector::Distance(FootPosition, HandPosition) / 40.0f;
	const float CurrentAxisPosition = FVector::DotProduct(
		CurrentLocation - LadderLocation,
		LadderUpVector);

	return CurrentLocation + LadderUpVector * (TargetAxisPosition - CurrentAxisPosition);
}

void UClimbComponent::RegisterClimbObject(AActor* RegistObject)
{
	if (!IsValid(RegistObject))
	{
		DeRegisterClimbObject();
		return;
	}
	ClimbObject = RegistObject;
	if (ClimbObject->GetClass()->ImplementsInterface(UClimbObjectInterface::StaticClass()))
	{
		if (ClimbObject->ActorHasTag(FName("Ladder")))
		{
			GripList1D = IClimbObjectInterface::Execute_GetGripList1D(ClimbObject);
			if (!GripList1D.IsEmpty())
			{
				SetGrip1DRelation(MinGripInterval, MaxGripInterval);

				const USceneComponent* InitBottomTarget = ILadderInterface::Execute_GetInitEnterTarget(ClimbObject, false);
				if (IsValid(InitBottomTarget))
				{
					const float BottomLocalHeight = ClimbObject->GetActorTransform()
						.InverseTransformPosition(InitBottomTarget->GetComponentLocation()).Z;
					SetLowestGrip1D(MinFirstGripHeight, BottomLocalHeight);
				}
			}
		}
	}
}

void UClimbComponent::DeRegisterClimbObject()
{
	LimbToGripNode.Empty();
	GripList1D.Empty();
	ClimbObject = nullptr;
	bIsClimbing = false;
	AnimTime = 0.0f;
	LadderStance = EClimbPhase::Idle;
	SetComponentTickEnabled(false);
}

AActor* UClimbComponent::GetClimbObject()
{
	return ClimbObject == nullptr ? nullptr : ClimbObject;
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

int32 UClimbComponent::GetLimbPlaceGripIndex(ELimbList LimbName) const
{
	if (!LimbToGripNode.Contains(LimbName))
	{
		UE_LOG(LogTemp, Warning, TEXT("[ClimbComponent] Bone is not Located on the Ladder [Bone Name : %s]"), *UEnum::GetValueAsString(LimbName));
		return INDEX_NONE;
	}

	return LimbToGripNode[LimbName].LimbTargetGripIndex;
}

FVector UClimbComponent::GetLimbIKTarget(ELimbList LimbName)
{
	if (!LimbToGripNode.Contains(LimbName))
	{
		UE_LOG(LogTemp, Warning, TEXT("[ClimbComponent] Bone is not Located on the Ladder [Bone Name : %s]"), *UEnum::GetValueAsString(LimbName));
		return FVector::ZeroVector;
	}

	return LimbName == ELimbList::Body ? BodyLocation : LimbToGripNode[LimbName].LimbLocation;
}

void UClimbComponent::ResetClimbState()
{
	bIsClimbing = false;
	AnimTime = 0.0f;
	LadderStance = EClimbPhase::Idle;
	SetComponentTickEnabled(false);

	LimbToGripNode[ELimbList::HandR].LimbLocation = SetBoneIKTargetLadder(LimbToGripNode[ELimbList::HandR].LimbTargetGripIndex, FVector(), -15.0f);
	LimbToGripNode[ELimbList::FootL].LimbLocation = SetBoneIKTargetLadder(LimbToGripNode[ELimbList::FootL].LimbTargetGripIndex, FVector(), 15.0f);
	LimbToGripNode[ELimbList::HandL].LimbLocation = SetBoneIKTargetLadder(LimbToGripNode[ELimbList::HandL].LimbTargetGripIndex, FVector(), 15.0f);
	LimbToGripNode[ELimbList::FootR].LimbLocation = SetBoneIKTargetLadder(LimbToGripNode[ELimbList::FootR].LimbTargetGripIndex, FVector(), -15.0f);
}

FVector UClimbComponent::SetBoneIKTargetLadder(int32 TargetGripIndex, const FVector CurveValue, const float LimbXDistance, int32 StartGripIndex, const float LimbYDistance, bool IsDebug)
{
	const FGripNode1D* TargetGrip = GetGripNode(TargetGripIndex);
	const FGripNode1D* StartGrip = GetGripNode(StartGripIndex);
	if (!TargetGrip || !IsValid(ClimbObject))
	{
		UE_LOG(LogTemp, Error, TEXT("[ClimbComponent] Cannot calculate ladder IK without a valid ladder and target grip."));
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

FVector UClimbComponent::SetBoneIKTargetLadder(const FVector TargetLoc, const FVector CurveValue, const FVector StartLoc, const float LimbXDistance, const float LimbYDistance, bool IsDebug)
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
		//UE_LOG(LogTemp, Warning, TEXT("Current index = % f"), i);
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
