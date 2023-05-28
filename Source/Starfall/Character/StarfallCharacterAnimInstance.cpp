// Fill out your copyright notice in the Description page of Project Settings.


#include "StarfallCharacterAnimInstance.h"
#include "StarfallCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

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
}
