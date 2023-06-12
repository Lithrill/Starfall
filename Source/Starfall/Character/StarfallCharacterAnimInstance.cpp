// Fill out your copyright notice in the Description page of Project Settings.


#include "StarfallCharacterAnimInstance.h"
#include "StarfallCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"


void UStarfallCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	StarfallCharacter = Cast<AStarfallCharacter>(TryGetPawnOwner());
}

void UStarfallCharacterAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (StarfallCharacter == nullptr)
	{
		StarfallCharacter = Cast<AStarfallCharacter>(TryGetPawnOwner());
	}
	if (StarfallCharacter == nullptr) return;
	
	FVector Velocity = StarfallCharacter->GetVelocity();
	Velocity.Z = 0.f;
	Speed = Velocity.Size();

	bIsInAir = StarfallCharacter->GetCharacterMovement()->IsFalling();

	bIsAccelerating = StarfallCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;

	bWeaponEquipped = StarfallCharacter->IsWeaponEquipped();

	bIsCrouched = StarfallCharacter->bIsCrouched;

	bAiming = StarfallCharacter->IsAiming();

	//Offset Yaw for Strafing
	FRotator AimRotation = StarfallCharacter->GetBaseAimRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(StarfallCharacter->GetVelocity());
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 5.f);
	YawOffset = DeltaRotation.Yaw;
}
