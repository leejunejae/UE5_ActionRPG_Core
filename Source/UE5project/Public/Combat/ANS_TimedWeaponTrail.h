// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AnimNotifyState_TimedNiagaraEffect.h"
#include "ANS_TimedWeaponTrail.generated.h"

/**
 * 
 */

class IEquipmentDataInterface;
class UFXSystemComponent;
struct FBoneTransformSegment;

struct FWeaponTrailRuntimeState
{
	const FBoneTransformSegment* Segment = nullptr;
	float NotifyStartTime = 0.0f;
	float NotifyEndTime = 0.0f;
	float LastSampleTime = 0.0f;
	bool bNeedsInitialSample = true;
	int32 NextLinkOrder = 0;
	bool bHasPreviousDebugSample = false;
	FVector PreviousDebugStart = FVector::ZeroVector;
	FVector PreviousDebugEnd = FVector::ZeroVector;
};

UCLASS()
class UE5PROJECT_API UANS_TimedWeaponTrail : public UAnimNotifyState_TimedNiagaraEffect
{
	GENERATED_BODY()
	
public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, const FAnimNotifyEventReference& EventReference) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData")
		bool bSubWeapon = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData")
		FName TrailStartParameter = TEXT("TrailStart");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData")
		FName TrailEndParameter = TEXT("TrailEnd");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Trajectory")
		FName WindowName = TEXT("Trail");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Trajectory")
		FName TargetBone = TEXT("Hand_R");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Trajectory", meta = (ClampMin = "0.001", UIMin = "0.001"))
		float TrajectorySampleInterval = 0.004f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Trajectory", meta = (ClampMin = "1", UIMin = "1"))
		int32 MaxSamplesPerFrame = 16;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Trajectory")
		FName TrailStartSamplesParameter = TEXT("User.TrailStartSamples");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Trajectory")
		FName TrailEndSamplesParameter = TEXT("User.TrailEndSamples");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Trajectory")
		FName TrailSampleCountParameter = TEXT("User.TrailSampleCount");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Trajectory")
		FName TrailBatchDurationParameter = TEXT("User.TrailBatchDuration");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Trajectory")
		FName TrailLinkOrderSamplesParameter = TEXT("User.TrailLinkOrderSamples");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Trajectory")
		FName TrailUSamplesParameter = TEXT("User.TrailUSamples");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Fade")
		FName TrailIsEndingParameter = TEXT("User.TrailIsEnding");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Fade")
		FName TrailFadeDurationParameter = TEXT("User.TrailFadeDuration");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Fade", meta = (ClampMin = "0.01", UIMin = "0.01"))
		float TrailFadeDuration = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Debug")
		bool bDebugDrawTrajectory = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Debug", meta = (ClampMin = "0.0", UIMin = "0.0"))
		float DebugDrawDuration = 2.0f;

protected:
	virtual UFXSystemComponent* SpawnEffect(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation) const override;

private:
	TMap<TWeakObjectPtr<USkeletalMeshComponent>, FWeaponTrailRuntimeState> RuntimeStates;

};
