// Fill out your copyright notice in the Description page of Project Settings.


#include "Quest/QuestComponent.h"

// Sets default values for this component's properties
UQuestComponent::UQuestComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UQuestComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UQuestComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UQuestComponent::AddQuest(FName QuestID)
{

}

void UQuestComponent::CompleteQuest(FName QuestID)
{
}

bool UQuestComponent::CanAccepted(FName QuestID)
{
	return false;
}

bool UQuestComponent::IsCompleted(FName QuestID)
{
	return false;
}

bool UQuestComponent::HasPreQuest(FName QuestID)
{
	return false;
}

bool UQuestComponent::IsActivated(FName QuestID)
{
	return false;
}

bool UQuestComponent::IsValid(FName QuestID)
{
	return false;
}

QuestState UQuestComponent::CheckQuest(FName QuestID)
{
	if (IsCompleted(QuestID))
	{
		return QuestState::Completed;
	}
	else if (IsActivated(QuestID))
	{
		return QuestState::Activated;
	}
	else if (HasPreQuest(QuestID))
	{
		return QuestState::Precondition;
	}
	else if (!IsValid(QuestID))
	{
		return QuestState::Invalid;
	}
	return QuestState::Executable;
}

