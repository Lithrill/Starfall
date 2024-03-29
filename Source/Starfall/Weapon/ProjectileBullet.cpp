// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileBullet.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Starfall/Character/StarfallCharacter.h"
#include "Weapon.h"
#include "Starfall/Environment/WindowColourChanger.h"

AProjectileBullet::AProjectileBullet()
{
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);
	
}



void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
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
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter)
	{
	
		AController* OwnerController = OwnerCharacter->Controller;
		if (OwnerController)
		{

			
			if (HasAuthority())
			{
				UGameplayStatics::ApplyDamage(OtherActor, Damage, OwnerController, this, UDamageType::StaticClass());
			}
			
			if (HasAuthority())
			{
				AWeapon* StarfallWeapon = Cast<AWeapon>(OtherActor);
				if (StarfallWeapon)
				{
					FVector ProjectileVelocity = GetVelocity();
					FVector ProjectileImpactDirection = ProjectileVelocity.GetSafeNormal();
					StarfallWeapon->GetWeaponMesh()->AddImpulseAtLocation(ProjectileImpactDirection * WeaponImpactImpulseForce, Hit.ImpactPoint, Hit.BoneName);
				}
			}
			
		}
	}
	

	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
