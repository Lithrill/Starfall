// Fill out your copyright notice in the Description page of Project Settings.


#include "StarfallCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Starfall/Weapon/Weapon.h"
#include "Starfall/StarfallComponents/CombatComponent.h"

// Sets default values
AStarfallCharacter::AStarfallCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
}

void AStarfallCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AStarfallCharacter, OverlappingWeapon, COND_OwnerOnly);
}

// Called when the game starts or when spawned
void AStarfallCharacter::BeginPlay()
{
	Super::BeginPlay();

	
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(StarfallCharacterMappingContext, 0);
		}
	}
	
}

void AStarfallCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void AStarfallCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AStarfallCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AStarfallCharacter::Look);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &AStarfallCharacter::Jump);
		EnhancedInputComponent->BindAction(EquipAction, ETriggerEvent::Triggered, this, &AStarfallCharacter::Equip);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Triggered, this, &AStarfallCharacter::Crouch);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Triggered, this, &AStarfallCharacter::Aim);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &AStarfallCharacter::AimEnd);
	}
}

void AStarfallCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (Combat)
	{
		Combat->Character = this;
	}
}

void AStarfallCharacter::Move(const FInputActionValue& Value)
{
	
	const FVector2D MovementVector = Value.Get<FVector2D>();

	const FRotator Rotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);
	
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	AddMovementInput(ForwardDirection, MovementVector.Y);

	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	AddMovementInput(RightDirection, MovementVector.X);
}
void AStarfallCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisValue = Value.Get<FVector2D>();
	if (GetController())
	{
		AddControllerYawInput(LookAxisValue.X);
		AddControllerPitchInput(LookAxisValue.Y);
	}
}
void AStarfallCharacter::Jump()
{
	Super::Jump();
}

void AStarfallCharacter::Equip()
{
	if (Combat)
	{
		if (HasAuthority())
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else
		{
			ServerEquipButtonPressed();
		}
	}
}

void AStarfallCharacter::Crouch()
{
	if (bIsCrouched)
	{
		ACharacter::UnCrouch();
	}
	else
	{
		ACharacter::Crouch();
	}
}

void AStarfallCharacter::Aim()
{
	if (Combat)
	{
		Combat->SetAiming(true);
	}
}

void AStarfallCharacter::AimEnd()
{
	if (Combat)
	{
		Combat->SetAiming(false);
	}
}

void AStarfallCharacter::ServerEquipButtonPressed_Implementation()
{
	if (Combat)
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}
}

void AStarfallCharacter::SetOVerlappingWeapon(AWeapon* Weapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}
	
	OverlappingWeapon = Weapon;

	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

void AStarfallCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

bool AStarfallCharacter::IsWeaponEquipped()
{
	return (Combat && Combat->EquippedWeapon);
}

bool AStarfallCharacter::IsAiming()
{
	return (Combat && Combat->bAiming);
}




