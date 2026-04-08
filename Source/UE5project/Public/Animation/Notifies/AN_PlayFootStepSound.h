// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundCue.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_PlayFootStepSound.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UAN_PlayFootStepSound : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

    UPROPERTY(EditDefaultsOnly, Category = "Sound")
        float StepSoundMultiplier;

private:
    UPROPERTY(EditDefaultsOnly, Category = "Sound")
        TArray<USoundCue*> DefaultSounds;

    UPROPERTY(EditDefaultsOnly, Category = "Sound")
        TArray<USoundCue*> SnowSounds;

    UPROPERTY(EditDefaultsOnly, Category = "Sound")
        TArray<USoundCue*> DirtSounds;

    UPROPERTY(EditDefaultsOnly, Category = "Sound")
        TArray<USoundCue*> GrassSounds;

    UPROPERTY(EditDefaultsOnly, Category = "Sound")
        TArray<USoundCue*> WoodSounds;

    UPROPERTY(EditDefaultsOnly, Category = "Sound")
        TArray<USoundCue*> LightGrassSounds;

    UPROPERTY(EditDefaultsOnly, Category = "Sound")
        TArray<USoundCue*> RockSounds;

    UPROPERTY(EditDefaultsOnly, Category = "Sound")
        TArray<USoundCue*> MetalSounds;

    UPROPERTY(EditDefaultsOnly, Category = "Sound")
        TArray<USoundCue*> RockWithSnowSounds;

    UPROPERTY(EditDefaultsOnly, Category = "Sound")
        TArray<USoundCue*> WoodWithSnowSounds;

    UPROPERTY(EditDefaultsOnly, Category = "Sound")
        TArray<USoundCue*> GrassWithSnowSounds;

    UPROPERTY(EditDefaultsOnly, Category = "Sound")
        TArray<USoundCue*> LightGrassWithSnowSounds;

    UPROPERTY(EditDefaultsOnly, Category = "Sound")
        TArray<USoundCue*> RockWithLightGrassSounds;

    USoundCue* GetRandomSoundCue(const TArray<USoundCue*>& SoundCues) const;
};
