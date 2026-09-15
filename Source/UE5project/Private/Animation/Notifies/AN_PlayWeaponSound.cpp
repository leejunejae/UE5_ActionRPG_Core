#include "Animation/Notifies/AN_PlayWeaponSound.h"

#include "Characters/Interfaces/WeaponRuntimeSourceInterface.h"
#include "Core/Subsystems/GameInstanceSystem/WeaponDataSubsystem.h"
#include "Components/ActorComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

namespace
{
	UObject* FindWeaponRuntimeSource(const USkeletalMeshComponent* MeshComp)
	{
		AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
		if (!Owner) return nullptr;
		if (Owner->GetClass()->ImplementsInterface(UWeaponRuntimeSourceInterface::StaticClass()))
		{
			return Owner;
		}

		for (UActorComponent* Component : Owner->GetComponents())
		{
			if (Component && Component->GetClass()->ImplementsInterface(UWeaponRuntimeSourceInterface::StaticClass()))
			{
				return Component;
			}
		}
		return nullptr;
	}
}

FString UAN_PlayWeaponSound::GetNotifyName_Implementation() const
{
	return bSubWeapon ? TEXT("Play Sub Weapon Sound") : TEXT("Play Weapon Sound");
}

void UAN_PlayWeaponSound::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	UWorld* World = MeshComp ? MeshComp->GetWorld() : nullptr;
	if (!World || World->GetNetMode() == NM_DedicatedServer) return;

	UObject* WeaponSource = FindWeaponRuntimeSource(MeshComp);
	USoundBase* WeaponSound = WeaponSource
		? IWeaponRuntimeSourceInterface::Execute_GetWeaponSound(WeaponSource, WeaponSoundTag, bSubWeapon)
		: nullptr;
	if (!WeaponSound) return;

	const FName StartSocket = IWeaponRuntimeSourceInterface::Execute_GetWeaponTrailStartSocket(WeaponSource, bSubWeapon);
	const FVector SoundLocation = IWeaponRuntimeSourceInterface::Execute_GetWeaponSocketLocation(
		WeaponSource, StartSocket, bSubWeapon);
	const UWeaponDataSubsystem* WeaponSubsystem = World->GetGameInstance()
		? World->GetGameInstance()->GetSubsystem<UWeaponDataSubsystem>()
		: nullptr;
	UGameplayStatics::PlaySoundAtLocation(
		MeshComp, WeaponSound, SoundLocation, VolumeMultiplier, PitchMultiplier, 0.0f,
		WeaponSubsystem ? WeaponSubsystem->GetWeaponSoundAttenuation() : nullptr);
}
