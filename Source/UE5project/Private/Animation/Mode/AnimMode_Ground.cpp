#include "Animation/Mode/AnimMode_Ground.h"
#include "Characters/CharacterBase.h"
#include "Characters/CharacterBaseAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Characters/Components/CharacterStatusComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "KismetAnimationLibrary.h"
#include "Utils/CoreLog.h"

void UAnimMode_Ground::OnModeEnter()
{
	bFirstTick = true;
}

void UAnimMode_Ground::Tick(float DeltaSeconds)
{
	if (!Character.IsValid() || !AnimInst.IsValid()) return;

	// 상태/스탠스
	AnimInst->CurLocomotionGait = Character->GetCurLocomotionGait();

	// 이동량
	AnimInst->Direction = ComputeCurrentDirection();
	if (bFirstTick) { OnModeEnter(); bFirstTick = false; }


	// 점프/낙하/착지 판정
	AnimInst->IsInAir = Character->GetMovementComponent()->IsFalling();         
	AnimInst->IsJumping = AnimInst->IsFalling = AnimInst->IsLanding = false;
	if (AnimInst->IsInAir)
	{
		Character->GetVelocity().Z > 0 ? AnimInst->IsJumping = true : AnimInst->IsFalling = true; 
	}

	// 런/조그 블렌드, 가속/퀵턴, 등
	AnimInst->MovementAlpha = FMath::GetRangePct(400.f, 600.f, AnimInst->Speed);

	// 발 IK
	UpdateFootIK(DeltaSeconds, AnimInst->Speed);
	UpdateTwoHandWeaponIK(DeltaSeconds);
}

float UAnimMode_Ground::ComputeCurrentDirection()
{
	FVector WorldVelocity = Character->GetVelocity();
	WorldVelocity.Z = 0.0f;
	FRotator WorldRotation = Character->GetActorRotation();
	float LocalDirection = UKismetAnimationLibrary::CalculateDirection(WorldVelocity, WorldRotation);
	return LocalDirection;
}

TTuple<FVector, float> UAnimMode_Ground::FootTrace(
	USkeletalMeshComponent* Mesh, UCapsuleComponent* Capsule, FName Socket)
{
	const FVector FootLoc = Mesh->GetSocketLocation(Socket);
	const FVector Start(FootLoc.X, FootLoc.Y, Mesh->GetOwner()->GetActorLocation().Z);
	const FVector End = Start - FVector(0, 0, Capsule->GetScaledCapsuleHalfHeight() + TraceDistance);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.bTraceComplex = true;

	// 속도 기준 채널 선택은 Anim에서 하던 방식 유지 가능(여기서는 Visibility로 통일하거나 필요시 주입)
	const ECollisionChannel Characterannel = ECC_Visibility;

	const bool bHit = Mesh->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, Characterannel, Params);
	if (bHit)
	{
		const float Offset = ((Hit.ImpactPoint - Hit.TraceEnd).Length()) - (TraceDistance - 3.f);
		return MakeTuple(Hit.ImpactNormal, Offset);
	}
	return MakeTuple(FVector::ZeroVector, 0.f);
}

void UAnimMode_Ground::FootRotation(float DeltaSeconds, const FVector& TargetNormal, FRotator& InOutRot)
{
	const float AtanX = -1.f * UKismetMathLibrary::DegAtan2(TargetNormal.X, TargetNormal.Z);
	const float AtanY = UKismetMathLibrary::DegAtan2(TargetNormal.Y, TargetNormal.Z);
	const FRotator Target(AtanX, 0.f, AtanY);
	InOutRot = UKismetMathLibrary::RInterpTo(InOutRot, Target, DeltaSeconds, RotInterpSpeed);
}

void UAnimMode_Ground::UpdateFootIK(float DeltaSeconds, float Speed)
{
	auto* Mesh = Character->GetMesh();
	auto* Capsule = Character->GetCapsuleComponent();

	const auto L = FootTrace(Mesh, Capsule, FName("Foot_L"));
	const auto R = FootTrace(Mesh, Capsule, FName("Foot_R"));

	FootRotation(DeltaSeconds, L.Key, AnimInst->LeftFootRotator);
	FootRotation(DeltaSeconds, R.Key, AnimInst->RightFootRotator);

	AnimInst->PelvisOffset = FMath::FInterpTo(
		AnimInst->PelvisOffset,
		UKismetMathLibrary::Min(L.Value, R.Value), DeltaSeconds, IKInterpSpeed);

	AnimInst->LeftFootGroundOffset.Z = FMath::FInterpTo(AnimInst->LeftFootGroundOffset.Z, L.Value - AnimInst->PelvisOffset, DeltaSeconds, IKInterpSpeed);
	AnimInst->RightFootGroundOffset.Z = FMath::FInterpTo(AnimInst->RightFootGroundOffset.Z, R.Value - AnimInst->PelvisOffset, DeltaSeconds, IKInterpSpeed);

	// 필요 시 X 오프셋 이동(원 코드 Speed>100 조건)은 여기서 처리 가능:contentReference[oaicite:8]{index=8}
}

void UAnimMode_Ground::UpdateTwoHandWeaponIK(float DeltaSeconds)
{
	UStaticMeshComponent* Mesh = Character->GetMainWeaponMesh();

	if (!Mesh)
	{
		//UE_LOG(Log_Anim_IK, Warning, TEXT("[AnimMode_Ground] %s HandIK Target Mesh Not Valid"), *Character->GetName());
		return;
	}

	AnimInst->LeftHandWeaponOffset = Mesh->GetSocketLocation(FName("Hand_L_IK"));
	FVector Hand_L_Location = Character->GetMesh()->GetSocketLocation(FName("Hand_L_Offset"));
	FVector Weapon_L_Location = Character->GetMesh()->GetSocketLocation(FName("Hand_L_WeaponIK"));
	AnimInst->LeftHandWeaponOffset -= Weapon_L_Location - Hand_L_Location;
}
