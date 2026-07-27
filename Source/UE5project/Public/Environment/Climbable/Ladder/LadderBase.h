// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Environment/Climbable/ClimbableObjectBase.h"
#include "Interaction/Climb/Data/ClimbHeader.h"
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

	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Ladder|Transition")
	const USceneComponent* GetInitEnterTarget(bool bIsTop) const;
	virtual const USceneComponent* GetInitEnterTarget_Implementation(bool bIsTop) const { return bIsTop ? ClimbTopLocation : ClimbBottomLocation; }

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Ladder|Transition")
	const USceneComponent* GetTopEnterHandTarget(bool bIsRight) const;
	virtual const USceneComponent* GetTopEnterHandTarget_Implementation(bool bIsRight) const { return bIsRight ? TopEnterRightHandTarget : TopEnterLeftHandTarget; }

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Ladder|Space")
	float GetLadderProgressAtWorldLocation(const FVector& WorldLocation) const;
	virtual float GetLadderProgressAtWorldLocation_Implementation(const FVector& WorldLocation) const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Ladder|Space")
	FVector GetLadderWorldLocationAtProgress(
		float LadderProgress,
		float ForwardOffset,
		float RightOffset) const;
	virtual FVector GetLadderWorldLocationAtProgress_Implementation(
		float LadderProgress,
		float ForwardOffset,
		float RightOffset) const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Ladder|Transition")
	FTransform GetBottomAttachBaseTransform() const;
	virtual FTransform GetBottomAttachBaseTransform_Implementation() const;

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
	void DrawLadderSpaceDebug() const;

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

	UPROPERTY(EditAnywhere, Category = "LadderSetting|Debug")
	bool bDrawLadderSpaceDebug = false;

	UPROPERTY(EditAnywhere, Category = Mesh)
		TArray<UStaticMeshComponent*> ClimbMeshes;

	UPROPERTY(EditAnywhere, Category = Mesh)
		TObjectPtr<USceneComponent> TopEnterLeftHandTarget;

	UPROPERTY(EditAnywhere, Category = Mesh)
		TObjectPtr<USceneComponent> TopEnterRightHandTarget;
#pragma endregion
};
