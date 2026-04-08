// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractComponent.generated.h"

DECLARE_DELEGATE(FOnSingleDelegate);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UE5PROJECT_API UInteractComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UInteractComponent();

private:
	FTimerHandle InteractTimerHandle;
	FTimerDelegate InteractTimerDelegate;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	TSet<AActor*> InteractableList;

	AActor* InteractActor;

public:	
	// Called every frame
	AActor* GetInteractActor();

	bool CheckInteractListValid();
	void AddInteractObject(AActor* InteractObject);
	void RemoveInteractObject(AActor* InteractObject);
	bool SetInteractActorByDegree(AActor* StartActor, float SearchDegrees);
	bool MovetoInteractPos();
	void InteractPosCheckTimer(USceneComponent* Target);

	UFUNCTION()
		void OnMovetoInteractPosEnd();

public:
	FOnSingleDelegate OnArrivedInteractionPoint;
};
