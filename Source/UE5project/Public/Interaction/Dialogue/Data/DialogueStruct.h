// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"
#include "DialogueStruct.generated.h"

USTRUCT(Atomic, BlueprintType)
struct FDialogueParticipantStruct : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FText DisplayName;
};

USTRUCT(Atomic, BlueprintType)
struct FDialogueContentStruct : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FDataTableRowHandle Participant;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FText Text;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FName JumpTo;
};

UCLASS()
class UE5PROJECT_API ADialogueStruct : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADialogueStruct();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

};
