// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Enemies/EnemyBase.h"
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
#include "UI/OverheadHPWidget.h"

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
	HPBarWidgetComponent->SetupAttachment(GetMesh());  // 또는 GetRootComponent()
	HPBarWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);  // 카메라 향함, 거리 무관 크기 유지
	HPBarWidgetComponent->SetDrawSize(FVector2D(200.0f, 20.0f));
	HPBarWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 220.0f));  // 머리 위 (필요에 따라 조정)
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

	if(const UWeaponDataAsset* WeaponDataAsset = InstanceData->WeaponData)
	{ 
		if (WeaponDataAsset->WeaponInstance.IsValid())
		{
			MainWeapon->SetStaticMesh(WeaponDataAsset->WeaponInstance.WeaponMesh);
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
	float HitAngle = HitReactionComponent->CalculateHitAngle(AttackInfo.HitPoint);
	
	EHitResponse Response = HitReactionComponent->EvaluateHitResponse(AttackInfo);

	UE_LOG(Log_Hit, Log, TEXT("[EnemyBase] %s was hit by attack that required a %s"), *this->GetName(), *StaticEnum<EHitResponse>()->GetNameStringByValue((int64)Response));

	switch (Response)
	{
	case EHitResponse::Flinch:
	case EHitResponse::KnockBack:
	case EHitResponse::KnockDown:
	{
		GetStatComponent()->ApplyDamage(AttackInfo.Damage, AttackInfo.AttackType);
		GetStatComponent()->ChangePoise(AttackInfo.PoiseDamage, EStatChangeType::Damage);
		if (GetStatComponent()->GetNPCStats().BaseStats.Poise.Current <= 0.0f && !CharacterStatusComponent->IsDead())
		{
			UE_LOG(Log_Hit, Log, TEXT("[EnemyBase] %s stagger occurred"), *Owner->GetName());
			FHitReactionRequest InputReaction = { Response,HitAngle };
			GetCharacterStatusComponent()->RequestAction(TAG_Action_HitReact);
			GetHitReactionComponent()->ExecuteHitResponse(InputReaction);
			HitReactionComponent->ExecuteHitResponse(InputReaction);
		}
		break;
	}
	case EHitResponse::HitAir:
	{
		GetStatComponent()->ApplyDamage(AttackInfo.Damage, AttackInfo.AttackType);
		GetStatComponent()->ChangePoise(AttackInfo.PoiseDamage, EStatChangeType::Damage);
		if (GetStatComponent()->GetNPCStats().BaseStats.Poise.Current <= 0.0f || CharacterStatusComponent->IsDead())
		{
			CharacterBaseAnim->SetHitAir(true);
			GetCharacterStatusComponent()->RequestAction(TAG_Action_HitReact);
		}
		break;
	}
	case EHitResponse::Block:
	case EHitResponse::BlockLarge:
	{
		bool IsStaminaEnough = GetStatComponent()->ChangeStance(AttackInfo.StanceDamage, EStatChangeType::Damage);
		if (IsStaminaEnough)
		{
			float ApplyNegationDamage = AttackInfo.Damage * (1.0f - GuardNegation / 100.0f);
			GetStatComponent()->ApplyDamage(ApplyNegationDamage, AttackInfo.AttackType);
			if (!CharacterStatusComponent->IsDead())
			{
				FHitReactionRequest InputReaction = { Response, HitAngle };
				HitReactionComponent->ExecuteHitResponse(InputReaction);
			}
		}
		else
		{
			GetStatComponent()->ApplyDamage(AttackInfo.Damage, AttackInfo.AttackType);
			Response = Response == EHitResponse::Block ? EHitResponse::BlockBreak : EHitResponse::BlockStun;
		}
		break;
	}
	}
}

void AEnemyBase::HandleDeathStarted()
{
	Super::HandleDeathStarted();

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