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

	UWeaponDataAsset* LoadedDefinition = FindWeapon->WeaponDefenition.LoadSynchronous();
	if (!LoadedDefinition)
	{
		UE_LOG(Log_Equip_Weapon, Error, TEXT("[EquipmentComponent] Failed to load weapon data: %s"), *WeaponKey.ToString());
		return;
	}

	if (!LoadedDefinition->WeaponInstance.IsValid() && WeaponKey != FName("Hand_Unarmed_01"))
	{
		UE_LOG(Log_Equip_Weapon, Error, TEXT("[EquipmentComponent] Weapon mesh missing: %s"), *WeaponKey.ToString());
		return;
	}

	const EWeaponGripMode PreviousGripMode = CurrentGripMode;
	const EWeaponPresentationState PreviousOffHandPresentation = OffHandPresentationState;

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
	EquippedWeaponDefinition = LoadedDefinition;
	const UWeaponDataAsset* MainDefinition = EquippedWeaponDefinition;
	switch (MainDefinition->GripType)
	{
	case EWeaponGripType::OneHanded:
		CurrentGripMode = EWeaponGripMode::OneHanded;
		break;
	case EWeaponGripType::TwoHanded:
		CurrentGripMode = EWeaponGripMode::TwoHanded;
		break;
	case EWeaponGripType::Versatile:
		CurrentGripMode = PreviousGripMode;
		break;
	default:
		CurrentGripMode = EWeaponGripMode::OneHanded;
		break;
	}

	if (!MainDefinition->bCanEquipOffHand && EquippedOffHandWeapon)
	{
		EquippedOffHandWeapon = nullptr;
		EquippedOffHandWeaponKey = NAME_None;
		EquippedOffHandWeaponDefinition = nullptr;
		CachedOffHandWeaponSounds.Reset();
		SubWeaponTrailSystem = nullptr;
		SubWeaponTrailMaterial = nullptr;
		OffHandPresentationState = EWeaponPresentationState::Holstered;
	}
	else if (EquippedOffHandWeapon)
	{
		OffHandPresentationState = CurrentGripMode == EWeaponGripMode::OneHanded
			? PreviousOffHandPresentation
			: EWeaponPresentationState::Holstered;
	}
	WeaponMesh->SetStaticMesh(MainDefinition->WeaponInstance.Mesh.LoadSynchronous());
	WeaponMesh->SetRelativeScale3D(MainDefinition->WeaponInstance.WeaponConfig.WeaponScale);
	MainWeaponTrailSystem = MainDefinition->WeaponInstance.WeaponConfig.TrailSystem.LoadSynchronous();
	MainWeaponTrailMaterial = MainDefinition->WeaponInstance.WeaponConfig.TrailMaterial.LoadSynchronous();
	CacheWeaponSounds(MainDefinition, CachedWeaponSounds);

	if (EquippedOffHandWeapon && !EquippedOffHandWeaponDefinition)
	{
		EquippedOffHandWeaponDefinition = EquippedOffHandWeapon->WeaponDefinition.LoadSynchronous();
	}
	if (const UOffHandWeaponDataAsset* OffHandDefinition = EquippedOffHandWeaponDefinition.Get())
	{
		SubEquipMesh->SetStaticMesh(OffHandDefinition->WeaponInstance.Mesh.LoadSynchronous());
		SubEquipMesh->SetRelativeScale3D(OffHandDefinition->WeaponInstance.WeaponConfig.WeaponScale);
		SubWeaponTrailSystem = OffHandDefinition->WeaponInstance.WeaponConfig.TrailSystem.LoadSynchronous();
		SubWeaponTrailMaterial = OffHandDefinition->WeaponInstance.WeaponConfig.TrailMaterial.LoadSynchronous();
	}
	RefreshOffHandPresentation();

	RecalcEquipLoad();
	RefreshCombatStyle();
}

void UEquipmentComponent::UnequipWeapon()
{
	// 보조무기는 주무기 호환성에 종속되므로 주무기와 함께 실제 장착 해제한다.
	EquippedOffHandWeapon = nullptr;
	EquippedOffHandWeaponKey = NAME_None;
	EquippedOffHandWeaponDefinition = nullptr;
	CachedOffHandWeaponSounds.Reset();
	SubWeaponTrailSystem = nullptr;
	SubWeaponTrailMaterial = nullptr;
	OffHandPresentationState = EWeaponPresentationState::Holstered;

	EquipedWeapon = nullptr;
	EquipedWeaponKey = NAME_None;
	EquippedWeaponDefinition = nullptr;
	CachedWeaponSounds.Reset();
	MainWeaponTrailSystem = nullptr;
	MainWeaponTrailMaterial = nullptr;
	CurrentGripMode = EWeaponGripMode::OneHanded;

	if (WeaponMesh)
	{
		WeaponMesh->SetStaticMesh(nullptr);
		WeaponMesh->SetRelativeScale3D(FVector::OneVector);
	}
	if (SubEquipMesh)
	{
		SubEquipMesh->SetStaticMesh(nullptr);
		SubEquipMesh->SetRelativeScale3D(FVector::OneVector);
		SubEquipMesh->SetVisibility(false, true);
	}

	RecalcEquipLoad();
	RefreshCombatStyle();
}

void UEquipmentComponent::EquipOffHandWeapon(FName WeaponKey)
{
	UWorld* World = GetWorld();
	if (!SubEquipMesh || !World) return;

	UWeaponDataSubsystem* WeaponSubsystem = World->GetGameInstance()->GetSubsystem<UWeaponDataSubsystem>();
	const FOffHandWeaponSetsInfo* Found = WeaponSubsystem ? WeaponSubsystem->GetOffHandWeaponInfo(WeaponKey) : nullptr;
	UOffHandWeaponDataAsset* Definition = Found ? Found->WeaponDefinition.LoadSynchronous() : nullptr;
	if (!Definition || !Definition->WeaponInstance.IsValid())
	{
		UE_LOG(Log_Equip_Weapon, Error, TEXT("[EquipmentComponent] Invalid off-hand weapon: %s"), *WeaponKey.ToString());
		return;
	}

	const UWeaponDataAsset* MainDefinition = EquippedWeaponDefinition;
	if (!MainDefinition)
	{
		UE_LOG(Log_Equip_Weapon, Warning, TEXT("[EquipmentComponent] Equip a main weapon before an off-hand item"));
		return;
	}
	if (!MainDefinition->bCanEquipOffHand)
	{
		UE_LOG(Log_Equip_Weapon, Warning, TEXT("[EquipmentComponent] Current main weapon does not allow an off-hand item"));
		return;
	}
	const bool bReplacingOffHand = EquippedOffHandWeapon != nullptr;
	const EWeaponPresentationState PreviousPresentationState = OffHandPresentationState;
	EquippedOffHandWeapon = Found;
	EquippedOffHandWeaponKey = WeaponKey;
	EquippedOffHandWeaponDefinition = Definition;
	if (bReplacingOffHand)
	{
		// 교체는 현재 사용 의도를 바꾸지 않는다. Drawn/Holstered 상태를 그대로 이어받는다.
		OffHandPresentationState = PreviousPresentationState;
	}
	else
	{
		// 빈 슬롯에 처음 장착할 때만 현재 파지에서 사용할 수 있으면 즉시 활성화한다.
		OffHandPresentationState = CanSetOffHandPresentationState(EWeaponPresentationState::Drawn)
			? EWeaponPresentationState::Drawn : EWeaponPresentationState::Holstered;
	}
	SubEquipMesh->SetStaticMesh(Definition->WeaponInstance.Mesh.LoadSynchronous());
	SubEquipMesh->SetRelativeScale3D(Definition->WeaponInstance.WeaponConfig.WeaponScale);
	SubWeaponTrailSystem = Definition->WeaponInstance.WeaponConfig.TrailSystem.LoadSynchronous();
	SubWeaponTrailMaterial = Definition->WeaponInstance.WeaponConfig.TrailMaterial.LoadSynchronous();
	CacheWeaponSounds(Definition, CachedOffHandWeaponSounds);
	RefreshOffHandPresentation();
	RecalcEquipLoad();
	RefreshCombatStyle();
}

void UEquipmentComponent::UnequipOffHandWeapon()
{
	EquippedOffHandWeapon = nullptr;
	EquippedOffHandWeaponKey = NAME_None;
	EquippedOffHandWeaponDefinition = nullptr;
	CachedOffHandWeaponSounds.Reset();
	OffHandPresentationState = EWeaponPresentationState::Holstered;

	if (SubEquipMesh)
	{
		SubEquipMesh->SetStaticMesh(nullptr);
		SubEquipMesh->SetRelativeScale3D(FVector::OneVector);
		SubEquipMesh->SetVisibility(false, true);
	}
	SubWeaponTrailSystem = nullptr;
	SubWeaponTrailMaterial = nullptr;
	RecalcEquipLoad();
	RefreshCombatStyle();
}

bool UEquipmentComponent::SetOffHandPresentationState(EWeaponPresentationState NewState)
{
	if (!CanSetOffHandPresentationState(NewState)) return false;
	if (OffHandPresentationState == NewState) return true;

	OffHandPresentationState = NewState;
	RefreshOffHandPresentation();
	RefreshCombatStyle();
	return true;
}

bool UEquipmentComponent::ToggleOffHandPresentation()
{
	const EWeaponPresentationState TargetState =
		OffHandPresentationState == EWeaponPresentationState::Drawn
			? EWeaponPresentationState::Holstered
			: EWeaponPresentationState::Drawn;
	return SetOffHandPresentationState(TargetState);
}

bool UEquipmentComponent::SetGripMode(EWeaponGripMode NewGripMode)
{
	if (!CanSetGripMode(NewGripMode)) return false;
	if (CurrentGripMode == NewGripMode) return true;

	CurrentGripMode = NewGripMode;
	// 파지 입력은 보조무기를 꺼내지 않는다. 두손 전환만 강제로 보관한다.
	if (CurrentGripMode == EWeaponGripMode::TwoHanded)
	{
		OffHandPresentationState = EWeaponPresentationState::Holstered;
	}
	RefreshOffHandPresentation();
	RefreshCombatStyle();
	return true;
}

bool UEquipmentComponent::CanSetGripMode(EWeaponGripMode NewGripMode) const
{
	const UWeaponDataAsset* Definition = EquippedWeaponDefinition;
	if (!Definition) return false;
	return Definition->GripType == EWeaponGripType::Versatile ||
		(Definition->GripType == EWeaponGripType::OneHanded && NewGripMode == EWeaponGripMode::OneHanded) ||
		(Definition->GripType == EWeaponGripType::TwoHanded && NewGripMode == EWeaponGripMode::TwoHanded);
}

bool UEquipmentComponent::IsOffHandActive() const
{
	const UWeaponDataAsset* MainDefinition = EquippedWeaponDefinition;
	return EquippedOffHandWeapon && MainDefinition &&
		CurrentGripMode == EWeaponGripMode::OneHanded && MainDefinition->bCanEquipOffHand &&
		OffHandPresentationState == EWeaponPresentationState::Drawn;
}

bool UEquipmentComponent::CanSetOffHandPresentationState(EWeaponPresentationState NewState) const
{
	return CanSetWeaponUseState(CurrentGripMode, NewState);
}

bool UEquipmentComponent::CanSetWeaponUseState(EWeaponGripMode NewGripMode,
	EWeaponPresentationState NewPresentationState) const
{
	if (!CanSetGripMode(NewGripMode) || !EquippedOffHandWeapon) return false;
	if (NewPresentationState == EWeaponPresentationState::Holstered) return true;

	const UWeaponDataAsset* MainDefinition = EquippedWeaponDefinition;
	return MainDefinition && NewGripMode == EWeaponGripMode::OneHanded &&
		MainDefinition->bCanEquipOffHand;
}

bool UEquipmentComponent::SetWeaponUseState(EWeaponGripMode NewGripMode,
	EWeaponPresentationState NewPresentationState)
{
	if (!CanSetWeaponUseState(NewGripMode, NewPresentationState)) return false;

	CurrentGripMode = NewGripMode;
	OffHandPresentationState = NewGripMode == EWeaponGripMode::TwoHanded
		? EWeaponPresentationState::Holstered : NewPresentationState;
	RefreshOffHandPresentation();
	RefreshCombatStyle();
	return true;
}

void UEquipmentComponent::RefreshOffHandPresentation()
{
	if (!SubEquipMesh) return;

	const UOffHandWeaponDataAsset* OffHandDefinition = EquippedOffHandWeaponDefinition;

	if (!OffHandDefinition)
	{
		OffHandPresentationState = EWeaponPresentationState::Holstered;
		SubEquipMesh->SetVisibility(false, true);
		return;
	}

	if (!CanSetOffHandPresentationState(OffHandPresentationState))
	{
		OffHandPresentationState = EWeaponPresentationState::Holstered;
	}
	const bool bDrawn = OffHandPresentationState == EWeaponPresentationState::Drawn;
	const FName TargetSocket = bDrawn
		? (OffHandDefinition->DrawnSocket.IsNone() ? SubEquipSocket : OffHandDefinition->DrawnSocket)
		: OffHandDefinition->HolsterSocket;

	if (TargetSocket.IsNone())
	{
		SubEquipMesh->SetVisibility(false, true);
		return;
	}

	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		SubEquipMesh->AttachToComponent(Character->GetMesh(),
			FAttachmentTransformRules::SnapToTargetNotIncludingScale, TargetSocket);
		SubEquipMesh->SetRelativeLocationAndRotation(
			bDrawn ? FVector::ZeroVector : OffHandDefinition->HolsterLocationOffset,
			bDrawn ? FRotator::ZeroRotator : OffHandDefinition->HolsterRotationOffset);
		SubEquipMesh->SetRelativeScale3D(OffHandDefinition->WeaponInstance.WeaponConfig.WeaponScale);
	}
	SubEquipMesh->SetVisibility(true, true);
}

FGameplayTag UEquipmentComponent::ResolveCombatStyle() const
{
	return ResolveCombatStyleForState(CurrentGripMode, OffHandPresentationState);
}

FGameplayTag UEquipmentComponent::ResolveCombatStyleForGripMode(EWeaponGripMode GripMode) const
{
	// 파지 입력은 보조무기 활성 상태를 만들지 않는다.
	return ResolveCombatStyleForState(GripMode, EWeaponPresentationState::Holstered);
}

FGameplayTag UEquipmentComponent::ResolveCombatStyleForWeaponState(
	EWeaponGripMode GripMode, EWeaponPresentationState PresentationState) const
{
	return ResolveCombatStyleForState(GripMode, PresentationState);
}

FGameplayTag UEquipmentComponent::ResolveCombatStyleForState(
	EWeaponGripMode GripMode, EWeaponPresentationState PresentationState) const
{
	const UWeaponDataAsset* MainDefinition = EquippedWeaponDefinition;
	if (!MainDefinition) return TAG_CombatStyle_Unarmed;

	const UOffHandWeaponDataAsset* OffHandDefinition = EquippedOffHandWeaponDefinition;
	const EWeaponCategory OffHandCategory =
		GripMode == EWeaponGripMode::OneHanded &&
		PresentationState == EWeaponPresentationState::Drawn &&
		MainDefinition->bCanEquipOffHand && OffHandDefinition
			? OffHandDefinition->GetEffectiveWeaponCategory()
			: EWeaponCategory::None;

	const UPlayerAnimRegistrySubsystem* Registry = GetWorld() && GetWorld()->GetGameInstance()
		? GetWorld()->GetGameInstance()->GetSubsystem<UPlayerAnimRegistrySubsystem>()
		: nullptr;
	if (Registry)
	{
		const FGameplayTag Resolved = Registry->ResolveCombatStyle(
			MainDefinition->GetEffectiveWeaponCategory(), OffHandCategory, GripMode);
		if (Resolved.IsValid())
		{
			return Resolved;
		}
	}

	UE_LOG(Log_Equip_Weapon, Warning,
		TEXT("[EquipmentComponent] No CombatStyle rule for main=%d offhand=%d grip=%d"),
		static_cast<int32>(MainDefinition->GetEffectiveWeaponCategory()),
		static_cast<int32>(OffHandCategory), static_cast<int32>(GripMode));
	return TAG_CombatStyle_Unarmed;
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
	if (IsSubWeapon && EquippedOffHandWeapon && !IsOffHandActive()) return nullptr;
	return IsSubWeapon ? SubWeaponTrailSystem.Get() : MainWeaponTrailSystem.Get();
}

UMaterialInterface* UEquipmentComponent::GetWeaponTrailMaterial_Implementation(bool IsSubWeapon) const
{
	if (IsSubWeapon && EquippedOffHandWeapon && !IsOffHandActive()) return nullptr;
	return IsSubWeapon ? SubWeaponTrailMaterial.Get() : MainWeaponTrailMaterial.Get();
}

FName UEquipmentComponent::GetWeaponTrailStartSocket_Implementation(bool IsSubWeapon) const
{
	if (!EquippedWeaponDefinition) return TEXT("Start");
	if (IsSubWeapon && EquippedOffHandWeaponDefinition)
	{
		return EquippedOffHandWeaponDefinition->WeaponInstance.WeaponConfig.TrailStartSocket;
	}
	return IsSubWeapon
		? TEXT("Start")
		: EquippedWeaponDefinition->WeaponInstance.WeaponConfig.TrailStartSocket;
}

FName UEquipmentComponent::GetWeaponTrailEndSocket_Implementation(bool IsSubWeapon) const
{
	if (!EquippedWeaponDefinition) return TEXT("End");
	if (IsSubWeapon && EquippedOffHandWeaponDefinition)
	{
		return EquippedOffHandWeaponDefinition->WeaponInstance.WeaponConfig.TrailEndSocket;
	}
	return IsSubWeapon
		? TEXT("End")
		: EquippedWeaponDefinition->WeaponInstance.WeaponConfig.TrailEndSocket;
}

void UEquipmentComponent::CacheWeaponSounds(const UWeaponEquipmentDataAsset* WeaponDefinition,
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
	if (IsSubWeapon && (!EquippedOffHandWeapon || !IsOffHandActive())) return nullptr;
	const TMap<FGameplayTag, FLoadedWeaponSoundSet>& Source = IsSubWeapon
		? CachedOffHandWeaponSounds : CachedWeaponSounds;
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

	const UWeaponDataAsset* MainWeaponData = EquippedWeaponDefinition;
	if (!MainWeaponData) return OutData;

	switch (AttackSourceType)
	{
	case EAttackSourceType::MainHand:
		OutData.TraceComponent = WeaponMesh;
		ApplyWeaponConfig(MainWeaponData->WeaponInstance.WeaponConfig);
		break;
	case EAttackSourceType::OffHand:
	{
		if (!EquippedOffHandWeapon || !IsOffHandActive()) break;
		const UOffHandWeaponDataAsset* OffHandWeaponData = EquippedOffHandWeaponDefinition;
		if (!OffHandWeaponData) break;
		OutData.TraceComponent = SubEquipMesh;
		ApplyWeaponConfig(OffHandWeaponData->WeaponInstance.WeaponConfig);
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
	const FWeaponStatsRow* SourceWeapon = nullptr;
	switch (AttackSourceType)
	{
	case EAttackSourceType::MainHand:
		SourceWeapon = static_cast<const FWeaponStatsRow*>(EquipedWeapon);
		break;

	case EAttackSourceType::OffHand:
		if (!EquippedOffHandWeapon || !IsOffHandActive())
		{
			return FAttackDamageSource();
		}
		SourceWeapon = static_cast<const FWeaponStatsRow*>(EquippedOffHandWeapon);
		break;

	case EAttackSourceType::Custom:
	default:
		return FAttackDamageSource();
	}

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
