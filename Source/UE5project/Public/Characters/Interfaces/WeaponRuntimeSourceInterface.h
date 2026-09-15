#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Items/Weapons/Data/WeaponAudioData.h"
#include "WeaponRuntimeSourceInterface.generated.h"

class UMaterialInterface;
class UNiagaraSystem;
class USoundBase;

/** Read-only access to the weapon currently represented by an actor. */
UINTERFACE(MinimalAPI)
class UWeaponRuntimeSourceInterface : public UInterface
{
	GENERATED_BODY()
};

class UE5PROJECT_API IWeaponRuntimeSourceInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	FVector GetWeaponSocketLocation(FName SocketName, bool bSubWeapon = false) const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	UNiagaraSystem* GetWeaponTrailSystem(bool bSubWeapon = false) const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	UMaterialInterface* GetWeaponTrailMaterial(bool bSubWeapon = false) const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	FName GetWeaponTrailStartSocket(bool bSubWeapon = false) const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	FName GetWeaponTrailEndSocket(bool bSubWeapon = false) const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	USoundBase* GetWeaponSound(FGameplayTag WeaponSoundTag, bool bSubWeapon = false) const;
};
