// Fill out your copyright notice in the Description page of Project Settings.


#include "HumanElimAnimation.h"
#include "Components/SkeletalMeshComponent.h"
#include "Math/Vector.h"
#include "Kismet/KismetMathLibrary.h"
#include "TimerManager.h"


// Sets default values
AHumanElimAnimation::AHumanElimAnimation()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	ElimVehicleMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ElimVehicleMesh"));
	SetRootComponent(ElimVehicleMesh);
	
	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));
}

void AHumanElimAnimation::TimerObjectDestroy()
{
	Destroy();
}



// Called when the game starts or when spawned
void AHumanElimAnimation::BeginPlay()
{
	Super::BeginPlay();

	if (DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);

		ElimVehicleMesh->SetMaterial(0, DynamicDissolveMaterialInstance);
		ElimVehicleMesh->SetMaterial(1, DynamicDissolveMaterialInstance);
		ElimVehicleMesh->SetMaterial(2, DynamicDissolveMaterialInstance);
		ElimVehicleMesh->SetMaterial(3, DynamicDissolveMaterialInstance);
		ElimVehicleMesh->SetMaterial(4, DynamicDissolveMaterialInstance);
		ElimVehicleMesh->SetMaterial(5, DynamicDissolveMaterialInstance);
		ElimVehicleMesh->SetMaterial(6, DynamicDissolveMaterialInstance);
		ElimVehicleMesh->SetMaterial(7, DynamicDissolveMaterialInstance);
		ElimVehicleMesh->SetMaterial(8, DynamicDissolveMaterialInstance);
		ElimVehicleMesh->SetMaterial(9, DynamicDissolveMaterialInstance);
		ElimVehicleMesh->SetMaterial(10, DynamicDissolveMaterialInstance);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 2.f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	StartDissolve();
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}
	GetWorldTimerManager().SetTimer(DestroyTimerHandle, this, &AHumanElimAnimation::TimerObjectDestroy, 3.f, false, 3.f);
}

void AHumanElimAnimation::FlightDirection(float DeltaTime)
{
	FVector Location = GetActorLocation();

	Location += GetActorForwardVector() * Speed * DeltaTime;

	SetActorLocation(Location);

	
}

//void AHumanElimAnimation::MulticastHuamanElim_Implementation()
//{
//
//}

void AHumanElimAnimation::UpdateDissolveMaterial(float DissolveValue)
{
	if (DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);

	}
}

void AHumanElimAnimation::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &AHumanElimAnimation::UpdateDissolveMaterial);
	if (DissolveCurve && DissolveTimeline)
	{
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeline->Play();
	}
}

// Called every frame
void AHumanElimAnimation::Tick(float DeltaTime) 
{
	Super::Tick(DeltaTime);
	if (HasAuthority())
	{
	float DeltaTime = GetWorld()->GetDeltaSeconds();
	FlightDirection(DeltaTime);
	//MulticastHuamanElim_Implementation();
	}
}

