// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "StarfallGameMode.generated.h"

/**
 * 
 */
UCLASS()
class STARFALL_API AStarfallGameMode : public AGameMode
{
	GENERATED_BODY()
	
public:
	virtual void PlayerEliminated(class AStarfallCharacter* ElimmedCharacter, class AStarfallPlayerController* VictimController, class AStarfallPlayerController* AttackerController);
};
