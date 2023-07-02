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
	virtual void OnRep_Score() override;
	void AddToScore(float ScoreAmount);

private:
	class AStarfallCharacter* Character;
	class AStarfallPlayerController* Controller;
};
