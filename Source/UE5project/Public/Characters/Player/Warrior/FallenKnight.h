 
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/Player/PlayerBase.h"
#include "Components/TimeLineComponent.h"
#include "FallenKnight.generated.h"

/**
 * 
 */

class UCharacterBaseAnimInstance;

UCLASS()
class UE5PROJECT_API AFallenKnight : public APlayerBase
{
	GENERATED_BODY()
	
public:
	AFallenKnight(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void PostInitializeComponents() override;

private:
	UPROPERTY(EditAnyWhere, Category = Equipment)
		FName DefaultWeaponKey = FName("SNS_HardenedIron_01");
	UPROPERTY(EditAnyWhere, Category = Equipment)
		FName DefaultHeadKey = FName("Wanderer_Cloak");
	UPROPERTY(EditAnyWhere, Category = Equipment)
		FName DefaultChestKey = FName("Wanderer_Tunic");
	UPROPERTY(EditAnyWhere, Category = Equipment)
		FName DefaultLegsKey = FName("Wanderer_Pants");
	UPROPERTY(EditAnyWhere, Category = Equipment)
		FName DefaultHandsKey = FName("Wanderer_Bracers");
};