// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TargetPoint.h"

#include "SpawnMarker.generated.h"

/**
 * 
 */
class APatrolRoute;

UCLASS()
class UE5PROJECT_API ASpawnMarker : public ATargetPoint
{
	GENERATED_BODY()

public:
	ASpawnMarker();

protected:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(EditAnywhere, Category = "ID")
		FGuid SpawnID;

	UPROPERTY(EditAnywhere, Category = "ID")
		FGuid DefaultRouteID;

	UPROPERTY(EditAnywhere, Category = "ID")
		FName EnemyID;

public:
	FGuid GetSpawnID() const { return SpawnID; }
	FGuid GetRouteID() const { return DefaultRouteID; }
	FName GetEnemyID() const { return EnemyID; }

#if WITH_EDITORONLY_DATA
	UPROPERTY(Transient, EditAnywhere, Category = "Route (Pick)", meta = (AllowedClasses = "PatrolRoute"))
		TObjectPtr<APatrolRoute> DefaultRoute = nullptr;
#endif

#if WITH_EDITOR
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostDuplicate(bool bDuplicateForPIE) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

};
