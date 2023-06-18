// Fill out your copyright notice in the Description page of Project Settings.


#include "Casing.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Sound/SoundCue.h"

ACasing::ACasing()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasingMesh"));
	SetRootComponent(CasingMesh);
	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	CasingMesh->SetSimulatePhysics(true);
	CasingMesh->SetEnableGravity(true);
	CasingMesh->SetNotifyRigidBodyCollision(true);
	ShellEjectionImpulse = 5.f;
	SoundOnce = true;
}


void ACasing::BeginPlay()
{
	Super::BeginPlay();
	
	CasingMesh->OnComponentHit.AddDynamic(this, &ACasing::OnHit);
	CasingMesh->AddImpulse(GetActorForwardVector() * ShellEjectionImpulse);
}

void ACasing::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (ShellSound && SoundOnce)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ShellSound, GetActorLocation());
		SoundOnce = false;
	}

	// Set the delay time in seconds
	float DelayTime = 5.0f;

	// Create a timer handle
	FTimerHandle TimerHandle;

	// Bind the timer delegate to your function
	FTimerDelegate TimerDelegate = FTimerDelegate::CreateUObject(this, &ACasing::ShellTimer);
	
}

void ACasing::ShellTimer()
{
	Destroy();
}



