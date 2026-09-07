// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/ANS_TimedWeaponTrail.h"
#include "Combat/Trajectory/WeaponTrailRuntime.h"
#include "Animation/AnimSequenceBase.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

void UANS_TimedWeaponTrail::BeginDestroy()
{
	delete RuntimeManager;
	RuntimeManager = nullptr;
	Super::BeginDestroy();
}

FWeaponTrailRuntimeManager& UANS_TimedWeaponTrail::GetRuntimeManager()
{
	if (!RuntimeManager) RuntimeManager = new FWeaponTrailRuntimeManager();
	return *RuntimeManager;
}

void UANS_TimedWeaponTrail::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	GetRuntimeManager().Begin(*this, MeshComp, Anim, TotalDuration, EventReference);
}

void UANS_TimedWeaponTrail::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim,
	float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	if (RuntimeManager) RuntimeManager->Tick(*this, MeshComp, Anim, FrameDeltaTime, EventReference);
}

void UANS_TimedWeaponTrail::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim,
	const FAnimNotifyEventReference& EventReference)
{
	if (RuntimeManager) RuntimeManager->End(*this, MeshComp, Anim, EventReference);
}

UFXSystemComponent* UANS_TimedWeaponTrail::SpawnEffect(
	USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) const
{
	UActorComponent* Equipment = FWeaponTrailRuntimeManager::FindEquipmentComponent(MeshComp);
	UNiagaraSystem* TrailSystem = FWeaponTrailRuntimeManager::ResolveTrailSystem(Equipment, bSubWeapon);
	if (!TrailSystem)
	{
		return Super::SpawnEffect(MeshComp, Animation);
	}
	return FWeaponTrailRuntimeManager::SpawnWeaponEffect(
		MeshComp, Animation, Equipment, TrailSystem, bSubWeapon, bDestroyAtEnd,
		bApplyRateScaleAsTimeDilation, TrailFadeDuration);
}
