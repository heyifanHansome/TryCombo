// Copyright Epic Games, Inc. All Rights Reserved.


#include "CombatCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "CombatLifeBar.h"
#include "Engine/DamageEvents.h"
#include "TimerManager.h"
#include "Engine/LocalPlayer.h"
#include "CombatPlayerController.h"
#include "Components/CombatWeaponCollisionComponent.h"
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

	static ConstructorHelpers::FObjectFinder<UInputAction> MoveActionAsset(TEXT("/Game/Input/Actions/IA_Move.IA_Move"));
	if (MoveActionAsset.Succeeded())
	{
		MoveAction = MoveActionAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> LookActionAsset(TEXT("/Game/Input/Actions/IA_Look.IA_Look"));
	if (LookActionAsset.Succeeded())
	{
		LookAction = LookActionAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> MouseLookActionAsset(TEXT("/Game/Input/Actions/IA_MouseLook.IA_MouseLook"));
	if (MouseLookActionAsset.Succeeded())
	{
		MouseLookAction = MouseLookActionAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> ComboAttackActionAsset(TEXT("/Game/Variant_Combat/Input/Actions/IA_ComboAttack.IA_ComboAttack"));
	if (ComboAttackActionAsset.Succeeded())
	{
		ComboAttackAction = ComboAttackActionAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> ChargedAttackActionAsset(TEXT("/Game/Variant_Combat/Input/Actions/IA_ChargedAttack.IA_ChargedAttack"));
	if (ChargedAttackActionAsset.Succeeded())
	{
		ChargedAttackAction = ChargedAttackActionAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> ToggleCameraActionAsset(TEXT("/Game/Variant_Combat/Input/Actions/IA_ToggleCameraSide.IA_ToggleCameraSide"));
	if (ToggleCameraActionAsset.Succeeded())
	{
		ToggleCameraAction = ToggleCameraActionAsset.Object;
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
	LifeBar->SetVisibility(false);
	LifeBar->SetHiddenInGame(true);

	WeaponCollision = CreateDefaultSubobject<UCombatWeaponCollisionComponent>(TEXT("WeaponCollision"));

	WeaponRoot = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponRoot"));
	WeaponRoot->SetupAttachment(GetMesh(), SheathedWeaponAttachSocketName);

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
	bComboAttackHeld = true;

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
		if (bPendingComboSheathe)
		{
			return;
		}

		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			if (ComboAttackMontage && AnimInstance->Montage_IsPlaying(ComboAttackMontage))
			{
				QueueComboInputIfAllowed();
			}
		}

		return;
	}

	// perform a combo attack
	ComboAttack();
}

void ACombatCharacter::DoComboAttackEnd()
{
	bComboAttackHeld = false;
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

void ACombatCharacter::BeginComboInputWindow(float WindowDuration)
{
	if (!bIsAttacking || bIsChargingAttack || bPendingComboSheathe)
	{
		return;
	}

	bComboInputQueued = false;
	bComboInputWindowOpen = true;
	bComboInputWindowConsumed = false;
	ComboInputWindowElapsedTime = 0.0f;
	ComboInputWindowDuration = FMath::Max(0.0f, WindowDuration);
	ComboInputWindowAlpha = 0.0f;

	UE_LOG(LogTemp, Warning, TEXT("ComboWindow Begin: Combo=%d Duration=%.3f Held=%d"), ComboCount, ComboInputWindowDuration, IsComboAttackInputHeld() ? 1 : 0);

}

void ACombatCharacter::TickComboInputWindow(float DeltaSeconds, float WindowDuration)
{
	if (!bComboInputWindowOpen)
	{
		return;
	}

	ComboInputWindowDuration = FMath::Max(ComboInputWindowDuration, WindowDuration);
	ComboInputWindowElapsedTime = FMath::Max(0.0f, ComboInputWindowElapsedTime + DeltaSeconds);

	if (ComboInputWindowDuration > 0.0f)
	{
		ComboInputWindowAlpha = FMath::Clamp(ComboInputWindowElapsedTime / ComboInputWindowDuration, 0.0f, 1.0f);
	}

}

void ACombatCharacter::EndComboInputWindow(FName NoInputSectionName)
{
	bComboInputWindowOpen = false;
	ComboInputWindowAlpha = ComboInputWindowDuration > 0.0f ? 1.0f : ComboInputWindowAlpha;

	UE_LOG(LogTemp, Warning, TEXT("ComboWindow End: Combo=%d Queued=%d Held=%d"), ComboCount, bComboInputQueued ? 1 : 0, IsComboAttackInputHeld() ? 1 : 0);

	if (bCheckComboOnInputWindowEnd)
	{
		CheckComboOrSheatheSection(NoInputSectionName);
	}
}

void ACombatCharacter::EndComboMontage(float BlendOutTime)
{
	if (WeaponCollision)
	{
		WeaponCollision->EndCollisionWindow();
	}

	bComboInputQueued = false;
	bComboInputWindowOpen = false;
	bComboInputWindowConsumed = false;
	ComboInputWindowElapsedTime = 0.0f;
	ComboInputWindowDuration = 0.0f;
	ComboInputWindowAlpha = 0.0f;
	CachedAttackInputTime = 0.0f;
	bPendingComboSheathe = false;

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		if (ComboAttackMontage && AnimInstance->Montage_IsPlaying(ComboAttackMontage))
		{
			AnimInstance->Montage_Stop(BlendOutTime, ComboAttackMontage);
			return;
		}
	}

	bIsAttacking = false;
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
	CachedAttackInputTime = 0.0f;

	// raise the attacking flag
	bIsAttacking = true;

	if (WeaponCollision)
	{
		WeaponCollision->ResetHitActors();
	}

	// reset the combo count
	ComboCount = 0;
	bComboInputQueued = false;
	bComboInputWindowOpen = false;
	bComboInputWindowConsumed = false;
	ComboInputWindowElapsedTime = 0.0f;
	ComboInputWindowDuration = 0.0f;
	ComboInputWindowAlpha = 0.0f;

	// notify enemies they are about to be attacked
	NotifyEnemiesOfIncomingAttack();

	// play the attack montage
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		const float MontageLength = AnimInstance->Montage_Play(ComboAttackMontage, ComboAttackMontagePlayRate, EMontagePlayReturnType::MontageLength, 0.0f, true);

		// subscribe to montage completed and interrupted events
		if (MontageLength > 0.0f)
		{
			// set the end delegate for the montage
			AnimInstance->Montage_SetEndDelegate(OnAttackMontageEnded, ComboAttackMontage);
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
		const float MontageLength = AnimInstance->Montage_Play(WeaponModeMontage, WeaponModeMontagePlayRate, EMontagePlayReturnType::MontageLength, 0.0f, true);
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
		UpdateWeaponAttachment();
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
bool ACombatCharacter::TryJumpToComboSheatheSection(FName OverrideSectionName)
{
	if (!ComboAttackMontage)
	{
		return false;
	}

	const FName SheatheSectionName = OverrideSectionName != NAME_None
		? OverrideSectionName
		: (ComboSheatheSectionNames.IsValidIndex(ComboCount) ? ComboSheatheSectionNames[ComboCount] : NAME_None);
	if (SheatheSectionName == NAME_None)
	{
		return false;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		bComboInputQueued = false;
		CachedAttackInputTime = 0.0f;
		bPendingComboSheathe = true;
		AnimInstance->Montage_SetNextSection(SheatheSectionName, NAME_None, ComboAttackMontage);
		AnimInstance->Montage_JumpToSection(SheatheSectionName, ComboAttackMontage);
		return true;
	}

	return false;
}

void ACombatCharacter::ChargedAttack()
{
	// raise the attacking flag
	bIsAttacking = true;

	if (WeaponCollision)
	{
		WeaponCollision->ResetHitActors();
	}

	// reset the charge loop flag
	bHasLoopedChargedAttack = false;

	// notify enemies they are about to be attacked
	NotifyEnemiesOfIncomingAttack();

	// play the charged attack montage
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		const float MontageLength = AnimInstance->Montage_Play(ChargedAttackMontage, ChargedAttackMontagePlayRate, EMontagePlayReturnType::MontageLength, 0.0f, true);

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
	if (WeaponCollision)
	{
		WeaponCollision->EndCollisionWindow();
	}

	const bool bWasComboMontage = Montage == ComboAttackMontage;
	const bool bWasComboSheathing = bWasComboMontage && bPendingComboSheathe;

	// reset the attacking flag
	bIsAttacking = false;

	if (bWasComboMontage)
	{
		bComboInputQueued = false;
		bComboInputWindowOpen = false;
		bComboInputWindowConsumed = false;
		ComboInputWindowElapsedTime = 0.0f;
		ComboInputWindowDuration = 0.0f;
		ComboInputWindowAlpha = 0.0f;
		CachedAttackInputTime = 0.0f;
		bPendingComboSheathe = false;
	}

	if (bWasComboSheathing)
	{
		if (!bInterrupted)
		{
			bIsWeaponDrawn = false;
			ApplyWeaponMovementState();
			UpdateWeaponAttachment();
		}
	}

	if (bWasComboMontage)
	{
		return;
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
	if (WeaponCollision)
	{
		WeaponCollision->PerformAttackTrace(GetMesh(), GetActiveWeaponCollisionId(), DamageSourceBone);
	}
}

void ACombatCharacter::CheckCombo()
{
	CheckComboOrSheatheSection();
}

// ==================== Codex新增：连招继续/收刀判断核心逻辑 ====================
void ACombatCharacter::CheckComboOrSheatheSection(FName OverrideSheatheSectionName)
{
	if (!bIsAttacking || bIsChargingAttack || bPendingComboSheathe)
	{
		UE_LOG(LogTemp, Warning, TEXT("ComboWindow Check skipped: Attacking=%d Charging=%d PendingSheathe=%d"), bIsAttacking ? 1 : 0, bIsChargingAttack ? 1 : 0, bPendingComboSheathe ? 1 : 0);
		return;
	}

	const bool bShouldContinueCombo = bComboInputQueued;
	CachedAttackInputTime = 0.0f;
	bComboInputQueued = false;
	bComboInputWindowOpen = false;
	bComboInputWindowConsumed = false;
	ComboInputWindowElapsedTime = 0.0f;
	ComboInputWindowDuration = 0.0f;
	ComboInputWindowAlpha = 0.0f;

	const int32 NextComboIndex = ComboCount + 1;
	if (bShouldContinueCombo)
	{
		ComboCount = NextComboIndex;

		NotifyEnemiesOfIncomingAttack();

		if (WeaponCollision)
		{
			WeaponCollision->ResetHitActors();
		}

		UE_LOG(LogTemp, Warning, TEXT("ComboWindow ContinueNaturally: Index=%d"), ComboCount);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("ComboWindow Sheathe: Combo=%d ShouldContinue=%d NextIndex=%d HasSheathe=%d Override=%s"),
		ComboCount,
		bShouldContinueCombo ? 1 : 0,
		NextComboIndex,
		ComboSheatheSectionNames.IsValidIndex(ComboCount) ? 1 : 0,
		*OverrideSheatheSectionName.ToString());

	if (!TryJumpToComboSheatheSection(OverrideSheatheSectionName))
	{
		UE_LOG(LogTemp, Warning, TEXT("ComboWindow StopNoInput: Combo=%d"), ComboCount);
		EndComboMontage();
	}
}

void ACombatCharacter::QueueComboInputIfAllowed()
{
	const bool bCanAcceptComboInput = !bRequireComboInputWindow || bComboInputWindowOpen;
	if (!bCanAcceptComboInput || bComboInputWindowConsumed)
	{
		return;
	}

	bComboInputQueued = true;
	bComboInputWindowConsumed = true;
	CachedAttackInputTime = 0.0f;
	UE_LOG(LogTemp, Warning, TEXT("ComboWindow Queued: Combo=%d"), ComboCount);
}

bool ACombatCharacter::IsComboAttackInputHeld() const
{
	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (ComboAttackKey.IsValid() && PlayerController->IsInputKeyDown(ComboAttackKey))
		{
			return true;
		}
	}

	return bComboAttackHeld;
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

void ACombatCharacter::ApplyCameraDebugSettings()
{
	if (!CameraBoom)
	{
		return;
	}

	if (bUseFrontDebugCamera)
	{
		CameraBoom->bUsePawnControlRotation = false;
		CameraBoom->TargetArmLength = FrontDebugCameraDistance;
		CameraBoom->SetRelativeRotation(FRotator(FrontDebugCameraPitch, FrontDebugCameraYaw, 0.0f));
		return;
	}

	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->TargetArmLength = DefaultCameraDistance;
	CameraBoom->SetRelativeRotation(FRotator::ZeroRotator);
}

void ACombatCharacter::UpdateWeaponAttachment()
{
	AttachWeaponToSocket(ShouldAttachWeaponAsDrawn() ? DrawnWeaponAttachSocketName : SheathedWeaponAttachSocketName);
}

bool ACombatCharacter::ShouldAttachWeaponAsDrawn() const
{
	const UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return bStartWithWeaponDrawn;
	}

	return bIsWeaponDrawn;
}

void ACombatCharacter::AttachWeaponToDrawnSocket()
{
	AttachWeaponToSocket(DrawnWeaponAttachSocketName);
}

void ACombatCharacter::AttachWeaponToSheathedSocket()
{
	AttachWeaponToSocket(SheathedWeaponAttachSocketName);
}

void ACombatCharacter::AttachWeaponToSocket(FName SocketName)
{
	if (SocketName == NAME_None)
	{
		return;
	}

	if (WeaponRoot)
	{
		WeaponRoot->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
	}

	AttachNamedWeaponComponents(SocketName);
}

void ACombatCharacter::AttachComponentToWeaponSocket(USceneComponent* ComponentToAttach, FName SocketName)
{
	if (!ComponentToAttach || !GetMesh() || SocketName == NAME_None)
	{
		return;
	}

	if (ComponentToAttach == GetMesh())
	{
		return;
	}

	ComponentToAttach->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, SocketName);
}

void ACombatCharacter::AttachNamedWeaponComponents(FName SocketName)
{
	TArray<USceneComponent*> SceneComponents;
	GetComponents<USceneComponent>(SceneComponents);

	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (!SceneComponent)
		{
			continue;
		}

		const FName ComponentName = SceneComponent->GetFName();
		if (ComponentName == DefaultWeaponHitBoxComponentName || ComponentName == DefaultWeaponMeshComponentName)
		{
			AttachComponentToWeaponSocket(SceneComponent, SocketName);
		}
		else if (ComponentName == DefaultWeaponSheathMeshComponentName)
		{
			AttachComponentToWeaponSocket(SceneComponent, SheathedWeaponAttachSocketName);
		}
	}
}

void ACombatCharacter::AutoFitDefaultWeaponHitBox()
{
	if (!bAutoFitDefaultWeaponHitBoxToWeaponMesh || DefaultWeaponMeshComponentName == NAME_None || DefaultWeaponHitBoxComponentName == NAME_None)
	{
		return;
	}

	USceneComponent* WeaponMeshComponent = nullptr;
	UBoxComponent* HitBoxComponent = nullptr;

	TArray<USceneComponent*> SceneComponents;
	GetComponents<USceneComponent>(SceneComponents);
	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (!SceneComponent)
		{
			continue;
		}

		if (SceneComponent->GetFName() == DefaultWeaponMeshComponentName)
		{
			WeaponMeshComponent = SceneComponent;
		}
		else if (SceneComponent->GetFName() == DefaultWeaponHitBoxComponentName)
		{
			HitBoxComponent = Cast<UBoxComponent>(SceneComponent);
		}
	}

	if (!WeaponMeshComponent || !HitBoxComponent)
	{
		return;
	}

	FVector LocalMin = FVector::ZeroVector;
	FVector LocalMax = FVector::ZeroVector;
	if (UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(WeaponMeshComponent))
	{
		StaticMeshComponent->GetLocalBounds(LocalMin, LocalMax);
	}
	else if (USkinnedMeshComponent* SkinnedMeshComponent = Cast<USkinnedMeshComponent>(WeaponMeshComponent))
	{
		const FBoxSphereBounds LocalBounds = SkinnedMeshComponent->GetLocalBounds();
		LocalMin = LocalBounds.Origin - LocalBounds.BoxExtent;
		LocalMax = LocalBounds.Origin + LocalBounds.BoxExtent;
	}
	else
	{
		return;
	}

	const FVector FullSize = LocalMax - LocalMin;
	if (FullSize.IsNearlyZero())
	{
		return;
	}

	const int32 LongAxis = FullSize.X >= FullSize.Y && FullSize.X >= FullSize.Z ? 0 : (FullSize.Y >= FullSize.Z ? 1 : 2);
	const FVector FullCenter = (LocalMin + LocalMax) * 0.5f;
	FVector BladeCenter = FullCenter;
	FVector BladeExtent = FullSize * 0.5f;

	BladeExtent[LongAxis] *= AutoFitBladeLengthRatio;
	BladeCenter[LongAxis] += FullSize[LongAxis] * AutoFitBladeCenterBias;

	for (int32 Axis = 0; Axis < 3; ++Axis)
	{
		if (Axis != LongAxis)
		{
			BladeExtent[Axis] *= AutoFitBladeThicknessRatio;
		}

		BladeExtent[Axis] += AutoFitBladePadding;
	}

	HitBoxComponent->AttachToComponent(WeaponMeshComponent, FAttachmentTransformRules::KeepRelativeTransform);
	HitBoxComponent->SetRelativeLocation(BladeCenter);
	HitBoxComponent->SetRelativeRotation(FRotator::ZeroRotator);
	HitBoxComponent->SetRelativeScale3D(FVector::OneVector);
	HitBoxComponent->SetBoxExtent(BladeExtent, true);
}

void ACombatCharacter::RegisterDefaultWeaponHitBox()
{
	if (!bAutoRegisterDefaultWeaponHitBox || !WeaponCollision || DefaultWeaponHitBoxComponentName == NAME_None)
	{
		return;
	}

	TArray<UPrimitiveComponent*> PrimitiveComponents;
	GetComponents<UPrimitiveComponent>(PrimitiveComponents);

	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (!PrimitiveComponent || PrimitiveComponent->GetFName() != DefaultWeaponHitBoxComponentName)
		{
			continue;
		}

		WeaponCollision->RegisterCollisionBody(PrimitiveComponent, GetActiveWeaponCollisionId());
		return;
	}
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

FName ACombatCharacter::GetActiveWeaponCollisionId() const
{
	switch (GetActiveWeaponType())
	{
	case ECombatWeaponType::Katana:
		return TEXT("Katana");
	case ECombatWeaponType::Unarmed:
	default:
		return NAME_None;
	}
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
	if (!bUseFrontDebugCamera)
	{
		GetCameraBoom()->TargetArmLength = DeathCameraDistance;
	}

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
	LifeBar->SetVisibility(bShowLifeBar);
	LifeBar->SetHiddenInGame(!bShowLifeBar);

	// initialize the camera
	ApplyCameraDebugSettings();

	// save the relative transform for the mesh so we can reset the ragdoll later
	MeshStartingTransform = GetMesh()->GetRelativeTransform();

	// set the life bar color
	LifeBarWidget->SetBarColor(LifeBarColor);

	// reset HP to maximum
	ResetHP();

	if (WeaponCollision)
	{
		WeaponCollision->OnDamageDealt.AddUniqueDynamic(this, &ACombatCharacter::HandleWeaponDamageDealt);
	}

	CurrentWeaponType = DefaultWeaponType;
	bIsWeaponDrawn = bStartWithWeaponDrawn && CurrentWeaponType != ECombatWeaponType::Unarmed;
	bPendingWeaponDrawn = bIsWeaponDrawn;
	bIsWeaponModeChanging = false;
	ApplyWeaponMovementState();
	UpdateWeaponAttachment();
	AutoFitDefaultWeaponHitBox();
	RegisterDefaultWeaponHitBox();
}

void ACombatCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (WeaponCollision)
	{
		WeaponCollision->OnDamageDealt.RemoveAll(this);
	}

	Super::EndPlay(EndPlayReason);

	// clear the respawn timer
	GetWorld()->GetTimerManager().ClearTimer(RespawnTimer);
}

void ACombatCharacter::HandleWeaponDamageDealt(float Damage, const FVector& ImpactPoint)
{
	DealtDamage(Damage, ImpactPoint);
}

void ACombatCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyCameraDebugSettings();
	UpdateWeaponAttachment();
	AutoFitDefaultWeaponHitBox();
	RegisterDefaultWeaponHitBox();
}

void ACombatCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		}

		// Moving
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACombatCharacter::Move);
		}

		// Looking
		if (LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACombatCharacter::Look);
		}

		if (MouseLookAction)
		{
			EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ACombatCharacter::Look);
		}

		// Combo Attack
		if (ComboAttackAction)
		{
			EnhancedInputComponent->BindAction(ComboAttackAction, ETriggerEvent::Started, this, &ACombatCharacter::ComboAttackPressed);
			EnhancedInputComponent->BindAction(ComboAttackAction, ETriggerEvent::Completed, this, &ACombatCharacter::DoComboAttackEnd);
		}

		// Charged Attack
		if (ChargedAttackAction)
		{
			EnhancedInputComponent->BindAction(ChargedAttackAction, ETriggerEvent::Started, this, &ACombatCharacter::ChargedAttackPressed);
			EnhancedInputComponent->BindAction(ChargedAttackAction, ETriggerEvent::Completed, this, &ACombatCharacter::ChargedAttackReleased);
		}

		// Camera Side Toggle
		if (ToggleCameraAction)
		{
			EnhancedInputComponent->BindAction(ToggleCameraAction, ETriggerEvent::Triggered, this, &ACombatCharacter::ToggleCamera);
		}

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
		PlayerInputComponent->BindKey(ComboAttackKey, IE_Released, this, &ACombatCharacter::DoComboAttackEnd);
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

