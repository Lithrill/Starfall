// Fill out your copyright notice in the Description page of Project Settings.


#include "StarfallPlayerController.h"
#include "Starfall/HUD/StarfallHUD.h"
#include "Starfall/HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Starfall/Character/StarfallCharacter.h"
#include "Starfall/HUD/ScoreHUD.h"

void AStarfallPlayerController::BeginPlay()
{
	Super::BeginPlay();

	StarfallHUD = Cast<AStarfallHUD>(GetHUD());
}

void AStarfallPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	AStarfallCharacter* StarfallCharacter = Cast<AStarfallCharacter>(InPawn);
	if (StarfallCharacter)
	{
		SetHUDHealth(StarfallCharacter->GetHealth(), StarfallCharacter->GetMaxHealth());
	}
}

void AStarfallPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	StarfallHUD = StarfallHUD == nullptr ? Cast<AStarfallHUD>(GetHUD()) : StarfallHUD;

	bool bHUDValid = StarfallHUD && 
		StarfallHUD->CharacterOverlay && 
		StarfallHUD->CharacterOverlay->HealthBar && 
		StarfallHUD->CharacterOverlay->HealthText;
	if (bHUDValid)
	{
		const float HealthPercent = Health / MaxHealth;
		StarfallHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
		FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		StarfallHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
}

void AStarfallPlayerController::SetHUDScore(float Score)
{
	StarfallHUD = StarfallHUD == nullptr ? Cast<AStarfallHUD>(GetHUD()) : StarfallHUD;
	bool bHUDValid = StarfallHUD &&
		StarfallHUD->ScoreHUD &&
		StarfallHUD->ScoreHUD->ScoreAmount;
	if (bHUDValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		StarfallHUD->ScoreHUD->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
		
}


