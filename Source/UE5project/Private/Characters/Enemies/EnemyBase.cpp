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

#include "Core/Subsystems/GameInstanceSystem/EnemyDataSubsystem.h"
#include "Core/Subsystems/GameInstanceSystem/NPCAnimRegistrySubsystem.h"

#include "Utils/CoreLog.h"

#include "Characters/Enemies/EnemyBase.h"

#include "Blueprint/AIBlueprintHelperLibrary.h"

// Sets default values
AEnemyBase::AEnemyBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	StatComponent = CreateDefaultSubobject<UCharacterStatComponent>(TEXT("StatComponent"));
	StatComponent->bAutoActivate = true;

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
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->bOrientRotationToMovement = true;

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

	//CharacterBaseAnim = Cast<UEnemyBaseAnimInstance>(GetMesh()->GetAnimInstance());
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
	RuntimeStats.BaseStats.Resistance = Stat->Resistance;
	RuntimeStats.Stance.InitResource(Stat->Stance);

	StatComponent->InitializeNPCStats(RuntimeStats);

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
	}
	case EAttackSourceType::OffHand:
	{
		OutSource.TraceComponent = SubEquip;
		OutSource.Radius = 10.0f;
	}
	}

	return OutSource;
}

FAttackDamageSource AEnemyBase::GetAttackDamageSource() const
{
	FAttackDamageSource OutData;

	//OutData.AttackRating = StatComponent->GetCommonStats().
	return FAttackDamageSource();
}

void AEnemyBase::OnHit_Implementation(const FAttackRequest& AttackInfo)
{
	float HitAngle = HitReactionComponent->CalculateHitAngle(AttackInfo.HitPoint);
	
	EHitResponse Response = HitReactionComponent->EvaluateHitResponse(AttackInfo);
	//FHitReactionRequest InputReaction = { Response, HitAngle };
	//HitReactionComponent->ExecuteHitResponse(InputReaction);

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
			HitReactionComponent->ExecuteHitResponse(InputReaction);
		}
		break;
	}
	case EHitResponse::None:
	{
		GetStatComponent()->ApplyDamage(AttackInfo.Damage, AttackInfo.AttackType);
		GetStatComponent()->ChangePoise(AttackInfo.PoiseDamage, EStatChangeType::Damage);
		if (GetStatComponent()->GetNPCStats().BaseStats.Poise.Current <= 0.0f || CharacterStatusComponent->IsDead())
		{
			CharacterBaseAnim->SetHitAir(true);
		}
		break;
	}
	case EHitResponse::Block:
	case EHitResponse::BlockLarge:
	{
		bool IsStaminaEnough = StatComponent->ChangeStance(AttackInfo.StanceDamage, EStatChangeType::Damage);
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

void AEnemyBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

}