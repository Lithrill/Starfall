// Fill out your copyright notice in the Description page of Project Settings.


#include "WindowColourChanger.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"

// Sets default values
AWindowColourChanger::AWindowColourChanger()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	WindowMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WindowMesh"));
	SetRootComponent(WindowMesh);
	WindowMesh->SetMaterial(0, BlueMaterialInstance);
}

void AWindowColourChanger::OnWindowBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ChangeWindowColour();
}

void AWindowColourChanger::OnWindowHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit)
{
	ChangeWindowColour();
}

void AWindowColourChanger::ChangeWindowColour()
{
	WindowMesh->SetMaterial(0, RedMaterialInstance);

	GetWorldTimerManager().SetTimer(ChangeColourTimerHandle, this, &AWindowColourChanger::TimerColourChange, 1.f, false, 1.f);
	UE_LOG(LogTemp, Error, TEXT("Colour changed 1"));
}

void AWindowColourChanger::TimerColourChange()
{
	WindowMesh->SetMaterial(0, BlueMaterialInstance);
	UE_LOG(LogTemp, Error, TEXT("Colour changed 2"));
}

// Called when the game starts or when spawned
void AWindowColourChanger::BeginPlay()
{
	Super::BeginPlay();
	WindowMesh->OnComponentBeginOverlap.AddDynamic(this, &AWindowColourChanger::OnWindowBeginOverlap);
	OnActorHit.AddDynamic(this, &AWindowColourChanger::OnWindowHit);

}



// Called every frame
void AWindowColourChanger::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

