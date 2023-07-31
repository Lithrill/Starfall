// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "ProjectileRocket.generated.h"

/**
 * 
 */
UCLASS()
class STARFALL_API AProjectileRocket : public AProjectile
{
	GENERATED_BODY()

public:
	AProjectileRocket();
	virtual void Destroyed() override;


	/*UPROPERTY(EditAnywhere, Category = Impulse)
	TSubclassOf<class ARocketRadialForce> RadialForceObject;*/

	UPROPERTY(EditAnywhere, Category = Impulse)
	TSubclassOf<class ARocketRadialForce> RadialForceClass;
	
protected:
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
	virtual void BeginPlay() override;
	void DestroyTimerFinished();

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* TrailSystem;

	UPROPERTY()
	class UNiagaraComponent* TrailSystemComponent;

	UPROPERTY(EditAnywhere)
	USoundCue* ProjectileLoop;

	UPROPERTY()
	UAudioComponent* ProjectileLoopComponent;

	UPROPERTY(EditAnywhere)
	USoundAttenuation* LoopingSoundAttenuation;

	UPROPERTY(visibleAnywhere)
	class URocketMovementComponent* RocketMovementComponent;
	
	
private:

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* RocketMesh;

	FTimerHandle DestroyTimer;

	UPROPERTY(EditAnywhere)
	float DestroyTime = 3.f;

	UPROPERTY(EditAnywhere)
	float DamageInnerRadius = 200.f;

	UPROPERTY(EditAnywhere)
	float DamageOuterRadius = 1400.f;

	UPROPERTY(EditAnywhere)
	float DamageFalloff = 1.25f;

	UPROPERTY(EditAnywhere)
	float ExplosionImpactImpulseForce = 100000.f;

	UPROPERTY(EditAnywhere)
	float MaxLaunchSpeed = 7000.f;
	
};