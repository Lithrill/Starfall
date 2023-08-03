// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Starfall/StarfallTypes/ControllerInputState.h"
#include "PickUpWidget.generated.h"

/**
 * 
 */
UCLASS()
class STARFALL_API UPickUpWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	UPROPERTY(meta = (BindWidget))
	UTextBlock* PickupText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ControllerPickupText;
};
