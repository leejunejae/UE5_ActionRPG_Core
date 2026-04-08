// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/Rideable/Ride.h"
#include "Components/TimeLineComponent.h"
#include "PlayerRide.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API APlayerRide : public ARide
{
	GENERATED_BODY()

public:
	APlayerRide();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

#pragma region Effect
private:
	UPROPERTY(VisibleAnywhere, Category = Particle)
		class UNiagaraComponent* DespawnEffect;

	UPROPERTY(VisibleAnywhere, Category = Particle)
		class UNiagaraComponent* SpawnEffect;

	UPROPERTY(VisibleAnywhere, Category = Timeline)
		UCurveFloat* MountCurve;

	FTimeline MountTimeline;
	float MountTimelineLength;

	UFUNCTION()
		void MountUpdate(float Value);

	void DespawnFin();
	void SpawnFin();

#pragma endregion
	

#pragma region Mount And DisMount
	virtual void Mount_Implementation(ACharacter* RiderCharacter, FVector InitVelocity) override;
	virtual bool TryDisMount() override;
#pragma endregion
};
