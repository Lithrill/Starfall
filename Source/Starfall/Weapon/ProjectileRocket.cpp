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
#include "PhysicsEngine/RadialForceComponent.h"
#include "Components/SphereComponent.h"
#include "RocketRadialForce.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"


AProjectileRocket::AProjectileRocket()
{
	RocketMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rocket Mesh"));
	RocketMesh->SetupAttachment(RootComponent);
	RocketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RocketMovementComponent = CreateDefaultSubobject<URocketMovementComponent>(TEXT("RocketMovementComponent"));
	RocketMovementComponent->bRotationFollowsVelocity = true;
	RocketMovementComponent->SetIsReplicated(true);

	PrimaryActorTick.bCanEverTick = false;



	
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

	if (OtherComp && OtherComp->GetCollisionObjectType() == ECC_GameTraceChannel3)
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
					//if (!StarfallCharacter && !Weapon)
					//{
					//	//SetChaosDamage && change gravity property
					//	bool bIsDuplicate = false;
					//	for (AActor* ExistingAffectedActor : ActorsRefs)
					//	{
					//		if (ExistingAffectedActor == AffectedActor && ExistingAffectedActor != Weapon && ExistingAffectedActor != StarfallCharacter)
					//		{
					//			bIsDuplicate = true;
					//			break;
					//		}
					//	}
					//	if (!bIsDuplicate)
					//	{
					//		UPrimitiveComponent* ActorRootComponent = AffectedActor->FindComponentByClass<UPrimitiveComponent>();
					//		if (ActorRootComponent)
					//		{
					//			ActorRootComponent->SetSimulatePhysics(true);
					//			ActorRootComponent->SetEnableGravity(true);
					//			UE_LOG(LogTemp, Error, TEXT("Gravity switched on"));
					//		}
					//		
					//	}
					//}

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

				ExplosionFieldSystem = GetWorld()->SpawnActor<UBlueprint>(GetActorLocation(), GetActorRotation());
				if (ExplosionFieldSystem)
				{
					// Optionally, you can do something with the spawned object, such as set its properties or add it to an array for future reference.
				}


			}
		}
	}


	ClientExplosionForce();
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

void AProjectileRocket::ClientExplosionForce()
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
			//if (!StarfallCharacter && !Weapon)
			//{
			//	//SetChaosDamage && change gravity property
			//	bool bIsDuplicate = false;
			//	for (AActor* ExistingAffectedActor : ActorsRefs)
			//	{
			//		if (ExistingAffectedActor == AffectedActor && ExistingAffectedActor != Weapon && ExistingAffectedActor != StarfallCharacter)
			//		{
			//			bIsDuplicate = true;
			//			break;
			//		}
			//	}
			//	if (!bIsDuplicate)
			//	{
			//		UPrimitiveComponent* ActorRootComponent = AffectedActor->FindComponentByClass<UPrimitiveComponent>();
			//		if (ActorRootComponent)
			//		{
			//			ActorRootComponent->SetSimulatePhysics(true);
			//			ActorRootComponent->SetEnableGravity(true);
			//			UE_LOG(LogTemp, Error, TEXT("Gravity switched on"));
			//		}
			//		
			//	}
			//}

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
