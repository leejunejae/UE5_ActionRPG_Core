// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/Components/AttackComponent.h"
#include "GameFramework/Character.h"

#include "Combat/Interfaces/HitReactionInterface.h"
#include "Combat/Interfaces/CombatInterface.h"
#include "Combat/Interfaces/AttackSourceInterface.h"

#include "GameFramework/CharacterMovementComponent.h"

#include "Engine/StaticMeshSocket.h"
#include "Utils/AttackBoneDataRegistry.h"
#include "Utils/AnimBoneDataRegistryRoot.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

#include "NiagaraSystem.h"
#include "NiagaraComponent.h"

#include "Core/Subsystems/GameInstanceSystem/AnimBoneDataSubsystem.h"
#include "Engine/GameInstance.h"
#include "GameplayTags.h"

#include "Utils/CoreLog.h"


// Sets default values for this component's properties
UAttackComponent::UAttackComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
}


// Called when the game starts
void UAttackComponent::BeginPlay()
{
	Super::BeginPlay();

	ACharacter* Character = Cast<ACharacter>(GetOwner());

	if (!Character) return;


	if (Character->GetClass()->ImplementsInterface(UAttackSourceInterface::StaticClass()))
	{
		AttackSourceInterface = TScriptInterface<IAttackSourceInterface>(Character);
	}
	else
	{
		for (UActorComponent* Comp : Character->GetComponents())
		{
			if (Comp->GetClass()->ImplementsInterface(UAttackSourceInterface::StaticClass()))
			{
				AttackSourceInterface = TScriptInterface<IAttackSourceInterface>(Comp);
				break;
			}
		}
	}
}

void UAttackComponent::ExecuteAttack(FName AttackName, float Playrate)
{
	bool CanPlayAttack = false;

	// 실행중인 공격이 있으면 실행중인 공격과 입력이 같은지 확인	
	if (CurAttackContext.AttackName == AttackName)
	{
		CanPlayAttack = true;
		// 다음 콤보가 있으면 ComboIndex증가 아니면 첫콤보로 초기화
		ComboIndex = CurAttackContext.AttackDetail.IsValidIndex(ComboIndex + 1) ? ComboIndex + 1 : 0;
	}
	else
	{
		if (CurAttackContextSet.Contexts.IsEmpty()) return;
		FAttackContext DataForFind;
		DataForFind.AttackName = AttackName;
		const FAttackContext* FoundData = CurAttackContextSet.Contexts.Find(DataForFind);
		CurAttackContext = FoundData ? *FoundData : FAttackContext{};

		if (CurAttackContext.Anim && !CurAttackContext.AttackDetail.IsEmpty())
		{
			CanPlayAttack = true;
			ComboIndex = 0;
		}
	}

	if (CanPlayAttack)
	{
		HitActorListCurrentAttack.Empty();
		PlayAnimation(CurAttackContext, ComboIndex, Playrate);
	}
}

void UAttackComponent::PlayAnimation(FAttackContext AttackInfo, int32 index, float Playrate)
{
	ACharacter* Char = Cast<ACharacter>(GetOwner());
	if (!Char) return;

	UAnimInstance* Anim = Char->GetMesh()->GetAnimInstance();
	if (!Anim) return;

	Anim->Montage_Play(AttackInfo.Anim, Playrate);
	Anim->Montage_JumpToSection(AttackInfo.AttackDetail[index].SectionName, AttackInfo.Anim);

	FOnMontageEnded MontageEndDelegate;
	MontageEndDelegate.BindUObject(this, &UAttackComponent::OnMontageEnded);
	Anim->Montage_SetEndDelegate(MontageEndDelegate, AttackInfo.Anim);
}

void UAttackComponent::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	ACharacter* Char = Cast<ACharacter>(GetOwner());
	if (!Char) return;

	UAnimInstance* Anim = Char->GetMesh()->GetAnimInstance();
	if (!Anim) return;

	FString EndMontage = Montage ? Montage->GetName() : TEXT("None");
	UAnimMontage* CurrentMontage = Anim ? Anim->GetCurrentActiveMontage() : nullptr;
	FString CurMontage = CurrentMontage ? CurrentMontage->GetName() : TEXT("None");

	if (!(EndMontage.Equals(CurMontage)))
	{
		ComboIndex = 0;
		CurAttackContext = FAttackContext();
	}

	Anim->OnMontageEnded.RemoveDynamic(this, &UAttackComponent::OnMontageEnded);
}

void UAttackComponent::ExecuteAttackTrace(float StartTime, float EndTime, bool bDrawDebug)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());

	if (!AttackSourceInterface.GetInterface() || !AttackSourceInterface.GetObject())
	{
		UE_LOG(Log_Attack, Error, TEXT("[UAttackComponent] AttackSourceInterface is null"));
		return;
	}

	if (!Character)
	{
		UE_LOG(Log_Attack, Error, TEXT("[UAttackComponent] Owner Character Invalid"));
		return;
	}

	if (!CurrentSeg)
	{
		UE_LOG(Log_Attack, Error, TEXT("[UAttackComponent] AnimBoneData was not set"));
		return;
	}

	// 이전프레임과 현재프레임 사이를 0.001초 간격으로 나눔
	int32 TraceCorrectionCount = (EndTime - StartTime) / 0.001f;

	// 현재 루트본의 위치
	FTransform CurrentRootWorldTransform = Character->GetMesh()->GetBoneTransform(0);

	FAttackTraceSource TraceSource = IAttackSourceInterface::Execute_GetAttackTraceSource(AttackSourceInterface.GetObject(), CurAttackContext.AttackDetail[ComboIndex].AttackSource);

	FTransform TargetWeaponOffset = TraceSource.TraceComponent->GetRelativeTransform();

	const FVector StartSocketRelativeLocation = TraceSource.TraceComponent->GetSocketTransform(TEXT("Start"), RTS_Component).GetLocation();
	const FVector EndSocketRelativeLocation = TraceSource.TraceComponent->GetSocketTransform(TEXT("End"), RTS_Component).GetLocation();

	const float subDT = (EndTime - StartTime) / TraceCorrectionCount;

	for (int32 i = 1; i <= TraceCorrectionCount; ++i)
	{
		const float PrevTime = StartTime + (i * 0.001f);

		const FTransform BoneData = CurrentSeg->GetTransformAtTime(PrevTime);

		// 데이터(손) -> 월드(root) 적용
		const FTransform HandWorld = BoneData * CurrentRootWorldTransform;

		// 손월드 -> 무기월드 오프셋 적용
		const FTransform WeaponWorld = TargetWeaponOffset * HandWorld;

		// 소켓 로컬(무기 기준) -> 월드
		const FVector StartLoc = WeaponWorld.TransformPosition(StartSocketRelativeLocation);
		const FVector EndLoc = WeaponWorld.TransformPosition(EndSocketRelativeLocation);

		float CurWeaponLength = FVector::Distance(StartLoc, EndLoc);
		float CurHalfHeight = (CurWeaponLength * 0.5f);

		TArray<FHitResult> HitResults;
		FCollisionQueryParams CollisionParams;
		CollisionParams.AddIgnoredActor(GetOwner());

		float Radius = TraceSource.Radius;

		FCollisionShape DetectShape = FCollisionShape::MakeCapsule(Radius, CurHalfHeight);

		bool bHit = GetWorld()->SweepMultiByChannel(
			HitResults,
			StartLoc,
			EndLoc,
			FQuat::Identity,
			ECC_GameTraceChannel3,
			DetectShape,
			CollisionParams
		);
		
		if (bHit)
		{
			for (const FHitResult& Result : HitResults)
			{
				AActor* HitActor = Result.GetActor();
				if (HitActor && !HitActorListCurrentAttack.Contains(HitActor))
				{
					HitActorListCurrentAttack.Add(HitActor);

					UE_LOG(Log_Attack, Log, TEXT("[AttackComponent] %s Attacked %s"), *Character->GetName(), *HitActor->GetName());

					if (HitActor->Implements<UHitReactionInterface>())
					{
						FAttackDamageSource DamageSource = IAttackSourceInterface::Execute_GetAttackDamageSource(AttackSourceInterface.GetObject());

						float OutDamage = DamageSource.AttackRating * CurAttackContext.AttackDetail[ComboIndex].DamageMultiplier;
						float OutPoiseDamage = DamageSource.PoiseRating * CurAttackContext.AttackDetail[ComboIndex].PoiseDamageMultiplier;
						float OutStanceDamage = DamageSource.StanceRating * CurAttackContext.AttackDetail[ComboIndex].StanceDamageMultiplier;
						EHitResponse OutResponse = CurAttackContext.AttackDetail[ComboIndex].Response;
						EDamageType OutAttackType = CurAttackContext.AttackDetail[ComboIndex].DamageType;
						FVector OutHitPoint = Result.ImpactPoint;
						FString OutHitPointName = Result.PhysMaterial.IsValid() ? Result.PhysMaterial->GetName() : FString();
						bool OutCanBlocked = CurAttackContext.AttackDetail[ComboIndex].CanBlocked;
						bool OutCanParried = CurAttackContext.AttackDetail[ComboIndex].CanParried;
						bool OutCanAvoid = CurAttackContext.AttackDetail[ComboIndex].CanAvoid;
						TArray<FStatusEffect> OutStatusEffect = CurAttackContext.AttackDetail[ComboIndex].StatusEffectList;

						FAttackRequest OutAttackData(
							OutDamage,
							OutStanceDamage,
							OutPoiseDamage,
							OutResponse,
							OutHitPoint,
							OutHitPointName,
							OutCanBlocked,
							OutCanParried,
							OutCanAvoid,
							OutStatusEffect
						);

						IHitReactionInterface::Execute_OnHit(HitActor, OutAttackData);

					}
				}
			}
		}
		
		if (bDrawDebug)
		{
			FVector CurCapsuleCenter = (StartLoc + EndLoc) * 0.5f;
			FVector CurCapsuleAxis = (EndLoc - StartLoc).GetSafeNormal();
			FQuat CurCapsuleRotation = FRotationMatrix::MakeFromZ(CurCapsuleAxis).ToQuat();
			DrawDebugCapsule(GetWorld(), CurCapsuleCenter, CurHalfHeight, Radius, CurCapsuleRotation, FColor::Red, false, 5.0f);
		}
	}
}

void UAttackComponent::BeginAttackTrace(FGameplayTag Profile, const UAnimSequence* AnimKey, FName WindowName)
{
	if (!AnimKey) return;

	UAnimBoneDataSubsystem* Subsys = GetWorld()->GetGameInstance()->GetSubsystem<UAnimBoneDataSubsystem>();
	if (!Subsys) return;

	CurrentSeg = Subsys->GetAnimBoneData(Profile, AnimKey, WindowName);
}
