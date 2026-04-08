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
	if (MeshComp && MeshComp->GetAnimInstance())
	{
		for (UActorComponent* Comp : MeshComp->GetOwner()->GetComponents())
		{
			if (Comp->GetClass()->ImplementsInterface(UEquipmentDataInterface::StaticClass()))
				CachedWeaponInterface = TScriptInterface<IEquipmentDataInterface>(Comp);
		}
	}
}

void UANS_TimedWeaponTrail::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	if (MeshComp && MeshComp->GetAnimInstance() && CachedWeaponInterface.GetObject() && CachedWeaponInterface.GetInterface())
	{
		if(UFXSystemComponent* TargetFX = GetSpawnedEffect(MeshComp))
		{
			FVector TrailStart = IEquipmentDataInterface::Execute_GetWeaponSocketLocation(CachedWeaponInterface.GetObject(), FName("Start"), bSubWeapon);
			FVector TrailEnd = IEquipmentDataInterface::Execute_GetWeaponSocketLocation(CachedWeaponInterface.GetObject(), FName("End"), bSubWeapon);

			UE_LOG(LogTemp, Warning, TEXT("TrailStart = [X : %f, Y : %f, Z : %f]"),
				TrailStart.X,
				TrailStart.Y,
				TrailStart.Z
			);

			TargetFX->SetVectorParameter(FName("TrailStart"), TrailStart);
			TargetFX->SetVectorParameter(FName("TrailEnd"), TrailEnd);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("FX NOT VALID"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Notify Ignored"));
	}
}

void UANS_TimedWeaponTrail::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, const FAnimNotifyEventReference& EventReference)
{
	CachedWeaponInterface = TScriptInterface<IEquipmentDataInterface>();
}
