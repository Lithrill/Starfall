// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Starfall/StarfallTypes/TurningInPlace.h"
#include "Starfall/Interfaces/InteractWithCrosshairsInterface.h"
#include "Components/TimelineComponent.h"
#include "Starfall/StarfallTypes/CombatState.h"
#include "Starfall/StarfallTypes/ControllerInputState.h"
#include "StarfallCharacter.generated.h"

class UInputMappingContext;
class UInputAction;


UCLASS()
class STARFALL_API AStarfallCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:
	
	AStarfallCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;

	void PlayFireMontage(bool bAiming);
	void PlayReloadMontage();
	void PlayElimMontage();
	void PlayThrowGrenadeMontage();
	virtual void OnRep_ReplicatedMovement() override;

	void Elim();

	void Destroy();

	void SetUpPlayerInput();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim();

	
	UPROPERTY(EditAnywhere, Category = Combat)
	TSubclassOf<class AHumanElimAnimation> ElimVehicleToSpawn;

	UPROPERTY()
	EControllerInputState PlayerInputState;

	UPROPERTY()
	class AStarfallHUD* StarfallHUDRef;

	bool WasScorePressed = false;

	void UpdateMinimapMat();

	

	UPROPERTY()
	UMaterialInstanceDynamic* DynamicMinimapMaterialInstance;

	UPROPERTY(VisibleAnywhere)
	class UCanvasRenderTarget2D* MinimapRender;
	
	UPROPERTY(Replicated)
	bool bDisableGameplay = false;

	//Setting Ragdoll impact characteristics
	UPROPERTY()
	float ImpactImpulseForce;
	UPROPERTY()
	FVector ImpactDirection;
	UPROPERTY()
	FVector ImpactPoint;
	UPROPERTY()
	FName BoneImpactName;

	//Rocket characteristics

	UPROPERTY()
	float ExplosionRadius;

	UPROPERTY()
	FVector ExplosionPoint;

	UPROPERTY()
	float ExplosionForce;


	//RocketJumping

	UPROPERTY()
	bool bRocketForce = false;

	UPROPERTY()
	FVector	RocketForceDirection;


	UPROPERTY()
	class AStarfallPlayerController* ThisPlayerController;

	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool bShowScope);

	void UpdateHUDHealth();

	void SpawnDefaultWeapon();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputMappingContext* StarfallCharacterMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* QuickJumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* EquipAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* CrouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* AimAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* FireAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* ScoreAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* ReloadAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* GrenadeAction;


	UFUNCTION()
	void ClientCheckCurrentInputMode();

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	virtual void Jump() override;
	
	void Equip();
	void Crouch();
	void Aim();
	void AimEnd();
	void AimOffset(float DeltaTime);
	void CalculateAO_Pitch();
	void SimProxiesTurn();
	void FirePressed();
	void FireReleased();
	void ScorePressed();
	void ScoreReleased();
	void ReloadPressed();
	void GrenadePressed();
	void QuickJump();

	void RocketJump();



	void PlayHitReactMontage();

	UPROPERTY()
	UInputAction* PlayerActions;

	

	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);
	

	// Poll for any relevant classes and initialize our hud
	void PollInit();
	void RotateInPlace(float DeltaTime);

	void UpdateHUDAmmo();
private:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* CameraBoom;
	
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* OverheadWidget;

	/*
	* MiniMap
	*/
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* MinimapCameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USceneCaptureComponent2D* MinimapCaptureComponent2D;

	UPROPERTY(VisibleAnywhere)
	class UCanvasRenderTarget2D* MinimapRenderTarget;

	

	UPROPERTY(VisibleAnywhere)
	UMaterialInstanceDynamic* DynamicMinimapIconMaterialInstance;

	UPROPERTY(EditAnywhere)
	UMaterialInterface* MinimapIconMaterial;


	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	class AWeapon* OverlappingWeapon;

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCombatComponent* Combat;

	UPROPERTY(VisibleAnywhere)
	class UBuffComponent* Buff;

	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();

	UFUNCTION(Server, Reliable)
	void ServerQuickJumpPressed();

	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;

	ETurningInPlace TurningInPlace;
	void TurnInPlace(float DeltaTime);

	/*
	*	Animation Montages
	*/

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* HitReactMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* ElimMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* ReloadMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* ThrowGrenadeMontage;

	void HideCameraIfCharacterClose();

	UPROPERTY(EditAnywhere)
	float CameraThreshold = 200.f;

	bool bRotateRootBone;
	float TurnThreshold = 0.5f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeSinceLastMovementReplication;
	float CalculateSpeed();

	FTimerHandle HitReactHandle;

	void HitReactEnd();

	/*
	*Player Health
	*/

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats")
	float Health = 100.f;

	UFUNCTION()
	void OnRep_Health(float LastHealth);

	UPROPERTY()
	class AStarfallPlayerController* StarfallPlayerController;



	bool bElimmed = false;

	FTimerHandle ElimTimer;

	UPROPERTY(EditDefaultsOnly)
	float ElimDelay = 3.f;

	void ElimTimerFinished();

	/*
	* Dissolve effect
	*/

	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimeline;
	FOnTimelineFloat DissolveTrack;

	UPROPERTY(EditAnywhere)
	UCurveFloat* DissolveCurve;

	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);

	void StartDissolve();

	//Dynamic Instance that we can change at runtime
	UPROPERTY(VisibleAnywhere, Category = Elim)
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;

	//Material instance set on the Blueprint, used with the dynamic material instance
	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* DissolveMaterialInstance;

	UPROPERTY()
	class AStarfallPlayerState* StarfallPlayerState;

	virtual void PossessedBy(AController* NewController) override;
	
	/*
	* Grenade
	*/

	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* AttachedGrenade;

	/*
	* Default weapon
	*/

	UPROPERTY(EditAnywhere)
	TSubclassOf<AWeapon> DefaultWeaponClass;

public:	
	
	void SetOverlappingWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped();
	bool IsAiming();
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw;  }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	AWeapon* GetEquippedWeapon();
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FVector GetHitTarget() const;
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE bool IsElimmed() const { return bElimmed; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE void SetHealth(float Amount) { Health = Amount; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	ECombatState GetCombatState() const;
	FORCEINLINE UCombatComponent* GetCombat() const { return Combat; }
	FORCEINLINE bool GetDisabledGameplay() const { return bDisableGameplay; }
	FORCEINLINE UStaticMeshComponent* GetAttachedGrenade() const { return AttachedGrenade; }
	FORCEINLINE UBuffComponent* GetBuff() const { return Buff; }
};
