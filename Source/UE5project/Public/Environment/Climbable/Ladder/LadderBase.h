// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interaction/Climb/Interfaces/LadderInterface.h"
#include "Environment/Climbable/ClimbableObjectBase.h"
#include "Interaction/Climb/Data/ClimbHeader.h"
#include "LadderBase.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API ALadderBase : public AClimbableObjectBase, public ILadderInterface
{
	GENERATED_BODY()
	
public:
	ALadderBase();

	virtual const USceneComponent* GetInitClimbTarget_Implementation() const {	return EnterPosition; }
	virtual const USceneComponent* GetInitEnterTarget_Implementation(bool IsTop) const { return IsTop ? ClimbTopLocation : ClimbBottomLocation; }
	virtual const USceneComponent* GetTopEnterHandTarget_Implementation(bool IsRight) const { return IsRight ? TopEnterRightHandTarget : TopEnterLeftHandTarget; }

#pragma region Ladder Basic Composition
////////////////////////////////////
// Methods For Ladder Basic Composition
////////////////////////////////////
private:
	void SetInitTopPosition();
	void SetInitBottomPosition();

protected:
	virtual void BeginPlay() override;

	virtual void OnConstruction(const FTransform& Transform) override;

////////////////////////////////////
// Variables For Ladder Basic Composition
////////////////////////////////////
protected:
	UPROPERTY(VisibleAnywhere, Category = Interact)
	TObjectPtr<USceneComponent> EnterPosition;

	UPROPERTY(EditAnywhere, Category = LadderSetting) // Layer for modular ladder
		int32 LadderLevel;

	UPROPERTY(EditAnywhere, Category = LadderSetting)
		FVector LadderScale;

	UPROPERTY(EditAnywhere, Category = LadderSetting)
		float AdditionalHeight;

	UPROPERTY(EditAnywhere, Category = Mesh)
		TArray<UStaticMeshComponent*> ClimbMeshes;

	UPROPERTY(EditAnywhere, Category = Mesh)
		TArray<UBoxComponent*> GripCollisions;

	UPROPERTY(EditAnywhere, Category = Mesh)
		TObjectPtr<USceneComponent> TopEnterLeftHandTarget;

	UPROPERTY(EditAnywhere, Category = Mesh)
		TObjectPtr<USceneComponent> TopEnterRightHandTarget;
#pragma endregion
};
