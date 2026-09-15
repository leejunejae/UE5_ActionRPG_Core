// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/Components/EquipmentComponent.h"
#include "Characters/Player/Components/PlayerStatComponent.h"

#include "Characters/Player/PlayerBase.h"
#include "Core/Subsystems/GameInstanceSystem/WeaponDataSubsystem.h"
#include "Core/Subsystems/GameInstanceSystem/PlayerAnimRegistrySubsystem.h"
#include "Core/Subsystems/GameInstanceSystem/ArmorDataSubsystem.h"
#include "Items/Weapons/Data/WeaponDataAsset.h"
#include "Items/Weapons/Data/WeaponAudioData.h"
#include "Items/Weapons/Data/WeaponAudioUtility.h"
#include "Items/Armor/Data/ArmorDataAsset.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraSystem.h"
#include "Utils/CoreLog.h"
#include "Utils/GameplayTagsBase.h"

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
	WeaponMesh->SetRelativeScale3D(FVector::OneVector);
	SubEquipMesh->SetRelativeScale3D(FVector::OneVector);
	SubEquipMesh->SetVisibility(true, true);
	MainWeaponTrailSystem = nullptr;
	SubWeaponTrailSystem = nullptr;
	MainWeaponTrailMaterial = nullptr;
	SubWeaponTrailMaterial = nullptr;
	CachedWeaponSounds.Reset();

	EquipedWeapon = FindWeapon;
	EquipedWeaponKey = WeaponKey;
	const UWeaponDataAsset* MainDefinition = EquipedWeapon->WeaponDefenition.Get();
	CurrentGripMode = MainDefinition->GripType == EWeaponGripType::TwoHanded
		? EWeaponGripMode::TwoHanded : EWeaponGripMode::OneHanded;
	if (EquippedOffHandWeapon &&
		(MainDefinition->GripType == EWeaponGripType::TwoHanded || !MainDefinition->bCanEquipOffHand))
	{
		EquippedOffHandWeapon = nullptr;
		EquippedOffHandWeaponKey = NAME_None;
		CachedOffHandWeaponSounds.Reset();
	}
	WeaponMesh->SetStaticMesh(EquipedWeapon->WeaponDefenition.Get()->WeaponInstance.Mesh.LoadSynchronous());
	WeaponMesh->SetRelativeScale3D(MainDefinition->WeaponInstance.WeaponConfig.WeaponScale);
	MainWeaponTrailSystem = EquipedWeapon->WeaponDefenition.Get()->WeaponInstance.WeaponConfig.TrailSystem.LoadSynchronous();
	MainWeaponTrailMaterial = EquipedWeapon->WeaponDefenition.Get()->WeaponInstance.WeaponConfig.TrailMaterial.LoadSynchronous();
	CacheWeaponSounds(EquipedWeapon->WeaponDefenition.Get(), CachedWeaponSounds);

	if (EquippedOffHandWeapon && EquippedOffHandWeapon->WeaponDefenition.LoadSynchronous())
	{
		const UWeaponDataAsset* OffHandDefinition = EquippedOffHandWeapon->WeaponDefenition.Get();
		SubEquipMesh->SetStaticMesh(OffHandDefinition->WeaponInstance.Mesh.LoadSynchronous());
		SubEquipMesh->SetRelativeScale3D(OffHandDefinition->WeaponInstance.WeaponConfig.WeaponScale);
		SubWeaponTrailSystem = OffHandDefinition->WeaponInstance.WeaponConfig.TrailSystem.LoadSynchronous();
		SubWeaponTrailMaterial = OffHandDefinition->WeaponInstance.WeaponConfig.TrailMaterial.LoadSynchronous();
	}
	else if (MainDefinition->WeaponInstance.HasSubWeapon)
	{
		// 기존 검+방패 묶음 에셋 호환 경로.
		SubEquipMesh->SetStaticMesh(MainDefinition->WeaponInstance.SubMesh.LoadSynchronous());
		SubEquipMesh->SetRelativeScale3D(MainDefinition->WeaponInstance.SubConfig.WeaponScale);
		SubWeaponTrailSystem = MainDefinition->WeaponInstance.SubConfig.TrailSystem.LoadSynchronous();
		SubWeaponTrailMaterial = MainDefinition->WeaponInstance.SubConfig.TrailMaterial.LoadSynchronous();
	}

	RecalcEquipLoad();
	RefreshCombatStyle();
}

void UEquipmentComponent::EquipOffHandWeapon(FName WeaponKey)
{
	UWorld* World = GetWorld();
	if (!SubEquipMesh || !World) return;

	UWeaponDataSubsystem* WeaponSubsystem = World->GetGameInstance()->GetSubsystem<UWeaponDataSubsystem>();
	const FWeaponSetsInfo* Found = WeaponSubsystem ? WeaponSubsystem->GetWeaponInfo(WeaponKey) : nullptr;
	UWeaponDataAsset* Definition = Found ? Found->WeaponDefenition.LoadSynchronous() : nullptr;
	if (!Definition || !Definition->WeaponInstance.IsValid())
	{
		UE_LOG(Log_Equip_Weapon, Error, TEXT("[EquipmentComponent] Invalid off-hand weapon: %s"), *WeaponKey.ToString());
		return;
	}

	if (Definition->AllowedSlot == EEquipmentHandSlot::MainHand)
	{
		UE_LOG(Log_Equip_Weapon, Warning, TEXT("[EquipmentComponent] %s cannot be equipped in the off hand"), *WeaponKey.ToString());
		return;
	}

	const UWeaponDataAsset* MainDefinition = EquipedWeapon && EquipedWeapon->WeaponDefenition.Get()
		? EquipedWeapon->WeaponDefenition.Get() : nullptr;
	if (!MainDefinition)
	{
		UE_LOG(Log_Equip_Weapon, Warning, TEXT("[EquipmentComponent] Equip a main weapon before an off-hand item"));
		return;
	}
	if (MainDefinition->GripType == EWeaponGripType::TwoHanded || !MainDefinition->bCanEquipOffHand)
	{
		UE_LOG(Log_Equip_Weapon, Warning, TEXT("[EquipmentComponent] Current main weapon does not allow an off-hand item"));
		return;
	}

	EquippedOffHandWeapon = Found;
	EquippedOffHandWeaponKey = WeaponKey;
	SubEquipMesh->SetStaticMesh(Definition->WeaponInstance.Mesh.LoadSynchronous());
	SubEquipMesh->SetRelativeScale3D(Definition->WeaponInstance.WeaponConfig.WeaponScale);
	SubWeaponTrailSystem = Definition->WeaponInstance.WeaponConfig.TrailSystem.LoadSynchronous();
	SubWeaponTrailMaterial = Definition->WeaponInstance.WeaponConfig.TrailMaterial.LoadSynchronous();
	CacheWeaponSounds(Definition, CachedOffHandWeaponSounds);
	CurrentGripMode = EWeaponGripMode::OneHanded;
	RecalcEquipLoad();
	RefreshCombatStyle();
}

void UEquipmentComponent::UnequipOffHandWeapon()
{
	EquippedOffHandWeapon = nullptr;
	EquippedOffHandWeaponKey = NAME_None;
	CachedOffHandWeaponSounds.Reset();

	const UWeaponDataAsset* MainDefinition = EquipedWeapon && EquipedWeapon->WeaponDefenition.Get()
		? EquipedWeapon->WeaponDefenition.Get() : nullptr;
	if (MainDefinition && MainDefinition->WeaponInstance.HasSubWeapon)
	{
		// 이행 기간에는 기존 검+방패 묶음 에셋의 보조 메시를 독립 보조무기로 오인해 제거하지 않는다.
		if (SubEquipMesh) SubEquipMesh->SetStaticMesh(MainDefinition->WeaponInstance.SubMesh.LoadSynchronous());
		if (SubEquipMesh) SubEquipMesh->SetRelativeScale3D(MainDefinition->WeaponInstance.SubConfig.WeaponScale);
		SubWeaponTrailSystem = MainDefinition->WeaponInstance.SubConfig.TrailSystem.LoadSynchronous();
		SubWeaponTrailMaterial = MainDefinition->WeaponInstance.SubConfig.TrailMaterial.LoadSynchronous();
	}
	else
	{
		if (SubEquipMesh)
		{
			SubEquipMesh->SetStaticMesh(nullptr);
			SubEquipMesh->SetRelativeScale3D(FVector::OneVector);
		}
		SubWeaponTrailSystem = nullptr;
		SubWeaponTrailMaterial = nullptr;
	}
	RecalcEquipLoad();
	RefreshCombatStyle();
}

bool UEquipmentComponent::SetGripMode(EWeaponGripMode NewGripMode)
{
	const UWeaponDataAsset* Definition = EquipedWeapon && EquipedWeapon->WeaponDefenition.Get()
		? EquipedWeapon->WeaponDefenition.Get() : nullptr;
	if (!Definition) return false;

	const bool bAllowed = Definition->GripType == EWeaponGripType::Versatile ||
		(Definition->GripType == EWeaponGripType::OneHanded && NewGripMode == EWeaponGripMode::OneHanded) ||
		(Definition->GripType == EWeaponGripType::TwoHanded && NewGripMode == EWeaponGripMode::TwoHanded);
	if (!bAllowed) return false;
	if (CurrentGripMode == NewGripMode) return true;

	if (NewGripMode == EWeaponGripMode::TwoHanded && EquippedOffHandWeapon)
	{
		// 양손 파지는 독립 보조 슬롯을 실제로 비운다. 메시만 숨겨 두면 이후 한손 파지/무기 교체 시
		// 이전 보조무기가 사용자 요청 없이 다시 나타나고 장비 무게도 계속 적용된다.
		EquippedOffHandWeapon = nullptr;
		EquippedOffHandWeaponKey = NAME_None;
		CachedOffHandWeaponSounds.Reset();
		if (SubEquipMesh)
		{
			SubEquipMesh->SetStaticMesh(nullptr);
			SubEquipMesh->SetRelativeScale3D(FVector::OneVector);
		}
		SubWeaponTrailSystem = nullptr;
		SubWeaponTrailMaterial = nullptr;
		RecalcEquipLoad();
	}

	CurrentGripMode = NewGripMode;
	if (SubEquipMesh)
	{
		// 기존 HasSubWeapon 묶음 에셋은 독립 슬롯 정보가 없으므로 이행 기간에만 가시성으로 처리한다.
		SubEquipMesh->SetVisibility(NewGripMode == EWeaponGripMode::OneHanded, true);
	}
	RefreshCombatStyle();
	return true;
}

FGameplayTag UEquipmentComponent::ResolveCombatStyle() const
{
	const UWeaponDataAsset* MainDefinition = EquipedWeapon && EquipedWeapon->WeaponDefenition.Get()
		? EquipedWeapon->WeaponDefenition.Get() : nullptr;
	if (!MainDefinition) return TAG_CombatStyle_Unarmed;

	const UWeaponDataAsset* OffHandDefinition = EquippedOffHandWeapon && EquippedOffHandWeapon->WeaponDefenition.Get()
		? EquippedOffHandWeapon->WeaponDefenition.Get() : nullptr;
	const EWeaponCategory OffHandCategory =
		CurrentGripMode == EWeaponGripMode::OneHanded && OffHandDefinition
			? OffHandDefinition->GetEffectiveWeaponCategory()
			: EWeaponCategory::None;

	const UPlayerAnimRegistrySubsystem* Registry = GetWorld() && GetWorld()->GetGameInstance()
		? GetWorld()->GetGameInstance()->GetSubsystem<UPlayerAnimRegistrySubsystem>()
		: nullptr;
	if (Registry)
	{
		const FGameplayTag Resolved = Registry->ResolveCombatStyle(
			MainDefinition->GetEffectiveWeaponCategory(), OffHandCategory, CurrentGripMode);
		if (Resolved.IsValid())
		{
			return Resolved;
		}
	}

	return GetLegacyCombatStyleForWeaponType(MainDefinition->WeaponType);
}

void UEquipmentComponent::RefreshCombatStyle()
{
	const FGameplayTag ResolvedStyle = ResolveCombatStyle();
	CurrentCombatStyle = ResolvedStyle;
	OnWeaponChangedDelegate.Broadcast(CurrentCombatStyle);
}

FVector UEquipmentComponent::GetWeaponSocketLocation_Implementation(FName SocketName, bool IsSubWeapon) const
{
	const UStaticMeshComponent* TargetMesh = IsSubWeapon ? SubEquipMesh.Get() : WeaponMesh.Get();
	return TargetMesh ? TargetMesh->GetSocketLocation(SocketName) : FVector::ZeroVector;
}

UNiagaraSystem* UEquipmentComponent::GetWeaponTrailSystem_Implementation(bool IsSubWeapon) const
{
	return IsSubWeapon ? SubWeaponTrailSystem.Get() : MainWeaponTrailSystem.Get();
}

UMaterialInterface* UEquipmentComponent::GetWeaponTrailMaterial_Implementation(bool IsSubWeapon) const
{
	return IsSubWeapon ? SubWeaponTrailMaterial.Get() : MainWeaponTrailMaterial.Get();
}

FName UEquipmentComponent::GetWeaponTrailStartSocket_Implementation(bool IsSubWeapon) const
{
	if (!EquipedWeapon || !EquipedWeapon->WeaponDefenition.Get()) return TEXT("Start");
	if (IsSubWeapon && EquippedOffHandWeapon && EquippedOffHandWeapon->WeaponDefenition.Get())
	{
		return EquippedOffHandWeapon->WeaponDefenition.Get()->WeaponInstance.WeaponConfig.TrailStartSocket;
	}
	const FWeaponInstance& Instance = EquipedWeapon->WeaponDefenition.Get()->WeaponInstance;
	return IsSubWeapon ? Instance.SubConfig.TrailStartSocket : Instance.WeaponConfig.TrailStartSocket;
}

FName UEquipmentComponent::GetWeaponTrailEndSocket_Implementation(bool IsSubWeapon) const
{
	if (!EquipedWeapon || !EquipedWeapon->WeaponDefenition.Get()) return TEXT("End");
	if (IsSubWeapon && EquippedOffHandWeapon && EquippedOffHandWeapon->WeaponDefenition.Get())
	{
		return EquippedOffHandWeapon->WeaponDefenition.Get()->WeaponInstance.WeaponConfig.TrailEndSocket;
	}
	const FWeaponInstance& Instance = EquipedWeapon->WeaponDefenition.Get()->WeaponInstance;
	return IsSubWeapon ? Instance.SubConfig.TrailEndSocket : Instance.WeaponConfig.TrailEndSocket;
}

void UEquipmentComponent::CacheWeaponSounds(const UWeaponDataAsset* WeaponDefinition,
	TMap<FGameplayTag, FLoadedWeaponSoundSet>& OutSounds)
{
	const UWeaponDataSubsystem* WeaponSubsystem = GetWorld() && GetWorld()->GetGameInstance()
		? GetWorld()->GetGameInstance()->GetSubsystem<UWeaponDataSubsystem>()
		: nullptr;
	WeaponAudioUtility::LoadWeaponSounds(
		WeaponDefinition, WeaponSubsystem, OutSounds);
}

USoundBase* UEquipmentComponent::GetWeaponSound_Implementation(
	FGameplayTag WeaponSoundTag, bool IsSubWeapon) const
{
	const TMap<FGameplayTag, FLoadedWeaponSoundSet>& Source =
		IsSubWeapon && EquippedOffHandWeapon ? CachedOffHandWeaponSounds : CachedWeaponSounds;
	return WeaponAudioUtility::SelectRandomWeaponSound(Source, WeaponSoundTag);
}

FAttackTraceSource UEquipmentComponent::GetAttackTraceSource(EAttackSourceType AttackSourceType) const
{
	FAttackTraceSource OutData;

	if (!EquipedWeapon) return OutData;

	auto ApplyWeaponConfig = [&OutData](const FWeaponConfig& Config)
	{
		OutData.Shape = Config.TraceShape;
		OutData.StartSocket = Config.TraceStartSocket;
		OutData.EndSocket = Config.TraceEndSocket;
		OutData.Radius = Config.HitBoxRadius;
		OutData.CenterSocket = Config.TraceCenterSocket;
		OutData.BoxHalfExtent = Config.BoxHalfExtent;
	};

	const UWeaponDataAsset* MainWeaponData = EquipedWeapon->WeaponDefenition.Get();
	if (!MainWeaponData) return OutData;

	switch (AttackSourceType)
	{
	case EAttackSourceType::MainHand:
		OutData.TraceComponent = WeaponMesh;
		ApplyWeaponConfig(MainWeaponData->WeaponInstance.WeaponConfig);
		break;
	case EAttackSourceType::OffHand:
	{
		OutData.TraceComponent = SubEquipMesh;
		const UWeaponDataAsset* OffHandWeaponData = EquippedOffHandWeapon
			? EquippedOffHandWeapon->WeaponDefenition.Get()
			: nullptr;
		ApplyWeaponConfig(OffHandWeaponData
			? OffHandWeaponData->WeaponInstance.WeaponConfig
			: MainWeaponData->WeaponInstance.SubConfig);
		break;
	}
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

FAttackDamageSource UEquipmentComponent::GetAttackDamageSource(EAttackSourceType AttackSourceType) const
{
	const FWeaponSetsInfo* SourceWeapon =
		AttackSourceType == EAttackSourceType::OffHand && EquippedOffHandWeapon
			? EquippedOffHandWeapon : EquipedWeapon;
	if (!SourceWeapon) return FAttackDamageSource();

	float PerformanceRatio = 1.0f;
	APlayerBase* Player = Cast<APlayerBase>(GetOwner());
	if (!Player) return FAttackDamageSource() ;

	if (const UPlayerStatComponent* PlayerStat = Player->GetStatComponent())
	{
		PerformanceRatio = PlayerStat->GetWeaponPerformanceRatio(SourceWeapon->RequiredAttributes.ToCharacterStats());
	}

	float StrengthBonus, DexterityBonus, AffinityBonus;
	GetCurrentAttackBonuses(StrengthBonus, DexterityBonus, AffinityBonus);

	return CalculateWeaponAttackDamageSource(SourceWeapon, PerformanceRatio, StrengthBonus, DexterityBonus, AffinityBonus);
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
	if (EquippedOffHandWeapon)
	{
		TotalWeight += EquippedOffHandWeapon->WeightValue;
	}

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
