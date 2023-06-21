// Fill out your copyright notice in the Description page of Project Settings.


#include "StarfallCharacterAnimInstance.h"
#include "StarfallCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Starfall/Weapon/Weapon.h"


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
	EquippedWeapon = StarfallCharacter->GetEquippedWeapon();
	bIsCrouched = StarfallCharacter->bIsCrouched;
	bAiming = StarfallCharacter->IsAiming();
	TurningInPlace = StarfallCharacter->GetTurningInPlace();

	//Offset Yaw for Strafing
	FRotator AimRotation = StarfallCharacter->GetBaseAimRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(StarfallCharacter->GetVelocity());
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 5.f);
	YawOffset = DeltaRotation.Yaw;

	AO_Yaw = StarfallCharacter->GetAO_Yaw();
	AO_Pitch = StarfallCharacter->GetAO_Pitch();

	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && StarfallCharacter->GetMesh())
	{
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
		FVector OutPosition;
		FRotator OutRotation;
		StarfallCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		if (StarfallCharacter->IsLocallyControlled())
		{
			bLocallyControlled = true;
			FTransform RightHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("hand_r"), ERelativeTransformSpace::RTS_World);
			RightHandRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - StarfallCharacter->GetHitTarget()));
		}

		
	}
}
