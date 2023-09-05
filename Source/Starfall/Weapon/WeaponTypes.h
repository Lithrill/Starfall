#pragma once

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	EWT_AssaultRifle UMETA(DisplayName = "Assault Rifle"),
	EWT_RocketLauncher UMETA(DisplayName = "Rocket Launcher"),
	EWT_GalacticCrocketLauncher UMETA(DisplayName = "Galactic Crocket Launcher"),
	EWT_RocketJumperLauncher UMETA(DisplayName = "Rocket Jumper Launcher"),
	EWT_Pistol UMETA(DisplayName = "Pistol"),
	EWT_SubmachineGun UMETA(DisplayName = "Submachine Gun"),

	EWT_MAX UMETA(DisplayName = "DefaultMAX")
};