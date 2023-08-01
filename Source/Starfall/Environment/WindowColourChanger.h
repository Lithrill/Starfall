// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WindowColourChanger.generated.h"

UCLASS()
class STARFALL_API AWindowColourChanger : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWindowColourChanger();

	void ChangeWindowColour();

	void TimerColourChange();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


	UPROPERTY(EditAnywhere, Category = WindowColour)
	UMaterialInstance* RedMaterialInstance;

	UPROPERTY(EditAnywhere, Category = WindowColour)
	UMaterialInstance* BlueMaterialInstance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Window)
	UStaticMeshComponent* WindowMesh;

	FTimerHandle ChangeColourTimerHandle;

	UFUNCTION()
	void OnWindowBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	

	UFUNCTION()
	void OnWindowHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
