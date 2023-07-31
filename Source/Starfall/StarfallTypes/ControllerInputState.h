#pragma once

UENUM(BlueprintType)
enum class EControllerInputState : uint8
{
	ECIS_Keyboard UMETA(DisplayName = "Keyboard"),

	ECIS_Controller UMETA(DisplayName = "Controller")

};