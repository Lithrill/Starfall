// Fill out your copyright notice in the Description page of Project Settings.


#include "StarfallGameMode.h"
#include "Starfall/Character/StarfallCharacter.h"
#include "Starfall/PlayerController/StarfallPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Starfall/PlayerState/StarfallPlayerState.h"

namespace MatchState
{
	const FName Cooldown = FName("Cooldown");
}

AStarfallGameMode::AStarfallGameMode()
{
	bDelayedStart = true;
}

void AStarfallGameMode::BeginPlay()
{
	Super::BeginPlay();

	LevelStartingTime = GetWorld()->GetTimeSeconds();
}



void AStarfallGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MatchState == MatchState::WaitingToStart)
	{
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			StartMatch();
		}
	}
	else if (MatchState == MatchState::InProgress)
	{
		CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			SetMatchState(MatchState::Cooldown);
		}
	}
	else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = CooldownTime + WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			RestartGame();
		}
	}
}

void AStarfallGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AStarfallPlayerController* StarfallPlayer = Cast<AStarfallPlayerController>(*It);
		if (StarfallPlayer)
		{
			StarfallPlayer->OnMatchStateSet(MatchState);
		}
	}
}

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


