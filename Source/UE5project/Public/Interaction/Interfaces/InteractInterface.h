// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "GameplayTagContainer.h"
#include "InteractInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UInteractInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class UE5PROJECT_API IInteractInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		void RegisterInteractActor(ACharacter* InteractActor);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		void Interact(ACharacter* InteractActor);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		void EndInteract();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		USceneComponent* GetEnterInteractLocation(AActor* Target = nullptr);

	void GetInteractionTags(FGameplayTagContainer& OutTags) const;
};
