// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DataTable.h"
#include "AffiliationData.generated.h"

/**
 * 
 */


/*
USTRUCT(BlueprintType)
struct FFactionRelationRow : public FTableRowBase
{
	GENERATED_BODY()

		UPROPERTY(EditAnywhere, BlueprintReadWrite)
		EFaction SourceFaction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		EFaction TargetFaction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float FriendlyLevel = 0.0f; // -100 = 완전 적대, 0 = 중립, 100 = 완전 우호
};
*/

UCLASS()
class UE5PROJECT_API UAffiliationData : public UObject
{
	GENERATED_BODY()
	
};
