// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Engine/DataTable.h"
#include "Interaction/Interfaces/InteractInterface.h"
#include "GameplayTagContainer.h"
#include "PBNPCBase.generated.h"

class UDialogueSystem;
class UBoxComponent;
class UScriptWidget;
class UDialogueSystem;

UCLASS()
class UE5PROJECT_API APBNPCBase : public ACharacter, public IInteractInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	APBNPCBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	UFUNCTION()
		void TriggerBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
		void TriggerEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void PostInitializeComponents() override;

	virtual void Interact_Implementation(ACharacter* InteractActor);

	virtual void EndInteract_Implementation();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags")
		FGameplayTagContainer NPCTags;

protected:
	UPROPERTY(EditAnywhere)
		TSubclassOf<class UScriptWidget> DialogueWidgetClass;

	UPROPERTY(EditAnywhere)
		class UScriptWidget* DialogueWidget;

	UPROPERTY(VisibleAnywhere, Category = Interact)
		UBoxComponent* InteractTrigger;

	UPROPERTY(VisibleAnywhere, Category = Dialog)
		UDialogueSystem* DialogueComponent;

	UPROPERTY(VisibleAnywhere, Meta = (AllowPrivateAccess = true))
		FName NPCName;

	UPROPERTY(VisibleAnywhere, Meta = (AllowPrivateAccess = true))
		class UDataTable* DialogueDT;
};
