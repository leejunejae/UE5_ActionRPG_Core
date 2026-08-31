// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Player/Components/LockOnComponent.h"
#include "GameFramework/Character.h"
#include "Characters/Enemies/EnemyBase.h"
#include "Characters/Components/CharacterStatusComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"

// Sets default values for this component's properties
ULockOnComponent::ULockOnComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void ULockOnComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	AActor* Owner = GetOwner();
	if (!Owner) return;

	// 후보 수집용 스피어를 런타임에 생성해서 붙임
	CandidateSphere = NewObject<USphereComponent>(Owner, TEXT("LockOnCandidateSphere"));
	if (!CandidateSphere) return;

	CandidateSphere->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	CandidateSphere->SetSphereRadius(LockOnRadius);
	CandidateSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CandidateSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CandidateSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CandidateSphere->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap);
	CandidateSphere->SetGenerateOverlapEvents(true);

	CandidateSphere->OnComponentBeginOverlap.AddDynamic(this, &ULockOnComponent::OnSphereBeginOverlap);
	CandidateSphere->OnComponentEndOverlap.AddDynamic(this, &ULockOnComponent::OnSphereEndOverlap);
	CandidateSphere->RegisterComponent();
	CandidateSphere->UpdateOverlaps();

	// 컴포넌트 등록 시점에 이미 범위 안에 있던 대상도 후보에 포함한다.
	TArray<AActor*> InitialOverlaps;
	CandidateSphere->GetOverlappingActors(InitialOverlaps, AEnemyBase::StaticClass());
	for (AActor* OverlapActor : InitialOverlaps)
	{
		if (IsValidCandidate(OverlapActor))
		{
			Candidates.Add(OverlapActor);
		}
	}

	if (ACharacter* OwnerCharacter = Cast<ACharacter>(Owner))
	{
		if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController());
			PC && PC->IsLocalController() && LockOnMarkerWidgetClass)
		{
			LockOnMarkerWidget = CreateWidget<UUserWidget>(PC, LockOnMarkerWidgetClass);
			if (LockOnMarkerWidget)
			{
				LockOnMarkerWidget->AddToViewport();
				LockOnMarkerWidget->SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
				LockOnMarkerWidget->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
}

void ULockOnComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (LockOnMarkerWidget)
	{
		LockOnMarkerWidget->RemoveFromParent();
		LockOnMarkerWidget = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void ULockOnComponent::OnSphereBeginOverlap(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	if (IsValidCandidate(OtherActor))
	{
		Candidates.Add(OtherActor);
	}
}

void ULockOnComponent::OnSphereEndOverlap(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32)
{
	if (OtherActor)
	{
		Candidates.Remove(OtherActor);
		if (LockedOnTarget.Get() == OtherActor)
		{
			// 범위를 벗어났는데도 Grace로 유지할지 여부는 TickLockOn에서 판단
			// 여기서는 즉시 Clear하지 않음.
		}
	}
}

bool ULockOnComponent::IsValidCandidate(AActor* Actor) const
{
	if (!Actor || Actor == GetOwner()) return false;

	const AEnemyBase* Enemy = Cast<AEnemyBase>(Actor);
	return Enemy && Enemy->GetCharacterStatusComponent() &&
		!Enemy->GetCharacterStatusComponent()->IsDead();
}

FVector ULockOnComponent::GetTargetPoint(const AActor* Target) const
{
	if (!Target)
	{
		return FVector::ZeroVector;
	}

	FVector TargetPoint = Target->GetActorLocation();
	if (const ACharacter* TargetCharacter = Cast<ACharacter>(Target))
	{
		if (const UCapsuleComponent* Capsule = TargetCharacter->GetCapsuleComponent())
		{
			TargetPoint.Z += Capsule->GetScaledCapsuleHalfHeight() * TargetHeightRatio;
		}
	}
	return TargetPoint;
}

FVector ULockOnComponent::GetCurrentTargetPoint() const
{
	return GetTargetPoint(LockedOnTarget.Get());
}

bool ULockOnComponent::GetScreenPos(AActor* Target, FVector2D& OutScreen) const
{
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar) return false;

	APlayerController* PC = Cast<APlayerController>(OwnerChar->GetController());
	if (!PC) return false;

	return PC->ProjectWorldLocationToScreen(GetTargetPoint(Target), OutScreen, true);
}

float ULockOnComponent::ComputeScore(AActor* Target, const FVector2D& ScreenPos) const
{
	AActor* Owner = GetOwner();
	if (!Owner || !Target) return -FLT_MAX;

	ACharacter* OwnerChar = Cast<ACharacter>(Owner);
	APlayerController* PC = OwnerChar ? Cast<APlayerController>(OwnerChar->GetController()) : nullptr;
	if (!PC) return -FLT_MAX;

	int32 SizeX, SizeY;
	PC->GetViewportSize(SizeX, SizeY);
	if (SizeX <= 0 || SizeY <= 0) return -FLT_MAX;

	const FVector2D Center(SizeX * 0.5f, SizeY * 0.5f);
	const FVector2D Delta = (ScreenPos - Center);

	// 0~1 정규화된 화면 중심 거리 (대각 기준)
	const float Diag = FMath::Sqrt(float(SizeX * SizeX + SizeY * SizeY));
	const float ScreenDistNorm = Delta.Size() / FMath::Max(1.f, Diag);

	if (ScreenDistNorm > MaxScreenRadiusNorm)
		return -FLT_MAX; // 화면 가장자리/바깥쪽은 후보에서 제외(감성)

	const float ScreenCenterScore = 1.f - FMath::Clamp(ScreenDistNorm / MaxScreenRadiusNorm, 0.f, 1.f);

	const float Dist = FVector::Dist(Owner->GetActorLocation(), Target->GetActorLocation());
	const float DistScore = 1.f - FMath::Clamp(Dist / LockOnRadius, 0.f, 1.f);

	// 각도(전방 기준)
	const FVector ToTarget = (Target->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal2D();
	const FVector Forward = Owner->GetActorForwardVector().GetSafeNormal2D();
	const float Dot = FVector::DotProduct(Forward, ToTarget); // -1~1
	const float AngleScore = FMath::Clamp((Dot + 1.f) * 0.5f, 0.f, 1.f);

	float Score =
		ScreenCenterScore * W_ScreenCenter +
		DistScore * W_Distance +
		AngleScore * W_Angle;

	if (HasLineOfSight(Target))
		Score += LOSBonus;

	return Score;
}

TWeakObjectPtr<AActor> ULockOnComponent::FindBestTarget() const
{
	FLockOnCandidateScore Best;

	for (const TWeakObjectPtr<AActor>& It : Candidates)
	{
		AActor* Target = It.Get();
		if (!IsValidCandidate(Target)) continue;
		if (!HasLineOfSight(Target)) continue;

		FVector2D ScreenPos;
		if (!GetScreenPos(Target, ScreenPos)) continue;

		const float S = ComputeScore(Target, ScreenPos);
		if (S > Best.Score)
		{
			Best.Score = S;
			Best.Target = Target;
			Best.ScreenPos = ScreenPos;
		}
	}

	return Best.Target;
}

bool ULockOnComponent::ToggleLockOn()
{
	if (IsLockedOn())
	{
		ClearLockOn();
		return false;
	}

	TWeakObjectPtr<AActor> Best = FindBestTarget();
	if (Best.IsValid())
	{
		SetLockedOnTarget(Best.Get());
		TimeSinceLOSLost = 0.f;
		TimeSinceOffscreen = 0.f;
		return true;
	}

	return false;
}

void ULockOnComponent::ClearLockOn()
{
    SetLockedOnTarget(nullptr);  // 변경 (Reset 대신)
    TimeSinceLOSLost = 0.f;
    TimeSinceOffscreen = 0.f;
}

bool ULockOnComponent::HasLineOfSight(AActor* Target) const
{
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar || !Target) return false;

	APlayerController* PC = Cast<APlayerController>(OwnerChar->GetController());
	if (!PC) return false;

	// 간단 LOS: 카메라가 아니라 캐릭터 눈 위치 기준(필요하면 카메라로 바꿔도 됨)
	FVector ViewLoc;
	FRotator ViewRot;
	PC->GetPlayerViewPoint(ViewLoc, ViewRot);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(LockOnLOS), false, OwnerChar);
	Params.AddIgnoredActor(Target);

	FHitResult Hit;
	const bool bHit = OwnerChar->GetWorld()->LineTraceSingleByChannel(
		Hit, ViewLoc, GetTargetPoint(Target), ECC_Visibility, Params);

	// 무엇이든 막히면 LOS false
	return !bHit;
}

void ULockOnComponent::SetLockedOnTarget(AActor* NewTarget)
{
	AActor* OldTarget = LockedOnTarget.Get();
	if (OldTarget == NewTarget) return;

	if (OldTarget)
	{
		OldTarget->OnDestroyed.RemoveDynamic(this, &ULockOnComponent::OnLockedTargetDestroyed);
	}

	LockedOnTarget = NewTarget;
	if (NewTarget)
	{
		NewTarget->OnDestroyed.AddUniqueDynamic(this, &ULockOnComponent::OnLockedTargetDestroyed);
	}

	// 이전 타겟이 적이면 락온 해제 알림
	if (AEnemyBase* OldEnemy = Cast<AEnemyBase>(OldTarget))
	{
		OldEnemy->OnLockedOnByPlayer(false);
	}

	// 새 타겟이 적이면 락온 시작 알림
	if (AEnemyBase* NewEnemy = Cast<AEnemyBase>(NewTarget))
	{
		NewEnemy->OnLockedOnByPlayer(true);
	}

	OnLockOnTargetChanged.Broadcast(NewTarget);
	UpdateLockOnMarker();
}

void ULockOnComponent::OnLockedTargetDestroyed(AActor* DestroyedActor)
{
	if (LockedOnTarget.Get() == DestroyedActor)
	{
		ClearLockOn();
	}
}

bool ULockOnComponent::IsTargetStillValid(AActor* Target) const
{
	if (!IsValidCandidate(Target)) return false;

	AActor* Owner = GetOwner();
	if (!Owner) return false;

	const float Dist = FVector::Dist(Owner->GetActorLocation(), Target->GetActorLocation());
	if (Dist > BreakRadius)
		return false;
	return true;
}

void ULockOnComponent::TickLockOn(float DeltaTime)
{
	if (!IsLockedOn()) return;

	AActor* Target = LockedOnTarget.Get();
	if (!Target)
	{
		ClearLockOn();
		return;
	}

	// 기본 유효성(거리 등)
	if (!IsTargetStillValid(Target))
	{
		ClearLockOn();
		return;
	}

	// LOS 유예
	if (HasLineOfSight(Target))
	{
		TimeSinceLOSLost = 0.f;
	}
	else
	{
		TimeSinceLOSLost += DeltaTime;
		if (TimeSinceLOSLost > LOSGraceTime)
		{
			ClearLockOn();
			return;
		}
	}

	// Offscreen 유예
	FVector2D ScreenPos;
	bool bProjected = GetScreenPos(Target, ScreenPos);

	if (bProjected)
	{
		ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
		APlayerController* PC = OwnerChar ? Cast<APlayerController>(OwnerChar->GetController()) : nullptr;

		int32 SizeX = 0, SizeY = 0;
		if (PC) PC->GetViewportSize(SizeX, SizeY);

		const bool bOffscreen =
			ScreenPos.X < -50.f || ScreenPos.Y < -50.f ||
			ScreenPos.X > SizeX + 50.f || ScreenPos.Y > SizeY + 50.f;

		if (!bOffscreen) TimeSinceOffscreen = 0.f;
		else TimeSinceOffscreen += DeltaTime;
	}
	else
	{
		TimeSinceOffscreen += DeltaTime;
	}

	if (TimeSinceOffscreen > OffscreenGraceTime)
	{
		ClearLockOn();
		return;
	}

	UpdateLockOnMarker();
	ApplyLockOnRotation(DeltaTime);
}

void ULockOnComponent::ApplyLockOnRotation(float DeltaTime)
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	AActor* Target = LockedOnTarget.Get();
	APlayerController* PC = OwnerCharacter
		? Cast<APlayerController>(OwnerCharacter->GetController()) : nullptr;
	if (!OwnerCharacter || !Target || !PC)
	{
		return;
	}

	const FVector OwnerLocation = OwnerCharacter->GetActorLocation();
	const FVector TargetLocation = GetTargetPoint(Target);
	FVector ViewLocation;
	FRotator ViewRotation;
	PC->GetPlayerViewPoint(ViewLocation, ViewRotation);

	FRotator DesiredCameraRotation = (TargetLocation - ViewLocation).Rotation();
	const FRotator DesiredCharacterRotation(
		0.f, (TargetLocation - OwnerLocation).Rotation().Yaw, 0.f);
	DesiredCameraRotation.Yaw = DesiredCharacterRotation.Yaw;
	DesiredCameraRotation.Roll = 0.f;

	const FRotator CurrentCameraRotation = PC->GetControlRotation();
	PC->SetControlRotation(FMath::RInterpTo(
		CurrentCameraRotation, DesiredCameraRotation,
		DeltaTime, CameraTurnInterpSpeed));

	OwnerCharacter->SetActorRotation(FMath::RInterpTo(
		OwnerCharacter->GetActorRotation(), DesiredCharacterRotation,
		DeltaTime, CharacterTurnInterpSpeed));
}

void ULockOnComponent::UpdateLockOnMarker()
{
	if (!LockOnMarkerWidget)
	{
		return;
	}

	AActor* Target = LockedOnTarget.Get();
	FVector2D ScreenPos;
	if (!Target || !GetScreenPos(Target, ScreenPos))
	{
		LockOnMarkerWidget->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	APlayerController* PC = OwnerCharacter
		? Cast<APlayerController>(OwnerCharacter->GetController()) : nullptr;
	int32 SizeX = 0;
	int32 SizeY = 0;
	if (!PC)
	{
		LockOnMarkerWidget->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	PC->GetViewportSize(SizeX, SizeY);
	if (ScreenPos.X < 0.0f || ScreenPos.Y < 0.0f ||
		ScreenPos.X > SizeX || ScreenPos.Y > SizeY)
	{
		LockOnMarkerWidget->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	LockOnMarkerWidget->SetPositionInViewport(ScreenPos + LockOnMarkerScreenOffset, true);
	LockOnMarkerWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
}

TWeakObjectPtr<AActor> ULockOnComponent::FindSwitchTarget(bool bRight) const
{
	if (!IsLockedOn()) return nullptr;

	AActor* Cur = LockedOnTarget.Get();
	if (!Cur) return nullptr;

	FVector2D CurScreen;
	if (!GetScreenPos(Cur, CurScreen)) return nullptr;

	FLockOnCandidateScore Best;
	Best.Score = -FLT_MAX;

	for (const TWeakObjectPtr<AActor>& It : Candidates)
	{
		AActor* T = It.Get();
		if (!IsValidCandidate(T) || T == Cur) continue;
		if (!HasLineOfSight(T)) continue;

		FVector2D S;
		if (!GetScreenPos(T, S)) continue;
		// 최초 획득과 동일하게 화면 중심 허용 반경 밖의 대상은 전환 후보로
		// 사용하지 않는다. Project 성공만으로는 화면 안이라는 보장이 없다.
		if (ComputeScore(T, S) == -FLT_MAX) continue;

		const float Dx = S.X - CurScreen.X;

		// 방향 필터
		if (bRight && Dx <= 0.f) continue;
		if (!bRight && Dx >= 0.f) continue;

		// “그 방향에서 가장 가까운” 후보 선택: dx가 우선, dy는 보조
		const float Dy = FMath::Abs(S.Y - CurScreen.Y);
		const float Metric = FMath::Abs(Dx) + Dy * 0.75f;

		// Metric이 작을수록 좋음 -> 점수로 변환
		const float Score = -Metric;

		if (Score > Best.Score)
		{
			Best.Score = Score;
			Best.Target = T;
			Best.ScreenPos = S;
		}
	}

	return Best.Target;
}

bool ULockOnComponent::SwitchTarget(bool bRight)
{
	TWeakObjectPtr<AActor> Next = FindSwitchTarget(bRight);
	if (Next.IsValid())
	{
		SetLockedOnTarget(Next.Get());  // 변경
		TimeSinceLOSLost = 0.f;
		TimeSinceOffscreen = 0.f;
		return true;
	}
	return false;
}
