// Fill out your copyright notice in the Description page of Project Settings.


#include "StarfallCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Starfall/StarfallTypes/ControllerInputState.h"
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
#include "Starfall/Weapon/WeaponTypes.h"
#include "Starfall/GameMode/StarfallGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "InputCoreTypes.h"
#include "Starfall/HUD/PickUpWidget.h"
#include "Components/SceneCaptureComponent2D.h"
#include "PaperSpriteComponent.h"
#include "PaperSprite.h"
#include "Engine/CanvasRenderTarget2D.h"

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

	MinimapCameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("MiniMapCameraBoom"));
	MinimapCameraBoom->SetupAttachment(GetMesh());
	MinimapCameraBoom->TargetArmLength = 400.0f;
	MinimapCameraBoom->bUsePawnControlRotation = false;

	MinimapCaptureComponent2D = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("MiniMapCameraComponent"));
	MinimapCaptureComponent2D->SetupAttachment(MinimapCameraBoom, USpringArmComponent::SocketName);

	 /*PlayerSpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteComponent"));
	 PlayerSpriteComponent->SetupAttachment(GetMesh());*/

	 /*PlayerSprite = CreateDefaultSubobject<UPaperSprite>(TEXT("PlayerSprite"));
	 PlayerSprite->Attach*/

	//MinimapRenderTarget = CreateDefaultSubobject<UTextureRenderTarget2D>(TEXT("PlayerMinimapRenderTarget"));
	//
	//
	
	
	//
	
	
}

void AStarfallCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AStarfallCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(AStarfallCharacter, Health);
	DOREPLIFETIME(AStarfallCharacter, bDisableGameplay);
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

	if (!HasAuthority())
	{
		SetUpPlayerInput();
	}
	
}

void AStarfallCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	SetUpPlayerInput();
}

void AStarfallCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


	RotateInPlace(DeltaTime);
	HideCameraIfCharacterClose();
	PollInit();
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


	//Ragdoll the player

	//GetCharacterMovement()->DisableMovement();
	//GetCharacterMovement()->StopMovementImmediately();
	//GetCharacterMovement()->SetComponentTickEnabled(false);

	//// Disable capsule collision
	//GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	//GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	//GetMesh()->SetAllBodiesSimulatePhysics(true);
	//GetMesh()->SetSimulatePhysics(true);
	//GetMesh()->WakeAllRigidBodies();

	//bool bExplosionImpactSettings = (ExplosionPoint == FVector::ZeroVector && ExplosionForce == 0.f && ExplosionRadius == 0.f);
	//if (!bExplosionImpactSettings)
	//{
	//	GetMesh()->AddRadialImpulse(ExplosionPoint, ExplosionRadius, ExplosionForce, ERadialImpulseFalloff::RIF_Linear);
	//}

	//bool bImpactSettings = (ImpactDirection == FVector::ZeroVector && ImpactPoint == FVector::ZeroVector && BoneImpactName == FName() && ImpactImpulseForce == 0.f);
	//if (!bImpactSettings)
	//{
	//	GetMesh()->AddImpulseAtLocation(ImpactDirection * ImpactImpulseForce, ImpactPoint, BoneImpactName);
	//}
	//
}

void AStarfallCharacter::Destroy()
{
	AStarfallGameMode* StarfallGameMode = Cast<AStarfallGameMode>(UGameplayStatics::GetGameMode(this));
	bool bMatchNotInProgress = (StarfallGameMode->GetMatchState() != MatchState::InProgress);

	if (Combat && Combat->EquippedWeapon && bMatchNotInProgress)
	{
		Combat->EquippedWeapon->Destroy();
	}
}

void AStarfallCharacter::SetUpPlayerInput()
{
	if (AStarfallPlayerController* PlayerController = Cast<AStarfallPlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(StarfallCharacterMappingContext, 0);
		}

		if (PlayerController)
		{
			ThisPlayerController = PlayerController;
		}
	}
	AStarfallPlayerController* MyPlayerController = IsValid(this) ? Cast<AStarfallPlayerController>(this->GetController()) : nullptr;
	//ThisPlayerController = MyPlayerController;

	if (IsLocallyControlled() && !HasAuthority())
	{
		MinimapRenderTarget = NewObject<UCanvasRenderTarget2D>(this);
		MinimapRenderTarget->InitAutoFormat(1024, 1024); // Set the dimensions and format as needed



		if (MinimapRenderTarget && MinimapCaptureComponent2D)
		{
			MinimapCaptureComponent2D->TextureTarget = MinimapRenderTarget;
			if (MinimapIconMaterial)
			{
				DynamicMinimapIconMaterialInstance = UMaterialInstanceDynamic::Create(MinimapIconMaterial, this);
			}
			if (DynamicMinimapIconMaterialInstance && StarfallPlayerController && DynamicMinimapIconMaterialInstance != nullptr)
			{
				UE_LOG(LogTemp, Warning, TEXT("DynamicMinimapIconMaterialInstance: %s"), DynamicMinimapIconMaterialInstance ? TEXT("Valid") : TEXT("Invalid"));

				DynamicMinimapIconMaterialInstance->SetTextureParameterValue("MyTextureParam", MinimapRenderTarget);
				StarfallPlayerController->DynamicMinimapMaterialInstance = DynamicMinimapIconMaterialInstance;

				StarfallPlayerController->UpdateMinimapMat();
			}
		}
	}
	else if (HasAuthority())
	{

	}

	
	

}

void AStarfallCharacter::MulticastElim_Implementation()
{
	if (StarfallPlayerController)
	{
		StarfallPlayerController->SetHUDWeaponAmmo(0);
		StarfallPlayerController->SetHUDCarriedAmmo(0);
	}
	
	//PlayElimMontage();

	
	

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


	
	bDisableGameplay = true;
	if (Combat)
	{
		Combat->FireButtonPressed(false);
	}
	// Disable collision
	/*GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);*/


	//Ragdoll the player

	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->SetComponentTickEnabled(false);

	// Disable capsule collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetAllBodiesSimulatePhysics(true);
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->WakeAllRigidBodies();

	bool bExplosionImpactSettings = (ExplosionPoint == FVector::ZeroVector && ExplosionForce == 0.f && ExplosionRadius == 0.f);
	if (!bExplosionImpactSettings)
	{
		GetMesh()->AddRadialImpulse(ExplosionPoint, ExplosionRadius, ExplosionForce, ERadialImpulseFalloff::RIF_Linear);
	}

	bool bImpactSettings = (ImpactDirection == FVector::ZeroVector && ImpactPoint == FVector::ZeroVector && BoneImpactName == FName() && ImpactImpulseForce == 0.f);
	if (!bImpactSettings)
	{
		GetMesh()->AddImpulseAtLocation(ImpactDirection * ImpactImpulseForce, ImpactPoint, BoneImpactName);
	}

	bElimmed = true;
	
}

void AStarfallCharacter::ElimTimerFinished()
{
	AStarfallGameMode* StarfallGameMode = GetWorld()->GetAuthGameMode<AStarfallGameMode>();
	if (StarfallGameMode)
	{
		StarfallGameMode->RequestRespawn(this, Controller);
	}
}



void AStarfallCharacter::RotateInPlace(float DeltaTime)
{
	if (bDisableGameplay)
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}
		
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
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AStarfallCharacter::FirePressed);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AStarfallCharacter::FireReleased);
		EnhancedInputComponent->BindAction(ScoreAction, ETriggerEvent::Triggered, this, &AStarfallCharacter::ScorePressed);
		EnhancedInputComponent->BindAction(ScoreAction, ETriggerEvent::Completed, this, &AStarfallCharacter::ScoreReleased);
		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &AStarfallCharacter::ReloadPressed);
	}
}

void  AStarfallCharacter::ClientCheckCurrentInputMode()
{
	if (ThisPlayerController)
	{

		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(ThisPlayerController->GetLocalPlayer()))
		{
			TArray<FKey> QueryKeys = Subsystem->QueryKeysMappedToAction(PlayerActions);
			for (FKey key : QueryKeys)
			{
				if (key.IsGamepadKey())
				{
					if (ThisPlayerController->IsInputKeyDown(EKeys::A))
					{

						PlayerInputState = EControllerInputState::ECIS_Keyboard;
						//IsUsingKeyBoard = true;
						//IsUsingController = false;
					}
					else if (ThisPlayerController->IsInputKeyDown(EKeys::W))
					{

						PlayerInputState = EControllerInputState::ECIS_Keyboard;
						//UE_LOG(LogTemp, Warning, TEXT("keyboard"));
						//UE_LOG(LogTemp, Warning, TEXT("PlayerInputState: %s"), *UEnum::GetValueAsString(PlayerInputState));
					}
					else if (ThisPlayerController->IsInputKeyDown(EKeys::Gamepad_LeftStick_Up))
					{

						PlayerInputState = EControllerInputState::ECIS_Controller;
						//UE_LOG(LogTemp, Warning, TEXT("Gamepad"));
						//UE_LOG(LogTemp, Warning, TEXT("PlayerInputState: %s"), *UEnum::GetValueAsString(PlayerInputState));
					}
				}
				else if (key.IsMouseButton())
				{
					//UE_LOG(LogTemp, Warning, TEXT("Is Mouse input"));
					//UE_LOG(LogTemp, Warning, TEXT("PlayerInputState: %s"), *UEnum::GetValueAsString(PlayerInputState));
				}
			}
		}
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

void AStarfallCharacter::PlayReloadMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);
		FName SectionName;
	
		switch (Combat->EquippedWeapon->GetWeaponType())
		{
		case EWeaponType::EWT_AssaultRifle:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_RocketLauncher:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_GalacticCrocketLauncher:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_Pistol:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_RocketJumperLauncher:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_SubmachineGun:
			SectionName = FName("Rifle");
			break;
		}

		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void AStarfallCharacter::PlayElimMontage()
{

	//UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	//if (AnimInstance && ElimMontage)
	//{
	//	AnimInstance->Montage_Play(ElimMontage);

	//	// Generate a random index for selecting a section
	//	FRandomStream RandomStream;
	//	RandomStream.GenerateNewSeed();

	//	// Get the section names of the "elimslot" slot
	//	TArray<FName> SectionNames = { FName("Death"), FName("Death2"), FName("Death3"), 
	//		FName("Death4"), FName("Death5"), FName("Death6"), 
	//		FName("Death7"), FName("Death8"), FName("Death9"), 
	//		FName("Death10"), FName("Death11"), FName("Death12"), 
	//		FName("Death13"), FName("Death14"), FName("Death15") 
	//	};

	//	if (SectionNames.Num() > 0)
	//	{
	//		int32 RandomIndex = RandomStream.RandRange(0, SectionNames.Num() - 1);

	//		// Get the random section name
	//		FName RandomSectionName = SectionNames[RandomIndex];

	//		// Jump to the random section
	//		AnimInstance->Montage_JumpToSection(RandomSectionName);
	//	}
	//}
}

void AStarfallCharacter::PlayHitReactMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	/*UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
		FName SectionName("FromFront");
		AnimInstance->Montage_JumpToSection(SectionName);
	}*/
}

void AStarfallCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
	UpdateHUDHealth();
	/*PlayHitReactMontage();*/

	
	
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
	if (bDisableGameplay) return;
	PlayerActions = MoveAction;
	ClientCheckCurrentInputMode();
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
	PlayerActions = LookAction;
	ClientCheckCurrentInputMode();
	const FVector2D LookAxisValue = Value.Get<FVector2D>();
	if (GetController())
	{
		AddControllerYawInput(LookAxisValue.X);
		AddControllerPitchInput(LookAxisValue.Y);
	}
	
}
void AStarfallCharacter::Jump()
{
	if (bDisableGameplay) return;
	PlayerActions = JumpAction;
	ClientCheckCurrentInputMode();
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
	if (bDisableGameplay) return;
	PlayerActions = EquipAction;
	ClientCheckCurrentInputMode();
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
	if (bDisableGameplay) return;
	PlayerActions = CrouchAction;
	ClientCheckCurrentInputMode();
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
	if (bDisableGameplay) return;
	PlayerActions = AimAction;
	ClientCheckCurrentInputMode();
	if (Combat)
	{
		Combat->SetAiming(true);
	}
	
}

void AStarfallCharacter::AimEnd()
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		Combat->SetAiming(false);
	}
}

void AStarfallCharacter::FirePressed()
{
	if (bDisableGameplay) return;
	PlayerActions = FireAction;
	ClientCheckCurrentInputMode();
	if (Combat)
	{
		Combat->FireButtonPressed(true);
	}

}

void AStarfallCharacter::ScorePressed()
{
	if (bDisableGameplay) return;
	PlayerActions = ScoreAction;
	ClientCheckCurrentInputMode();
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
	if (bDisableGameplay) return;
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

void AStarfallCharacter::ReloadPressed()
{
	if (bDisableGameplay) return;
	PlayerActions = ReloadAction;
	ClientCheckCurrentInputMode();
	if (Combat)
	{
		Combat->Reload();
	}
	
}

void AStarfallCharacter::FireReleased()
{
	if (bDisableGameplay) return;
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

void AStarfallCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{

		if (OverlappingWeapon)
		{
			if (OverlappingWeapon)
			{
				
				/*if (PlayerInputState == EControllerInputState::ECIS_Keyboard && OverlappingWeapon->PickUpWidgetClass && OverlappingWeapon->PickUpWidgetClass->ControllerPickupText && OverlappingWeapon->PickUpWidgetClass->PickupText)
				{

					OverlappingWeapon->PickUpWidgetClass->PickupText->SetIsEnabled(true);
					OverlappingWeapon->PickUpWidgetClass->ControllerPickupText->SetIsEnabled(false);
					OverlappingWeapon->PickUpWidgetClass->PickupText->SetVisibility(ESlateVisibility::Visible);
					OverlappingWeapon->PickUpWidgetClass->ControllerPickupText->SetVisibility(ESlateVisibility::Hidden);
					OverlappingWeapon->ShowPickupWidget(false);
				}
				else if (PlayerInputState == EControllerInputState::ECIS_Controller && OverlappingWeapon->PickUpWidgetClass && OverlappingWeapon->PickUpWidgetClass->ControllerPickupText && OverlappingWeapon->PickUpWidgetClass->PickupText)
				{
					OverlappingWeapon->PickUpWidgetClass->ControllerPickupText->SetIsEnabled(true);
					OverlappingWeapon->PickUpWidgetClass->PickupText->SetIsEnabled(false);
					OverlappingWeapon->PickUpWidgetClass->PickupText->SetVisibility(ESlateVisibility::Hidden);
					OverlappingWeapon->PickUpWidgetClass->ControllerPickupText->SetVisibility(ESlateVisibility::Visible);
					OverlappingWeapon->ShowPickupWidget(false);
				}*/
				OverlappingWeapon->ShowPickupWidget(false);
			}
			
		}
	
	
	
	OverlappingWeapon = Weapon;


	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			UE_LOG(LogTemp, Error, TEXT("Is client activated"));
			if (PlayerInputState == EControllerInputState::ECIS_Keyboard && OverlappingWeapon->PickUpWidgetClass && OverlappingWeapon->PickUpWidgetClass->ControllerPickupText && OverlappingWeapon->PickUpWidgetClass->PickupText)
			{
				OverlappingWeapon->PickUpWidgetClass->PickupText->SetIsEnabled(true);
				OverlappingWeapon->PickUpWidgetClass->ControllerPickupText->SetIsEnabled(false);
				OverlappingWeapon->PickUpWidgetClass->PickupText->SetVisibility(ESlateVisibility::Visible);
				OverlappingWeapon->PickUpWidgetClass->ControllerPickupText->SetVisibility(ESlateVisibility::Hidden);
				OverlappingWeapon->ShowPickupWidget(true);
			}
			else if (PlayerInputState == EControllerInputState::ECIS_Controller && OverlappingWeapon->PickUpWidgetClass && OverlappingWeapon->PickUpWidgetClass->ControllerPickupText && OverlappingWeapon->PickUpWidgetClass->PickupText)
			{
				OverlappingWeapon->PickUpWidgetClass->ControllerPickupText->SetIsEnabled(true);
				OverlappingWeapon->PickUpWidgetClass->PickupText->SetIsEnabled(false);
				OverlappingWeapon->PickUpWidgetClass->PickupText->SetVisibility(ESlateVisibility::Hidden);
				OverlappingWeapon->PickUpWidgetClass->ControllerPickupText->SetVisibility(ESlateVisibility::Visible);
				OverlappingWeapon->ShowPickupWidget(true);
			}

		}
	}

}

void AStarfallCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (OverlappingWeapon)
	{
		UE_LOG(LogTemp, Error, TEXT("Replicated"));
		if (PlayerInputState == EControllerInputState::ECIS_Keyboard && OverlappingWeapon->PickUpWidgetClass && OverlappingWeapon->PickUpWidgetClass->ControllerPickupText && OverlappingWeapon->PickUpWidgetClass->PickupText)
		{
			OverlappingWeapon->PickUpWidgetClass->PickupText->SetIsEnabled(true);
			OverlappingWeapon->PickUpWidgetClass->ControllerPickupText->SetIsEnabled(false);
			OverlappingWeapon->PickUpWidgetClass->PickupText->SetVisibility(ESlateVisibility::Visible);
			OverlappingWeapon->PickUpWidgetClass->ControllerPickupText->SetVisibility(ESlateVisibility::Hidden);
			OverlappingWeapon->ShowPickupWidget(true);
		}
		else if (PlayerInputState == EControllerInputState::ECIS_Controller && OverlappingWeapon->PickUpWidgetClass &&  OverlappingWeapon->PickUpWidgetClass->ControllerPickupText && OverlappingWeapon->PickUpWidgetClass->PickupText)
		{
			OverlappingWeapon->PickUpWidgetClass->ControllerPickupText->SetIsEnabled(true);
			OverlappingWeapon->PickUpWidgetClass->PickupText->SetIsEnabled(false);
			OverlappingWeapon->PickUpWidgetClass->PickupText->SetVisibility(ESlateVisibility::Hidden);
			OverlappingWeapon->PickUpWidgetClass->ControllerPickupText->SetVisibility(ESlateVisibility::Visible);
			OverlappingWeapon->ShowPickupWidget(true);
		}
		
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

ECombatState AStarfallCharacter::GetCombatState() const
{
	if (Combat == nullptr) return ECombatState::ECS_MAX;
	return Combat->CombatState;
}




