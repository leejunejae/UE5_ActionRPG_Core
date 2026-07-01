// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Preview/CharacterPreviewActor.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Characters/Components/EquipmentComponent.h"
#include "Characters/Player/PlayerBase.h"
#include "Characters/Player/Controller/ControllerBase.h"
#include "Kismet/GameplayStatics.h"
#include "Utils/CoreLog.h"

ACharacterPreviewActor::ACharacterPreviewActor()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;

	EquipmentComponent = CreateDefaultSubobject<UEquipmentComponent>(TEXT("EquipmentComponent"));

	CaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("CaptureComponent"));
	CaptureComponent->SetupAttachment(RootComponent);
	CaptureComponent->bCaptureEveryFrame = false; // SetCaptureActive로만 제어
	CaptureComponent->bCaptureOnMovement = false;
	CaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
}

void ACharacterPreviewActor::BeginPlay()
{
	Super::BeginPlay();
	PlayIdleAnimation();
}

void ACharacterPreviewActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (BoundPlayer.IsValid())
	{
		if (UEquipmentComponent* Equip = BoundPlayer->GetEquipmentComponent())
		{
			Equip->OnWeaponSetChanged().Remove(WeaponChangedHandle);
			Equip->OnArmorChangedDelegate.Remove(ArmorChangedHandle);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void ACharacterPreviewActor::SetCaptureActive(bool bActive)
{
	if (!CaptureComponent) return;

	CaptureComponent->bCaptureEveryFrame = bActive;
	if (bActive)
	{
		CaptureComponent->CaptureScene(); // 다음 틱까지 기다리지 않고 즉시 1회 캡처
	}
}

void ACharacterPreviewActor::PlayIdleAnimation()
{
	if (IdleAnimAsset && MeshComponent)
	{
		MeshComponent->PlayAnimation(IdleAnimAsset, /*bLooping=*/true);
	}
}

void ACharacterPreviewActor::BindToPlayer(APlayerBase* Player)
{
	if (!Player || BoundPlayer == Player) return;

	UEquipmentComponent* Equip = Player->GetEquipmentComponent();
	if (!Equip)
	{
		UE_LOG(Log_UI, Warning, TEXT("[CharacterPreviewActor] BindToPlayer: player has no EquipmentComponent"));
		return;
	}

	BoundPlayer = Player;
	WeaponChangedHandle = Equip->OnWeaponSetChanged().AddUObject(this, &ACharacterPreviewActor::HandleWeaponChanged);
	ArmorChangedHandle = Equip->OnArmorChangedDelegate.AddUObject(this, &ACharacterPreviewActor::HandleArmorChanged);

	SyncEquipmentFrom(Player);
}

void ACharacterPreviewActor::HandleWeaponChanged(EWeaponType WeaponType)
{
	SyncEquipmentFrom(BoundPlayer.Get());
}

void ACharacterPreviewActor::HandleArmorChanged(EArmorSlot Slot)
{
	SyncEquipmentFrom(BoundPlayer.Get());
}

void ACharacterPreviewActor::SyncEquipmentFrom(APlayerBase* SourcePlayer)
{
	if (!SourcePlayer || !EquipmentComponent) return;

	UEquipmentComponent* SourceEquipment = SourcePlayer->GetEquipmentComponent();
	if (!SourceEquipment) return;

	const FName WeaponKey = SourceEquipment->GetEquipedWeaponKey();
	if (WeaponKey != NAME_None)
	{
		EquipmentComponent->EquipWeapon_Implementation(WeaponKey);
	}

	static const TArray<EArmorSlot> AllSlots = { EArmorSlot::Head, EArmorSlot::Chest, EArmorSlot::Hands, EArmorSlot::Legs };
	for (EArmorSlot Slot : AllSlots)
	{
		const FName ArmorKey = SourceEquipment->GetEquipedArmorKey(Slot);
		if (ArmorKey != NAME_None)
		{
			EquipmentComponent->EquipArmor(ArmorKey);
		}
		else
		{
			EquipmentComponent->UnequipArmor(Slot);
		}
	}
}