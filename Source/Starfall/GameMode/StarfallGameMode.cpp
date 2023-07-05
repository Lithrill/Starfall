// Fill out your copyright notice in the Description page of Project Settings.


#include "StarfallGameMode.h"
#include "Starfall/Character/StarfallCharacter.h"
#include "Starfall/PlayerController/StarfallPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Starfall/PlayerState/StarfallPlayerState.h"

void AStarfallGameMode::PlayerEliminated(class AStarfallCharacter* ElimmedCharacter, class AStarfallPlayerController* VictimController, class AStarfallPlayerController* AttackerController)
{
	AStarfallPlayerState* AttackerPlayerState = AttackerController ? Cast<AStarfallPlayerState>(AttackerController->PlayerState) : nullptr;
	AStarfallPlayerState* VictimPlayerState = VictimController ? Cast<AStarfallPlayerState>(VictimController->PlayerState) : nullptr;

	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState)
	{
		AttackerPlayerState->AddToScore(1.f);
	}
	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1);
	}

	if (ElimmedCharacter)
	{
		ElimmedCharacter->Elim();
	}
}

void AStarfallGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Reset();
		ElimmedCharacter->Destroy();
	}
	if (ElimmedController)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]);
	}
}
