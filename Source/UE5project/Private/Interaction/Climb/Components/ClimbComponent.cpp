// Fill out your copyright notice in the Description page of Project Settings.


#include "Interaction/Climb/Components/ClimbComponent.h"
#include "Environment/Climbable/Interfaces/ClimbObjectInterface.h"
#include "Interaction/Climb/Interfaces/LadderInterface.h"
#include "Interaction/Interfaces/InteractInterface.h"
#include "Characters/Interfaces/CharacterStatusInterface.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"

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
	
	static ConstructorHelpers::FObjectFinder<UAnimMontage> EnterLadderMontage_Asset(TEXT("/Game/04_Animations/Player/Ladder/Ladder/Ladder_Enter_Montage.Ladder_Enter_Montage"));
	if (EnterLadderMontage_Asset.Succeeded())
	{
		EnterLadderMontage = EnterLadderMontage_Asset.Object;
	}
	// ...
}


// Called when the game starts
void UClimbComponent::BeginPlay()
{
	Super::BeginPlay();
	// ..
	CachedCharacter = Cast<ACharacter>(GetOwner());

	if (!CachedCharacter.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Owner Character Not Valid"));
		return;
	}

	CachedAnim = CachedCharacter->GetMesh()->GetAnimInstance();

	for (UActorComponent* Comp : CachedCharacter->GetComponents())
	{
		if (Comp->GetClass()->ImplementsInterface(UCharacterStatusInterface::StaticClass()))
			CachedPlayerStatus = TScriptInterface<ICharacterStatusInterface>(Comp);;
	}

	if (!CachedPlayerStatus.GetObject() || !CachedPlayerStatus.GetInterface())
	{
		UE_LOG(LogTemp, Warning, TEXT("Character Status Component Not Valid"));
		return;
	}

	if (ClimbCurveDA)
	{
		//ClimbCurveSet = ClimbCurveDA->LadderClimbCurveSet;
		ClimbCurveSet = ClimbCurveDA->CurveSet;
	}
}

void UClimbComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsClimbing) return;

	FVector BodyCurveValue;
	FVector HandCurveValue;
	FVector FootCurveValue;

	AnimTime += DeltaTime;
	if (UCurveVector* BodyCurve = ClimbCurveSet.Curves[FClimbCurveKey{ LadderStance, ELimbList::Body }].LoadSynchronous())
	{
		BodyCurveValue = BodyCurve->GetVectorValue(AnimTime);
	}

	FVector NewLocation = FMath::Lerp(ClimbLocation.Key, ClimbLocation.Value, BodyCurveValue);

	CachedCharacter->SetActorLocation(NewLocation);
	//LimbToGripNode[ELimbList::Body].LimbLocation = NewLocation;

	switch (LadderStance)
	{
	case EClimbPhase::Enter_From_Bottom:
	{

		break;
	}
	case EClimbPhase::Enter_From_Top:
	{
		const USceneComponent* EnterTopPoint = ILadderInterface::Execute_GetInitEnterTarget(ClimbObject, true);
		const FRotator StartRotator = EnterTopPoint->GetComponentRotation();
		const USceneComponent* InitTargetPoint = ILadderInterface::Execute_GetInitClimbTarget(ClimbObject);
		const FRotator TargetRotator = InitTargetPoint->GetComponentRotation();
		const float StartYaw = StartRotator.Yaw > 0.0f ? FMath::Fmod(StartRotator.Yaw, 360.0f) : FMath::Fmod(StartRotator.Yaw, 360.0f) + 360.0f;
		const float TargetYaw = TargetRotator.Yaw > 0.0f ? FMath::Fmod(TargetRotator.Yaw, 360.0f) : FMath::Fmod(TargetRotator.Yaw, 360.0f) + 360.0f;
		float EnterRotation = EnterRotatorCurve.Get()->GetFloatValue(AnimTime);

		const float NewRotatorYaw = FMath::Lerp(StartYaw, TargetYaw, EnterRotation);
		const FRotator NewRotator = FRotator(CachedCharacter->GetActorRotation().Pitch, NewRotatorYaw, CachedCharacter->GetActorRotation().Roll);
		CachedCharacter->SetActorRotation(NewRotator);

		const USceneComponent* InitLeftHandPoint = ILadderInterface::Execute_GetTopEnterHandTarget(ClimbObject, true);
		const USceneComponent* InitRightHandPoint = ILadderInterface::Execute_GetTopEnterHandTarget(ClimbObject, false);

		FVector HandRTarget = LimbToGripNode[ELimbList::HandR].LimbTargetGrip->Position;
		HandRTarget += ClimbObject->GetActorRightVector() * -15.0f;

		FVector HandLTarget = LimbToGripNode[ELimbList::HandL].LimbTargetGrip->Position;
		HandLTarget += ClimbObject->GetActorRightVector() * 15.0f;

		FGripNode1D* FootLTargetGrip = LimbToGripNode[ELimbList::FootL].LimbTargetGrip;

		if (UCurveVector* HandCurve = ClimbCurveSet.Curves[FClimbCurveKey{ LadderStance, ELimbList::HandR }].LoadSynchronous())
		{
			HandCurveValue = HandCurve->GetVectorValue(AnimTime);
		}

		if (UCurveVector* FootCurve = ClimbCurveSet.Curves[FClimbCurveKey{ LadderStance, ELimbList::FootL }].LoadSynchronous())
		{
			FootCurveValue = FootCurve->GetVectorValue(AnimTime);
		}

		FVector HandLCurveValue;

		if (UCurveVector* HandLCurve = ClimbCurveSet.Curves[FClimbCurveKey{ LadderStance, ELimbList::HandL }].LoadSynchronous())
		{
			HandLCurveValue = HandLCurve->GetVectorValue(AnimTime);
		}

		LimbToGripNode[ELimbList::HandR].LimbLocation = SetBoneIKTargetLadder(HandRTarget, HandCurveValue, InitRightHandPoint->GetComponentLocation());
		LimbToGripNode[ELimbList::HandL].LimbLocation = SetBoneIKTargetLadder(HandLTarget, HandLCurveValue, InitLeftHandPoint->GetComponentLocation());
		LimbToGripNode[ELimbList::FootL].LimbLocation = SetBoneIKTargetLadder(FootLTargetGrip, FootCurveValue, 15.0f, FootLTargetGrip->PrevGrip);

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
		if (UCurveVector* HandCurve = ClimbCurveSet.Curves[FClimbCurveKey{ LadderStance, ELimbList::HandR }].LoadSynchronous())
		{
			HandCurveValue = HandCurve->GetVectorValue(AnimTime);
		}

		if (UCurveVector* FootCurve = ClimbCurveSet.Curves[FClimbCurveKey{ LadderStance, ELimbList::FootL }].LoadSynchronous())
		{
			FootCurveValue = FootCurve->GetVectorValue(AnimTime);
		}

		FGripNode1D* HandRTargetGrip = LimbToGripNode[ELimbList::HandR].LimbTargetGrip;
		FGripNode1D* FootLTargetGrip = LimbToGripNode[ELimbList::FootL].LimbTargetGrip;

		LimbToGripNode[ELimbList::HandR].LimbLocation = SetBoneIKTargetLadder(HandRTargetGrip, HandCurveValue, -15.0f, HandRTargetGrip->PrevGrip);
		LimbToGripNode[ELimbList::FootL].LimbLocation = SetBoneIKTargetLadder(FootLTargetGrip, FootCurveValue, 15.0f, FootLTargetGrip->PrevGrip);

		break;
	}
	case EClimbPhase::ClimbUp_Left:
	{
		if (UCurveVector* HandCurve = ClimbCurveSet.Curves[FClimbCurveKey{ LadderStance, ELimbList::HandL }].LoadSynchronous())
		{
			HandCurveValue = HandCurve->GetVectorValue(AnimTime);
		}

		if (UCurveVector* FootCurve = ClimbCurveSet.Curves[FClimbCurveKey{ LadderStance, ELimbList::FootR }].LoadSynchronous())
		{
			FootCurveValue = FootCurve->GetVectorValue(AnimTime);
		}

		FGripNode1D* HandLTargetGrip = LimbToGripNode[ELimbList::HandL].LimbTargetGrip;
		FGripNode1D* FootRTargetGrip = LimbToGripNode[ELimbList::FootR].LimbTargetGrip;

		LimbToGripNode[ELimbList::HandL].LimbLocation = SetBoneIKTargetLadder(HandLTargetGrip, HandCurveValue, 15.0f, HandLTargetGrip->PrevGrip);
		LimbToGripNode[ELimbList::FootR].LimbLocation = SetBoneIKTargetLadder(FootRTargetGrip, FootCurveValue, -15.0f, FootRTargetGrip->PrevGrip);
		break;
	}
	case EClimbPhase::ClimbDown_Right:
	{
		if (UCurveVector* HandCurve = ClimbCurveSet.Curves[FClimbCurveKey{ LadderStance, ELimbList::HandR }].LoadSynchronous())
		{
			HandCurveValue = HandCurve->GetVectorValue(AnimTime);
		}

		if (UCurveVector* FootCurve = ClimbCurveSet.Curves[FClimbCurveKey{ LadderStance, ELimbList::FootL }].LoadSynchronous())
		{
			FootCurveValue = FootCurve->GetVectorValue(AnimTime);
		}

		FGripNode1D* HandRTargetGrip = LimbToGripNode[ELimbList::HandR].LimbTargetGrip;
		FGripNode1D* FootLTargetGrip = LimbToGripNode[ELimbList::FootL].LimbTargetGrip;

		LimbToGripNode[ELimbList::HandR].LimbLocation = SetBoneIKTargetLadder(HandRTargetGrip, HandCurveValue, -15.0f, HandRTargetGrip->PrevGrip);
		LimbToGripNode[ELimbList::FootL].LimbLocation = SetBoneIKTargetLadder(FootLTargetGrip, FootCurveValue, 15.0f, FootLTargetGrip->PrevGrip);

		break;
	}
	case EClimbPhase::ClimbDown_Left:
	{
		if (UCurveVector* HandCurve = ClimbCurveSet.Curves[FClimbCurveKey{ LadderStance, ELimbList::HandL }].LoadSynchronous())
		{
			HandCurveValue = HandCurve->GetVectorValue(AnimTime);
		}

		if (UCurveVector* FootCurve = ClimbCurveSet.Curves[FClimbCurveKey{ LadderStance, ELimbList::FootR }].LoadSynchronous())
		{
			FootCurveValue = FootCurve->GetVectorValue(AnimTime);
		}

		FGripNode1D* HandLTargetGrip = LimbToGripNode[ELimbList::HandL].LimbTargetGrip;
		FGripNode1D* FootRTargetGrip = LimbToGripNode[ELimbList::FootR].LimbTargetGrip;

		LimbToGripNode[ELimbList::HandL].LimbLocation = SetBoneIKTargetLadder(HandLTargetGrip, HandCurveValue, 15.0f, HandLTargetGrip->PrevGrip);
		LimbToGripNode[ELimbList::FootR].LimbLocation = SetBoneIKTargetLadder(FootRTargetGrip, FootCurveValue, -15.0f, FootRTargetGrip->PrevGrip);

		break;
	}
	default:
		return;
	}
	//UE_LOG(LogTemp, Warning, TEXT("[AnimTime : %f], [CurveValue : %f]"), AnimTime, BodyCurveValue.Z);
}


bool UClimbComponent::RequestEnterLadder(AActor* TargetLadder)
{
	RegisterClimbObject(TargetLadder);
	USceneComponent* ClimbPoint = IInteractInterface::Execute_GetEnterInteractLocation(TargetLadder, GetOwner());

	CachedCharacter->GetCapsuleComponent()->IgnoreActorWhenMoving(TargetLadder, true);

	//ResetClimbState();

	const USceneComponent* InitTargetPoint = ILadderInterface::Execute_GetInitClimbTarget(TargetLadder);
	FVector InitCharacterPosition = InitTargetPoint->GetComponentLocation();

	if (ClimbPoint->ComponentHasTag("Bottom"))
	{
		LimbToGripNode.Add(ELimbList::FootL, FLimbData(GetLowestGrip1D(), SetBoneIKTargetLadder(GetLowestGrip1D(), FVector(), 15.0f)));
		LimbToGripNode.Add(ELimbList::FootR, FLimbData(LimbToGripNode[ELimbList::FootL].LimbTargetGrip->GetNeighborUp(), SetBoneIKTargetLadder(LimbToGripNode[ELimbList::FootL].LimbTargetGrip->GetNeighborUp(), FVector(), -15.0f)));
		LimbToGripNode.Add(ELimbList::HandR, FLimbData(LimbToGripNode[ELimbList::FootR].LimbTargetGrip->GetNeighborUp(), SetBoneIKTargetLadder(LimbToGripNode[ELimbList::FootR].LimbTargetGrip->GetNeighborUp(), FVector(), -15.0f)));
		LimbToGripNode.Add(ELimbList::HandL, FLimbData(LimbToGripNode[ELimbList::HandR].LimbTargetGrip->GetNeighborUp(), SetBoneIKTargetLadder(LimbToGripNode[ELimbList::HandR].LimbTargetGrip->GetNeighborUp(), FVector(), 15.0f)));
		LimbToGripNode.Add(ELimbList::Body, FLimbData(nullptr, CachedCharacter->GetActorLocation()));
		LadderStance = EClimbPhase::Enter_From_Bottom;
	}
	else
	{
		//CachedCharacter->GetCapsuleComponent()->SetCapsuleHalfHeight(50.0f);
		const USceneComponent* InitLeftHandPoint = ILadderInterface::Execute_GetTopEnterHandTarget(TargetLadder, false);
		const USceneComponent* InitRightHandPoint = ILadderInterface::Execute_GetTopEnterHandTarget(TargetLadder, true);

		FGripNode1D InitLeftHandTarget;
		FGripNode1D InitRightHandTarget;

		InitLeftHandTarget.Position = InitLeftHandPoint->GetComponentLocation();
		InitRightHandTarget.Position = InitRightHandPoint->GetComponentLocation();

		LimbToGripNode.Add(ELimbList::HandL, FLimbData(GetHighestGrip1D(), SetBoneIKTargetLadder(GetHighestGrip1D(), FVector(), 0.0f, &InitLeftHandTarget)));
		LimbToGripNode.Add(ELimbList::HandR, FLimbData(LimbToGripNode[ELimbList::HandL].LimbTargetGrip->GetNeighborDown(), SetBoneIKTargetLadder(LimbToGripNode[ELimbList::HandL].LimbTargetGrip->GetNeighborDown(), FVector(), 0.0f, &InitRightHandTarget)));
		LimbToGripNode.Add(ELimbList::FootR, FLimbData(LimbToGripNode[ELimbList::HandR].LimbTargetGrip->GetNeighborDown(), SetBoneIKTargetLadder(LimbToGripNode[ELimbList::HandR].LimbTargetGrip->GetNeighborDown(), FVector(), -15.0f)));
		LimbToGripNode.Add(ELimbList::FootL, FLimbData(LimbToGripNode[ELimbList::FootR].LimbTargetGrip->GetNeighborDown(), SetBoneIKTargetLadder(LimbToGripNode[ELimbList::FootR].LimbTargetGrip->GetNeighborDown(), FVector(), 15.0f, LimbToGripNode[ELimbList::HandR].LimbTargetGrip)));
		LimbToGripNode.Add(ELimbList::Body, FLimbData(nullptr, CachedCharacter->GetActorLocation()));
		LadderStance = EClimbPhase::Enter_From_Top;
	}

	InitCharacterPosition.Z = (LimbToGripNode[ELimbList::FootL].LimbTargetGrip->Position.Z + LimbToGripNode[ELimbList::HandL].LimbTargetGrip->Position.Z) / 2.0f
		+ FVector::Distance(LimbToGripNode[ELimbList::FootL].LimbTargetGrip->Position, LimbToGripNode[ELimbList::HandL].LimbTargetGrip->Position) / 40.0f;

	ClimbLocation = MakeTuple(CachedCharacter->GetActorLocation(), InitCharacterPosition);

	ICharacterStatusInterface::Execute_SetCharacterState(CachedPlayerStatus.GetObject(), ECharacterState::Ladder);

	SetComponentTickEnabled(true);
	bIsClimbing = true;
	
	return true;
}

bool UClimbComponent::RequestExitLadder(bool bExitTop)
{
	if (bExitTop)
	{
		const USceneComponent* ExitPoint = ILadderInterface::Execute_GetInitEnterTarget(ClimbObject, true);
		FVector ExitLocation = ExitPoint->GetComponentLocation();
		ExitLocation.Z += CachedCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

		ClimbLocation = MakeTuple(CachedCharacter->GetActorLocation(), ExitLocation);

		LadderStance = EClimbPhase::Exit_From_Top_Left;
	}
	else
	{
		FVector StartLoc = CachedCharacter->GetActorLocation();
		FVector EndLoc = StartLoc;
		EndLoc.Z -= CachedCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 2.0f;

		FHitResult HitResult;
		FCollisionQueryParams CollisionParams;
		CollisionParams.AddIgnoredActor(GetOwner());
		float Radius = CachedCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
		float HalfHeight = CachedCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

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
		ExitLocation.Z += CachedCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		ClimbLocation = MakeTuple(CachedCharacter->GetActorLocation(), ExitLocation);

		LadderStance = EClimbPhase::Exit_From_Bottom_Left;
	}
	bIsClimbing = true;
	SetComponentTickEnabled(true);

	return true;
}

void UClimbComponent::EnterLadderFloat()
{
	CachedCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
}

void UClimbComponent::ExitLadderFloat()
{
	ICharacterStatusInterface::Execute_SetCharacterState(CachedPlayerStatus.GetObject(), ECharacterState::Ground);
	CachedCharacter->GetCapsuleComponent()->IgnoreActorWhenMoving(ClimbObject, false);
	CachedCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	LimbToGripNode.Empty();
	GripList1D.Empty();
	ClimbObject = nullptr;
	bIsClimbing = false;
	AnimTime = 0.0f;
	LadderStance = EClimbPhase::Idle;
	SetComponentTickEnabled(false);
	UE_LOG(LogTemp, Warning, TEXT("ExitLadderFloat"));
}

void UClimbComponent::ClimbUpLadder()
{
	if (bIsClimbing)
		return;

	const FVector LadderLocation = ClimbObject->GetActorLocation();
	const FVector LadderUpVector = ClimbObject->GetActorUpVector().GetSafeNormal();

	const FVector Hand_L_Pos = LimbToGripNode[ELimbList::HandL].LimbTargetGrip->Position;
	const FVector Hand_R_Pos = LimbToGripNode[ELimbList::HandR].LimbTargetGrip->Position;

	//UE_LOG(LogTemp, Warning, TEXT("[Bone Name : %s] [Target Grip : %d]"), *FString("Hand_L"), LimbToGripNode[ELimbList::HandL].LimbTargetGrip->GripIndex);
	//UE_LOG(LogTemp, Warning, TEXT("[Bone Name : %s] [Target Grip : %d]"), *FString("Hand_R"), LimbToGripNode[ELimbList::HandR].LimbTargetGrip->GripIndex);

	const float Hand_L_By_LadderAxis = FVector::DotProduct(Hand_L_Pos - LadderLocation, LadderUpVector);
	const float Hand_R_By_LadderAxis = FVector::DotProduct(Hand_R_Pos - LadderLocation, LadderUpVector);

	bool bClimbRight = Hand_L_By_LadderAxis > Hand_R_By_LadderAxis;

	const FVector CurrentLocation = CachedCharacter->GetActorLocation();
	FVector NewTargetLocation = CurrentLocation;

	if (bClimbRight)
	{
		FGripNode1D* HandRPrevGrip = LimbToGripNode[ELimbList::HandR].LimbTargetGrip;
		LimbToGripNode[ELimbList::HandR].LimbTargetGrip = LimbToGripNode[ELimbList::HandL].LimbTargetGrip->NeighborUp.Neighbor;
		if (!LimbToGripNode[ELimbList::HandR].LimbTargetGrip)
		{
			RequestExitLadder(true);
			return;
		}
		LimbToGripNode[ELimbList::HandR].LimbTargetGrip->PrevGrip = HandRPrevGrip;


		FGripNode1D* FootLPrevGrip = LimbToGripNode[ELimbList::FootL].LimbTargetGrip;
		LimbToGripNode[ELimbList::FootL].LimbTargetGrip = LimbToGripNode[ELimbList::FootR].LimbTargetGrip->NeighborUp.Neighbor;
		LimbToGripNode[ELimbList::FootL].LimbTargetGrip->PrevGrip = FootLPrevGrip;

		NewTargetLocation.Z = (LimbToGripNode[ELimbList::FootR].LimbTargetGrip->Position.Z + LimbToGripNode[ELimbList::HandR].LimbTargetGrip->Position.Z) / 2.0f
			+ FVector::Distance(LimbToGripNode[ELimbList::FootR].LimbTargetGrip->Position, LimbToGripNode[ELimbList::HandR].LimbTargetGrip->Position) / 40.0f;

		LadderStance = EClimbPhase::ClimbUp_Right;
	}
	else
	{
		FGripNode1D* HandLPrevGrip = LimbToGripNode[ELimbList::HandL].LimbTargetGrip;
		LimbToGripNode[ELimbList::HandL].LimbTargetGrip = LimbToGripNode[ELimbList::HandR].LimbTargetGrip->NeighborUp.Neighbor;
		if (!LimbToGripNode[ELimbList::HandL].LimbTargetGrip)
		{
			RequestExitLadder(true);
			return;
		}
		LimbToGripNode[ELimbList::HandL].LimbTargetGrip->PrevGrip = HandLPrevGrip;

		FGripNode1D* FootRPrevGrip = LimbToGripNode[ELimbList::FootR].LimbTargetGrip;
		LimbToGripNode[ELimbList::FootR].LimbTargetGrip = LimbToGripNode[ELimbList::FootL].LimbTargetGrip->NeighborUp.Neighbor;
		LimbToGripNode[ELimbList::FootR].LimbTargetGrip->PrevGrip = FootRPrevGrip;

		NewTargetLocation.Z = (LimbToGripNode[ELimbList::FootL].LimbTargetGrip->Position.Z + LimbToGripNode[ELimbList::HandL].LimbTargetGrip->Position.Z) / 2.0f
			+ FVector::Distance(LimbToGripNode[ELimbList::FootL].LimbTargetGrip->Position, LimbToGripNode[ELimbList::HandL].LimbTargetGrip->Position) / 40.0f;

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

	const FVector LadderLocation = ClimbObject->GetActorLocation();
	const FVector LadderUpVector = ClimbObject->GetActorUpVector().GetSafeNormal();

	const FVector Hand_L_Pos = LimbToGripNode[ELimbList::HandL].LimbTargetGrip->Position;
	const FVector Hand_R_Pos = LimbToGripNode[ELimbList::HandR].LimbTargetGrip->Position;

	//UE_LOG(LogTemp, Warning, TEXT("[Bone Name : %s] [Target Grip : %d]"), *FString("Hand_L"), LimbToGripNode[ELimbList::HandL].LimbTargetGrip->GripIndex);
	//UE_LOG(LogTemp, Warning, TEXT("[Bone Name : %s] [Target Grip : %d]"), *FString("Hand_R"), LimbToGripNode[ELimbList::HandR].LimbTargetGrip->GripIndex);


	const float Hand_L_By_LadderAxis = FVector::DotProduct(Hand_L_Pos - LadderLocation, LadderUpVector);
	const float Hand_R_By_LadderAxis = FVector::DotProduct(Hand_R_Pos - LadderLocation, LadderUpVector);

	bool bClimbRight = Hand_L_By_LadderAxis < Hand_R_By_LadderAxis;

	const FVector CurrentLocation = CachedCharacter->GetActorLocation();
	FVector NewTargetLocation = CurrentLocation;

	if (bClimbRight)
	{
		FGripNode1D* FootLPrevGrip = LimbToGripNode[ELimbList::FootL].LimbTargetGrip;
		LimbToGripNode[ELimbList::FootL].LimbTargetGrip = LimbToGripNode[ELimbList::FootR].LimbTargetGrip->NeighborDown.Neighbor;
		if (!LimbToGripNode[ELimbList::FootL].LimbTargetGrip)
		{
			RequestExitLadder(false);
			return;
		}
		LimbToGripNode[ELimbList::FootL].LimbTargetGrip->PrevGrip = FootLPrevGrip;

		FGripNode1D* HandRPrevGrip = LimbToGripNode[ELimbList::HandR].LimbTargetGrip;
		LimbToGripNode[ELimbList::HandR].LimbTargetGrip = LimbToGripNode[ELimbList::HandL].LimbTargetGrip->NeighborDown.Neighbor;
		LimbToGripNode[ELimbList::HandR].LimbTargetGrip->PrevGrip = HandRPrevGrip;

		NewTargetLocation.Z = (LimbToGripNode[ELimbList::FootL].LimbTargetGrip->Position.Z + LimbToGripNode[ELimbList::HandL].LimbTargetGrip->Position.Z) / 2.0f
			+ FVector::Distance(LimbToGripNode[ELimbList::FootL].LimbTargetGrip->Position, LimbToGripNode[ELimbList::HandL].LimbTargetGrip->Position) / 40.0f;

		LadderStance = EClimbPhase::ClimbDown_Right;
	}
	else
	{
		FGripNode1D* FootRPrevGrip = LimbToGripNode[ELimbList::FootR].LimbTargetGrip;
		LimbToGripNode[ELimbList::FootR].LimbTargetGrip = LimbToGripNode[ELimbList::FootL].LimbTargetGrip->NeighborDown.Neighbor;
		if (!LimbToGripNode[ELimbList::FootR].LimbTargetGrip)
		{
			RequestExitLadder(false);
			return;
		}
		LimbToGripNode[ELimbList::FootR].LimbTargetGrip->PrevGrip = FootRPrevGrip;

		FGripNode1D* HandLPrevGrip = LimbToGripNode[ELimbList::HandL].LimbTargetGrip;
		LimbToGripNode[ELimbList::HandL].LimbTargetGrip = LimbToGripNode[ELimbList::HandR].LimbTargetGrip->NeighborDown.Neighbor;
		LimbToGripNode[ELimbList::HandL].LimbTargetGrip->PrevGrip = HandLPrevGrip;

		NewTargetLocation.Z = (LimbToGripNode[ELimbList::FootR].LimbTargetGrip->Position.Z + LimbToGripNode[ELimbList::HandR].LimbTargetGrip->Position.Z) / 2.0f
			+ FVector::Distance(LimbToGripNode[ELimbList::FootR].LimbTargetGrip->Position, LimbToGripNode[ELimbList::HandR].LimbTargetGrip->Position) / 40.0f;

		LadderStance = EClimbPhase::ClimbDown_Left;
	}

	ClimbLocation = MakeTuple(CurrentLocation, NewTargetLocation);

	bIsClimbing = true;
	SetComponentTickEnabled(true);
}

void UClimbComponent::CommitLadderState()
{
	FAnimMontageInstance* MontageInst = CachedAnim->GetActiveMontageInstance();

	if (!MontageInst || MontageInst->Montage != EnterLadderMontage)
	{
		// 재생이 취소/중단됐다면 상태 커밋 중단
		GetWorld()->GetTimerManager().ClearTimer(LadderBlendCheckTimer);
		return;
	}
	
	const float BlendWeight = MontageInst->GetWeight();
	if (BlendWeight >= 0.98f)
	{
		ICharacterStatusInterface::Execute_SetCharacterState(CachedPlayerStatus.GetObject(), ECharacterState::Ladder);
		GetWorld()->GetTimerManager().ClearTimer(LadderBlendCheckTimer);
	}
}

void UClimbComponent::OnEnterClimbMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	CachedCharacter->SetActorLocation(ClimbLocation.Value);
	LadderStance = EClimbPhase::Idle;
	//CachedCharacter->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

void UClimbComponent::RegisterClimbObject(AActor* RegistObject)
{
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

				SetLowestGrip1D(MinFirstGripHeight, InitBottomTarget->GetComponentLocation().Z);
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

FGripNode1D* UClimbComponent::GetLimbPlaceGrip(ELimbList LimbName)
{
	if (!LimbToGripNode.Contains(LimbName))
	{
		UE_LOG(LogTemp, Warning, TEXT("Bone is not Located on the Ladder [Bone Name : %s]"), *UEnum::GetValueAsString(LimbName));
		return nullptr;
	}

	return LimbToGripNode[LimbName].LimbTargetGrip;
}

FVector UClimbComponent::GetLimbIKTarget(ELimbList LimbName)
{
	if (!LimbToGripNode.Contains(LimbName))
	{
		UE_LOG(LogTemp, Warning, TEXT("Bone is not Located on the Ladder [Bone Name : %s]"), *UEnum::GetValueAsString(LimbName));
		return FVector::ZeroVector;
	}

	/*
	UE_LOG(LogTemp, Warning, TEXT("%s Target Grip = [X : %f, Y : %f, Z : %f], IK Position = [X : %f, Y : %f, Z : %f]"), 
		*UEnum::GetValueAsString(LimbName), 
		LimbToGripNode[LimbName].LimbTargetGrip->Position.X,
		LimbToGripNode[LimbName].LimbTargetGrip->Position.Y,
		LimbToGripNode[LimbName].LimbTargetGrip->Position.Z,
		LimbToGripNode[LimbName].LimbLocation.X,
		LimbToGripNode[LimbName].LimbLocation.Y,
		LimbToGripNode[LimbName].LimbLocation.Z
	);
	*/

	return LimbToGripNode[LimbName].LimbLocation;
}

void UClimbComponent::ResetClimbState()
{
	bIsClimbing = false;
	AnimTime = 0.0f;
	LadderStance = EClimbPhase::Idle;
	SetComponentTickEnabled(false);

	LimbToGripNode[ELimbList::HandR].LimbLocation = SetBoneIKTargetLadder(LimbToGripNode[ELimbList::HandR].LimbTargetGrip, FVector(), -15.0f);
	LimbToGripNode[ELimbList::FootL].LimbLocation = SetBoneIKTargetLadder(LimbToGripNode[ELimbList::FootL].LimbTargetGrip, FVector(), 15.0f);
	LimbToGripNode[ELimbList::HandL].LimbLocation = SetBoneIKTargetLadder(LimbToGripNode[ELimbList::HandL].LimbTargetGrip, FVector(), 15.0f);
	LimbToGripNode[ELimbList::FootR].LimbLocation = SetBoneIKTargetLadder(LimbToGripNode[ELimbList::FootR].LimbTargetGrip, FVector(), -15.0f);
}

FVector UClimbComponent::SetBoneIKTargetLadder(const FGripNode1D* TargetGrip, const FVector CurveValue, const float LimbXDistance, const FGripNode1D* StartGrip, const float LimbYDistance, bool IsDebug)
{
	FVector TargetLoc;
	FVector LimbOffset = ClimbObject->GetActorRightVector() * LimbXDistance;

	if (StartGrip)
	{
		TargetLoc = FMath::Lerp(StartGrip->Position, TargetGrip->Position, CurveValue.Z) + LimbOffset;
		
		FVector ForwardVector = CachedCharacter->GetActorForwardVector();
		FVector ForwardOffset = (ForwardVector * LimbYDistance) * CurveValue.Y;

		FVector RightVector = CachedCharacter->GetActorRightVector();
		FVector RightOffset = RightVector * CurveValue.X;

		TargetLoc += ForwardOffset + RightOffset;
	}
	else
	{
		TargetLoc = TargetGrip->Position + LimbOffset;
	}

	return TargetLoc;
}

FVector UClimbComponent::SetBoneIKTargetLadder(const FVector TargetLoc, const FVector CurveValue, const FVector StartLoc, const float LimbXDistance, const float LimbYDistance, bool IsDebug)
{
	FVector OutLoc;

	OutLoc = FMath::Lerp(StartLoc, TargetLoc, CurveValue.Z);

	FVector ForwardVector = CachedCharacter->GetActorForwardVector();
	FVector ForwardOffset = (ForwardVector * LimbYDistance) * CurveValue.Y;

	FVector RightVector = CachedCharacter->GetActorRightVector();
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
		//UE_LOG(LogTemp, Warning, TEXT("Current index = % f"), i);
		int32 lowerindex = i - 1;
		int32 upperindex = i + 1;

		while (GripList1D.IsValidIndex(lowerindex))
		{
			float DistanceToLowerGrip = FVector::Dist(GripList1D[i].Position, GripList1D[lowerindex].Position);
			if (DistanceToLowerGrip >= MinInterval && DistanceToLowerGrip <= MaxInterval)
			{
				GripList1D[i].NeighborDown = { &GripList1D[lowerindex], DistanceToLowerGrip };
				//UE_LOG(LogTemp, Warning, TEXT("Neighbors Position Z of GripList[%d] LowerGrip = %f"), i, GripList1D[lowerindex].Position.Z);
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
			float DistanceToUpperGrip = FVector::Dist(GripList1D[i].Position, GripList1D[upperindex].Position);
			if (DistanceToUpperGrip >= MinInterval && DistanceToUpperGrip <= MaxInterval)
			{
				GripList1D[i].NeighborUp = { &GripList1D[upperindex], DistanceToUpperGrip };
				//UE_LOG(LogTemp, Warning, TEXT("Neighbors Position Z of GripList[%d] UpperGrip = %f"), i, GripList1D[upperindex].Position.Z);
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
		if (GripNode.Position.Z - Comparison > MinHeight)
		{
			GripNode.Tag.Add(FName("LowestGrip"));
			return;
		}
	}
}

FGripNode1D* UClimbComponent::GetLowestGrip1D()
{
	if (!CheckGripListValid())
	{
		return nullptr;
	}

	for (FGripNode1D& GripNode : GripList1D)
	{
		if(GripNode.Tag.Contains(FName("LowestGrip")))
			return &GripNode;
	}

	return &GripList1D[0];
}

FGripNode1D* UClimbComponent::GetHighestGrip1D()
{
	if (!CheckGripListValid())
	{
		return nullptr;
	}

	return &GripList1D.Last();
}