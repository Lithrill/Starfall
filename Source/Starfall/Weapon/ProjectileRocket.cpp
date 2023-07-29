// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileRocket.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Sound/SoundCue.h"
#include "Components/BoxComponent.h"
#include "Sound/SoundCue.h"
#include "Components/AudioComponent.h"
#include "RocketMovementComponent.h"
#include "Starfall/Character/StarfallCharacter.h"
#include "Weapon.h"
#include "CollisionQueryParams.h"

AProjectileRocket::AProjectileRocket()
{
	RocketMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rocket Mesh"));
	RocketMesh->SetupAttachment(RootComponent);
	RocketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RocketMovementComponent = CreateDefaultSubobject<URocketMovementComponent>(TEXT("RocketMovementComponent"));
	RocketMovementComponent->bRotationFollowsVelocity = true;
	RocketMovementComponent->SetIsReplicated(true);
}



void AProjectileRocket::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectileRocket::OnHit);
	}

	if (TrailSystem)
	{
		TrailSystemComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			TrailSystem,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false
		);
	}
	if (ProjectileLoop && LoopingSoundAttenuation)
	{
		ProjectileLoopComponent = UGameplayStatics::SpawnSoundAttached(
			ProjectileLoop,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			EAttachLocation::KeepWorldPosition,
			false,
			1.f,
			1.f,
			0.f,
			LoopingSoundAttenuation,
			(USoundConcurrency*) nullptr,
			false
		);
	}
}

void AProjectileRocket::DestroyTimerFinished()
{
	Destroy();
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor == GetOwner())
	{
		return;
	}
	APawn* FiringPawn = GetInstigator();
	if (FiringPawn && HasAuthority())
	{
		AController* FiringController = FiringPawn->GetController();
		if (FiringController)
		{
			//Apply Directional Impulse 

			bool bTraceComplex = false; // Set this to true if you want precise collision checks
			TArray<AStarfallCharacter*> StarfallCharacters;
			TArray<AWeapon*> WeaponActors;
			//TSet<AStarfallCharacter*> StarfallCharacters;

			TArray<AActor*> IgnoreActors;
			IgnoreActors.Add(this);

			TArray<FHitResult> OutHits;
			TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
			ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));
			ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldDynamic));


			// Perform a capsule trace for each character in the level
			// Adjust the capsule radius and length according to your needs
			bool bHit = UKismetSystemLibrary::CapsuleTraceMultiForObjects(
				this, // World context object
				GetActorLocation(), // Start location (origin of the explosion)
				GetActorLocation(), // End location (use the same as the start for a point-based explosion)
				DamageOuterRadius, // Capsule radius (damage outer radius)
				DamageOuterRadius / 2, // Capsule half-height (damage outer radius + damage falloff)
				ObjectTypes, // Object types to trace (here, we're tracing against pawns/characters)
				bTraceComplex, // Trace complex shapes (true for precise checks, false for simple checks)
				IgnoreActors, // Actors to ignore (in this case, the projectile actor itself)
				EDrawDebugTrace::None, // Draw debug traces (useful for debugging)
				OutHits, // Output array of hit results
				true // Ignore self (set to true to ignore the damage causer itself)
			);

			if (bHit)
			{
				for (const FHitResult& HitResult : OutHits)
				{
					AActor* AffectedActor = HitResult.GetActor();
					AStarfallCharacter* StarfallCharacter = Cast<AStarfallCharacter>(AffectedActor);
					AWeapon* Weapon = Cast<AWeapon>(AffectedActor);
					if (StarfallCharacter && !StarfallCharacters.Contains(StarfallCharacter))
					{
						// Check if DamagedActor is already in the DamagedActors array
						bool bIsDuplicate = false;
						for (AActor* ExistingAffectedActor : StarfallCharacters)
						{
							if (ExistingAffectedActor == AffectedActor)
							{
								bIsDuplicate = true;
								break;
							}
						}

						if (!bIsDuplicate)
						{
							// Perform a line trace from the explosion origin to the hit actor's location
							FHitResult LineTraceHit;
							FVector EndLocation = HitResult.Location; // Use the hit actor's location as the end location of the line trace
							FCollisionQueryParams LineTraceParams;
							LineTraceParams.AddIgnoredActor(this); // Ignore the projectile actor itself


							if (!GetWorld()->LineTraceSingleByChannel(LineTraceHit, GetActorLocation(), EndLocation, ECC_GameTraceChannel2, LineTraceParams))
							{
								if (StarfallCharacter)
								{
									StarfallCharacter->ExplosionRadius = DamageOuterRadius;
									StarfallCharacter->ExplosionForce = ExplosionImpactImpulseForce;
									StarfallCharacter->ExplosionPoint = GetActorLocation();

									StarfallCharacter->GetMesh()->AddRadialImpulse(GetActorLocation(), DamageOuterRadius, ExplosionImpactImpulseForce, ERadialImpulseFalloff::RIF_Linear);
								}
								
							}
						}
					}
					if (Weapon && !WeaponActors.Contains(Weapon))
					{
						// Check if DamagedActor is already in the DamagedActors array
						bool bIsDuplicate = false;
						for (AActor* ExistingAffectedActor : WeaponActors)
						{
							if (ExistingAffectedActor == AffectedActor)
							{
								bIsDuplicate = true;
								break;
							}
						}

						if (!bIsDuplicate)
						{
							// Perform a line trace from the explosion origin to the hit actor's location
							FHitResult LineTraceHit;
							FVector EndLocation = HitResult.Location; // Use the hit actor's location as the end location of the line trace
							FCollisionQueryParams LineTraceParams;
							LineTraceParams.AddIgnoredActor(this); // Ignore the projectile actor itself


							if (!GetWorld()->LineTraceSingleByChannel(LineTraceHit, GetActorLocation(), EndLocation, ECC_GameTraceChannel2, LineTraceParams))
							{
								if (Weapon)
								{
									Weapon->GetWeaponMesh()->AddRadialImpulse(GetActorLocation(), DamageOuterRadius, ExplosionImpactImpulseForce, ERadialImpulseFalloff::RIF_Linear);
								}
							}
						}
					}
				}



				//Apply Damage

				UGameplayStatics::ApplyRadialDamageWithFalloff(
					this, // World context object
					Damage, //BaseDamage
					10.f, // MinimumDamage
					GetActorLocation(), // Origin
					DamageInnerRadius, // DamageInnerRadius
					DamageOuterRadius, // DamageOuterRadius
					DamageFalloff, // DamageFalloff
					UDamageType::StaticClass(), //Damage type class
					TArray<AActor*>(), // IgnoreActors
					this, // DamageCauser
					FiringController, // InstigatorController
					ECC_GameTraceChannel2
				);
			}
		}
	}

	GetWorldTimerManager().SetTimer(
		DestroyTimer,
		this,
		&AProjectileRocket::DestroyTimerFinished,
		DestroyTime
	);

	if (ImpactParticles)
	{
		FTransform SpawnTransform = GetActorTransform();
		UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			ImpactParticles,
			SpawnTransform.GetLocation(),
			SpawnTransform.GetRotation().Rotator()
		);
	}
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
	if (RocketMesh)
	{
		RocketMesh->SetVisibility(false);
	}
	if (CollisionBox)
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (TrailSystem && TrailSystemComponent->GetSystemInstance())
	{
		TrailSystemComponent->GetSystemInstance()->Deactivate();
	}
	if (ProjectileLoopComponent && ProjectileLoopComponent->IsPlaying())
	{
		ProjectileLoopComponent->Stop();
	}
}

void AProjectileRocket::Destroyed()
{

}
