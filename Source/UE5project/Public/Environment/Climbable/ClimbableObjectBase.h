// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

// 기본 헤더
#include "CoreMinimal.h"
#include "Environment/MapObjectBase.h"

// 인터페이스
#include "Interaction/Interfaces/InteractInterface.h"

// 구조체, 자료형
#include "Interaction/Climb/Data/ClimbHeader.h"


#include "ClimbableObjectBase.generated.h"

class UBoxComponent;
/**
 * 
 */
UCLASS()
class UE5PROJECT_API AClimbableObjectBase : public AMapObjectBase, public IInteractInterface
{
	GENERATED_BODY()
	
public:
	AClimbableObjectBase();

	virtual void PostInitializeComponents() override;

	virtual USceneComponent* GetEnterInteractLocation_Implementation(AActor* Target) override;
	virtual USceneComponent* GetNavigationInteractLocation_Implementation(AActor* Target) override;

	const TArray<FGripNode1D>& GetGripList1D() const { return GripList1D; }

protected:
	UFUNCTION()
		void TriggerBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
		void TriggerEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UPROPERTY(VisibleAnywhere, Category = Interact)
		TObjectPtr<UBoxComponent> ClimbTopTrigger;

	UPROPERTY(VisibleAnywhere, Category = Interact)
		TObjectPtr<UBoxComponent> ClimbBottomTrigger;

	UPROPERTY(VisibleAnywhere, Category = Interact)
		TObjectPtr<USceneComponent> ClimbTopLocation;

	UPROPERTY(VisibleAnywhere, Category = Interact)
		TObjectPtr<USceneComponent> ClimbTopApproachLocation;

	UPROPERTY(VisibleAnywhere, Category = Interact)
		TObjectPtr<USceneComponent> ClimbTopExitLocation;

	UPROPERTY(VisibleAnywhere, Category = Interact)
		TObjectPtr<USceneComponent> ClimbBottomLocation;

	UPROPERTY(EditAnywhere, Category = Mesh)
		TObjectPtr<UStaticMesh> ClimbStaticMesh;

	TArray<FGripNode1D> GripList1D;
};
