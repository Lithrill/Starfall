// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "StarfallPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class STARFALL_API AStarfallPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	/*
	*	Replication notifies
	*/

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnRep_Score() override;

	UFUNCTION()
	virtual void OnRep_Defeats();

	void AddToScore(float ScoreAmount);
	void AddToDefeats(int32 DefeatsAmount);
	
	
private:
	UPROPERTY()
	class AStarfallCharacter* Character;
	UPROPERTY()
	class AStarfallPlayerController* Controller;

	UPROPERTY(ReplicatedUsing = OnRep_Defeats)
	int32 Defeats;
};
