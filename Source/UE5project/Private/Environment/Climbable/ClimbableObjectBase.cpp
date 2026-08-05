// Fill out your copyright notice in the Description page of Project Settings.


#include "Environment/Climbable/ClimbableObjectBase.h"

// 컴포넌트
#include "Components/BoxComponent.h"

// 인터페이스
#include "Characters/Player/Interfaces/PlayerInterface.h"

AClimbableObjectBase::AClimbableObjectBase()
{
	PrimaryActorTick.bCanEverTick = false;

	ClimbTopTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("ClimbTopTrigger"));
	ClimbTopTrigger->SetupAttachment(ObjectRoot);

	ClimbBottomTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("ClimbBottomTrigger"));
	ClimbBottomTrigger->SetupAttachment(ObjectRoot);

	ClimbTopLocation = CreateDefaultSubobject<USceneComponent>(TEXT("ClimbTopLocation"));
	ClimbTopLocation->SetupAttachment(ObjectRoot);

	ClimbTopApproachLocation = CreateDefaultSubobject<USceneComponent>(TEXT("ClimbTopApproachLocation"));
	ClimbTopApproachLocation->SetupAttachment(ObjectRoot);

	ClimbTopExitLocation = CreateDefaultSubobject<USceneComponent>(TEXT("ClimbTopExitLocation"));
	ClimbTopExitLocation->SetupAttachment(ObjectRoot);

	ClimbBottomLocation = CreateDefaultSubobject<USceneComponent>(TEXT("ClimbBottomLocation"));
	ClimbBottomLocation->SetupAttachment(ObjectRoot);
	ClimbBottomLocation->ComponentTags.Add(FName("Bottom"));
}

void AClimbableObjectBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	ClimbTopTrigger->OnComponentBeginOverlap.AddUniqueDynamic(this, &AClimbableObjectBase::TriggerBegin);
	ClimbTopTrigger->OnComponentEndOverlap.AddUniqueDynamic(this, &AClimbableObjectBase::TriggerEnd);
	ClimbBottomTrigger->OnComponentBeginOverlap.AddUniqueDynamic(this, &AClimbableObjectBase::TriggerBegin);
	ClimbBottomTrigger->OnComponentEndOverlap.AddUniqueDynamic(this, &AClimbableObjectBase::TriggerEnd);
}

USceneComponent* AClimbableObjectBase::GetEnterInteractLocation_Implementation(AActor* Target)
{
	if (!IsValid(Target)) return nullptr;

	if (ClimbTopTrigger->IsOverlappingActor(Target)) return ClimbTopLocation;
	if (ClimbBottomTrigger->IsOverlappingActor(Target)) return ClimbBottomLocation;

	FVector DistTopLoc = Target->GetActorLocation() - ClimbTopLocation->GetComponentLocation();
	FVector DistBottomLoc = Target->GetActorLocation() - ClimbBottomLocation->GetComponentLocation();

	return DistTopLoc.Length() < DistBottomLoc.Length() ? ClimbTopLocation : ClimbBottomLocation;
}

USceneComponent* AClimbableObjectBase::GetNavigationInteractLocation_Implementation(AActor* Target)
{
	USceneComponent* EntryLocation = IInteractInterface::Execute_GetEnterInteractLocation(this, Target);
	return EntryLocation == ClimbTopLocation
		? ClimbTopApproachLocation.Get()
		: EntryLocation;
}

void AClimbableObjectBase::TriggerBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (IsValid(OtherActor) && OtherActor->Implements<UPlayerInterface>())
	{
		IPlayerInterface::Execute_RegisterInteractableActor(OtherActor, this);
	}
}

void AClimbableObjectBase::TriggerEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (IsValid(OtherActor) && OtherActor->Implements<UPlayerInterface>())
	{
		if (ClimbTopTrigger->IsOverlappingActor(OtherActor) || ClimbBottomTrigger->IsOverlappingActor(OtherActor))
		{
			return;
		}

		IPlayerInterface::Execute_DeRegisterInteractableActor(OtherActor, this);
	}
}
