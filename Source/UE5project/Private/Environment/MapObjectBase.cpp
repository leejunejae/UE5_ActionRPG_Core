// Fill out your copyright notice in the Description page of Project Settings.


#include "Environment/MapObjectBase.h"

// Sets default values
AMapObjectBase::AMapObjectBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	//PrimaryActorTick.bCanEverTick = true;
	ObjectRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ObjectRoot"));
	RootComponent = ObjectRoot;

	Tags.Add("MapObject");
}

// Called when the game starts or when spawned
void AMapObjectBase::BeginPlay()
{
	Super::BeginPlay();
	
}