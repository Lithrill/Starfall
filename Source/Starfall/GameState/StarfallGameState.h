// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "StarfallGameState.generated.h"

/**
 * 
 */
UCLASS()
class STARFALL_API AStarfallGameState : public AGameState
{
	GENERATED_BODY()
	

public:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void UpdateTopScore(class AStarfallPlayerState* ScoringPlayer);


	UPROPERTY(Replicated)
	TArray<class AStarfallPlayerState*> TopScoringPlayers;

private:

	float TopScore = 0.f;
};
