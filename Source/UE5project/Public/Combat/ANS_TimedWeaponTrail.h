// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AnimNotifyState_TimedNiagaraEffect.h"
#include "ANS_TimedWeaponTrail.generated.h"

/**
 * 
 */

class IEquipmentDataInterface;
class UActorComponent;
class UFXSystemComponent;
class UNiagaraComponent;
class USceneComponent;
class UWorld;
struct FBoneTransformSegment;
class FWeaponTrailRuntimeManager;

UCLASS()
class UE5PROJECT_API UANS_TimedWeaponTrail : public UAnimNotifyState_TimedNiagaraEffect
{
	GENERATED_BODY()
	
public:
	virtual void BeginDestroy() override;
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, const FAnimNotifyEventReference& EventReference) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData")
		bool bSubWeapon = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Trajectory")
		FName WindowName = TEXT("Trail");

	// 베이크 시 이 본의 Transform을 FBoneTransformSegment에 저장한다.
	// 값을 변경한 뒤에는 해당 애니메이션의 본 데이터를 다시 빌드해야 한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Trajectory",
		meta = (ToolTip = "트레일 궤적을 베이크할 기준 본. 변경 후 애니메이션 본 데이터를 다시 빌드해야 합니다."))
		FName TargetBone = TEXT("Hand_R");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Trajectory", meta = (ClampMin = "0.001", UIMin = "0.001"))
		float TrajectorySampleInterval = 0.004f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Trajectory", meta = (ClampMin = "1", UIMin = "1"))
		int32 MaxSamplesPerFrame = 16;

	// Ribbon 진행률 계산에서 Start 소켓 궤적이 차지하는 비율이다.
	// Start/End 가중치는 합계로 정규화되므로 반드시 합이 1일 필요는 없다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Trajectory|Distance",
		meta = (ClampMin = "0.0", UIMin = "0.0"))
		float TrailStartDistanceWeight = 0.5f;

	// Ribbon 진행률 계산에서 End 소켓 궤적이 차지하는 비율이다.
	// 긴 무기의 칼끝 움직임을 강조하려면 이 값을 Start보다 크게 설정한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrailData|Trajectory|Distance",
		meta = (ClampMin = "0.0", UIMin = "0.0"))
		float TrailEndDistanceWeight = 0.5f;

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
	friend class FWeaponTrailRuntimeManager;
	FWeaponTrailRuntimeManager& GetRuntimeManager();
	FWeaponTrailRuntimeManager* RuntimeManager = nullptr;

};
