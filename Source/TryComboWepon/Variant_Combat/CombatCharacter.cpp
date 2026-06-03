// Copyright Epic Games, Inc. All Rights Reserved.


#include "CombatCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "CombatLifeBar.h"
#include "Engine/DamageEvents.h"
#include "TimerManager.h"
#include "Engine/LocalPlayer.h"
#include "CombatPlayerController.h"
#include "UObject/ConstructorHelpers.h"

ACombatCharacter::ACombatCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// bind the attack montage ended delegate
	OnAttackMontageEnded.BindUObject(this, &ACombatCharacter::AttackMontageEnded);
	OnWeaponModeMontageEnded.BindUObject(this, &ACombatCharacter::WeaponModeMontageEnded);

	static ConstructorHelpers::FObjectFinder<UInputAction> JumpActionAsset(TEXT("/Game/Input/Actions/IA_Jump.IA_Jump"));
	if (JumpActionAsset.Succeeded())
	{
		JumpAction = JumpActionAsset.Object;
	}

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(35.0f, 90.0f);

	// Configure character movement
	GetCharacterMovement()->MaxWalkSpeed = SheathedMaxWalkSpeed;

	// create the camera boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);

	CameraBoom->TargetArmLength = DefaultCameraDistance;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->bEnableCameraRotationLag = true;

	// create the orbiting camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// create the life bar widget component
	LifeBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("LifeBar"));
	LifeBar->SetupAttachment(RootComponent);

	// set the player tag
	Tags.Add(FName("Player"));
}

void ACombatCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void ACombatCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void ACombatCharacter::ComboAttackPressed()
{
	// route the input
	DoComboAttackStart();
}

void ACombatCharacter::ChargedAttackPressed()
{
	// route the input
	DoChargedAttackStart();
}

void ACombatCharacter::ChargedAttackReleased()
{
	// route the input
	DoChargedAttackEnd();
}

void ACombatCharacter::ToggleCamera()
{
	// call the BP hook
	BP_ToggleCamera();
}

void ACombatCharacter::ToggleWeaponModePressed()
{
	ToggleWeaponMode();
}

void ACombatCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void ACombatCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ACombatCharacter::DoComboAttackStart()
{
	if (!bIsWeaponDrawn || bIsWeaponModeChanging)
	{
		CachedAttackInputTime = GetWorld()->GetTimeSeconds();
		bQueuedWeaponModeChargedAttack = false;

		if (bAutoDrawWeaponOnAttack && !bIsWeaponDrawn)
		{
			SetWeaponDrawn(true);
		}

		return;
	}

	// are we already playing an attack animation?
	if (bIsAttacking)
	{
		// cache the input time so we can check it later
		CachedAttackInputTime = GetWorld()->GetTimeSeconds();
		bComboInputQueued = true;

		return;
	}

	// perform a combo attack
	ComboAttack();
}

void ACombatCharacter::DoComboAttackEnd()
{
	// stub
}

// ==================== Codex新增：动态绑定连招Montage ====================
void ACombatCharacter::SetComboAttackMontage(UAnimMontage* NewMontage)
{
	ComboAttackMontage = NewMontage;
}

void ACombatCharacter::SetCurrentWeaponType(ECombatWeaponType NewWeaponType)
{
	CurrentWeaponType = NewWeaponType;

	if (CurrentWeaponType == ECombatWeaponType::Unarmed)
	{
		bIsWeaponDrawn = false;
		bPendingWeaponDrawn = false;
		bIsWeaponModeChanging = false;
	}

	ApplyWeaponMovementState();
}

// ==================== Codex新增：持剑/收刀模式切换入口 ====================
void ACombatCharacter::SetWeaponDrawn(bool bNewWeaponDrawn)
{
	if (bNewWeaponDrawn && CurrentWeaponType == ECombatWeaponType::Unarmed)
	{
		CurrentWeaponType = DefaultWeaponType;
	}

	if (bNewWeaponDrawn && CurrentWeaponType == ECombatWeaponType::Unarmed)
	{
		return;
	}

	if (bIsWeaponModeChanging || bIsWeaponDrawn == bNewWeaponDrawn)
	{
		return;
	}

	PlayWeaponModeMontage(bNewWeaponDrawn);
}

// ==================== Codex新增：持剑/收刀模式取反 ====================
void ACombatCharacter::ToggleWeaponMode()
{
	SetWeaponDrawn(!bIsWeaponDrawn);
}

// ==================== Codex新增：动态绑定拔刀/收刀Montage ====================
void ACombatCharacter::SetWeaponModeMontages(UAnimMontage* NewDrawMontage, UAnimMontage* NewSheatheMontage)
{
	DrawWeaponMontage = NewDrawMontage;
	SheatheWeaponMontage = NewSheatheMontage;
}

float ACombatCharacter::GetGroundSpeed() const
{
	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		return 0.0f;
	}

	return FVector(MovementComponent->Velocity.X, MovementComponent->Velocity.Y, 0.0f).Size();
}

bool ACombatCharacter::IsMovingOnGround() const
{
	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	return MovementComponent && MovementComponent->IsMovingOnGround();
}

void ACombatCharacter::DoChargedAttackStart()
{
	// raise the charging attack flag
	bIsChargingAttack = true;

	if (!bIsWeaponDrawn || bIsWeaponModeChanging)
	{
		CachedAttackInputTime = GetWorld()->GetTimeSeconds();
		bQueuedWeaponModeChargedAttack = true;

		if (bAutoDrawWeaponOnAttack && !bIsWeaponDrawn)
		{
			SetWeaponDrawn(true);
		}

		return;
	}

	if (bIsAttacking)
	{
		// cache the input time so we can check it later
		CachedAttackInputTime = GetWorld()->GetTimeSeconds();

		return;
	}

	ChargedAttack();
}

void ACombatCharacter::DoChargedAttackEnd()
{
	// lower the charging attack flag
	bIsChargingAttack = false;

	// if we've done the charge loop at least once, release the charged attack right away
	if (bHasLoopedChargedAttack)
	{
		CheckChargedAttack();
	}
}

void ACombatCharacter::ResetHP()
{
	// reset the current HP total
	CurrentHP = MaxHP;

	// update the life bar
	LifeBarWidget->SetLifePercentage(1.0f);
}

void ACombatCharacter::ComboAttack()
{
	if (!ComboAttackMontage || !bIsWeaponDrawn || bIsWeaponModeChanging)
	{
		return;
	}

	bPendingComboSheathe = false;

	// raise the attacking flag
	bIsAttacking = true;

	// reset the combo count
	ComboCount = 0;
	bComboInputQueued = false;

	// notify enemies they are about to be attacked
	NotifyEnemiesOfIncomingAttack();

	// play the attack montage
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		const float MontageLength = AnimInstance->Montage_Play(ComboAttackMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);

		// subscribe to montage completed and interrupted events
		if (MontageLength > 0.0f)
		{
			// set the end delegate for the montage
			AnimInstance->Montage_SetEndDelegate(OnAttackMontageEnded, ComboAttackMontage);

			if (ComboSectionNames.IsValidIndex(ComboCount) && ComboSectionNames[ComboCount] != NAME_None)
			{
				AnimInstance->Montage_JumpToSection(ComboSectionNames[ComboCount], ComboAttackMontage);
			}
		}
	}

}

// ==================== Codex新增：播放拔刀/收刀Montage ====================
void ACombatCharacter::PlayWeaponModeMontage(bool bDrawWeapon)
{
	bPendingWeaponDrawn = bDrawWeapon;
	bIsWeaponModeChanging = true;

	UAnimMontage* WeaponModeMontage = bDrawWeapon ? DrawWeaponMontage : SheatheWeaponMontage;
	if (!WeaponModeMontage)
	{
		FinishWeaponModeTransition(false);
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		const float MontageLength = AnimInstance->Montage_Play(WeaponModeMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);
		if (MontageLength > 0.0f)
		{
			AnimInstance->Montage_SetEndDelegate(OnWeaponModeMontageEnded, WeaponModeMontage);
			return;
		}
	}

	FinishWeaponModeTransition(true);
}

// ==================== Codex新增：拔刀/收刀结束后统一设置状态 ====================
void ACombatCharacter::FinishWeaponModeTransition(bool bInterrupted)
{
	if (!bInterrupted)
	{
		bIsWeaponDrawn = bPendingWeaponDrawn;
		ApplyWeaponMovementState();
	}

	bIsWeaponModeChanging = false;

	if (bIsWeaponDrawn && CachedAttackInputTime > 0.0f && GetWorld()->GetTimeSeconds() - CachedAttackInputTime <= AttackInputCacheTimeTolerance)
	{
		CachedAttackInputTime = 0.0f;

		if (bQueuedWeaponModeChargedAttack)
		{
			bQueuedWeaponModeChargedAttack = false;
			ChargedAttack();
		}
		else
		{
			ComboAttack();
		}
	}
}

// ==================== Codex新增：拔刀/收刀Montage结束回调 ====================
void ACombatCharacter::WeaponModeMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	FinishWeaponModeTransition(bInterrupted);
}

// ==================== Codex新增：跳转到当前段收刀Section ====================
bool ACombatCharacter::TryJumpToComboSheatheSection()
{
	if (!CanAutoSheatheFromCombo() || !ComboAttackMontage || !ComboSheatheSectionNames.IsValidIndex(ComboCount))
	{
		return false;
	}

	const FName SheatheSectionName = ComboSheatheSectionNames[ComboCount];
	if (SheatheSectionName == NAME_None)
	{
		return false;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_JumpToSection(SheatheSectionName, ComboAttackMontage);
		bPendingComboSheathe = true;
		return true;
	}

	return false;
}

void ACombatCharacter::ChargedAttack()
{
	// raise the attacking flag
	bIsAttacking = true;

	// reset the charge loop flag
	bHasLoopedChargedAttack = false;

	// notify enemies they are about to be attacked
	NotifyEnemiesOfIncomingAttack();

	// play the charged attack montage
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		const float MontageLength = AnimInstance->Montage_Play(ChargedAttackMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);

		// subscribe to montage completed and interrupted events
		if (MontageLength > 0.0f)
		{
			// set the end delegate for the montage
			AnimInstance->Montage_SetEndDelegate(OnAttackMontageEnded, ChargedAttackMontage);
		}
	}
}

void ACombatCharacter::AttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// reset the attacking flag
	bIsAttacking = false;

	if (Montage == ComboAttackMontage && bPendingComboSheathe)
	{
		if (!bInterrupted)
		{
			bIsWeaponDrawn = false;
			ApplyWeaponMovementState();
		}

		bPendingComboSheathe = false;
	}

	// check if we have a non-stale cached input
	if (CachedAttackInputTime > 0.0f && GetWorld()->GetTimeSeconds() - CachedAttackInputTime <= AttackInputCacheTimeTolerance)
	{
		// are we holding the charged attack button?
		if (bIsChargingAttack)
		{
			// do a charged attack
			ChargedAttack();
		}
		else
		{
			// do a regular attack
			ComboAttack();
		}
	}
}

void ACombatCharacter::DoAttackTrace(FName DamageSourceBone)
{
	// sweep for objects in front of the character to be hit by the attack
	TArray<FHitResult> OutHits;

	// start at the provided socket location, sweep forward
	const FVector TraceStart = GetMesh()->GetSocketLocation(DamageSourceBone);
	const FVector TraceEnd = TraceStart + (GetActorForwardVector() * MeleeTraceDistance);

	// check for pawn and world dynamic collision object types
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	// use a sphere shape for the sweep
	FCollisionShape CollisionShape;
	CollisionShape.SetSphere(MeleeTraceRadius);

	// ignore self
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (GetWorld()->SweepMultiByObjectType(OutHits, TraceStart, TraceEnd, FQuat::Identity, ObjectParams, CollisionShape, QueryParams))
	{
		// iterate over each object hit
		for (const FHitResult& CurrentHit : OutHits)
		{
			// check if we've hit a damageable actor
			ICombatDamageable* Damageable = Cast<ICombatDamageable>(CurrentHit.GetActor());

			if (Damageable)
			{
				// knock upwards and away from the impact normal
				const FVector Impulse = (CurrentHit.ImpactNormal * -MeleeKnockbackImpulse) + (FVector::UpVector * MeleeLaunchImpulse);

				// pass the damage event to the actor
				Damageable->ApplyDamage(MeleeDamage, this, CurrentHit.ImpactPoint, Impulse);

				// call the BP handler to play effects, etc.
				DealtDamage(MeleeDamage, CurrentHit.ImpactPoint);
			}
		}
	}
}

void ACombatCharacter::CheckCombo()
{
	CheckComboOrSheatheSection();
}

// ==================== Codex新增：连招继续/收刀判断核心逻辑 ====================
void ACombatCharacter::CheckComboOrSheatheSection()
{
	if (!bIsAttacking || bIsChargingAttack)
	{
		return;
	}

	const bool bHasCachedComboInput = CachedAttackInputTime > 0.0f && GetWorld()->GetTimeSeconds() - CachedAttackInputTime <= ComboInputCacheTimeTolerance;
	if (bComboInputQueued || bHasCachedComboInput)
	{
		CachedAttackInputTime = 0.0f;
		bComboInputQueued = false;
		++ComboCount;

		if (ComboCount < ComboSectionNames.Num())
		{
			NotifyEnemiesOfIncomingAttack();

			if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
			{
				AnimInstance->Montage_JumpToSection(ComboSectionNames[ComboCount], ComboAttackMontage);
			}
		}

		return;
	}

	CachedAttackInputTime = 0.0f;
	bComboInputQueued = false;
	bPendingComboSheathe = false;
	TryJumpToComboSheatheSection();
}

void ACombatCharacter::ApplyWeaponMovementState()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		return;
	}

	MovementComponent->MaxWalkSpeed = bIsWeaponDrawn ? WeaponDrawnMaxWalkSpeed : SheathedMaxWalkSpeed;
	MovementComponent->MaxAcceleration = bIsWeaponDrawn ? WeaponDrawnMaxAcceleration : SheathedMaxAcceleration;
	MovementComponent->BrakingDecelerationWalking = bIsWeaponDrawn ? WeaponDrawnBrakingDeceleration : SheathedBrakingDeceleration;
}

bool ACombatCharacter::CanAutoSheatheFromCombo() const
{
	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent || !MovementComponent->IsMovingOnGround())
	{
		return false;
	}

	const float GroundSpeed = FVector(MovementComponent->Velocity.X, MovementComponent->Velocity.Y, 0.0f).Size();
	return GroundSpeed <= AutoSheatheMaxGroundSpeed;
}

void ACombatCharacter::CheckChargedAttack()
{
	// raise the looped charged attack flag
	bHasLoopedChargedAttack = true;

	// jump to either the loop or the attack section depending on whether we're still holding the charge button
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_JumpToSection(bIsChargingAttack ? ChargeLoopSection : ChargeAttackSection, ChargedAttackMontage);
	}
}

void ACombatCharacter::NotifyEnemiesOfIncomingAttack()
{
	// sweep for objects in front of the character to be hit by the attack
	TArray<FHitResult> OutHits;

	// start at the actor location, sweep forward
	const FVector TraceStart = GetActorLocation();
	const FVector TraceEnd = TraceStart + (GetActorForwardVector() * DangerTraceDistance);

	// check for pawn object types only
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	// use a sphere shape for the sweep
	FCollisionShape CollisionShape;
	CollisionShape.SetSphere(DangerTraceRadius);

	// ignore self
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (GetWorld()->SweepMultiByObjectType(OutHits, TraceStart, TraceEnd, FQuat::Identity, ObjectParams, CollisionShape, QueryParams))
	{
		// iterate over each object hit
		for (const FHitResult& CurrentHit : OutHits)
		{
			// check if we've hit a damageable actor
			ICombatDamageable* Damageable = Cast<ICombatDamageable>(CurrentHit.GetActor());

			if (Damageable)
			{
				// notify the enemy
				Damageable->NotifyDanger(GetActorLocation(), this);
			}
		}
	}
}

void ACombatCharacter::ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse)
{
	// pass the damage event to the actor
	FDamageEvent DamageEvent;
	const float ActualDamage = TakeDamage(Damage, DamageEvent, nullptr, DamageCauser);

	// only process knockback and effects if we received nonzero damage
	if (ActualDamage > 0.0f)
	{
		// apply the knockback impulse
		GetCharacterMovement()->AddImpulse(DamageImpulse, true);

		// is the character ragdolling?
		if (GetMesh()->IsSimulatingPhysics())
		{
			// apply an impulse to the ragdoll
			GetMesh()->AddImpulseAtLocation(DamageImpulse * GetMesh()->GetMass(), DamageLocation);
		}

		// pass control to BP to play effects, etc.
		ReceivedDamage(ActualDamage, DamageLocation, DamageImpulse.GetSafeNormal());
	}

}

void ACombatCharacter::HandleDeath()
{
	// disable movement while we're dead
	GetCharacterMovement()->DisableMovement();

	// enable full ragdoll physics
	GetMesh()->SetSimulatePhysics(true);

	// hide the life bar
	LifeBar->SetHiddenInGame(true);

	// pull back the camera
	GetCameraBoom()->TargetArmLength = DeathCameraDistance;

	// schedule respawning
	GetWorld()->GetTimerManager().SetTimer(RespawnTimer, this, &ACombatCharacter::RespawnCharacter, RespawnTime, false);
}

void ACombatCharacter::ApplyHealing(float Healing, AActor* Healer)
{
	// stub
}

void ACombatCharacter::NotifyDanger(const FVector& DangerLocation, AActor* DangerSource)
{
	// stub
}

void ACombatCharacter::RespawnCharacter()
{
	// destroy the character and let it be respawned by the Player Controller
	Destroy();
}

float ACombatCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// only process damage if the character is still alive
	if (CurrentHP <= 0.0f)
	{
		return 0.0f;
	}

	// reduce the current HP
	CurrentHP -= Damage;

	// have we run out of HP?
	if (CurrentHP <= 0.0f)
	{
		// die
		HandleDeath();
	}
	else
	{
		// update the life bar
		LifeBarWidget->SetLifePercentage(CurrentHP / MaxHP);

		// enable partial ragdoll physics, but keep the pelvis vertical
		GetMesh()->SetPhysicsBlendWeight(0.5f);
		GetMesh()->SetBodySimulatePhysics(PelvisBoneName, false);
	}

	// return the received damage amount
	return Damage;
}

void ACombatCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	// is the character still alive?
	if (CurrentHP >= 0.0f)
	{
		// disable ragdoll physics
		GetMesh()->SetPhysicsBlendWeight(0.0f);
	}
}

void ACombatCharacter::BeginPlay()
{
	Super::BeginPlay();

	// get the life bar from the widget component
	LifeBarWidget = Cast<UCombatLifeBar>(LifeBar->GetUserWidgetObject());
	check(LifeBarWidget);

	// initialize the camera
	GetCameraBoom()->TargetArmLength = DefaultCameraDistance;

	// save the relative transform for the mesh so we can reset the ragdoll later
	MeshStartingTransform = GetMesh()->GetRelativeTransform();

	// set the life bar color
	LifeBarWidget->SetBarColor(LifeBarColor);

	// reset HP to maximum
	ResetHP();

	CurrentWeaponType = DefaultWeaponType;
	bIsWeaponDrawn = bStartWithWeaponDrawn && CurrentWeaponType != ECombatWeaponType::Unarmed;
	bPendingWeaponDrawn = bIsWeaponDrawn;
	bIsWeaponModeChanging = false;
	ApplyWeaponMovementState();
}

void ACombatCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// clear the respawn timer
	GetWorld()->GetTimerManager().ClearTimer(RespawnTimer);
}

void ACombatCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACombatCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACombatCharacter::Look);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ACombatCharacter::Look);

		// Combo Attack
		if (ComboAttackAction)
		{
			EnhancedInputComponent->BindAction(ComboAttackAction, ETriggerEvent::Started, this, &ACombatCharacter::ComboAttackPressed);
		}

		// Charged Attack
		EnhancedInputComponent->BindAction(ChargedAttackAction, ETriggerEvent::Started, this, &ACombatCharacter::ChargedAttackPressed);
		EnhancedInputComponent->BindAction(ChargedAttackAction, ETriggerEvent::Completed, this, &ACombatCharacter::ChargedAttackReleased);

		// Camera Side Toggle
		EnhancedInputComponent->BindAction(ToggleCameraAction, ETriggerEvent::Triggered, this, &ACombatCharacter::ToggleCamera);

		// Weapon Mode Toggle
		if (ToggleWeaponModeAction)
		{
			EnhancedInputComponent->BindAction(ToggleWeaponModeAction, ETriggerEvent::Started, this, &ACombatCharacter::ToggleWeaponModePressed);
		}
	}

	if (ToggleWeaponModeKey.IsValid())
	{
		PlayerInputComponent->BindKey(ToggleWeaponModeKey, IE_Pressed, this, &ACombatCharacter::ToggleWeaponModePressed);
	}

	if (ComboAttackKey.IsValid())
	{
		PlayerInputComponent->BindKey(ComboAttackKey, IE_Pressed, this, &ACombatCharacter::ComboAttackPressed);
	}
}

void ACombatCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// update the respawn transform on the Player Controller
	if (ACombatPlayerController* PC = Cast<ACombatPlayerController>(GetController()))
	{
		PC->SetRespawnTransform(GetActorTransform());
	}
}

