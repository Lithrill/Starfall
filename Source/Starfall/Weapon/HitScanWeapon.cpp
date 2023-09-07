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
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "Sound/SoundCue.h"
#include "WeaponTypes.h"



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
		

		FHitResult FireHit;
		FHitResult WindowHit;

		WeaponTraceHit(Start, HitTarget, FireHit, WindowHit);

		AStarfallCharacter* StarfallCharacter = Cast<AStarfallCharacter>(FireHit.GetActor());
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
				FireHit.ImpactPoint
			);
		}
			
		if (MuzzleFlash)
		{
			UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(),
				MuzzleFlash,
				SocketTransform.GetLocation(),
				SocketTransform.GetRotation().Rotator()
			);
		}
		if (FireSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				FireSound,
				GetActorLocation()
			);
		}
	}
}

void AHitScanWeapon::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit, FHitResult& WindowOutHit)
{
	
	UWorld* World = GetWorld();
	if (World)
	{
		FVector End = bUseScatter ? TraceEndWithScatter(TraceStart, HitTarget) : TraceStart + (HitTarget - TraceStart) * 1.25f;
		FVector WindowEnd = bUseScatter ? TraceEndWithScatter(TraceStart, HitTarget) : TraceStart + (HitTarget - TraceStart) * 1.25f;

		World->LineTraceSingleByChannel(
			OutHit,
			TraceStart,
			End,
			ECollisionChannel::ECC_Visibility
		);

		if (World)
		{
			World->LineTraceSingleByChannel(
				WindowOutHit,
				TraceStart,
				WindowEnd,
				ECollisionChannel::ECC_GameTraceChannel3
			);
		}

		FVector BeamEnd = End;

		if (OutHit.bBlockingHit)
		{
			BeamEnd = OutHit.ImpactPoint;
			AStarfallCharacter* StarfallCharacter = Cast<AStarfallCharacter>(OutHit.GetActor());
			if (StarfallCharacter)
			{
				StarfallPlayerCharacter = StarfallCharacter;
				WeaponImpactDirection = (End - TraceStart).GetSafeNormal();

				WeaponImpactPoint = OutHit.ImpactPoint;

				WeaponImpactBone = OutHit.BoneName;

				//OnRep_WeaponImpactImpulseForce();

				StarfallCharacter->ImpactImpulseForce = WeaponImpactImpulseForce;
				StarfallCharacter->ImpactDirection = WeaponImpactDirection;
				StarfallCharacter->BoneImpactName = WeaponImpactBone;
				StarfallCharacter->ImpactPoint = WeaponImpactPoint;
			}
		}

		AWeapon* StarfallWeapon = Cast<AWeapon>(OutHit.GetActor());
		{
			if (StarfallWeapon && HasAuthority())
			{
				FVector AWeaponImpactDirection = (End - TraceStart).GetSafeNormal();
				FVector ImpactPoint = OutHit.ImpactPoint;

				StarfallWeapon->GetWeaponMesh()->AddImpulseAtLocation(AWeaponImpactDirection * WeaponImpactImpulseForce, OutHit.ImpactPoint, OutHit.BoneName);
			}
		}

		//Check if it passes through a window or not
		if (WindowOutHit.bBlockingHit)
		{
			AWindowColourChanger* WindowActor = Cast<AWindowColourChanger>(WindowOutHit.GetActor());
			if (WindowActor)
			{
				WindowActor->ChangeWindowColour();
			}
		}

		if (BeamParticles)
		{
			//FTransform SpawnTransform = GetActorTransform();
			UNiagaraComponent* Beam = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(),
				BeamParticles,
				TraceStart,
				FRotator::ZeroRotator
			);
			if (Beam)
			{
				Beam->SetVectorParameter(FName("BeamEnd"), BeamEnd);
			}
		}
	}
}

FVector AHitScanWeapon::TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget)
{
	FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
	FVector EndLoc = SphereCenter + RandVec;
	FVector ToEndLoc = EndLoc - TraceStart;

	/*DrawDebugSphere(GetWorld(), SphereCenter, SphereRadius, 12, FColor::Red, true);
	DrawDebugSphere(GetWorld(), EndLoc, 4.f, 12, FColor::Orange, true);
	DrawDebugLine(GetWorld(), TraceStart, FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size()), FColor::Cyan, true);*/

	return FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size());
}


