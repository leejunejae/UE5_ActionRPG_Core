// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/Components/EquipmentComponent.h"
#include "Characters/Player/Components/PlayerStatComponent.h"

#include "Characters/Player/PlayerBase.h"
#include "Core/Subsystems/GameInstanceSystem/WeaponDataSubsystem.h"
#include "Core/Subsystems/GameInstanceSystem/ArmorDataSubsystem.h"
#include "Items/Weapons/Data/WeaponDataAsset.h"
#include "Items/Armor/Data/ArmorDataAsset.h"
#include "Utils/CoreLog.h"

UEquipmentComponent::UEquipmentComponent()
{
}

void UEquipmentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();

	APlayerBase* Character = Cast<APlayerBase>(GetOwner());
	if (!Character) return;

	// 무기 메시 동적 생성
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

#pragma region Weapon

void UEquipmentComponent::EquipWeapon_Implementation(FName WeaponKey)
{
	UWorld* World = GetWorld();
	if (!WeaponMesh || !World) return;

	UWeaponDataSubsystem* WeaponSubsystem = World->GetGameInstance()->GetSubsystem<UWeaponDataSubsystem>();
	if (!WeaponSubsystem)
	{
		UE_LOG(Log_Equip_Weapon, Error, TEXT("[EquipmentComponent] WeaponDataSubsystem not found"));
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
		UE_LOG(Log_Equip_Weapon, Error, TEXT("[EquipmentComponent] Failed to load weapon data: %s"), *WeaponKey.ToString());
		return;
	}

	if (!FindWeapon->WeaponDefenition.Get()->WeaponInstance.IsValid() && WeaponKey != FName("Hand_Unarmed_01"))
	{
		UE_LOG(Log_Equip_Weapon, Error, TEXT("[EquipmentComponent] Weapon mesh missing: %s"), *WeaponKey.ToString());
		return;
	}

	WeaponMesh->SetStaticMesh(nullptr);
	SubEquipMesh->SetStaticMesh(nullptr);

	EquipedWeapon = FindWeapon;
	EquipedWeaponKey = WeaponKey;
	WeaponMesh->SetStaticMesh(EquipedWeapon->WeaponDefenition.Get()->WeaponInstance.Mesh.LoadSynchronous());

	if (EquipedWeapon->WeaponDefenition.Get()->WeaponInstance.HasSubWeapon)
	{
		SubEquipMesh->SetStaticMesh(EquipedWeapon->WeaponDefenition.Get()->WeaponInstance.SubMesh.LoadSynchronous());
	}

	RecalcEquipLoad();

	OnWeaponChangedDelegate.Broadcast(EquipedWeapon->WeaponDefenition.Get()->WeaponType);
}

FVector UEquipmentComponent::GetWeaponSocketLocation_Implementation(FName SocketName, bool IsSubWeapon) const
{
	if (!EquipedWeapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EquipmentComponent] GetWeaponSocketLocation called with no valid weapon"));
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
		OutData.Radius = EquipedWeapon->WeaponDefenition.Get()->WeaponInstance.WeaponConfig.HitBoxRadius;
		break;
	case EAttackSourceType::OffHand:
		OutData.Radius = EquipedWeapon->WeaponDefenition.Get()->WeaponInstance.SubConfig.HitBoxRadius;
		break;
	}
	return OutData;
}

void UEquipmentComponent::GetCurrentAttackBonuses(float& OutStrengthBonus, float& OutDexterityBonus, float& OutAffinityBonus) const
{
	OutStrengthBonus = 0.f;
	OutDexterityBonus = 0.f;
	OutAffinityBonus = 0.f;

	APlayerBase* Player = Cast<APlayerBase>(GetOwner());
	if (!Player) return;

	if (const UPlayerStatComponent* PlayerStat = Player->GetStatComponent())
	{
		const FPlayerCombatStats& Combat = PlayerStat->GetCharacterStats().CombatStats;
		OutStrengthBonus = Combat.StrengthAttackBonus;
		OutDexterityBonus = Combat.DexterityAttackBonus;
	}
}

FAttackDamageSource UEquipmentComponent::GetAttackDamageSource() const
{
	if (!EquipedWeapon) return FAttackDamageSource();

	float PerformanceRatio = 1.0f;
	APlayerBase* Player = Cast<APlayerBase>(GetOwner());
	if (!Player) return FAttackDamageSource() ;

	if (const UPlayerStatComponent* PlayerStat = Player->GetStatComponent())
	{
		PerformanceRatio = PlayerStat->GetWeaponPerformanceRatio(EquipedWeapon->RequiredAttributes.ToCharacterStats());
	}

	float StrengthBonus, DexterityBonus, AffinityBonus;
	GetCurrentAttackBonuses(StrengthBonus, DexterityBonus, AffinityBonus);

	return CalculateWeaponAttackDamageSource(EquipedWeapon, PerformanceRatio, StrengthBonus, DexterityBonus, AffinityBonus);
}

FAttackDamageSource UEquipmentComponent::PreviewAttackDamageSource(float OverrideStrengthBonus, float OverrideDexterityBonus, float OverrideAffinityBonus) const
{
	if (!EquipedWeapon) return FAttackDamageSource();

	float PerformanceRatio = 1.0f;

	APlayerBase* Player = Cast<APlayerBase>(GetOwner());
	if (!Player) return FAttackDamageSource();

	if (const UPlayerStatComponent* PlayerStat = Player->GetStatComponent())
	{
		PerformanceRatio = PlayerStat->GetWeaponPerformanceRatio(EquipedWeapon->RequiredAttributes.ToCharacterStats());
	}

	return CalculateWeaponAttackDamageSource(EquipedWeapon, PerformanceRatio, OverrideStrengthBonus, OverrideDexterityBonus, OverrideAffinityBonus);
}

FWeaponRequirementBreakdown UEquipmentComponent::GetEquippedWeaponRequirementBreakdown() const
{
	APlayerBase* Player = Cast<APlayerBase>(GetOwner());
	if (!Player) return FWeaponRequirementBreakdown();

	UPlayerStatComponent* PlayerStat = Player->GetStatComponent();

	if (!EquipedWeapon || !PlayerStat) return FWeaponRequirementBreakdown();

	const FCharacterAttributes CurrentAttrs = PlayerStat->GetBaseAttributesLevel();

	float StrengthBonus, DexterityBonus, AffinityBonus;
	GetCurrentAttackBonuses(StrengthBonus, DexterityBonus, AffinityBonus);

	return CalculateWeaponRequirementBreakdown(EquipedWeapon, CurrentAttrs, StrengthBonus, DexterityBonus, AffinityBonus);
}

#pragma endregion Weapon


#pragma region Armor

// 슬롯별 SkeletalMeshComponent 생성 → 루트 메시를 리더로 지정해 본 동기화
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
		ArmorComp->SetSkeletalMesh(nullptr); // 초기 비어있음

		ArmorMeshes.Add(Slot, ArmorComp);
		EquipedArmors.Add(Slot, nullptr);
		EquipedArmorKeys.Add(Slot, NAME_None);
	}
}

// 1. DataTable에서 FArmorPieceInfo 조회
// 2. ArmorDefinition 에셋 로드 → ArmorSlot 읽어 슬롯 자동 판단
// 3. 해당 슬롯 메시 교체 + EquipedArmors 갱신
// 4. RecalcArmorStats() 호출 (내부에서 RecalcEquipLoad()까지 연결됨)
//
// NOTE: 모든 방어구 부위를 호환할 베이스 메시 처리가 아직 없어
// 장착 시 캐릭터 베이스 메시를 통째로 숨기는 임시 처리 포함 (의도된 동작)
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
		UE_LOG(Log_Equip_Armor, Error, TEXT("[EquipmentComponent] ArmorMesh not found for slot of: %s"), *ArmorKey.ToString());
		return;
	}

	USkeletalMesh* LoadedMesh = ArmorAsset->Mesh.IsNull() ? nullptr : ArmorAsset->Mesh.LoadSynchronous();
	(*MeshPtr)->SetSkeletalMesh(LoadedMesh);

	EquipedArmors[Slot] = PieceInfo;
	EquipedArmorKeys.FindOrAdd(Slot) = ArmorKey;

	RecalcArmorStats();

	OnArmorChangedDelegate.Broadcast(Slot);
}

void UEquipmentComponent::UnequipArmor(EArmorSlot Slot)
{
	TObjectPtr<USkeletalMeshComponent>* MeshPtr = ArmorMeshes.Find(Slot);
	if (!MeshPtr) return;

	(*MeshPtr)->SetSkeletalMesh(nullptr);
	EquipedArmors[Slot] = nullptr;
	EquipedArmorKeys.FindOrAdd(Slot) = NAME_None;

	RecalcArmorStats();

	OnArmorChangedDelegate.Broadcast(Slot);
}

// 장착된 모든 슬롯 합산 → PlayerStatComponent에 반영
// 마지막에 RecalcEquipLoad()를 호출해 무게도 갱신
void UEquipmentComponent::RecalcArmorStats()
{
	APlayerBase* Player = Cast<APlayerBase>(GetOwner());
	if (!Player) return;

	UPlayerStatComponent* PlayerStat = Player->GetStatComponent();
	if (!PlayerStat) return;

	float TotalDefense = 0.f;
	float TotalMagicDefense = 0.f;
	float TotalFireRes = 0.f;
	float TotalFrostRes = 0.f;
	float TotalPoisonRes = 0.f;
	float TotalBleedRes = 0.f;

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
		}
	}

	PlayerStat->ApplyArmorStats(
		TotalDefense, TotalMagicDefense,
		TotalFireRes, TotalFrostRes, TotalPoisonRes, TotalBleedRes);

	RecalcEquipLoad();
}

#pragma endregion


#pragma region Shared

// 무기 무게(검+방패 합산값) + 장착된 모든 방어구 무게 합산
// → PlayerStatComponent::ApplyEquipLoad()로 한 번에 반영
void UEquipmentComponent::RecalcEquipLoad()
{
	APlayerBase* Player = Cast<APlayerBase>(GetOwner());
	if (!Player) return;

	UPlayerStatComponent* PlayerStat = Player->GetStatComponent();
	if (!PlayerStat) return;

	float TotalWeight = EquipedWeapon ? EquipedWeapon->WeightValue : 0.f;

	for (const auto& [Slot, Info] : EquipedArmors)
	{
		if (Info)
		{
			TotalWeight += Info->WeightValue;
		}
	}

	PlayerStat->ApplyEquipLoad(TotalWeight);
}

#pragma endregion