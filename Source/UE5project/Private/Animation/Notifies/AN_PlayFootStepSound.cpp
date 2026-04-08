// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/Notifies/AN_PlayFootStepSound.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Kismet/GameplayStatics.h"

void UAN_PlayFootStepSound::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (!MeshComp || !MeshComp->GetOwner() || !Animation)
    {
        return;
    }

    FVector StartLoc = MeshComp->GetOwner()->GetActorLocation();
    FVector EndLoc = StartLoc + MeshComp->GetOwner()->GetActorUpVector() * -200.0f;

	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(MeshComp->GetOwner());
    CollisionParams.bReturnPhysicalMaterial = true;

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartLoc,
		EndLoc,
		ECC_Visibility,
		CollisionParams
	);
	
    //FColor DrawColor = bHit ? FColor::Green : FColor::Red;
    //DrawDebugLine(GetWorld(), StartLoc, EndLoc, DrawColor, false, 1.0f);

	if (bHit)
	{
        EPhysicalSurface SurfaceType = UGameplayStatics::GetSurfaceType(HitResult);
        USoundCue* SoundToPlay = nullptr;
        
        switch (SurfaceType)
        {
        case SurfaceType_Default:
            SoundToPlay = GetRandomSoundCue(DefaultSounds);
            //UE_LOG(LogTemp, Warning, TEXT("DefaultSounds"));
            break;
        case SurfaceType1:
            SoundToPlay = GetRandomSoundCue(SnowSounds);
            //UE_LOG(LogTemp, Warning, TEXT("SnowSounds"));
            break;
        case SurfaceType2:
            SoundToPlay = GetRandomSoundCue(DirtSounds);
            break;
        case SurfaceType3:
            SoundToPlay = GetRandomSoundCue(GrassSounds);
            break;
        case SurfaceType4:
            SoundToPlay = GetRandomSoundCue(WoodSounds);
            break;
        case SurfaceType5:
            SoundToPlay = GetRandomSoundCue(RockSounds);
            break;
        case SurfaceType6:
            SoundToPlay = GetRandomSoundCue(LightGrassSounds);
            break;
        case SurfaceType7:
            SoundToPlay = GetRandomSoundCue(MetalSounds);
            break;
        case SurfaceType8:
            SoundToPlay = GetRandomSoundCue(RockWithSnowSounds);
            break;
        case SurfaceType9:
            SoundToPlay = GetRandomSoundCue(WoodWithSnowSounds);
            break;
        case SurfaceType10:
            SoundToPlay = GetRandomSoundCue(GrassWithSnowSounds);
            break;
        case SurfaceType11:
            SoundToPlay = GetRandomSoundCue(LightGrassWithSnowSounds);
            break;
        case SurfaceType12:
            SoundToPlay = GetRandomSoundCue(RockWithLightGrassSounds);
            break;
        default:
            SoundToPlay = GetRandomSoundCue(DefaultSounds);
            break;
        }

       float SoundVolume = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 500.0f), FVector2D(0.0f, 1.0f), MeshComp->GetOwner()->GetVelocity().Length()) * StepSoundMultiplier;
      
       if (SoundToPlay)
        UGameplayStatics::PlaySoundAtLocation(this, SoundToPlay, HitResult.Location, SoundVolume);
        
       //UGameplayStatics::SpawnSoundAttached(SoundToPlay, HitResult.Component.Get(), NAME_None, HitResult.Location, EAttachLocation::KeepWorldPosition, false, SoundVolume);
	}
}

USoundCue* UAN_PlayFootStepSound::GetRandomSoundCue(const TArray<USoundCue*>& SoundCues) const
{
    if (SoundCues.Num() > 0)
    {
        int32 RandomIndex = FMath::RandRange(0, SoundCues.Num() - 1);
        return SoundCues[RandomIndex];
    }
    return nullptr;
}