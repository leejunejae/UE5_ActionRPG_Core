// Fill out your copyright notice in the Description page of Project Settings.


#include "Quest/Data/QuestStruct.h"

// Sets default values
AQuestStruct::AQuestStruct()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AQuestStruct::BeginPlay()
{
	Super::BeginPlay();
	
}

uint32 GetTypeHash(const FObjectiveDataStruct& ObjectiveData)
{
	return GetTypeHash(ObjectiveData.ID);
}