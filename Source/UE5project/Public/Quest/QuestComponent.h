// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Quest/Data/QuestStruct.h"
#include "Components/ActorComponent.h"
#include "QuestComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UE5PROJECT_API UQuestComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UQuestComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void AddQuest(FName QuestID);
	void CompleteQuest(FName QuestID);
	bool CanAccepted(FName QuestID);
	bool IsCompleted(FName QuestID);
	bool HasPreQuest(FName QuestID);
	bool IsActivated(FName QuestID);
	bool IsValid(FName QuestID);
	QuestState CheckQuest(FName QuestID);


private:
	//FQuestDataStruct
};
