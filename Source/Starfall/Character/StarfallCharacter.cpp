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
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "StarfallCharacterAnimInstance.h"
#include "Starfall/Starfall.h"
#include "Starfall/PlayerController/StarfallPlayerController.h"
#include "Starfall/GameMode/StarfallGameMode.h"
#include "Math/RandomStream.h"
#include "TimerManager.h"
#include "Starfall/ElimAnimations/HumanElimAnimation.h"
#include "Engine/World.h"
#include "Starfall/HUD/ScoreHUD.h"
#include "Starfall/HUD/StarfallHUD.h"
#include "Starfall/PlayerState/StarfallPlayerState.h"



// Sets default values
AStarfallCharacter::AStarfallCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 400.0f;
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
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetCharacterMovement()->RotationRate = FRotator(0.f, 0.f, 850.f);

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	NetUpdateFrequency = 125.f;
	MinNetUpdateFrequency = 100.f;

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));

	
}

void AStarfallCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AStarfallCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(AStarfallCharacter, Health);
}

void AStarfallCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();
	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0.f;
}

void AStarfallCharacter::Elim()
{
	if (Combat && Combat->EquippedWeapon)
	{
		Combat->EquippedWeapon->Dropped();
	}
	MulticastElim();
	GetWorldTimerManager().SetTimer(
		ElimTimer,
		this,
		&AStarfallCharacter::ElimTimerFinished,
		ElimDelay
	);

}

void AStarfallCharacter::MulticastElim_Implementation()
{
	if (StarfallPlayerController)
	{
		StarfallPlayerController->SetHUDWeaponAmmo(0);
	}
	bElimmed = true;
	PlayElimMontage();

	// Start dissolve effect

	if (DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);

		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		GetMesh()->SetMaterial(1, DynamicDissolveMaterialInstance);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	StartDissolve();

	// Disable Character movemenet

	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	if (StarfallPlayerController)
	{
		DisableInput(StarfallPlayerController);
	}
	// Disable collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	//Spawn in ElimVehicleObject
	if (ElimVehicleToSpawn)
	{
		UWorld* World = GetWorld();
		if (World == nullptr)
		{
			return;
		}

		FActorSpawnParameters SpawnParams;
		

		FVector SpawnCustomProperties;

		//SpawnCustomProperties.X = -1000.f;
		//SpawnCustomProperties.Y = -6000.f;
		SpawnCustomProperties.Z = 500.f;

		FVector SpawnLocation = GetActorLocation()  + SpawnCustomProperties + (GetActorForwardVector() * -5000); // Offset from character forward direction
		FRotator SpawnRotation = GetActorRotation();

		
		World->SpawnActor<AHumanElimAnimation>(ElimVehicleToSpawn, SpawnLocation, SpawnRotation, SpawnParams);

		if (ElimVehicleToSpawn != nullptr)
		{
			// Do something with the new object
		}
	}
}

void AStarfallCharacter::ElimTimerFinished()
{
	AStarfallGameMode* StarfallGameMode = GetWorld()->GetAuthGameMode<AStarfallGameMode>();
	if (StarfallGameMode)
	{
		StarfallGameMode->RequestRespawn(this, Controller);
	}
}

// Called when the game starts or when spawned
void AStarfallCharacter::BeginPlay()
{
	Super::BeginPlay();

	UpdateHUDHealth();

	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &AStarfallCharacter::ReceiveDamage);
	}
	
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

	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaTime);
	}
	else
	{
		TimeSinceLastMovementReplication += DeltaTime;
		if (TimeSinceLastMovementReplication > 0.25f)
		{
			OnRep_ReplicatedMovement();
		}
		CalculateAO_Pitch();
	}

	HideCameraIfCharacterClose();
	PollInit();
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
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, this, &AStarfallCharacter::FirePressed);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AStarfallCharacter::FireReleased);
		EnhancedInputComponent->BindAction(ScoreAction, ETriggerEvent::Triggered, this, &AStarfallCharacter::ScorePressed);
		EnhancedInputComponent->BindAction(ScoreAction, ETriggerEvent::Completed, this, &AStarfallCharacter::ScoreReleased);
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

void AStarfallCharacter::PlayFireMontage(bool bAiming)
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		FName SectionName;
		SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void AStarfallCharacter::PlayElimMontage()
{

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ElimMontage)
	{
		AnimInstance->Montage_Play(ElimMontage);

		// Generate a random index for selecting a section
		FRandomStream RandomStream;
		RandomStream.GenerateNewSeed();

		// Get the section names of the "elimslot" slot
		TArray<FName> SectionNames = { FName("Death"), FName("Death2"), FName("Death3"), 
			FName("Death4"), FName("Death5"), FName("Death6"), 
			FName("Death7"), FName("Death8"), FName("Death9"), 
			FName("Death10"), FName("Death11"), FName("Death12"), 
			FName("Death13"), FName("Death14"), FName("Death15") 
		};

		if (SectionNames.Num() > 0)
		{
			int32 RandomIndex = RandomStream.RandRange(0, SectionNames.Num() - 1);

			// Get the random section name
			FName RandomSectionName = SectionNames[RandomIndex];

			// Jump to the random section
			AnimInstance->Montage_JumpToSection(RandomSectionName);
		}
	}
}

void AStarfallCharacter::PlayHitReactMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
		FName SectionName("FromFront");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void AStarfallCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
	UpdateHUDHealth();
	PlayHitReactMontage();

	if (Health == 0.f)
	{
		AStarfallGameMode* StarfallGameMode = GetWorld()->GetAuthGameMode<AStarfallGameMode>();
		if (StarfallGameMode)
		{
			StarfallPlayerController = StarfallPlayerController == nullptr ? Cast<AStarfallPlayerController>(Controller) : StarfallPlayerController;
			AStarfallPlayerController* AttackerController = Cast<AStarfallPlayerController>(InstigatorController);
			StarfallGameMode->PlayerEliminated(this, StarfallPlayerController, AttackerController);
		}
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
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Super::Jump();
	}
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

void AStarfallCharacter::FirePressed()
{
	if (Combat)
	{
		Combat->FireButtonPressed(true);
	}
}

void AStarfallCharacter::ScorePressed()
{
	if (!WasScorePressed)
	{
		WasScorePressed = true;
		APlayerController* PlayerController = IsValid(this) ? Cast<APlayerController>(this->GetController()) : nullptr;
		if (PlayerController)
		{
			AStarfallHUD* StarfallHUDWidget = Cast<AStarfallHUD>(PlayerController->GetHUD());
			if (StarfallHUDWidget)
			{
				StarfallHUDWidget->ScoreWidgetAdd();
				
			}
		}
	}
}

void AStarfallCharacter::ScoreReleased()
{
	if (WasScorePressed)
	{
		WasScorePressed = false;
		APlayerController* PlayerController = IsValid(this) ? Cast<APlayerController>(this->GetController()) : nullptr;

		if (PlayerController)
		{
			AStarfallHUD* StarfallHUDWidget = Cast<AStarfallHUD>(PlayerController->GetHUD());
			if (StarfallHUDWidget)
			{
				StarfallHUDWidget->ScoreWidgetRemove();
				
			}
		}
	}
	
}

void AStarfallCharacter::FireReleased()
{
	if (Combat)
	{
		Combat->FireButtonPressed(false);
	}
}

float AStarfallCharacter::CalculateSpeed()
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	return Velocity.Size();
}

void AStarfallCharacter::AimOffset(float DeltaTime)
{
	if (Combat && Combat->EquippedWeapon == nullptr) return;
	float Speed = CalculateSpeed();
	bool bIsInAir = GetCharacterMovement()->IsFalling();

	if (Speed == 0.f && !bIsInAir) //Standing still, not jumping
	{
		bRotateRootBone = true;
		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}
		bUseControllerRotationYaw = true;
		TurnInPlace(DeltaTime);

	}
	if (Speed > 0.f || bIsInAir) // running, or jumping
	{
		bRotateRootBone = false;
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	CalculateAO_Pitch();
}

void AStarfallCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// map pitch from [270, 360) to [-90, 0)
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void AStarfallCharacter::SimProxiesTurn()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;
	bRotateRootBone = false;
	float Speed = CalculateSpeed();
	if (Speed > 0.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

	if (FMath::Abs(ProxyYaw) > TurnThreshold)
	{
		if (ProxyYaw > TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if (ProxyYaw < -TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
		else
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
		return;
	}
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}

void AStarfallCharacter::TurnInPlace(float DeltaTime)
{
	if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}
	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 4.f);
		AO_Yaw = InterpAO_Yaw;
		if(FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

void AStarfallCharacter::ServerEquipButtonPressed_Implementation()
{
	if (Combat)
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}
}

void AStarfallCharacter::HideCameraIfCharacterClose()
{
	if (!IsLocallyControlled()) return;
	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		GetMesh()->SetVisibility(false);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
}

void AStarfallCharacter::OnRep_Health()
{
	UpdateHUDHealth();
	PlayHitReactMontage();
}

void AStarfallCharacter::UpdateHUDHealth()
{
	StarfallPlayerController = StarfallPlayerController == nullptr ? Cast<AStarfallPlayerController>(Controller) : StarfallPlayerController;
	if (StarfallPlayerController)
	{
		StarfallPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

void AStarfallCharacter::PollInit()
{
	if (StarfallPlayerState == nullptr)
	{
		StarfallPlayerState = GetPlayerState<AStarfallPlayerState>();
		if (StarfallPlayerState)
		{
			StarfallPlayerState->AddToScore(0.f);
			StarfallPlayerState->AddToDefeats(0);
		}
	}
}



void AStarfallCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if (DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);

	}
}

void AStarfallCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &AStarfallCharacter::UpdateDissolveMaterial);
	if (DissolveCurve && DissolveTimeline)
	{
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeline->Play();
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

AWeapon* AStarfallCharacter::GetEquippedWeapon()
{
	if (Combat == nullptr) return nullptr;
	return Combat->EquippedWeapon;
}

FVector AStarfallCharacter::GetHitTarget() const
{
	if (Combat == nullptr) return FVector();
	return Combat->HitTarget;
}





