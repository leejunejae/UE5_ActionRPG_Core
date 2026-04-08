// Fill out your copyright notice in the Description page of Project Settings.


#include "Environment/Climbable/ClimbableObjectBase.h"

// 네비게이션
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavigationSystem.h"

// 컴포넌트
#include "Components/BrushComponent.h"
#include "Components/BoxComponent.h"

// 태그
#include "GameplayTagsManager.h"

// 인터페이스
#include "Characters/Player/Interfaces/PlayerInterface.h"

AClimbableObjectBase::AClimbableObjectBase()
{
	PrimaryActorTick.bCanEverTick = true;

	Tags.Add("Climbable");
	
	ClimbObjectTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Climbable")));
	ClimbObjectTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Interactable.Climb")));

	ClimbStaticMesh = CreateDefaultSubobject<UStaticMesh>(TEXT("ClimbStaticMesh"));

	ClimbTopTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("ClimbTopTrigger"));
	ClimbTopTrigger->SetupAttachment(ObjectRoot);

	ClimbBottomTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("ClimbBottomTrigger"));
	ClimbBottomTrigger->SetupAttachment(ObjectRoot);

	ClimbTopLocation = CreateDefaultSubobject<USceneComponent>(TEXT("ClimbTopLocation"));
	ClimbTopLocation->SetupAttachment(ObjectRoot);
	ClimbTopLocation->ComponentTags.Add(FName("Top"));

	ClimbBottomLocation = CreateDefaultSubobject<USceneComponent>(TEXT("ClimbBottomLocation"));
	ClimbBottomLocation->SetupAttachment(ObjectRoot);
	ClimbBottomLocation->ComponentTags.Add(FName("Bottom"));
}

void AClimbableObjectBase::BeginPlay()
{
	Super::BeginPlay();
}

void AClimbableObjectBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AClimbableObjectBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	ClimbTopTrigger->OnComponentBeginOverlap.AddDynamic(this, &AClimbableObjectBase::TriggerBegin);
	ClimbTopTrigger->OnComponentEndOverlap.AddDynamic(this, &AClimbableObjectBase::TriggerEnd);
	ClimbBottomTrigger->OnComponentBeginOverlap.AddDynamic(this, &AClimbableObjectBase::TriggerBegin);
	ClimbBottomTrigger->OnComponentEndOverlap.AddDynamic(this, &AClimbableObjectBase::TriggerEnd);
}

USceneComponent* AClimbableObjectBase::GetEnterInteractLocation_Implementation(AActor* Target)
{
	IInteractInterface::GetEnterInteractLocation_Implementation(Target);

	if (ClimbTopTrigger->IsOverlappingActor(Target)) return ClimbTopLocation;
	if (ClimbBottomTrigger->IsOverlappingActor(Target)) return ClimbBottomLocation;

	FVector DistTopLoc = Target->GetActorLocation() - ClimbTopLocation->GetComponentLocation();
	FVector DistBottomLoc = Target->GetActorLocation() - ClimbBottomLocation->GetComponentLocation();

	return DistTopLoc.Length() < DistBottomLoc.Length() ? ClimbTopLocation : ClimbBottomLocation;
}

void AClimbableObjectBase::GetInteractionTags_Implementation(FGameplayTagContainer& OutTags) const
{
	OutTags = ClimbObjectTags;
}

TArray<FGripNode1D> AClimbableObjectBase::GetGripList1D_Implementation()
{
	return GripList1D;
}

void AClimbableObjectBase::TriggerBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor->ActorHasTag("Player"))
	{
		if (OtherActor->GetClass()->ImplementsInterface(UPlayerInterface::StaticClass()))
		{
			IPlayerInterface::Execute_RegisterInteractableActor(OtherActor, this);
		}
	}
}

void AClimbableObjectBase::TriggerEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor->ActorHasTag("Player"))
	{
		if (OtherActor->GetClass()->ImplementsInterface(UPlayerInterface::StaticClass()))
		{
			IPlayerInterface::Execute_DeRegisterInteractableActor(OtherActor, this);
		}
	}
}
