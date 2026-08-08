#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Characters/Data/IKData.h"
#include "ANS_LadderSurfaceHandContact.generated.h"

UCLASS(meta = (DisplayName = "Ladder Surface Hand Contact"))
class UE5PROJECT_API UANS_LadderSurfaceHandContact : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Surface Contact")
	ELimbList Hand = ELimbList::HandL;

	UPROPERTY(EditAnywhere, Category = "Surface Contact", meta = (ClampMin = "0.0", Units = "cm"))
	float TraceUpDistance = 30.0f;

	UPROPERTY(EditAnywhere, Category = "Surface Contact", meta = (ClampMin = "0.0", Units = "cm"))
	float TraceDownDistance = 80.0f;

	UPROPERTY(EditAnywhere, Category = "Surface Contact", meta = (Units = "cm"))
	float SurfaceOffset = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Surface Contact")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bDrawDebug = false;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration,
	                         const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	                       const FAnimNotifyEventReference& EventReference) override;
};
