// Fill out your copyright notice in the Description page of Project Settings.


#include "AmmoPickup.h"
#include "Starfall/Character/StarfallCharacter.h"
#include "Starfall/StarfallComponents/CombatComponent.h"

void AAmmoPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent,AActor* OtherActor,UPrimitiveComponent* OtherComp,int32 OtherBodyIndex,bool bFromSweep,const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	AStarfallCharacter* StarfallCharacter = Cast<AStarfallCharacter>(OtherActor);
	if (StarfallCharacter)
	{
		UCombatComponent* Combat = StarfallCharacter->GetCombat();
		if (Combat)
		{
			Combat->PickupAmmo(WeaponType, AmmoAmount);
		}
		Destroy();
	}
}