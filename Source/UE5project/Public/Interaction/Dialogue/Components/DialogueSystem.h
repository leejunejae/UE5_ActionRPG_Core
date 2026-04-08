// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "UI/ScriptWidget.h"
#include "DialogueSystem.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UE5PROJECT_API UDialogueSystem : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UDialogueSystem();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SetDialogueDT(UDataTable* InitDialogueDT);
	void SetParticipant(FName CurParticipant);
	void SetIsDialogue(bool CurDialogue);
	UDataTable* GetDialogueDT();
	FName GetPrevDialogLoc();

	void StartDialogue(AActor* InteractActor);

private:
	UPROPERTY(EditAnywhere)
		TSubclassOf<class UScriptWidget> DialogueWidgetClass;

	UPROPERTY(EditAnywhere)
		class UScriptWidget* DialogueWidget;

	UPROPERTY(VisibleAnywhere, Meta = (AllowPrivateAccess = true))
		class UDataTable* NPCListDT;

	UPROPERTY(EditAnywhere, Meta = (AllowPrivateAccess = true))
		class UDataTable* DialogueDT;

	UPROPERTY(EditAnywhere, Meta = (AllowPrivateAccess = true))
		FDataTableRowHandle Participant;

	UPROPERTY(VisibleAnywhere, Meta = (AllowPrivateAccess = true))
		FName Mark;

	UPROPERTY(VisibleAnywhere, Meta = (AllowPrivateAccess = true))
		bool IsDialog;
};
