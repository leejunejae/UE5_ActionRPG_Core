// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Components/EquipmentComponent.h"
#include "Characters/Player/Components/PlayerStatComponent.h"

#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMeshSocket.h"

#include "Core/Subsystems/GameInstanceSystem/WeaponDataSubsystem.h"
#include "Items/Weapons/Data/WeaponDataAsset.h"
#include "Utils/CoreLog.h"

// Sets default values for this component's properties
UEquipmentComponent::UEquipmentComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	//PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	ACharacter* Character = Cast<ACharacter>(GetOwner());

	if (!Character)
		return;
	
	for (UActorComponent* Comp : Character->GetComponents())
	{
		if (Comp->GetClass()->ImplementsInterface(UStatInterface::StaticClass()))
		{
			CachedStat = TScriptInterface<IStatInterface>(Comp);
			break;
		}
	}

	WeaponMesh = NewObject<UStaticMeshComponent>(GetOwner(), UStaticMeshComponent::StaticClass(), TEXT("WeaponMesh"));
	SubEquipMesh = NewObject<UStaticMeshComponent>(GetOwner(), UStaticMeshComponent::StaticClass(), TEXT("SubEquipMesh"));

	if (WeaponMesh && SubEquipMesh)
	{
		GetOwner()->AddInstanceComponent(WeaponMesh);
		WeaponMesh->RegisterComponent();
		WeaponMesh->AttachToComponent(Character->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocket);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WeaponMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		WeaponMesh->SetCollisionProfileName(TEXT("NoCollision"));
		WeaponMesh->SetGenerateOverlapEvents(false);
		WeaponMesh->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

		GetOwner()->AddInstanceComponent(SubEquipMesh);
		SubEquipMesh->RegisterComponent();
		SubEquipMesh->AttachToComponent(Character->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, SubEquipSocket);
		SubEquipMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SubEquipMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		SubEquipMesh->SetCollisionProfileName(TEXT("NoCollision"));
		SubEquipMesh->SetGenerateOverlapEvents(false);
		SubEquipMesh->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	}
}


// Called every frame
void UEquipmentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UEquipmentComponent::EquipWeapon_Implementation(FName WeaponKey)
{
	UWorld* World = GetWorld();
	if (WeaponMesh && World )
	{
		UWeaponDataSubsystem* WeaponSubsystem = World->GetGameInstance()->GetSubsystem<UWeaponDataSubsystem>();
		if (!WeaponSubsystem)
		{
			UE_LOG(LogTemp, Error, TEXT("[EquipmentComponent] WeaponSubSystem not found"));
			return;
		}
		const FWeaponSetsInfo* FindWeapon = WeaponSubsystem->GetWeaponInfo(WeaponKey);

		if (!FindWeapon)
		{
			UE_LOG(Log_Equip_Weapon, Error, TEXT("[EquipmentComponent] No weapon found for %s"), *WeaponKey.ToString());
			return;
		}

		if (!FindWeapon->WeaponDefenition.LoadSynchronous())
		{
			UE_LOG(Log_Equip_Weapon, Error, TEXT("[EquipmentComponent] Failed to load data %s"), *WeaponKey.ToString());
			return;
		}

		if (!FindWeapon->WeaponDefenition.Get()->WeaponInstance.IsValid()
			&& WeaponKey != FName("Hand_Unarmed_01"))
		{
			UE_LOG(Log_Equip_Weapon, Error, TEXT("[EquipmentComponent] Weapon Mesh Asset is Missing"), *WeaponKey.ToString());
			return;
		}

		WeaponMesh->SetStaticMesh(nullptr);
		SubEquipMesh->SetStaticMesh(nullptr);

		EquipedWeapon = FindWeapon;

		WeaponMesh->SetStaticMesh(EquipedWeapon->WeaponDefenition.Get()->WeaponInstance.WeaponMesh);
		//WeaponMesh->SetRelativeTransform(EquipedWeapon->WeaponDefenition);

		if (EquipedWeapon->WeaponDefenition.Get()->HasSubWeapon)
		{
			SubEquipMesh->SetStaticMesh(EquipedWeapon->WeaponDefenition.Get()->SubInstance.WeaponMesh);
			//SubEquipMesh->SetRelativeTransform(EquipedWeapon->SubWeapon.WeaponTransform);
		}

		OnWeaponChangedDelegate.Broadcast(EquipedWeapon->WeaponDefenition.Get()->WeaponType);
	}
}

FVector UEquipmentComponent::GetWeaponSocketLocation_Implementation(FName SocketName, bool IsSubWeapon) const
{
	if (!EquipedWeapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("GetWeaponSocketLocation called with no valid weapon! IsSubWeapon=%d"), IsSubWeapon);
		return FVector::ZeroVector;
	}

	return !IsSubWeapon ? WeaponMesh->GetSocketLocation(SocketName) : SubEquipMesh->GetSocketLocation(SocketName);
}

FAttackTraceSource UEquipmentComponent::GetAttackTraceSource(EAttackSourceType AttackSourceType) const
{
	FAttackTraceSource OutData;

	switch (AttackSourceType)
	{
	case EAttackSourceType::MainHand:
	{
		OutData.TraceComponent = WeaponMesh;
		OutData.Radius = EquipedWeapon->WeaponDefenition.Get()->WeaponConfig.HitBoxRadius;
		break;
	}
	case EAttackSourceType::OffHand:
	{
		OutData.TraceComponent = SubEquipMesh;
		OutData.Radius = EquipedWeapon->WeaponDefenition.Get()->SubConfig.HitBoxRadius;
		break;
	}
	}
	return OutData;
}

FAttackDamageSource UEquipmentComponent::GetAttackDamageSource() const
{
	FAttackDamageSource OutData;
	if (!EquipedWeapon) return OutData;

	// 1. 요구치 충족률 계산 (기존 로직 유지)
	float PerformanceRatio = 1.0f;
	if (CachedStat)
	{
		PerformanceRatio = IStatInterface::Execute_GetWeaponPerformanceRatio(
			CachedStat.GetObject(),
			EquipedWeapon->RequiredStats.ToCharacterStats());
	}

	// 2. 특성 보정값 읽기 (근력/민첩 공격력 보정)
	float StrengthBonus = 0.f;
	float DexterityBonus = 0.f;
	float AffinityBonus = 0.f;
	if (CachedStat)
	{
		// UPlayerStatComponent만 FPlayerCombatStats를 가짐
		// IStatInterface를 통해 캐스트 없이 접근하려면 인터페이스 확장이 필요하므로
		// 여기서는 직접 캐스트 (EquipmentComponent는 항상 플레이어에 붙어있음)
		if (const UPlayerStatComponent* PlayerStat = Cast<UPlayerStatComponent>(CachedStat.GetObject()))
		{
			const FPlayerCombatStats& Combat = PlayerStat->GetCharacterStats_Native().CombatStats;
			StrengthBonus = Combat.StrengthAttackBonus;
			DexterityBonus = Combat.DexterityAttackBonus;
		}
	}

	// 3. 최종 공격력 계산
	// AttackRating = (무기 기본공격력 × 요구치 충족률)
	//              + (특성 보정값 × 등급 배율)
	const float BaseAttack = EquipedWeapon->AttackPower * PerformanceRatio;
	const float AttributeAttack = EquipedWeapon->CalcAttributeAttackBonus(StrengthBonus, DexterityBonus, AffinityBonus);
	OutData.AttackRating = BaseAttack + AttributeAttack;

	// Poise/Stance는 요구치 충족률만 적용 (특성 보정 없음)
	OutData.PoiseRating = EquipedWeapon->PoisePower * PerformanceRatio;
	OutData.StanceRating = EquipedWeapon->StancePower * PerformanceRatio;

	return OutData;
}
