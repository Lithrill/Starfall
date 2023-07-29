// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileBullet.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Starfall/Character/StarfallCharacter.h"

AProjectileBullet::AProjectileBullet()
{
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);
}

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter)
	{
		AController* OwnerController = OwnerCharacter->Controller;
		if (OwnerController)
		{

			AStarfallCharacter* StarfallCharacter = Cast<AStarfallCharacter>(OtherActor);
			if (StarfallCharacter)
			{
				FVector ProjectileVelocity = GetVelocity();
				FVector ProjectileImpactDirection = ProjectileVelocity.GetSafeNormal();

				StarfallCharacter->ImpactImpulseForce = WeaponImpactImpulseForce;
				StarfallCharacter->ImpactDirection = ProjectileImpactDirection;
				StarfallCharacter->BoneImpactName = Hit.BoneName;
				StarfallCharacter->ImpactPoint = Hit.ImpactPoint;
			}
			UGameplayStatics::ApplyDamage(OtherActor, Damage, OwnerController, this, UDamageType::StaticClass());
		}
	}
	

	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
