// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterOverlay.generated.h"

/**
 * 
 */
UCLASS()
class STARFALL_API UCharacterOverlay : public UUserWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* HealthBar;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* HealthText;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* WeaponAmmoAmount;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* CarriedAmmoAmount;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* MatchCountdownText;

	UPROPERTY(meta = (BindWidget))
	class UImage* Minimap;

	UPROPERTY(VisibleAnywhere)
	class UCanvasRenderTarget2D* MinimapTarget;

	UPROPERTY()
	UMaterialInstanceDynamic* CHDynamicMinimapIconMaterialInstance;

	void UpdateCharacterMinimapMaterial();
};
