// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterOverlay.h"
#include "Components/Image.h"

void UCharacterOverlay::UpdateCharacterMinimapMaterial()
{

	if (CHDynamicMinimapIconMaterialInstance != nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("This triggered"));
		Minimap->SetBrushFromMaterial(CHDynamicMinimapIconMaterialInstance);
	}
	
	
}