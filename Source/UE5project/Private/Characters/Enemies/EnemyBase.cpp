// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Enemies/EnemyBase.h"
#include "Characters/Player/PlayerBase.h"
#include "Characters/Enemies/EnemyBaseAnimInstance.h"
#include "Characters/Enemies/EnemyBaseAIController.h"

#include "Core/Subsystems/GameInstanceSystem/EnemyDataSubsystem.h"


#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Characters/Enemies/Components/CharacterStatComponent.h"
#include "Combat/Components/AttackComponent.h"
#include "Combat/Components/HitReactionComponent.h"
#include "Characters/Components/CharacterStatusComponent.h"

#include "Core/Subsystems/GameInstanceSystem/NPCAnimRegistrySubsystem.h"

#include "Utils/CoreLog.h"
#include "Utils/GameplayTagsBase.h"

#include "Blueprint/AIBlueprintHelperLibrary.h"

#include "Components/WidgetComponent.h"
#include "UI/HUD/OverheadHPWidget.h"

// Sets default values
AEnemyBase::AEnemyBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer
		.SetDefaultSubobjectClass<UCharacterStatComponent>(TEXT("StatComponent")))
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MainWeapon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MainWeapon"));
	MainWeapon->SetupAttachment(GetMesh());

	SubEquip = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SubEquip"));
	SubEquip->SetupAttachment(GetMesh());

	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Character_NPC"));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));

	AIControllerClass = AEnemyBaseAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);

	bUseControllerRotationYaw = false;
	//GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->bOrientRotationToMovement = true;

	GetCharacterMovement()->MaxAcceleration = 1024.f;      // 기본 2048, 절반으로 → 더 부드러운 가속
	GetCharacterMovement()->BrakingDecelerationWalking = 1024.f;  // 기본 2048, 절반
	GetCharacterMovement()->BrakingFriction = 2.f;         // 기본 0
	GetCharacterMovement()->bUseSeparateBrakingFriction = true;

	HPBarWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HPBarWidget"));
	HPBarWidgetComponent->SetupAttachment(GetRootComponent());
	HPBarWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);  // 카메라 향함, 거리 무관 크기 유지
	HPBarWidgetComponent->SetDrawSize(FVector2D(200.0f, 20.0f));
	HPBarWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
	HPBarWidgetComponent->SetVisibility(true);  // 컴포넌트 자체는 항상 활성, 위젯이 보이고 숨길 뿐
	static ConstructorHelpers::FClassFinder<UOverheadHPWidget> HPWidgetFinder(
		TEXT("/Game/08_UI/Screens/WBP_OverheadHP"));

	UE_LOG(Log_Character_Enemy, Warning, TEXT("[EnemyBase Ctor] FClassFinder Succeeded: %d"),
		HPWidgetFinder.Succeeded());

	if (HPWidgetFinder.Succeeded())
	{
		HPBarWidgetClass = HPWidgetFinder.Class;
		UE_LOG(Log_Character_Enemy, Warning, TEXT("[EnemyBase Ctor] HPBarWidgetClass set: %s"),
			HPBarWidgetClass ? *HPBarWidgetClass->GetName() : TEXT("NULL"));
	}
	else
	{
		UE_LOG(Log_Character_Enemy, Error, TEXT("[EnemyBase Ctor] FClassFinder FAILED for path: /Game/08_UI/Screens/WBP_OverheadHP"));
	}

	TeamID = 1;
}

// Called when the game starts or when spawned
void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();
	if (HitReactionComponent)
	{
		HitReactionComponent->HitEndDelegate.AddUObject(this, &AEnemyBase::HandleStanceBreakEnded);
	}
	
	UEnemyDataSubsystem* EnemyDataSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UEnemyDataSubsystem>();

	if (!EnemyDataSubsystem)
	{
		UE_LOG(Log_Spawn_NPC, Error, TEXT("[EnemyBase] EnemyDataSubsystem not found."));
		return;
	}

	const FEnemyDataSet* Data = EnemyDataSubsystem->GetEnemyData(EnemyID);

	if (!Data || !Data->Info || !Data->Stats)
	{
		UE_LOG(Log_Spawn_NPC, Warning, TEXT("[EnemyBase] Missing Enemy data for %s"), *GetName());
		return;
	}

	ApplyEnemyInfo(Data->Info);
	ApplyEnemyStats(Data->Stats);
	//UAIBlueprintHelperLibrary::SimpleMoveToLocation(GetController(), FVector(-230.f, 4400.f, 0.f));
}

// Called every frame
void AEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AEnemyBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	UE_LOG(Log_Character_Enemy, Warning, TEXT("[EnemyBase] PostInitializeComponents - WidgetComp: %s, WidgetClass: %s"),
		HPBarWidgetComponent ? TEXT("OK") : TEXT("NULL"),
		HPBarWidgetClass ? TEXT("OK") : TEXT("NULL"));

	if (HPBarWidgetComponent && HPBarWidgetClass)
	{
		HPBarWidgetComponent->SetWidgetClass(HPBarWidgetClass);
		HPBarWidgetComponent->InitWidget();

		UUserWidget* UW = HPBarWidgetComponent->GetUserWidgetObject();

		if (UOverheadHPWidget* OverheadHP = Cast<UOverheadHPWidget>(UW))
		{
			if (UStatComponent* Stat = GetStatComponent())
			{
				OverheadHP->BindToStatComponent(Stat);
			}
		}
	}
}

bool AEnemyBase::ApplyEnemyInfo(const FEnemyInfo* Info)
{
	bool bDataValid = true;

	if (!Info)
	{
		UE_LOG(Log_Spawn_NPC, Warning, TEXT("[EnemyBase] Enemy : %s Info Not Valid"), *GetName());
		return false;
	}

	const UEnemyInstanceDataAsset* InstanceData = Info->InstanceData.LoadSynchronous();
	if (!InstanceData)
	{
		UE_LOG(Log_Spawn_NPC, Warning, TEXT("[EnemyBase] Enemy : %s Instance Data Not Valid"), *GetName());
		return false;
	}

	const USkeletalMesh* SkeletalMesh = InstanceData->SkeletalMesh;
	if(!SkeletalMesh)
	{
		UE_LOG(Log_Spawn_NPC, Warning, TEXT("[EnemyBase] Enemy : %s Mesh Not Valid"), *GetName());
		return false;
	}

	const UClass* Anim = InstanceData->AnimBlueprint;
	if (!Anim)
	{
		UE_LOG(Log_Spawn_NPC, Warning, TEXT("[EnemyBase] Enemy : %s AnimInstance Not Valid"), *GetName());
		return false;
	}
	
	GetMesh()->SetSkeletalMesh(InstanceData->SkeletalMesh);
	GetMesh()->SetAnimInstanceClass(InstanceData->AnimBlueprint);
	if (HPBarWidgetComponent)
	{
		HPBarWidgetComponent->AttachToComponent(
			GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	}
	UpdateHealthBarPlacement(InstanceData->HealthBarHeightOffset);
	bCanBeCriticallyExecuted = InstanceData->bCanBeCriticallyExecuted;

	if(const UWeaponDataAsset* WeaponDataAsset = InstanceData->WeaponData)
	{ 
		if (WeaponDataAsset->WeaponInstance.IsValid())
		{
			MainWeapon->SetStaticMesh(WeaponDataAsset->WeaponInstance.Mesh.LoadSynchronous());
			MainWeapon->AttachToComponent(
				GetMesh(),
				FAttachmentTransformRules::SnapToTargetIncludingScale,
				FName("S_Weapon")
			);
		}
	}
	
	GaitData = InstanceData->LocomotionGaitData;
	SetCurLocomotionGait(ELocomotionGait::Walk);

	CharacterBaseAnim = Cast<UEnemyBaseAnimInstance>(GetMesh()->GetAnimInstance());

	UNPCAnimRegistrySubsystem* NPCAnimRegistrySystem = GetGameInstance()->GetSubsystem<UNPCAnimRegistrySubsystem>();
		
	if (!NPCAnimRegistrySystem)
	{
		UE_LOG(Log_Spawn_NPC, Warning, TEXT("[EnemyBase] Enemy : %s NPCAnimRegistrySystem Not Valid"), *GetName());
		return false;
	}

	FGameplayTag ProfileTag = InstanceData->UseDefaultAnim ? FGameplayTag::RequestGameplayTag(TEXT("Weapon.Unarmed")) : InstanceData->WeaponTag;
	if (!ProfileTag.IsValid())
	{
		UE_LOG(Log_Spawn_NPC, Warning, TEXT("[EnemyBase] Enemy : %s ProfileTag Not Valid"), *GetName());
		return false;
	}

	const FAnimDataSet* Profile = NPCAnimRegistrySystem->GetAnimProfile(InstanceData->SkeletonTag);
	if(!Profile)
	{
		UE_LOG(Log_Spawn_NPC, Warning, TEXT("[EnemyBase] Enemy : %s Profile Not Valid, [ProfileTag : %s]"), *GetName(), *ProfileTag.ToString());
		return false;
	}

	CurrentProfileTag = InstanceData->SkeletonTag;
	AnimProfiles.Add(ProfileTag, *Profile);
	CriticalExecutionVictimEntries = Profile->CriticalExecutions;
	CurrentWeaponTag = ProfileTag;

	CharacterBaseAnim->InitAnimationData(*AnimProfiles.Find(CurrentWeaponTag));
	HitReactionComponent->SetHitReactionDA(AnimProfiles.Find(CurrentWeaponTag)->HitReactionAnimSet);

	AEnemyBaseAIController* AI = Cast<AEnemyBaseAIController>(GetController());
	if (!AI)
	{
		UE_LOG(Log_Spawn_NPC, Warning, TEXT("[EnemyBase] Enemy : %s AI Not Valid"), *GetName());
		return false;
	}

	const UEnemyAIDataAsset* AIData = Info->AIData.LoadSynchronous();
	if (!AIData)
	{
		UE_LOG(Log_Spawn_NPC, Warning, TEXT("[EnemyBase] Enemy : %s AIData Not Valid"), *GetName());
		return false;
	}

	const UBehaviorTree* BehaviorTree = AIData->EnemyBehaviorTree;
	const UBlackboardData* Blackboard = AIData->EnemyBlackboard;
	if (!AIData->EnemyBehaviorTree || !AIData->EnemyBlackboard)
	{
		UE_LOG(Log_Spawn_NPC, Warning, TEXT("[EnemyBase] Enemy : %s AIControllerData Not Valid"), *GetName());
		return false;
	}

	AI->SetControllerData(AIData->EnemyBehaviorTree, AIData->EnemyBlackboard);

	const UEnemyComponentDataAsset* CombatData = Info->CombatData.LoadSynchronous();
	if (!CombatData)
	{
		UE_LOG(Log_Spawn_NPC, Warning, TEXT("[EnemyBase] Enemy : %s CombatData Not Valid"), *GetName());
		return false;
	}

	UDataTable* CombatPattern = CombatData->CombatPatternData;
	if (!CombatPattern)
	{
		UE_LOG(Log_Spawn_NPC, Warning, TEXT("[EnemyBase] Enemy : %s CombatPattern Not Valid"), *GetName());
		return false;
	}

	AI->SetCombatPatternData(CombatPattern);
	AttackComponent->InitAttackContextSet(&CombatData->AttackContextSet);

	return true;
}

bool AEnemyBase::ApplyEnemyStats(const FEnemyStats* Stat)
{
	if (!Stat)
		return false;

	// 가드 행동 자체는 AI 전투 계층이 담당하지만, 가드 판정에 쓰는 수치는
	// 적 스탯 데이터가 로드되는 시점에 런타임 캐릭터로 전달한다.
	GuardNegation = FMath::Clamp(Stat->GuardNegation, 0.0f, 100.0f);

	FNPCStats RuntimeStats;
	RuntimeStats.BaseStats.Health.InitResource(Stat->Health);
	RuntimeStats.BaseStats.Poise.InitResource(Stat->Poise);
	RuntimeStats.BaseStats.PhysicalDefense = Stat->PhysicalDefense;
	RuntimeStats.BaseStats.MagicDefense = Stat->MagicDefense;
	RuntimeStats.BaseStats.FireResistance = Stat->FireResistance;
	RuntimeStats.BaseStats.FrostResistance = Stat->FrostResistance;
	RuntimeStats.BaseStats.PoisonResistance = Stat->PoisonResistance;
	RuntimeStats.BaseStats.BleedResistance = Stat->BleedResistance;
	RuntimeStats.MagicAttackPower = Stat->MagicAttackPower;
	RuntimeStats.PhysicalAttackPower = Stat->PhysicalAttackPower;
	RuntimeStats.PoiseAttackPower = Stat->PoiseAttackPower;
	RuntimeStats.StaminaAttackPower = Stat->StaminaAttackPower;
	RuntimeStats.Stance.InitResource(Stat->Stance);

	GetStatComponent()->InitializeNPCStats(RuntimeStats);
	GetStatComponent()->BroadcastResourceStat(EResourceStatType::Health, RuntimeStats.BaseStats.Health);

	return true;
}

FAttackTraceSource AEnemyBase::GetAttackTraceSource(EAttackSourceType AttackSourceType) const
{
	FAttackTraceSource OutSource;

	switch (AttackSourceType)
	{
	case EAttackSourceType::MainHand:
	{
		OutSource.TraceComponent = MainWeapon;
		OutSource.Radius = 10.0f;
		break;
	}
	case EAttackSourceType::OffHand:
	{
		OutSource.TraceComponent = SubEquip;
		OutSource.Radius = 10.0f;
		break;
	}
	}

	return OutSource;
}

FAttackDamageSource AEnemyBase::GetAttackDamageSource() const
{
	FAttackDamageSource OutData;
	
	OutData.AttackRating = GetStatComponent()->GetNPCStats().PhysicalAttackPower;
	OutData.PoiseRating = GetStatComponent()->GetNPCStats().PoiseAttackPower;
	OutData.StanceRating = GetStatComponent()->GetNPCStats().StaminaAttackPower;

	return OutData;
}

void AEnemyBase::ReceiveParried(AActor* ParryInstigator)
{
	if (!CharacterStatusComponent || !HitReactionComponent || CharacterStatusComponent->IsDead())
	{
		return;
	}

	const FVector InstigatorLocation = IsValid(ParryInstigator)
		? ParryInstigator->GetActorLocation() : GetActorLocation() + GetActorForwardVector();
	BreakStance(HitReactionComponent->CalculateHitAngle(InstigatorLocation));
}

bool AEnemyBase::BreakStance(float HitAngle)
{
	if (!CharacterStatusComponent || !HitReactionComponent || CharacterStatusComponent->IsDead())
	{
		return false;
	}

	ResetCriticalExecutionWindow();
	if (AttackComponent)
	{
		AttackComponent->CancelAttack(EActionExitReason::Interrupted, true);
	}

	if (UCharacterStatComponent* EnemyStatComponent = GetStatComponent())
	{
		EnemyStatComponent->BreakStance();
	}
	StanceBroken = true;

	CharacterStatusComponent->SwitchAction(TAG_Action_HitReact, EActionExitReason::Interrupted);
	const FHitReactionRequest ReactionRequest{ ECombatReaction::StanceBreak, HitAngle };
	const bool bPlayedReaction = HitReactionComponent->ExecuteHitResponse(ReactionRequest);
	UE_LOG(Log_Hit, Log, TEXT("[StanceBreak] Enemy=%s Played=%d"),
		*GetNameSafe(this), bPlayedReaction);
	if (!bPlayedReaction &&
		CharacterStatusComponent->GetCurrentAction().MatchesTagExact(TAG_Action_HitReact))
	{
		CharacterStatusComponent->ClearAction();
	}
	if (!bPlayedReaction)
	{
		HandleStanceBreakEnded();
	}

	return bPlayedReaction;
}

bool AEnemyBase::CanReceiveCriticalExecution(const APlayerBase* Executor) const
{
	if (!Executor || !bCanBeCriticallyExecuted || !StanceBroken ||
		CriticalExecutionWindowCount <= 0 || bCriticalExecutionActive ||
		!CharacterStatusComponent || CharacterStatusComponent->IsDead())
	{
		return false;
	}

	FCriticalExecutionAttackerEntry AttackerData;
	FCriticalExecutionVictimEntry VictimData;
	return FindCriticalExecutionData(Executor, AttackerData, VictimData);
}

void AEnemyBase::BeginCriticalExecutionWindow()
{
	if (StanceBroken && !bCriticalExecutionActive && CharacterStatusComponent &&
		!CharacterStatusComponent->IsDead())
	{
		++CriticalExecutionWindowCount;
	}
}

void AEnemyBase::EndCriticalExecutionWindow()
{
	CriticalExecutionWindowCount = FMath::Max(0, CriticalExecutionWindowCount - 1);
}

void AEnemyBase::ResetCriticalExecutionWindow()
{
	CriticalExecutionWindowCount = 0;
}

ECriticalExecutionDirection AEnemyBase::CalculateCriticalExecutionDirection(const APlayerBase* Executor) const
{
	if (!Executor)
	{
		return ECriticalExecutionDirection::Front;
	}

	const FVector ToExecutor = Executor->GetActorLocation() - GetActorLocation();
	const float ExecutorYaw = ToExecutor.Rotation().Yaw;
	const float RelativeAngle = FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw, ExecutorYaw);

	if (FMath::Abs(RelativeAngle) <= 45.0f)
	{
		return ECriticalExecutionDirection::Front;
	}
	if (RelativeAngle > 45.0f && RelativeAngle < 135.0f)
	{
		return ECriticalExecutionDirection::Right;
	}
	if (RelativeAngle < -45.0f && RelativeAngle > -135.0f)
	{
		return ECriticalExecutionDirection::Left;
	}
	return ECriticalExecutionDirection::Back;
}

bool AEnemyBase::FindCriticalExecutionData(const APlayerBase* Executor,
	FCriticalExecutionAttackerEntry& OutAttacker,
	FCriticalExecutionVictimEntry& OutVictim) const
{
	if (!Executor)
	{
		return false;
	}

	const TArray<FCriticalExecutionAttackerEntry>& AttackerEntries =
		Executor->GetConfiguredCriticalExecutions();
	if (AttackerEntries.IsEmpty() || CriticalExecutionVictimEntries.IsEmpty())
	{
		return false;
	}

	auto TryFindDirection = [this, &AttackerEntries, &OutAttacker, &OutVictim](
		ECriticalExecutionDirection Direction) -> bool
	{
		// 플레이어 프로필의 배열 순서가 같은 방향 안에서의 우선순위다.
		for (const FCriticalExecutionAttackerEntry& AttackerEntry : AttackerEntries)
		{
			if (AttackerEntry.Direction != Direction || !AttackerEntry.IsConfigured())
			{
				continue;
			}

			const FCriticalExecutionVictimEntry* VictimEntry =
				CriticalExecutionVictimEntries.FindByPredicate(
					[&AttackerEntry, Direction](const FCriticalExecutionVictimEntry& Candidate)
					{
						return Candidate.Direction == Direction &&
							Candidate.ExecutionID == AttackerEntry.ExecutionID &&
							Candidate.IsConfigured();
					});
			if (VictimEntry)
			{
				OutAttacker = AttackerEntry;
				OutVictim = *VictimEntry;
				return true;
			}
		}
		return false;
	};

	const ECriticalExecutionDirection RequestedDirection =
		CalculateCriticalExecutionDirection(Executor);

	// 방향이 다른 몽타주 쌍으로 대체하지 않는다. 계산된 방향에 정확히 대응하는
	// 공격자/피격자 쌍이 없으면 Execution 대상이 아니며 일반 공격으로 넘어간다.
	return TryFindDirection(RequestedDirection);
}

bool AEnemyBase::BeginCriticalExecution(APlayerBase* Executor, UAnimMontage*& OutAttackerMontage)
{
	OutAttackerMontage = nullptr;
	if (!CanReceiveCriticalExecution(Executor))
	{
		return false;
	}

	FCriticalExecutionAttackerEntry AttackerData;
	FCriticalExecutionVictimEntry VictimData;
	if (!FindCriticalExecutionData(Executor, AttackerData, VictimData))
	{
		return false;
	}

	UAnimMontage* AttackerMontage = AttackerData.Montage.LoadSynchronous();
	UAnimMontage* VictimMontage = VictimData.Montage.LoadSynchronous();
	UAnimInstance* VictimAnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AttackerMontage || !VictimMontage || !VictimAnimInstance)
	{
		return false;
	}

	const FTransform AttackerWorldTransform =
		AttackerData.AttackerRelativeTransform * GetActorTransform();
	const UCapsuleComponent* ExecutorCapsule = Executor->GetCapsuleComponent();
	if (!ExecutorCapsule || !GetWorld())
	{
		return false;
	}

	// 상태를 전환하기 전에 스냅 위치가 월드 장애물 안인지 확인한다.
	// 피격 대상과 실행자 본인은 동기 연출을 위해 검사에서 제외한다.
	FCollisionQueryParams PlacementQueryParams(
		SCENE_QUERY_STAT(CriticalExecutionPlacement), false, Executor);
	PlacementQueryParams.AddIgnoredActor(this);
	const FCollisionShape ExecutorShape = FCollisionShape::MakeCapsule(
		ExecutorCapsule->GetScaledCapsuleRadius(),
		ExecutorCapsule->GetScaledCapsuleHalfHeight());
	if (GetWorld()->OverlapBlockingTestByProfile(
		AttackerWorldTransform.GetLocation(), FQuat::Identity,
		ExecutorCapsule->GetCollisionProfileName(), ExecutorShape, PlacementQueryParams))
	{
		return false;
	}

	bCriticalExecutionActive = true;
	ResetCriticalExecutionWindow();
	ActiveCriticalExecutor = Executor;
	ActiveCriticalVictimMontage = VictimMontage;
	if (HPBarWidgetComponent)
	{
		HPBarWidgetComponent->SetVisibility(false);
	}

	// StanceBreak 몽타주만 종료하고 자세 붕괴 상태는 처형이 끝날 때까지 유지한다.
	if (HitReactionComponent)
	{
		HitReactionComponent->CancelHitReaction(EActionExitReason::Interrupted, true);
	}
	if (AttackComponent)
	{
		AttackComponent->CancelAttack(EActionExitReason::Interrupted, true);
	}
	if (CharacterStatusComponent)
	{
		CharacterStatusComponent->SwitchAction(TAG_Action_HitReact, EActionExitReason::Interrupted);
	}
	GetCharacterMovement()->DisableMovement();

	Executor->SetActorLocationAndRotation(
		AttackerWorldTransform.GetLocation(), AttackerWorldTransform.Rotator(),
		false, nullptr, ETeleportType::TeleportPhysics);

	if (VictimAnimInstance->Montage_Play(VictimMontage) <= 0.0f)
	{
		FinishCriticalExecution(Executor, false);
		return false;
	}

	OutAttackerMontage = AttackerMontage;
	return true;
}

void AEnemyBase::FinishCriticalExecution(APlayerBase* Executor, bool bCompleted)
{
	if (!bCriticalExecutionActive || ActiveCriticalExecutor.Get() != Executor)
	{
		return;
	}

	bCriticalExecutionActive = false;
	ActiveCriticalExecutor.Reset();

	if (!bCompleted)
	{
		if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			if (ActiveCriticalVictimMontage && AnimInstance->Montage_IsPlaying(ActiveCriticalVictimMontage))
			{
				AnimInstance->Montage_Stop(0.1f, ActiveCriticalVictimMontage);
			}
		}
		ActiveCriticalVictimMontage = nullptr;

		// 외부 사망이 Execution을 중단시킨 경우에는 살아 있는 대상용 복구
		// (이동, 체력바, Stance, AI)를 적용하지 않는다.
		if (CharacterStatusComponent && CharacterStatusComponent->IsDead())
		{
			return;
		}

		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		if (HPBarWidgetComponent)
		{
			HPBarWidgetComponent->SetVisibility(true);
		}
		if (CharacterStatusComponent &&
			CharacterStatusComponent->GetCurrentAction().MatchesTagExact(TAG_Action_HitReact))
		{
			CharacterStatusComponent->ClearAction();
		}
		HandleStanceBreakEnded();
		if (AEnemyBaseAIController* EnemyController = Cast<AEnemyBaseAIController>(GetController()))
		{
			// Execution 진입 때 무시했던 HitEnd 복귀를 중단 시점에 명시적으로 수행한다.
			EnemyController->OnStaggeredEnded();
		}
		return;
	}

	ActiveCriticalVictimMontage = nullptr;
	bCriticalExecutionDeathPending = true;
	if (UCharacterStatComponent* EnemyStats = GetStatComponent())
	{
		EnemyStats->Kill();
	}
	else if (CharacterStatusComponent && !CharacterStatusComponent->IsDead())
	{
		CharacterStatusComponent->EnterDeath();
	}
}

void AEnemyBase::HandleStanceBreakEnded()
{
	if (!StanceBroken || bCriticalExecutionActive)
	{
		return;
	}

	ResetCriticalExecutionWindow();
	StanceBroken = false;
	if (UCharacterStatComponent* EnemyStatComponent = GetStatComponent())
	{
		EnemyStatComponent->RestoreStance();
	}
}

void AEnemyBase::OnLockedOnByPlayer(bool bIsLockedOn)
{
	if (!HPBarWidgetComponent) return;

	if (UOverheadHPWidget* OverheadHP = Cast<UOverheadHPWidget>(HPBarWidgetComponent->GetUserWidgetObject()))
	{
		OverheadHP->OnLockOnChanged(bIsLockedOn);
	}
}

void AEnemyBase::OnHit_Implementation(const FAttackRequest& AttackInfo)
{
	// 동기 처형 연출 중에는 실행자와 대상 모두 외부 공격에 영향받지 않는다.
	if (bCriticalExecutionActive)
	{
		return;
	}

	const FHitResolution Resolution = HitReactionComponent->ResolveHit(AttackInfo);
	const float HitAngle = Resolution.HitAngle;

	if (Resolution.Outcome == EHitOutcome::Avoided)
	{
		return;
	}

	if (Resolution.Outcome == EHitOutcome::Parried)
	{
		if (IAttackSourceInterface* AttackSource = Cast<IAttackSourceInterface>(AttackInfo.AttackCauser))
		{
			AttackSource->ReceiveParried(this);
		}
		return;
	}

	ECombatReaction Response = Resolution.Reaction;

	UE_LOG(Log_Hit, Log, TEXT("[EnemyBase] %s was hit by attack that required a %s"), *this->GetName(), *StaticEnum<ECombatReaction>()->GetNameStringByValue((int64)Response));

	if (Resolution.Outcome == EHitOutcome::Hit)
	{
		bool bPoiseBroken = false;
		if (!ApplyDirectHitStats(AttackInfo, bPoiseBroken))
		{
			return;
		}

		switch (Response)
		{
		case ECombatReaction::None:
			if (bPoiseBroken)
			{
				RestorePoise();
			}
			return;

		case ECombatReaction::Flinch:
		case ECombatReaction::KnockBack:
		case ECombatReaction::KnockDown:
		{
			const bool bUpgradeActiveReaction =
				HitReactionComponent->CanUpgradeActiveReaction(Response);
			if (bPoiseBroken || bUpgradeActiveReaction)
			{
				UE_LOG(Log_Hit, Log, TEXT("[EnemyBase] %s stagger occurred"), *GetName());
				const FHitReactionRequest InputReaction = { Response, HitAngle };
				const bool bExecuted = bUpgradeActiveReaction
					? HitReactionComponent->ExecuteHitResponse(InputReaction)
					: TryExecuteHitReaction(InputReaction);
				if (!bExecuted)
				{
					RestorePoise();
				}
			}
			return;
		}

		case ECombatReaction::HitAir:
			if (bPoiseBroken)
			{
				const FHitReactionRequest InputReaction = { ECombatReaction::HitAir, HitAngle };
				if (!TryExecuteHitReaction(InputReaction))
				{
					RestorePoise();
				}
			}
			return;

		default:
			UE_LOG(Log_Hit, Warning, TEXT("[EnemyBase] Unexpected direct-hit response: %s"),
				*StaticEnum<ECombatReaction>()->GetNameStringByValue(static_cast<int64>(Response)));
			return;
		}
	}

	switch (Response)
	{
	case ECombatReaction::GuardHit:
	case ECombatReaction::GuardHitHeavy:
	{
		bool IsStaminaEnough = GetStatComponent()->ChangeStance(AttackInfo.StanceDamage, EStatChangeType::Damage);
		if (IsStaminaEnough)
		{
			float ApplyNegationDamage = AttackInfo.Damage * (1.0f - GuardNegation / 100.0f);
			GetStatComponent()->ApplyDamage(ApplyNegationDamage, AttackInfo.AttackType);
			if (!CharacterStatusComponent->IsDead())
			{
				FHitReactionRequest InputReaction = { Response, HitAngle };
				TryExecuteHitReaction(InputReaction);
			}
		}
		else
		{
			GetStatComponent()->ApplyDamage(AttackInfo.Damage, AttackInfo.AttackType);
			if (!CharacterStatusComponent->IsDead())
			{
				GetStatComponent()->BreakStance();
				StanceBroken = true;
				const FHitReactionRequest InputReaction = { ECombatReaction::GuardBreak, HitAngle };
				if (!TryExecuteHitReaction(InputReaction))
				{
					HandleStanceBreakEnded();
				}
			}
		}
		break;
	}
	}
}

void AEnemyBase::UpdateHealthBarPlacement(float AdditionalHeight)
{
	if (!HPBarWidgetComponent || !GetMesh() || !GetMesh()->GetSkeletalMeshAsset())
	{
		return;
	}

	// 애니메이션 본에는 붙이지 않고, 캡슐 로컬 공간으로 변환한 메시 Bounds 상단을 사용한다.
	const FBoxSphereBounds MeshBounds = GetMesh()->CalcBounds(GetMesh()->GetRelativeTransform());
	const float BoundsTop = MeshBounds.Origin.Z + MeshBounds.BoxExtent.Z;
	HPBarWidgetComponent->SetRelativeLocation(
		FVector(0.0f, 0.0f, BoundsTop + FMath::Max(0.0f, AdditionalHeight)));
}

bool AEnemyBase::TryExecuteHitReaction(const FHitReactionRequest& ReactionRequest)
{
	if (!CharacterStatusComponent || !HitReactionComponent ||
		CharacterStatusComponent->IsDead())
	{
		return false;
	}

	// 피격은 입력 Window의 허가를 받는 행동이 아니라 외부에서 강제되는 전환이다.
	// 따라서 버퍼나 CanTryAction을 거치지 않고 현재 프레임에 즉시 전환한다.
	CharacterStatusComponent->SwitchAction(TAG_Action_HitReact, EActionExitReason::Interrupted);
	if (!HitReactionComponent->ExecuteHitResponse(ReactionRequest))
	{
		if (CharacterStatusComponent->GetCurrentAction().MatchesTagExact(TAG_Action_HitReact))
		{
			CharacterStatusComponent->ClearAction();
		}
		return false;
	}

	return true;
}

void AEnemyBase::HandleDeathStarted()
{
	ResetCriticalExecutionWindow();
	Super::HandleDeathStarted();
	if (HPBarWidgetComponent)
	{
		HPBarWidgetComponent->SetVisibility(false);
	}

	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		AI->UnPossess();   // OnUnPossess → StopHostileMonitoring + 타겟 정리, 회전 정지
		AI->Destroy();
	}
}

void AEnemyBase::HandleDeathFinalized()
{
	Super::HandleDeathFinalized(); // 캡슐 콜리전 off
	SetLifeSpan(5.0f);             // 시체 소멸(또는 디졸브 후 Destroy)
	// 루트 드랍 / 타겟 목록에서 제거 등
}

void AEnemyBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

}

/* ============================================================
 *  Component Getters
 * ============================================================ */
UCharacterStatComponent* AEnemyBase::GetStatComponent() const
{
	return Cast<UCharacterStatComponent>(StatComponent);
}
