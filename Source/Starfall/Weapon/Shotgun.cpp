// Fill out your copyright notice in the Description page of Project Settings.


#include "Shotgun.h"
#include "Sound/SoundCue.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Starfall/Character/StarfallCharacter.h"
#include "Weapon.h"
#include "Starfall/Environment/WindowColourChanger.h"
#include "Kismet/GameplayStatics.h"

void AShotgun::Fire(const FVector& HitTarget)
{
	AWeapon::Fire(HitTarget);
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;
	AController* InstigatorController = OwnerPawn->GetController();

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();
		
		TMap<AStarfallCharacter*, uint32> HitMap;
		for (uint32 i = 0; i < NumberOfPellets; i++)
		{
			FHitResult FireHit;
			FHitResult WindowHit;
			WeaponTraceHit(Start, HitTarget, FireHit, WindowHit);

			AStarfallCharacter* StarfallCharacter = Cast<AStarfallCharacter>(FireHit.GetActor());
			if (StarfallCharacter && HasAuthority() && InstigatorController)
			{
				if (HitMap.Contains(StarfallCharacter))
				{
					HitMap[StarfallCharacter]++;
				}
				else
				{
					HitMap.Emplace(StarfallCharacter, 1);
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
			if (HitSound)
			{
				UGameplayStatics::PlaySoundAtLocation(
					this,
					HitSound,
					FireHit.ImpactPoint,
					.5f,
					FMath::FRandRange(-.5f, +.5f)
				);
			}
		}
		
		for (auto HitPair : HitMap)
		{
			if (HitPair.Key && HasAuthority() && InstigatorController)
			{
				UGameplayStatics::ApplyDamage(
					HitPair.Key,
					Damage * HitPair.Value,
					InstigatorController,
					this,
					UDamageType::StaticClass()
				);
			}
		}

		
	}
}