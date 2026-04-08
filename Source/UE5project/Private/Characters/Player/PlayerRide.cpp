// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Player/PlayerRide.h"

// 머터리얼
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"

// 파티클
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"

APlayerRide::APlayerRide()
{
	PrimaryActorTick.bCanEverTick = true;

	DespawnEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("RideDespawn"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem>DESPAWNEFFECT(TEXT("/Game/05_Ride/NS_DespawnPlayerRide.NS_DespawnPlayerRide"));
	if (DESPAWNEFFECT.Succeeded())
	{
		DespawnEffect->SetAsset(DESPAWNEFFECT.Object);
	}
	DespawnEffect->SetupAttachment(GetMesh());
	DespawnEffect->bAutoActivate = false;

	SpawnEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("RideSpawn"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem>SPAWNEFFECT(TEXT("/Game/05_Ride/NS_SpawnPlayerRide.NS_SpawnPlayerRide"));
	if (SPAWNEFFECT.Succeeded())
	{
		SpawnEffect->SetAsset(SPAWNEFFECT.Object);
	}
	SpawnEffect->SetupAttachment(GetMesh());
	SpawnEffect->bAutoActivate = false;

	static ConstructorHelpers::FObjectFinder<UCurveFloat> MOUNTCURVE(TEXT("/Game/05_Ride/DissolveCurve.DissolveCurve"));
	if (MOUNTCURVE.Succeeded())
	{
		MountCurve = MOUNTCURVE.Object;
	}
}

void APlayerRide::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	MountTimeline.TickTimeline(DeltaTime);
}

void APlayerRide::BeginPlay()
{
	Super::BeginPlay();

	SpawnEffect->Activate(true);

	GetMesh()->CreateDynamicMaterialInstance(0);
	Reins->CreateDynamicMaterialInstance(0);
	Saddle->CreateDynamicMaterialInstance(0);

	if (MountCurve)
	{
		FOnTimelineFloat MountCallback;
		MountCallback.BindUFunction(this, FName("MountUpdate"));//&APlayerRide::MountUpdate);
		MountTimeline.AddInterpFloat(MountCurve, MountCallback);
	}
}

void APlayerRide::MountUpdate(float Value)
{
	GetMesh()->SetScalarParameterValueOnMaterials(FName("Opacity"), Value);
	Reins->SetScalarParameterValueOnMaterials(FName("Opacity"), Value);
	Saddle->SetScalarParameterValueOnMaterials(FName("Opacity"), Value);
}

void APlayerRide::DespawnFin()
{
	MountTimeline.SetTimelineFinishedFunc(FOnTimelineEvent());
	Destroy();
	
}

void APlayerRide::SpawnFin()
{
	MountTimeline.SetTimelineFinishedFunc(FOnTimelineEvent());
	CanDismount = true;
}

void APlayerRide::Mount_Implementation(ACharacter* RiderCharacter, FVector InitVelocity)
{
	Super::Mount_Implementation(RiderCharacter, InitVelocity);
	FOnTimelineEventStatic MountCallbackFin;
	MountCallbackFin.BindUObject(this, &APlayerRide::SpawnFin);
	MountTimeline.SetTimelineFinishedFunc(MountCallbackFin);
	MountTimeline.PlayFromStart();
}

bool APlayerRide::TryDisMount()
{
	if (!Super::TryDisMount())
		return false;

	FOnTimelineEventStatic MountCallbackFin;
	MountCallbackFin.BindUObject(this, &APlayerRide::DespawnFin);
	MountTimeline.SetTimelineFinishedFunc(MountCallbackFin);
	MountTimeline.ReverseFromEnd();

	return true;
}
