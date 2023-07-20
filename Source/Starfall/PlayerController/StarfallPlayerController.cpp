// Fill out your copyright notice in the Description page of Project Settings.


#include "StarfallPlayerController.h"
#include "Starfall/HUD/StarfallHUD.h"
#include "Starfall/HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Starfall/Character/StarfallCharacter.h"
#include "Starfall/HUD/ScoreHUD.h"
#include "Net/UnrealNetwork.h"
#include "Starfall/GameMode/StarfallGameMode.h"
#include "Starfall/HUD/Announcement.h"
#include "Kismet/GameplayStatics.h"
#include "Starfall/StarfallComponents/CombatComponent.h"
#include "Starfall/Character/StarfallCharacter.h"
#include "Starfall/PlayerState/StarfallPlayerState.h"
#include "Starfall/GameState/StarfallGameState.h"
#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"



void AStarfallPlayerController::BeginPlay()
{
	Super::BeginPlay();

	StarfallHUD = Cast<AStarfallHUD>(GetHUD());
	ServerCheckMatchState();
}

void AStarfallPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AStarfallPlayerController, MatchState);
}

void AStarfallPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetHUDTime();
	CheckTimeSync(DeltaTime);
	PollInit();
}

void AStarfallPlayerController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime;
	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}
}



void AStarfallPlayerController::ServerCheckMatchState_Implementation()
{

	AStarfallGameMode* GameMode = Cast<AStarfallGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode)
	{
		WarmupTime = GameMode->WarmupTime;
		MatchTime = GameMode->MatchTime;
		CooldownTime = GameMode->CooldownTime;
		LevelStartingTime = GameMode->LevelStartingTime;
		MatchState = GameMode->GetMatchState();
		ClientJoinMidgame(MatchState, WarmupTime, MatchTime, CooldownTime, LevelStartingTime);
	}
}

void AStarfallPlayerController::ClientJoinMidgame_Implementation(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime)
{
	WarmupTime = Warmup;
	MatchTime = Match;
	CooldownTime = Cooldown;
	LevelStartingTime = StartingTime;
	MatchState = StateOfMatch;
	OnMatchStateSet(MatchState);


	if (StarfallHUD && MatchState == MatchState::WaitingToStart)
	{
		StarfallHUD->AddAnnouncement();
	}
	
}


void AStarfallPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	AStarfallCharacter* StarfallCharacter = Cast<AStarfallCharacter>(InPawn);
	if (StarfallCharacter)
	{
		SetHUDHealth(StarfallCharacter->GetHealth(), StarfallCharacter->GetMaxHealth());
	}

	AStarfallPlayerController* StarfallPlayerController = IsValid(this) ? Cast<AStarfallPlayerController>(this) : nullptr;
	if (StarfallPlayerController)
	{
		AStarfallCharacter* MyStarfallControlledCharacter = Cast<AStarfallCharacter>(StarfallPlayerController->GetPawn());
		if (MyStarfallControlledCharacter)
		{
			MyStarfallControlledCharacter->SetUpPlayerInput();
		}
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
	else
	{
		bInitializeCharacterOverlay = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
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
	else
	{
		bInitializeCharacterOverlay = true;
		HUDScore = Score;
	}
		
}

void AStarfallPlayerController::SetHUDDefeats(int32 Defeats)
{
	StarfallHUD = StarfallHUD == nullptr ? Cast<AStarfallHUD>(GetHUD()) : StarfallHUD;
	bool bHUDValid = StarfallHUD &&
		StarfallHUD->ScoreHUD &&
		StarfallHUD->ScoreHUD->DefeatsAmount;
	if (bHUDValid)
	{
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		StarfallHUD->ScoreHUD->DefeatsAmount->SetText(FText::FromString(DefeatsText));

	}
	else
	{
		bInitializeCharacterOverlay = true;
		HUDDefeats = Defeats;
	}
}

void AStarfallPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	StarfallHUD = StarfallHUD == nullptr ? Cast<AStarfallHUD>(GetHUD()) : StarfallHUD;
	bool bHUDValid = StarfallHUD &&
		StarfallHUD->CharacterOverlay &&
		StarfallHUD->CharacterOverlay->WeaponAmmoAmount;
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		StarfallHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));

	}
}

void AStarfallPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
	StarfallHUD = StarfallHUD == nullptr ? Cast<AStarfallHUD>(GetHUD()) : StarfallHUD;
	bool bHUDValid = StarfallHUD &&
		StarfallHUD->CharacterOverlay &&
		StarfallHUD->CharacterOverlay->CarriedAmmoAmount;
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		StarfallHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));

	}
}

void AStarfallPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
	StarfallHUD = StarfallHUD == nullptr ? Cast<AStarfallHUD>(GetHUD()) : StarfallHUD;
	bool bHUDValid = StarfallHUD &&
		StarfallHUD->CharacterOverlay &&
		StarfallHUD->CharacterOverlay->MatchCountdownText;

	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			StarfallHUD->CharacterOverlay->MatchCountdownText->SetText(FText());
			return;
		}
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;


		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		StarfallHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));

	}
}

void AStarfallPlayerController::SetHUDAnnouncementCountdown(float CountdownTime)
{
	StarfallHUD = StarfallHUD == nullptr ? Cast<AStarfallHUD>(GetHUD()) : StarfallHUD;
	bool bHUDValid = StarfallHUD &&
		StarfallHUD->Announcement &&
		StarfallHUD->Announcement->WarmupTime;

	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			StarfallHUD->Announcement->WarmupTime->SetText(FText());
			return;
		}
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;


		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		StarfallHUD->Announcement->WarmupTime->SetText(FText::FromString(CountdownText));

	}
}

void AStarfallPlayerController::SetHUDTime()
{
	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart) TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::InProgress) TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::Cooldown) TimeLeft = CooldownTime + WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;

	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);

	if (HasAuthority())
	{
		StarfallGameMode = StarfallGameMode == nullptr ? Cast<AStarfallGameMode>(UGameplayStatics::GetGameMode(this)) : StarfallGameMode;
		if (StarfallGameMode)
		{
			SecondsLeft = FMath::CeilToInt(StarfallGameMode->GetCountdownTime() + LevelStartingTime);
		}
	}

	if (CountdownInt != SecondsLeft)
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementCountdown(TimeLeft);
		}
		if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountdown(TimeLeft);
		}
	}

	CountdownInt = SecondsLeft;
}

void AStarfallPlayerController::PollInit()
{
	if (CharacterOverlay == nullptr)
	{
		if (StarfallHUD && StarfallHUD->CharacterOverlay)
		{
			CharacterOverlay = StarfallHUD->CharacterOverlay;
			if (CharacterOverlay)
			{
				SetHUDHealth(HUDHealth, HUDMaxHealth);
			}
		}
		
	}
	if (ScoreHUD == nullptr)
	{
		if (StarfallHUD && StarfallHUD->ScoreHUD)
		{
			ScoreHUD = StarfallHUD->ScoreHUD;
			if (ScoreHUD)
			{
				SetHUDScore(HUDScore);
				SetHUDDefeats(HUDDefeats);
			}
		}
	}

	
	//AStarfallPlayerController* StarfallPlayerController = IsValid(this) ? Cast<AStarfallPlayerController>(this) : nullptr;
	//if (StarfallPlayerController)
	//{
	//	AStarfallCharacter* MyStarfallControlledCharacter = Cast<AStarfallCharacter>(StarfallPlayerController->GetPawn());
	//	if (MyStarfallControlledCharacter)
	//	{
	//		MyStarfallControlledCharacter->SetUpPlayerInput();
	//	}
	//}
	
}



void AStarfallPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void AStarfallPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	float CurrentServerTime = TimeServerReceivedClientRequest + (0.5f * RoundTripTime);
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

float AStarfallPlayerController::GetServerTime()
{
	/*if (HasAuthority()) return GetWorld()->GetRealTimeSeconds();*/
	/*else */return GetWorld()->GetRealTimeSeconds() + ClientServerDelta;
}

void AStarfallPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}


}

void AStarfallPlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;


	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}


void AStarfallPlayerController::OnRep_MatchState()
{
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void AStarfallPlayerController::HandleMatchHasStarted()
{
	StarfallHUD = StarfallHUD == nullptr ? Cast<AStarfallHUD>(GetHUD()) : StarfallHUD;
	if (StarfallHUD)
	{
		StarfallHUD->AddCharacterOverlay();
		if (StarfallHUD->Announcement)
		{
			StarfallHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void AStarfallPlayerController::HandleCooldown()
{
	StarfallHUD = StarfallHUD == nullptr ? Cast<AStarfallHUD>(GetHUD()) : StarfallHUD;
	if (StarfallHUD)
	{
		StarfallHUD->CharacterOverlay->RemoveFromParent();
		StarfallHUD->ScoreHUD->RemoveFromParent();
		bool bHUDValid = StarfallHUD->Announcement && 
			StarfallHUD->Announcement->AnnouncementText && 
			StarfallHUD->Announcement->InfoText;

		if (bHUDValid)
		{
			StarfallHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
			FString AnnouncementText("New Match Starts In:");
			StarfallHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));
			
			
			AStarfallGameState* StarfallGameState = Cast<AStarfallGameState>(UGameplayStatics::GetGameState(this));
			AStarfallPlayerState* StarfallPlayerState = GetPlayerState<AStarfallPlayerState>();
			if (StarfallGameState && StarfallPlayerState)
			{
				TArray<AStarfallPlayerState*> TopPlayers = StarfallGameState->TopScoringPlayers;
				FString InfoTextString;
				if (TopPlayers.Num() == 0)
				{
					InfoTextString = FString("Legends were not born today.");
				}
				else if (TopPlayers.Num() == 1 && TopPlayers[0] == StarfallPlayerState)
				{
					InfoTextString = FString("Your deeds are known,\nyou are born a legend!");
				}
				else if (TopPlayers.Num() == 1)
				{
					InfoTextString = FString::Printf(TEXT("A legend was born: \n%s"), *TopPlayers[0]->GetPlayerName());
				}
				else if (TopPlayers.Num() > 1)
				{
					InfoTextString = FString("The legends that are born:\n");
					for (auto TiedPlayer : TopPlayers)
					{
						InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
						
					}
				}
				UUniformGridSlot* GridSlot = StarfallHUD->Announcement->InfoTextGridBox->AddChildToUniformGrid(StarfallHUD->Announcement->InfoText);
				GridSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Top);
				GridSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Center);
				StarfallHUD->Announcement->InfoText->SetText(FText::FromString(InfoTextString));
			}

			
		}
	}
	AStarfallCharacter* StarfallCharacter = Cast<AStarfallCharacter>(GetPawn());
	if (StarfallCharacter && StarfallCharacter->GetCombat())
	{
		StarfallCharacter->bDisableGameplay = true;
		StarfallCharacter->GetCombat()->FireButtonPressed(false);
	}
}
