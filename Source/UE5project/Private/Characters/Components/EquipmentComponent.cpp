// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Components/EquipmentComponent.h"
#include "Characters/Player/Components/PlayerStatComponent.h"

#include "GameFramework/Character.h"

#include "Core/Subsystems/GameInstanceSystem/WeaponDataSubsystem.h"
#include "Core/Subsystems/GameInstanceSystem/ArmorDataSubsystem.h"

#include "Items/Weapons/Data/WeaponDataAsset.h"
#include "Items/Armor/Data/ArmorDataAsset.h"

#include "Utils/CoreLog.h"

// Sets default values for this component's properties
UEquipmentComponent::UEquipmentComponent()
{

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

	auto SetupStaticMesh = [&](UStaticMeshComponent* Comp, FName Socket)
		{
			GetOwner()->AddInstanceComponent(Comp);
			Comp->RegisterComponent();
			Comp->AttachToComponent(Character->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, Socket);
			Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Comp->SetCollisionResponseToAllChannels(ECR_Ignore);
			Comp->SetCollisionProfileName(TEXT("NoCollision"));
			Comp->SetGenerateOverlapEvents(false);
			Comp->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
		};

	SetupStaticMesh(WeaponMesh, WeaponSocket);
	SetupStaticMesh(SubEquipMesh, SubEquipSocket);

	// 방어구 메시 컴포넌트 초기화
	InitArmorMeshComponents(Character);
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

/* ============================================================
 *  방어구 메시 컴포넌트 초기화
 *  EArmorSlot 순회로 슬롯별 컴포넌트 생성 → TMap에 저장
 * ============================================================ */
void UEquipmentComponent::InitArmorMeshComponents(ACharacter* Character)
{
	const TArray<TPair<EArmorSlot, FName>> SlotDefs = {
		{ EArmorSlot::Head,  TEXT("ArmorMesh_Head")  },
		{ EArmorSlot::Chest, TEXT("ArmorMesh_Chest") },
		{ EArmorSlot::Hands, TEXT("ArmorMesh_Hands") },
		{ EArmorSlot::Legs,  TEXT("ArmorMesh_Legs")  },
	};

	for (const auto& [Slot, CompName] : SlotDefs)
	{
		USkeletalMeshComponent* ArmorComp = NewObject<USkeletalMeshComponent>(
			GetOwner(), USkeletalMeshComponent::StaticClass(), CompName);

		GetOwner()->AddInstanceComponent(ArmorComp);
		ArmorComp->RegisterComponent();
		ArmorComp->AttachToComponent(Character->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		ArmorComp->SetLeaderPoseComponent(Character->GetMesh());
		ArmorComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ArmorComp->SetCollisionResponseToAllChannels(ECR_Ignore);
		ArmorComp->SetCollisionProfileName(TEXT("NoCollision"));
		ArmorComp->SetGenerateOverlapEvents(false);
		ArmorComp->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
		ArmorComp->SetSkeletalMesh(nullptr); // 초기 비어있음 → 언더메시 노출

		ArmorMeshes.Add(Slot, ArmorComp);
		EquipedArmors.Add(Slot, nullptr);
	}
}

/* ============================================================
 *  방어구 장착
 *  1. DataTable에서 FArmorPieceInfo 조회
 *  2. ArmorDefinition 에셋 로드 → ArmorSlot 읽어 슬롯 자동 판단
 *  3. 해당 슬롯 메시 교체 + EquipedArmors 갱신
 *  4. RecalcArmorStats 호출
 * ============================================================ */
void UEquipmentComponent::EquipArmor(FName ArmorKey)
{
	if (ArmorKey == NAME_None)
	{
		UE_LOG(Log_Equip_Armor, Warning, TEXT("[EquipmentComponent] EquipArmor: ArmorKey is None. Use UnequipArmor(EArmorSlot) to remove."));
		return;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	UArmorDataSubsystem* ArmorSubsystem = World->GetGameInstance()->GetSubsystem<UArmorDataSubsystem>();
	if (!ArmorSubsystem)
	{
		UE_LOG(Log_Equip_Armor, Error, TEXT("[EquipmentComponent] ArmorDataSubsystem not found"));
		return;
	}

	const FArmorPieceInfo* PieceInfo = ArmorSubsystem->GetArmorPieceInfo(ArmorKey);
	if (!PieceInfo)
	{
		UE_LOG(Log_Equip_Armor, Error, TEXT("[EquipmentComponent] ArmorPiece not found: %s"), *ArmorKey.ToString());
		return;
	}

	UArmorDataAsset* ArmorAsset = PieceInfo->ArmorDefinition.LoadSynchronous();
	if (!ArmorAsset)
	{
		UE_LOG(Log_Equip_Armor, Error, TEXT("[EquipmentComponent] ArmorDefinition failed to load: %s"), *ArmorKey.ToString());
		return;
	}

	const EArmorSlot Slot = ArmorAsset->ArmorSlot;

	TObjectPtr<USkeletalMeshComponent>* MeshPtr = ArmorMeshes.Find(Slot);
	if (!MeshPtr)
	{
		UE_LOG(Log_Equip_Armor, Error, TEXT("[EquipmentComponent] ArmorMesh not found for slot: %s"), *ArmorKey.ToString());
		return;
	}

	// 메시 교체
	USkeletalMesh* LoadedMesh = ArmorAsset->Mesh.IsNull() ? nullptr : ArmorAsset->Mesh.LoadSynchronous();
	(*MeshPtr)->SetSkeletalMesh(LoadedMesh);

	// 수치 데이터 갱신
	EquipedArmors[Slot] = PieceInfo;

	RecalcArmorStats();

	ACharacter* Character = Cast<ACharacter>(GetOwner());

	if (!Character)
		return;

	Character->GetMesh()->SetHiddenInGame(true, false);
}

/* ============================================================
 *  방어구 해제
 * ============================================================ */
void UEquipmentComponent::UnequipArmor(EArmorSlot Slot)
{
	TObjectPtr<USkeletalMeshComponent>* MeshPtr = ArmorMeshes.Find(Slot);
	if (!MeshPtr) return;

	(*MeshPtr)->SetSkeletalMesh(nullptr);
	EquipedArmors[Slot] = nullptr;

	RecalcArmorStats();

	ACharacter* Character = Cast<ACharacter>(GetOwner());

	if (!Character)
		return;

	Character->GetMesh()->SetHiddenInGame(false, false);
}

/* ============================================================
 *  방어력 · 저항력 · 장비 하중 재계산
 *  장착된 모든 슬롯 합산 → PlayerStatComponent에 반영
 * ============================================================ */
void UEquipmentComponent::RecalcArmorStats()
{
	if (!CachedStat) return;

	UPlayerStatComponent* PlayerStat = Cast<UPlayerStatComponent>(CachedStat.GetObject());
	if (!PlayerStat) return;

	float TotalDefense = 0.f;
	float TotalMagicDefense = 0.f;
	float TotalFireRes = 0.f;
	float TotalFrostRes = 0.f;
	float TotalPoisonRes = 0.f;
	float TotalBleedRes = 0.f;
	float TotalWeight = 0.f;

	for (const auto& [Slot, Info] : EquipedArmors)
	{
		if (Info)
		{
			TotalDefense += Info->DefenseValue;
			TotalMagicDefense += Info->MagicDefenseValue;
			TotalFireRes += Info->FireResistance;
			TotalFrostRes += Info->FrostResistance;
			TotalPoisonRes += Info->PoisonResistance;
			TotalBleedRes += Info->BleedResistance;
			TotalWeight += Info->WeightValue;
		}
	}

	PlayerStat->ApplyArmorStats(
		TotalDefense, TotalMagicDefense,
		TotalFireRes, TotalFrostRes, TotalPoisonRes, TotalBleedRes,
		TotalWeight);
}