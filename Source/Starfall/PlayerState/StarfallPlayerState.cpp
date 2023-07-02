// Fill out your copyright notice in the Description page of Project Settings.


#include "StarfallPlayerState.h"
#include "Starfall/Character/StarfallCharacter.h"
#include "Starfall/PlayerController/StarfallPlayerController.h"
void AStarfallPlayerState::AddToScore(float ScoreAmount)
{
	Score += ScoreAmount;
	Character = Character == nullptr ? Cast<AStarfallCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<AStarfallPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{

		}
	}
}
void AStarfallPlayerState::OnRep_Score()
{
	Super::OnRep_Score();

	Character = Character == nullptr ? Cast<AStarfallCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<AStarfallPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{

		}
	}
}


