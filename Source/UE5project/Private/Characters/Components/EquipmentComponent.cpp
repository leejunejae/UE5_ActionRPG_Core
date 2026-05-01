// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Components/EquipmentComponent.h"

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

	OutData.AttackRating = EquipedWeapon->AttackPower;
	OutData.PoiseRating = EquipedWeapon->PoisePower;
	OutData.StanceRating = EquipedWeapon->StancePower;

	if (CachedStat)
	{
		float PerformanceRatio = IStatInterface::Execute_GetWeaponPerformanceRatio(CachedStat.GetObject(), EquipedWeapon->RequiredStats.ToCharacterStats());
		OutData.AttackRating *= PerformanceRatio;
		OutData.PoiseRating *= PerformanceRatio;
		OutData.StanceRating *= PerformanceRatio;
	}

	return OutData;
}
