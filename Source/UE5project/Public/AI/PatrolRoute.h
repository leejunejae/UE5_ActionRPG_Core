// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "PatrolRoute.generated.h"

UCLASS()
class UE5PROJECT_API APatrolRoute : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APatrolRoute();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere)
		TObjectPtr<USplineComponent> RouteSpline;

	UPROPERTY(EditAnywhere, Category = "Route")
		FGuid RouteID;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	FORCEINLINE FGuid GetRouteID() const { return RouteID; }

	int32 CalculatePatrolIndex(int32 Current, bool& bIncrease);
	FVector GetPatrolTargetLocationByIndex(int32 SplineIndex);

#if WITH_EDITOR
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostDuplicate(bool bDuplicateForPIE) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
