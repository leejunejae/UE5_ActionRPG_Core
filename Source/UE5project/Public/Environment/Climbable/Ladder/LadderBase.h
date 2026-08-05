// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Environment/Climbable/ClimbableObjectBase.h"
#include "LadderBase.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API ALadderBase : public AClimbableObjectBase
{
	GENERATED_BODY()
	
public:
	ALadderBase();

	const USceneComponent* GetTopExitTarget() const { return ClimbTopExitLocation; }

#pragma region Ladder Basic Composition
////////////////////////////////////
// Methods For Ladder Basic Composition
////////////////////////////////////
private:
	void ClearGeneratedLadder();
	void RebuildLadder();
	void BuildRuntimeGripData();
	bool HasValidGeneratedMeshes() const;
	void SetInitTopPosition();
	void SetInitBottomPosition();

protected:
	virtual void BeginPlay() override;

	virtual void OnConstruction(const FTransform& Transform) override;

////////////////////////////////////
// Variables For Ladder Basic Composition
////////////////////////////////////
protected:
	UPROPERTY(EditAnywhere, Category = LadderSetting) // Layer for modular ladder
		int32 LadderLevel;

	UPROPERTY(EditAnywhere, Category = LadderSetting)
		FVector LadderScale;

	UPROPERTY(EditAnywhere, Category = LadderSetting)
		float AdditionalHeight;

	UPROPERTY(EditAnywhere, Category = Mesh)
		TArray<UStaticMeshComponent*> ClimbMeshes;

#pragma endregion
};
