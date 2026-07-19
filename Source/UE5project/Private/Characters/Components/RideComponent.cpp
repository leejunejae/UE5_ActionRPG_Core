// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/Components/RideComponent.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Characters/Player/PlayerBase.h"
#include "Characters/Player/PlayerBaseAnimInstance.h"
#include "Characters/Player/PlayerRide.h"
#include "Characters/Player/Components/PlayerStatusComponent.h"
#include "Characters/Rideable/Ride.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Utils/CoreLog.h"
#include "Utils/GameplayTagsBase.h"

URideComponent::URideComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URideComponent::BeginPlay()
{
	Super::BeginPlay();

	Player = Cast<APlayerBase>(GetOwner());
}

void URideComponent::SetCurrentRide(ARide* NewRide)
{
	CurrentRide = NewRide;
}

void URideComponent::ClearCurrentRide()
{
	CurrentRide = nullptr;
}

void URideComponent::RequestSpawnRide()
{
	if (!Player || !Player->GetRideClass())
		return;

	APlayerRide* SpawnedRide = GetWorld()->SpawnActor<APlayerRide>(Player->GetRideClass(), Player->GetActorTransform());
	if (!SpawnedRide)
	{
		UE_LOG(Log_RideSpawn, Warning, TEXT("[APlayerBase] %s : Horse was Not Spawned"), *Player->GetName());
		return;
	}

	SetCurrentRide(SpawnedRide);
	BeginRideCollision();

	SpawnedRide->Mount(Player, Player->GetVelocity());
	BlendPlayerCameraToRide(SpawnedRide, Player->GetVelocity());

	Player->GetCharacterMovement()->DisableMovement();

	CurRideAnimPhase = ERideAnimPhase::Mount;
	Player->GetCharacterStatusComponent()->SetState(TAG_State_Ride);

	GetWorld()->GetTimerManager().SetTimer(MountTimerHandle, this, &URideComponent::MountTimer, 0.01f, true);
}

bool URideComponent::RequestDismount(FVector InitVelocity)
{
	if (!Player || !CurrentRide)
		return false;

	ARide* Ride = CurrentRide;

	BlendRideCameraToPlayer();

	FDetachmentTransformRules DetachmentRules = FDetachmentTransformRules(
		EDetachmentRule::KeepWorld, false);

	const bool bJumpDismount = InitVelocity.SizeSquared2D() > FMath::Square(Player->GetMovingDismountSpeedThreshold());

	if (bJumpDismount)
	{
		Player->DetachFromActor(DetachmentRules);
		Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		Player->SetSkipJumpStart(true);

		FVector DisMountVelocity = InitVelocity * 0.4f;
		DisMountVelocity.Z = 600.0f;

		Player->LaunchCharacter(DisMountVelocity, true, true);
		Player->GetCharacterStatusComponent()->SetState(TAG_State_Ground);

		ClearCurrentRide();
		EndRideCollision(Ride);

		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
		{
			if (Player)
			{
				Player->GetCapsuleComponent()->SetCollisionProfileName(TEXT("Character_Player"));
			}
		}, 0.01f, false);
	}
	else
	{
		Player->GetCharacterMovement()->StopMovementImmediately();
		Player->GetCharacterMovement()->DisableMovement();

		Player->SetSkipJumpStart(false);
		CurRideAnimPhase = ERideAnimPhase::DisMount_Normal;
		NormalDismountStartTransform = Player->GetActorTransform();
		NormalDismountTargetTransform = Ride->GetDismountTransform();

		GetWorld()->GetTimerManager().SetTimer(NormalDismountTimerHandle, this, &URideComponent::NormalDismountTimer, 0.01f, true);
	}

	return true;
}

void URideComponent::HandleMountEnd()
{
	if (!Player || !CurrentRide)
		return;

	FTransform MountTransform = GetMountTransform();
	Player->SetActorLocation(MountTransform.GetLocation());
	Player->SetActorRotation(MountTransform.GetRotation().Rotator());
	CurrentRide->AttachRider();

	if (GetWorld()->GetTimerManager().IsTimerActive(MountTimerHandle))
	{
		Player->GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		GetWorld()->GetTimerManager().ClearTimer(MountTimerHandle);
	}
	CurRideAnimPhase = ERideAnimPhase::Riding;
}

void URideComponent::HandleDismountEnd()
{
	if (!Player)
		return;

	ARide* Ride = CurrentRide;

	if (CurRideAnimPhase == ERideAnimPhase::DisMount_Normal)
	{
		if (GetWorld()->GetTimerManager().IsTimerActive(NormalDismountTimerHandle))
		{
			GetWorld()->GetTimerManager().ClearTimer(NormalDismountTimerHandle);
		}

		Player->SetActorLocationAndRotation(
			NormalDismountTargetTransform.GetLocation(),
			NormalDismountTargetTransform.GetRotation().Rotator());

		EndRideCollision(Ride);

		if (Ride)
		{
			Ride->FinishDismount();
		}
	}

	ClearCurrentRide();

	if (CurRideAnimPhase != ERideAnimPhase::DisMount_Normal)
	{
		EndRideCollision(Ride);
	}

	Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	Player->SetSkipJumpStart(false);
	Player->GetCharacterStatusComponent()->SetState(TAG_State_Ground);

	FDetachmentTransformRules DetachmentRules = FDetachmentTransformRules(
		EDetachmentRule::KeepWorld, false);

	Player->DetachFromActor(DetachmentRules);
}

float URideComponent::GetRideSpeed() const
{
	return CurrentRide ? CurrentRide->GetRideSpeed() : 0.0f;
}

float URideComponent::GetRideDirection() const
{
	return CurrentRide ? CurrentRide->GetRideDirection() : 0.0f;
}

FTransform URideComponent::GetMountTransform() const
{
	return CurrentRide ? CurrentRide->GetMountTransform() : FTransform::Identity;
}

FTransform URideComponent::GetDisMountTransform() const
{
	return  CurrentRide ? CurrentRide->GetDismountTransform() : FTransform::Identity;
}

TOptional<FVector> URideComponent::GetRideIKTargetLoc(EBodyType BoneType) const
{
	if (!CurrentRide || !CurrentRide->GetMesh())
		return TOptional<FVector>();

	USkeletalMeshComponent* RideMesh = CurrentRide->GetMesh();
	FName SocketName;

	switch (BoneType)
	{
	case EBodyType::Hand_L:
		SocketName = FName("Reins_Bn_Hand_L");
		break;
	case EBodyType::Hand_R:
		SocketName = FName("Reins_Bn_Hand_R");
		break;
	case EBodyType::Foot_L:
		SocketName = FName("SaddleLeftFootPlace");
		break;
	case EBodyType::Foot_R:
		SocketName = FName("SaddleRightFootPlace");
		break;
	default:
		return TOptional<FVector>();
	}

	return RideMesh->DoesSocketExist(SocketName)
		? TOptional<FVector>(RideMesh->GetSocketLocation(SocketName))
		: TOptional<FVector>();
}

void URideComponent::BeginRideCollision()
{
	if (!Player)
		return;

	Player->GetCapsuleComponent()->SetCollisionProfileName(TEXT("Character_Riding"));

	if (CurrentRide)
	{
		Player->GetCapsuleComponent()->IgnoreActorWhenMoving(CurrentRide, true);
		CurrentRide->GetCapsuleComponent()->IgnoreActorWhenMoving(Player, true);
	}
}

void URideComponent::EndRideCollision(ARide* Ride)
{
	if (!Player)
		return;

	Player->GetCapsuleComponent()->SetCollisionProfileName(TEXT("Character_Player"));

	if (Ride)
	{
		Player->GetCapsuleComponent()->IgnoreActorWhenMoving(Ride, false);
		Ride->GetCapsuleComponent()->IgnoreActorWhenMoving(Player, false);
	}
}

void URideComponent::MountTimer()
{
	if (!Player || !CurrentRide)
		return;

	FVector StartLocation = CurrentRide->GetActorLocation();
	FVector TargetLocation = GetMountTransform().GetLocation();

	UPlayerBaseAnimInstance* AnimInstance = Player->GetPlayerAnimInstance();
	if (!AnimInstance)
		return;

	FVector CurLocation = FMath::Lerp(StartLocation, TargetLocation, AnimInstance->GetCurveValue(FName("Char_Translation_Y")));
	CurLocation.Z = FMath::Lerp(StartLocation.Z, TargetLocation.Z, AnimInstance->GetCurveValue(FName("Char_Translation_Z")));

	Player->SetActorLocation(CurLocation);
}

void URideComponent::NormalDismountTimer()
{
	if (!Player || !Player->GetPlayerAnimInstance())
		return;

	UPlayerBaseAnimInstance* AnimInstance = Player->GetPlayerAnimInstance();
	const float HorizontalAlpha = AnimInstance->GetCurveValue(FName("Char_Translation_Y"));
	const float VerticalAlpha = AnimInstance->GetCurveValue(FName("Char_Translation_Z"));
	const float RotationAlpha = FMath::Clamp(HorizontalAlpha, 0.0f, 1.0f);

	FVector CurLocation = FMath::Lerp(
		NormalDismountStartTransform.GetLocation(),
		NormalDismountTargetTransform.GetLocation(),
		HorizontalAlpha);

	CurLocation.Z = FMath::Lerp(
		NormalDismountStartTransform.GetLocation().Z,
		NormalDismountTargetTransform.GetLocation().Z,
		VerticalAlpha);

	const FQuat CurRotation = FQuat::Slerp(
		NormalDismountStartTransform.GetRotation(),
		NormalDismountTargetTransform.GetRotation(),
		RotationAlpha);

	Player->SetActorLocationAndRotation(CurLocation, CurRotation.Rotator());
}

void URideComponent::BlendPlayerCameraToRide(ARide* Ride, FVector InitVelocity)
{
	if (!Player || !Ride)
		return;

	FRotator SourceControlRotation = Player->GetControllerRotation();

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PlayerController)
	{
		PlayerController->bAutoManageActiveCameraTarget = false;

		ACameraActor* TransitionCamera = nullptr;
		if (PlayerController->PlayerCameraManager)
		{
			const FVector CurrentCameraLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
			const FRotator CurrentCameraRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
			const float CurrentFOV = PlayerController->PlayerCameraManager->GetFOVAngle();

			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			SpawnParams.ObjectFlags |= RF_Transient;

			TransitionCamera = GetWorld()->SpawnActor<ACameraActor>(CurrentCameraLocation, CurrentCameraRotation, SpawnParams);
			if (TransitionCamera)
			{
				TransitionCamera->GetCameraComponent()->SetFieldOfView(CurrentFOV);
				TransitionCamera->GetCameraComponent()->SetConstraintAspectRatio(false);
				TransitionCamera->SetLifeSpan(1.0f);
				PlayerController->SetViewTarget(TransitionCamera);
			}
		}

		if (!TransitionCamera)
		{
			PlayerController->SetViewTarget(Player);
		}

		PlayerController->Possess(Ride);
		PlayerController->SetControlRotation(SourceControlRotation);

		Ride->RefreshRideCameraComponents();

		PlayerController->SetViewTargetWithBlend(Ride, 0.25f, VTBlend_Cubic);
	}

	Ride->GetCharacterMovement()->Velocity = InitVelocity;
}

void URideComponent::BlendRideCameraToPlayer()
{
	if (!Player || !CurrentRide)
		return;

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	FRotator InitControllerRotator = FRotator::ZeroRotator;
	if (PlayerController)
	{
		InitControllerRotator = PlayerController->GetControlRotation();
	}

	InitControllerRotator = CurrentRide->GetControllerRotation();

	ACameraActor* TransitionCamera = nullptr;
	if (PlayerController)
	{
		PlayerController->bAutoManageActiveCameraTarget = false;

		if (PlayerController->PlayerCameraManager)
		{
			const FVector CurrentCameraLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
			const FRotator CurrentCameraRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
			const float CurrentFOV = PlayerController->PlayerCameraManager->GetFOVAngle();

			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			SpawnParams.ObjectFlags |= RF_Transient;

			TransitionCamera = GetWorld()->SpawnActor<ACameraActor>(CurrentCameraLocation, CurrentCameraRotation, SpawnParams);
			if (TransitionCamera)
			{
				TransitionCamera->GetCameraComponent()->SetFieldOfView(CurrentFOV);
				TransitionCamera->GetCameraComponent()->SetConstraintAspectRatio(false);
				TransitionCamera->SetLifeSpan(1.0f);
				PlayerController->SetViewTarget(TransitionCamera);
			}
		}

		if (!TransitionCamera)
		{
			PlayerController->SetViewTarget(CurrentRide);
		}

		PlayerController->Possess(Player);
		PlayerController->SetControlRotation(InitControllerRotator);

		Player->RefreshPlayerCameraComponents();

		PlayerController->SetViewTargetWithBlend(Player, 0.25f, VTBlend_Cubic);
	}
}

