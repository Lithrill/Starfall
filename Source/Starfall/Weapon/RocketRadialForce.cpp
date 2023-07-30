// Fill out your copyright notice in the Description page of Project Settings.


#include "RocketRadialForce.h"
#include "PhysicsEngine/RadialForceComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Starfall/Character/StarfallCharacter.h"


// Sets default values
ARocketRadialForce::ARocketRadialForce()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;


	// Create the Radial Force Component and attach it to the RootComponent
	MyRadialForceComponent = CreateDefaultSubobject<URadialForceComponent>(TEXT("MyRadialForce"));
	MyRadialForceComponent->SetupAttachment(RootComponent);
	//MyRadialForceComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MyRadialForceComponent->Radius = RadialOuterRadius;
	MyRadialForceComponent->ImpulseStrength = RadialImpactImpulseForce;
	MyRadialForceComponent->ForceStrength = RadialForceStrenght;
	//MyRadialForceComponent->SetActive(false);

	MySphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("MySphereComponent"));
	MySphereComponent->SetupAttachment(MyRadialForceComponent);
	MySphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

// Called when the game starts or when spawned
void ARocketRadialForce::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		/*AddRadialImpulse(GetActorLocation(), RadialOuterRadius, RadialImpactImpulseForce, ERadialImpulseFalloff::RIF_Constant, true);*/
		bool bTraceComplex = true;

		TArray<AActor*> IgnoreActors;
		IgnoreActors.Add(this);

		TArray<AStarfallCharacter*> StarfallCharacters;

		TArray<FHitResult> OutHits;

		TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
		ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));
		ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldDynamic));

		TArray<AActor*> OverlappingActors;
		TSubclassOf<AStarfallCharacter> PlayerCharacterClass = AStarfallCharacter::StaticClass();

		TArray<AActor*> OverlappingPlayerCharacters;

		GetOverlappingActors(OverlappingPlayerCharacters, PlayerCharacterClass);

		for(AActor* OverlappingActor : OverlappingActors)
		{
			AStarfallCharacter* PlayerCharacter = Cast<AStarfallCharacter>(OverlappingActor);
			if (PlayerCharacter)
			{
				FHitResult LineTraceHit;
				FVector StartLocation = GetActorLocation();
				FVector EndLocation = PlayerCharacter->GetActorLocation();

				FCollisionQueryParams TraceParams;
				TraceParams.AddIgnoredActor(this);

				bool bHit = GetWorld()->LineTraceSingleByChannel(LineTraceHit, StartLocation, EndLocation, ECC_GameTraceChannel2, TraceParams);
				if (!bHit)
				{
					PlayerCharacter->GetMesh()->AddRadialImpulse(GetActorLocation(), RadialOuterRadius, RadialImpactImpulseForce, ERadialImpulseFalloff::RIF_Constant, true);
				}
			}
		}

		

			
		
	}
}

// Called every frame
void ARocketRadialForce::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

