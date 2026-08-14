// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/ANS_TimedWeaponTrail.h"
#include "Particles/ParticleSystemComponent.h"
#include "Characters/Interfaces/EquipmentDataInterface.h"

void UANS_TimedWeaponTrail::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Anim, TotalDuration, EventReference);

#if WITH_EDITOR
	if (MeshComp->GetWorld() &&
		MeshComp->GetWorld()->WorldType != EWorldType::Game &&
		MeshComp->GetWorld()->WorldType != EWorldType::PIE)
	{
		return;
	}
#endif
}

void UANS_TimedWeaponTrail::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp || !MeshComp->GetOwner()) return;

	UActorComponent* EquipmentComponent = nullptr;
	for (UActorComponent* Component : MeshComp->GetOwner()->GetComponents())
	{
		if (Component && Component->GetClass()->ImplementsInterface(UEquipmentDataInterface::StaticClass()))
		{
			EquipmentComponent = Component;
			break;
		}
	}

	if (EquipmentComponent)
	{
		if(UFXSystemComponent* TargetFX = GetSpawnedEffect(MeshComp))
		{
			FVector TrailStart = IEquipmentDataInterface::Execute_GetWeaponSocketLocation(EquipmentComponent, FName("Start"), bSubWeapon);
			FVector TrailEnd = IEquipmentDataInterface::Execute_GetWeaponSocketLocation(EquipmentComponent, FName("End"), bSubWeapon);

			TargetFX->SetVectorParameter(FName("TrailStart"), TrailStart);
			TargetFX->SetVectorParameter(FName("TrailEnd"), TrailEnd);
		}
	}
}

void UANS_TimedWeaponTrail::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Anim, EventReference);
}
