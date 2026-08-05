// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractComponent.generated.h"

DECLARE_DELEGATE(FOnArrivedInteractionPointDelegate);
DECLARE_DELEGATE(FOnInteractionMoveCancelledDelegate);

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
	TSet<AActor*> InteractableList;

	UPROPERTY(Transient)
	TObjectPtr<AActor> InteractActor = nullptr;

public:	
	// Called every frame
	AActor* GetInteractActor();

	void AddInteractObject(AActor* InteractObject);
	void RemoveInteractObject(AActor* InteractObject);
	bool SetInteractActorByDegree(AActor* StartActor, float SearchDegrees);
	bool MovetoInteractPos();
	void CancelMoveToInteractPos();

private:
	bool CheckInteractListValid() const;
	void InteractPosCheckTimer(
		USceneComponent* NavigationTarget,
		USceneComponent* AlignmentTarget);

	UFUNCTION()
	void OnMovetoInteractPosEnd();

public:
	FOnArrivedInteractionPointDelegate OnArrivedInteractionPoint;
	FOnInteractionMoveCancelledDelegate OnInteractionMoveCancelled;
};
