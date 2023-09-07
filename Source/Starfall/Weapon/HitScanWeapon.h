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

protected:
	FVector TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget);

	void WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit, FHitResult& WindowOutHit);

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* ImpactParticles;

	UPROPERTY(EditAnywhere)
	class USoundCue* HitSound;

	UPROPERTY(EditAnywhere)
	float Damage = 20.f;

private:

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

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* MuzzleFlash;

	UPROPERTY(EditAnywhere)
	class USoundCue* FireSound;

	
	/**
	*Trace end with scatter
	*/

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float DistanceToSphere = 800.f;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float SphereRadius = 75.f;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	bool bUseScatter = false;
};
