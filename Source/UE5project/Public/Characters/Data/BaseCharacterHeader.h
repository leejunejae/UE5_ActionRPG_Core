// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseCharacterHeader.generated.h"


UENUM(BlueprintType)
enum class EBodyType : uint8
{
	Hand_R UMETA(DisplayName = "Hand_R"),
	Hand_L UMETA(DisplayName = "Hand_L"),
	Foot_R UMETA(DisplayName = "Foot_R"),
	Foot_L UMETA(DisplayName = "Foot_L"),
};

UCLASS()
class UE5PROJECT_API ABaseCharacterHeader : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABaseCharacterHeader();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

};
