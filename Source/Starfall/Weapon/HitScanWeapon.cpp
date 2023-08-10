// Fill out your copyright notice in the Description page of Project Settings.


#include "HitScanWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Starfall/Character/StarfallCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Weapon.h"
#include "Starfall/Environment/WindowColourChanger.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"
#include "DrawDebugHelpers.h"




void AHitScanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;
	AController* InstigatorController = OwnerPawn->GetController();

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();
		FVector End = Start + (HitTarget - Start) * 1.25f;
		FVector WindowEnd = Start + (HitTarget - Start) * 1.25f;

		FHitResult FireHit;
		FHitResult WindowHit;
		UWorld* World = GetWorld();
		if (World)
		{
			World->LineTraceSingleByChannel(
				FireHit,
				Start,
				End,
				ECollisionChannel::ECC_Visibility
			);
		}
		FVector BeamEnd = End;

		if (World)
		{
			World->LineTraceSingleByChannel(
				WindowHit,
				Start,
				WindowEnd,
				ECollisionChannel::ECC_GameTraceChannel3
			);
		}

		//Check if it passes through a window or not
		if (WindowHit.bBlockingHit)
		{
			AWindowColourChanger* WindowActor = Cast<AWindowColourChanger>(WindowHit.GetActor());
			if (WindowActor)
			{
				WindowActor->ChangeWindowColour();
			}
		}

		if (FireHit.bBlockingHit)
		{
			
			BeamEnd = FireHit.ImpactPoint;
			AStarfallCharacter* StarfallCharacter = Cast<AStarfallCharacter>(FireHit.GetActor());
			if (StarfallCharacter)
			{
				StarfallPlayerCharacter = StarfallCharacter;
				WeaponImpactDirection = (End - Start).GetSafeNormal();

				WeaponImpactPoint = FireHit.ImpactPoint;

				WeaponImpactBone = FireHit.BoneName;

				//OnRep_WeaponImpactImpulseForce();

				StarfallCharacter->ImpactImpulseForce = WeaponImpactImpulseForce;
				StarfallCharacter->ImpactDirection = WeaponImpactDirection;
				StarfallCharacter->BoneImpactName = WeaponImpactBone;
				StarfallCharacter->ImpactPoint = WeaponImpactPoint;
			}
			if (StarfallCharacter && HasAuthority() && InstigatorController)
			{
				//OnRep_ImpulseForce();
				UGameplayStatics::ApplyDamage(
					StarfallCharacter,
					Damage,
					InstigatorController,
					this,
					UDamageType::StaticClass()
				);

			}

			AWeapon* StarfallWeapon = Cast<AWeapon>(FireHit.GetActor());
			{
				if (StarfallWeapon && HasAuthority())
				{
					FVector AWeaponImpactDirection = (End - Start).GetSafeNormal();
					FVector ImpactPoint = FireHit.ImpactPoint;

					StarfallWeapon->GetWeaponMesh()->AddImpulseAtLocation(AWeaponImpactDirection * WeaponImpactImpulseForce, FireHit.ImpactPoint, FireHit.BoneName);
				}
			}


			if (ImpactParticles)
			{
				//FTransform SpawnTransform = GetActorTransform();
				UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
					GetWorld(),
					ImpactParticles,
					FireHit.ImpactPoint,
					FireHit.ImpactNormal.Rotation()
				);
			}	
		}
		if (BeamParticles)
		{
			//FTransform SpawnTransform = GetActorTransform();
			UNiagaraComponent* Beam = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(),
				BeamParticles,
				SocketTransform.GetLocation(),
				SocketTransform.GetRotation().Rotator()
			);
			if (Beam)
			{
				Beam->SetVectorParameter(FName("BeamEnd"), BeamEnd);
			}
		}
		
	}
}




