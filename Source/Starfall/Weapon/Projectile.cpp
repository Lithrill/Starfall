// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectile.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
//#include "Particles/ParticleSystemComponent.h"
//#include "Particles/ParticleSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Sound/SoundCue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Starfall/Starfall.h"
#include "Starfall/Character/StarfallCharacter.h"
#include "Weapon.h"

AProjectile::AProjectile()
{
 	
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);
	CollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECollisionResponse::ECR_Block);
}


void AProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	if (Tracer)
	{
		UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			Tracer,
			CollisionBox,
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			true // AutoDestroy
		);
	}

	if (HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
	}
}

void AProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	Destroy();
}

void AProjectile::SpawnTrailSystem()
{
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
}

void AProjectile::ExplodeDamage()
{
	
	APawn* FiringPawn = GetInstigator();
	if (FiringPawn && HasAuthority())
	{
		AController* FiringController = FiringPawn->GetController();
		if (FiringController)
		{


			//Apply Directional Impulse 

			bool bTraceComplex = false; // Set this to true if you want precise collision checks
			TArray<AStarfallCharacter*> StarfallCharacters;
			TArray<AActor*> ActorsRefs;
			TArray<AWeapon*> WeaponActors;
			//TSet<AStarfallCharacter*> StarfallCharacters;

			TArray<AActor*> IgnoreActors;
			IgnoreActors.Add(this);

			TArray<FHitResult> OutHits;
			TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
			ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));
			ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldDynamic));
			ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_GameTraceChannel1));


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
							FVector EndLocation = StarfallCharacter->GetActorLocation(); // Use the hit actor's location as the end location of the line trace
							FCollisionQueryParams LineTraceParams;



							LineTraceParams.AddIgnoredActor(this); // Ignore the projectile actor itself
							GetWorld()->LineTraceSingleByChannel(LineTraceHit, GetActorLocation(), EndLocation, ECC_EngineTraceChannel2, LineTraceParams);

							/*	DrawDebugLine(
									GetWorld(),
									GetActorLocation(),
									EndLocation,
									FColor::Red,
									false, 100.f, 0,
									50.f
								);*/

								// Check if the line trace hits something
							if (LineTraceHit.GetActor() && LineTraceHit.GetActor() != StarfallCharacter)
							{

							}
							else
							{


								if (StarfallCharacter)
								{
									StarfallCharacter->ExplosionRadius = DamageOuterRadius;
									StarfallCharacter->ExplosionForce = ExplosionImpactImpulseForce;
									StarfallCharacter->ExplosionPoint = GetActorLocation();



									UCharacterMovementComponent* StarfallCharacterMovement = StarfallCharacter->GetCharacterMovement();
									if (StarfallCharacterMovement)
									{
										UE_LOG(LogTemp, Error, TEXT("This has been accessed"));
										FVector NormalizedLaunchDirection = (StarfallCharacter->GetActorLocation() - GetActorLocation()).GetSafeNormal();


										float DistanceFromExplosionCenter = FVector::Distance(StarfallCharacter->GetActorLocation(), GetActorLocation());
										float FalloffRatio = FMath::Clamp(abs(DistanceFromExplosionCenter - DamageOuterRadius) / DamageOuterRadius, 0.f, 1.f);

										float LaunchSpeed = MaxLaunchSpeed * (1.f - FalloffRatio);
										FVector LaunchVelocity = NormalizedLaunchDirection * LaunchSpeed;


										//StarfallCharacter->RocketForceDirection = LaunchVelocity;
										//StarfallCharacter->bRocketForce = true;
										StarfallCharacterMovement->Launch(LaunchVelocity);
									}

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
							FVector EndLocation = Weapon->GetActorLocation(); // Use the hit actor's location as the end location of the line trace
							FCollisionQueryParams LineTraceParams;
							LineTraceParams.AddIgnoredActor(this); // Ignore the projectile actor itself
							//LineTraceParams.AddIgnoredActor(ECollisionChannel::ECC_GameTraceChannel2);

							if (GetWorld()->LineTraceSingleByChannel(LineTraceHit, GetActorLocation(), EndLocation, ECC_Visibility, LineTraceParams))
							{
								if (LineTraceHit.GetActor() && LineTraceHit.GetActor() != Weapon)
								{

								}
								else
								{
									if (Weapon)
									{
										Weapon->GetWeaponMesh()->AddRadialImpulse(GetActorLocation(), DamageOuterRadius, ExplosionImpactImpulseForce, ERadialImpulseFalloff::RIF_Linear);
									}
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
					ECC_GameTraceChannel2 // Channel to ignore
				);
			}
		}
	}
}

void AProjectile::ClientExplosionForce()
{
	bool bTraceComplex = false; // Set this to true if you want precise collision checks
	TArray<AStarfallCharacter*> StarfallCharacters;
	TArray<AActor*> ActorsRefs;
	TArray<AWeapon*> WeaponActors;
	//TSet<AStarfallCharacter*> StarfallCharacters;

	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(this);

	TArray<FHitResult> OutHits;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldDynamic));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_GameTraceChannel1));


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
					FVector EndLocation = StarfallCharacter->GetActorLocation(); // Use the hit actor's location as the end location of the line trace
					FCollisionQueryParams LineTraceParams;



					LineTraceParams.AddIgnoredActor(this); // Ignore the projectile actor itself
					GetWorld()->LineTraceSingleByChannel(LineTraceHit, GetActorLocation(), EndLocation, ECC_EngineTraceChannel2, LineTraceParams);

					/*	DrawDebugLine(
							GetWorld(),
							GetActorLocation(),
							EndLocation,
							FColor::Red,
							false, 100.f, 0,
							50.f
						);*/

						// Check if the line trace hits something
					if (LineTraceHit.GetActor() && LineTraceHit.GetActor() != StarfallCharacter)
					{

					}
					else
					{
						if (StarfallCharacter)
						{
							StarfallCharacter->ExplosionRadius = DamageOuterRadius;
							StarfallCharacter->ExplosionForce = ExplosionImpactImpulseForce;
							StarfallCharacter->ExplosionPoint = GetActorLocation();

						}

					}
				}
			}
		}
	}
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AProjectile::StartDestroyTimer()
{
	GetWorldTimerManager().SetTimer(
		DestroyTimer,
		this,
		&AProjectile::DestroyTimerFinished,
		DestroyTime
	);
}

void AProjectile::DestroyTimerFinished()
{
	Destroy();
}

void AProjectile::Destroyed()
{

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
	Super::Destroyed();
}

