// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/TimelineComponent.h"
#include "HumanElimAnimation.generated.h"

UCLASS()
class STARFALL_API AHumanElimAnimation : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AHumanElimAnimation();

	
	

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = MoveActor)
	USkeletalMeshComponent* ElimVehicleMesh;

	UPROPERTY(EditAnywhere, Category = MoveActor)
	float Speed;

	

private:

	UFUNCTION()
	void FlightDirection(float DeltaTime);

	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastHuamanElim();

	FTimerHandle ElimVehicleTimer;


	/*
	* Dissolve effect
	*/

	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimeline;
	FOnTimelineFloat DissolveTrack;

	UPROPERTY(EditAnywhere)
	UCurveFloat* DissolveCurve;

	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);

	void StartDissolve();

	//Dynamic Instance that we can change at runtime
	UPROPERTY(VisibleAnywhere, Category = Elim)
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;

	//Material instance set on the Blueprint, used with the dynamic material instance
	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* DissolveMaterialInstance;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
