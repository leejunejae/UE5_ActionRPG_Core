// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/NPC/PBNPCBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Characters/Player/Interfaces/PlayerInterface.h"
#include "UI/ScriptWidget.h"
#include "Blueprint/UserWidget.h"
#include "Interaction/Dialogue/Components/DialogueSystem.h"

// Sets default values
APBNPCBase::APBNPCBase()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	NPCTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Dialogue.NPC")));
	NPCTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Interactable.Dialogue")));

	RootComponent = GetCapsuleComponent();

	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));

	DialogueComponent = CreateDefaultSubobject<UDialogueSystem>(TEXT("DialogComponent"));
	DialogueComponent->bAutoActivate = true;

	InteractTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractTrigger"));
	InteractTrigger->SetupAttachment(GetMesh());
	InteractTrigger->SetRelativeLocation(FVector(0.0f, 30.0f, 110.0f));
	InteractTrigger->SetBoxExtent(FVector(100.0f, 100.0f, 100.0f));

	Tags.Add("NPC");
}

void APBNPCBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	InteractTrigger->OnComponentBeginOverlap.AddDynamic(this, &APBNPCBase::TriggerBegin);
	InteractTrigger->OnComponentEndOverlap.AddDynamic(this, &APBNPCBase::TriggerEnd);
}

void APBNPCBase::Interact_Implementation(ACharacter* InteractActor)
{

}

void APBNPCBase::EndInteract_Implementation()
{
	DialogueComponent->SetIsDialogue(false);
}

// Called when the game starts or when spawned
void APBNPCBase::BeginPlay()
{
	Super::BeginPlay();
	
}

void APBNPCBase::TriggerBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor->ActorHasTag("Player"))
	{
		if (OtherActor->GetClass()->ImplementsInterface(UPlayerInterface::StaticClass()))
		{
			IPlayerInterface::Execute_RegisterInteractableActor(OtherActor, this);
			UE_LOG(LogTemp, Warning, TEXT("Can Talk"));
		}
	}
}

void APBNPCBase::TriggerEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor->ActorHasTag("Player"))
	{
		if (OtherActor->GetClass()->ImplementsInterface(UPlayerInterface::StaticClass()))
		{
			IPlayerInterface::Execute_DeRegisterInteractableActor(OtherActor, this);
		}
	}
}

// Called every frame
void APBNPCBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void APBNPCBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

