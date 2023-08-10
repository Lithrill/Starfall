// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "HitScanWeapon.generated.h"

/**
 * 
 */
UCLASS()
class STARFALL_API AHitScanWeapon : public AWeapon
{
	GENERATED_BODY()
public:
	virtual void Fire(const FVector& HitTarget) override;

	//AHitScanWeapon();
private:
	UPROPERTY(EditAnywhere)
	float Damage = 20.f;

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* ImpactParticles;

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* BeamParticles;

	UPROPERTY(EditAnywhere)
	float WeaponImpactImpulseForce = 0.f;

	UPROPERTY()
	FVector WeaponImpactDirection;

	UPROPERTY()
	FName WeaponImpactBone;

	UPROPERTY()
	FVector WeaponImpactPoint;


	UPROPERTY()
	class AStarfallCharacter* StarfallPlayerCharacter;

	

};
