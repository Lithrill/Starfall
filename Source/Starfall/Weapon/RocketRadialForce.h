// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RocketRadialForce.generated.h"

UCLASS()
class STARFALL_API ARocketRadialForce : public AActor
{
	GENERATED_BODY()


	
	
public:	
	// Sets default values for this actor's properties
	ARocketRadialForce();

	UPROPERTY(EditAnywhere)
	float RadialImpactImpulseForce = 0.f;

	UPROPERTY(EditAnywhere)
	float RadialOuterRadius = 0.f;

	UPROPERTY(EditAnywhere)
	float RadialForceStrenght = 0.f;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	class URadialForceComponent* MyRadialForceComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USphereComponent* MySphereComponent;


private:

	

	UPROPERTY(EditAnywhere)
	bool bApplyRadialForce;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
