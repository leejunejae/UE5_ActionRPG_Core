// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Data/BaseCharacterHeader.h"

// Sets default values
ABaseCharacterHeader::ABaseCharacterHeader()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ABaseCharacterHeader::BeginPlay()
{
	Super::BeginPlay();
	
}