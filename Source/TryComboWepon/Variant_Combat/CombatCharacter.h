// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CombatAttacker.h"
#include "CombatDamageable.h"
#include "Animation/AnimInstance.h"
#include "InputCoreTypes.h"
#include "CombatCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class USceneComponent;
struct FInputActionValue;
class UCombatLifeBar;
class UCombatWeaponCollisionComponent;
class UWidgetComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class USoundBase;
class ACombatFlyingBasketball;
class ACombatBasketballSpawner;
class ACombatSummonMarker;
class ACombatSummonShot;
class ACombatShadowClone;

DECLARE_LOG_CATEGORY_EXTERN(LogCombatCharacter, Log, All);

UENUM(BlueprintType)
enum class ECombatWeaponType : uint8
{
	Unarmed UMETA(DisplayName="Unarmed"),
	Katana UMETA(DisplayName="Katana")
};

/**
 *  An enhanced Third Person Character with melee combat capabilities:
 *  - Combo attack string
 *  - Press and hold charged attack
 *  - Damage dealing and reaction
 *  - Death
 *  - Respawning
 */
UCLASS(abstract)
class ACombatCharacter : public ACharacter, public ICombatAttacker, public ICombatDamageable
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/** Life bar widget component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UWidgetComponent* LifeBar;

	/** Weapon hit detection and damage application */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCombatWeaponCollisionComponent* WeaponCollision;

	/** Attach visible weapon meshes and weapon collision bodies under this scene component. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* WeaponRoot;
	
protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;

	/** Combo Attack Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* ComboAttackAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	FKey ComboAttackKey = EKeys::LeftMouseButton;

	/** Charged Attack Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* ChargedAttackAction;

	/** Toggle Camera Side Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* ToggleCameraAction;

	/** 【Codex新增】切换持剑/收刀状态的输入 */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* ToggleWeaponModeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	FKey ToggleWeaponModeKey = EKeys::G;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	FKey FireJutsuKey = EKeys::F;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	FKey ShadowCloneKey = EKeys::Q;

	/** Max amount of HP the character will have on respawn */
	UPROPERTY(EditAnywhere, Category="Damage", meta = (ClampMin = 0, ClampMax = 100))
	float MaxHP = 5.0f;

	/** Current amount of HP the character has */
	UPROPERTY(VisibleAnywhere, Category="Damage")
	float CurrentHP = 0.0f;

	/** Life bar widget fill color */
	UPROPERTY(EditAnywhere, Category="Damage")
	FLinearColor LifeBarColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage")
	bool bShowLifeBar = false;

	/** Name of the pelvis bone, for damage ragdoll physics */
	UPROPERTY(EditAnywhere, Category="Damage")
	FName PelvisBoneName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage", meta=(ClampMin=0, ClampMax=5, Units="s"))
	float DamageReactionDuration = 1.0f;

	/** Pointer to the life bar widget */
	UPROPERTY(EditAnywhere, Category="Damage")
	TObjectPtr<UCombatLifeBar> LifeBarWidget;

	/** Max amount of time that may elapse for a non-combo attack input to not be considered stale */
	UPROPERTY(EditAnywhere, Category="Melee Attack", meta = (ClampMin = 0, ClampMax = 5, Units = "s"))
	float AttackInputCacheTimeTolerance = 1.0f;

	/** Time at which an attack button was last pressed */
	float CachedAttackInputTime = 0.0f;

	/** If true, the character is currently playing an attack animation */
	bool bIsAttacking = false;

	/** 【Codex新增】当前是否处于持剑/拔刀状态，AnimBP可以读取这个状态切换Locomotion */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon Mode")
	bool bIsWeaponDrawn = false;

	/** 【Codex新增】当前是否正在播放拔刀/收刀过渡Montage */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Mode")
	ECombatWeaponType DefaultWeaponType = ECombatWeaponType::Katana;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon Mode")
	ECombatWeaponType CurrentWeaponType = ECombatWeaponType::Unarmed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon Mode")
	bool bIsWeaponModeChanging = false;

	/** 【Codex新增】攻击时如果还没有持剑，是否先自动拔刀 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Mode")
	bool bAutoDrawWeaponOnAttack = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Mode")
	bool bStartWithWeaponDrawn = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Mode|Visual")
	FName DrawnWeaponAttachSocketName = TEXT("hand_rSocket");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Mode|Visual")
	FName SheathedWeaponAttachSocketName = TEXT("spine_03");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Collision|Auto Register")
	bool bAutoRegisterDefaultWeaponHitBox = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Collision|Auto Register")
	FName DefaultWeaponHitBoxComponentName = TEXT("KatanaHitBox");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Mode|Visual")
	FName DefaultWeaponMeshComponentName = TEXT("KatanaMesh");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Mode|Visual")
	FName DefaultWeaponSheathMeshComponentName = TEXT("KatanaSheathMesh");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Collision|Auto Fit")
	bool bAutoFitDefaultWeaponHitBoxToWeaponMesh = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Collision|Auto Fit", meta=(ClampMin=0.05, ClampMax=1.0))
	float AutoFitBladeLengthRatio = 0.82f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Collision|Auto Fit", meta=(ClampMin=0.05, ClampMax=1.0))
	float AutoFitBladeThicknessRatio = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Collision|Auto Fit", meta=(ClampMin=-1.0, ClampMax=1.0))
	float AutoFitBladeCenterBias = 0.09f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Collision|Auto Fit", meta=(ClampMin=0.0, ClampMax=50.0, Units="cm"))
	float AutoFitBladePadding = 2.0f;

	/** 【Codex新增】拔刀Montage */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Mode|Movement", meta = (ClampMin = 0, ClampMax = 1000, Units = "cm/s"))
	float AutoSheatheMaxGroundSpeed = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Mode|Movement", meta = (ClampMin = 0, ClampMax = 2000, Units = "cm/s"))
	float SheathedMaxWalkSpeed = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Mode|Movement", meta = (ClampMin = 0, ClampMax = 2000, Units = "cm/s"))
	float WeaponDrawnMaxWalkSpeed = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Mode|Movement", meta = (ClampMin = 0, ClampMax = 4000, Units = "cm/s^2"))
	float SheathedMaxAcceleration = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Mode|Movement", meta = (ClampMin = 0, ClampMax = 8000, Units = "cm/s^2"))
	float WeaponDrawnMaxAcceleration = 4000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Mode|Movement", meta = (ClampMin = 0, ClampMax = 8000, Units = "cm/s^2"))
	float SheathedBrakingDeceleration = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Mode|Movement", meta = (ClampMin = 0, ClampMax = 8000, Units = "cm/s^2"))
	float WeaponDrawnBrakingDeceleration = 1800.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Movement|Animation")
	float SmoothedGroundSpeed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Animation", meta=(ClampMin=0, Units="cm/s^2"))
	float AnimationSpeedAcceleration = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Animation", meta=(ClampMin=0, Units="cm/s^2"))
	float AnimationSpeedDeceleration = 2600.0f;

	UPROPERTY(EditAnywhere, Category="Weapon Mode")
	UAnimMontage* DrawWeaponMontage;

	/** 【Codex新增】收刀Montage */
	UPROPERTY(EditAnywhere, Category="Weapon Mode")
	UAnimMontage* SheatheWeaponMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Back Jump")
	UAnimMontage* BackJumpMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Back Jump", meta=(ClampMin=0, ClampMax=2000, Units="cm/s"))
	float BackJumpBackwardSpeed = 650.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Back Jump", meta=(ClampMin=0, ClampMax=1000, Units="cm/s"))
	float BackJumpUpSpeed = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Back Jump")
	bool bBackJumpStopAttackMontage = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Summon Test")
	UAnimMontage* SummonMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Summon Test")
	TSubclassOf<ACombatSummonMarker> SummonMarkerClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Summon Test")
	TSubclassOf<ACombatSummonShot> SummonShotClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Summon Test", meta=(ClampMin=1, ClampMax=40))
	int32 SummonMarkerCount = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Summon Test", meta=(ClampMin=0, Units="cm"))
	float SummonMarkerRadius = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Summon Test", meta=(ClampMin=0, Units="cm"))
	float SummonTargetSearchRadius = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Summon Test", meta=(ClampMin=1, ClampMax=100))
	int32 SummonMaxTargets = 20;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Summon Test", meta=(ClampMin=0, Units="cm"))
	float SummonShotSpawnHeight = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Summon Test", meta=(ClampMin=0, Units="cm"))
	float SummonShotSpawnRadius = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Summon Test")
	bool bSummonSpawnBasketballWave = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Summon Test", meta=(ClampMin=1, ClampMax=100))
	int32 SummonBasketballWaveCount = 20;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Summon Test", meta=(ClampMin=0, Units="cm"))
	float SummonSpawnerSearchRadius = 6000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fire Jutsu")
	UAnimMontage* FireJutsuMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fire Jutsu|Audio")
	TObjectPtr<USoundBase> FireJutsuSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fire Jutsu|Audio", meta=(ClampMin=0.0, UIMin=0.0, UIMax=2.0))
	float FireJutsuSoundVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fire Jutsu|Audio", meta=(ClampMin=0.1, UIMin=0.1, UIMax=3.0))
	float FireJutsuSoundPitch = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fire Jutsu")
	bool bFireJutsuTriggerVfxOnCast = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shadow Clone")
	TSubclassOf<ACombatShadowClone> ShadowCloneClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shadow Clone", meta=(ClampMin=1, ClampMax=12))
	int32 ShadowCloneCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shadow Clone", meta=(ClampMin=0, Units="cm"))
	float ShadowCloneSideSpacing = 130.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shadow Clone", meta=(Units="cm"))
	float ShadowCloneForwardOffset = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shadow Clone", meta=(ClampMin=0.1, Units="s"))
	float ShadowCloneLifeSeconds = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shadow Clone", meta=(ClampMin=0, Units="s"))
	float ShadowCloneCooldown = 1.0f;

	float LastShadowCloneTime = -1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fire Jutsu|VFX")
	UNiagaraSystem* FireJutsuVfx;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fire Jutsu|VFX")
	FName FireJutsuSocketName = TEXT("Mouth_Fire_Socket");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fire Jutsu|VFX")
	FVector FireJutsuVfxRelativeLocation = FVector(20.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fire Jutsu|VFX")
	FRotator FireJutsuVfxRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fire Jutsu|VFX")
	bool bFireJutsuUseControllerAim = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fire Jutsu|VFX", meta=(Units="deg"))
	float FireJutsuAttachedVfxYawOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fire Jutsu|VFX", meta=(ClampMin=0.01))
	FVector FireJutsuVfxScale = FVector(4.0f, 4.0f, 4.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fire Jutsu|VFX", meta=(ClampMin=1, ClampMax=24))
	int32 FireJutsuVfxSegments = 9;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fire Jutsu|VFX", meta=(ClampMin=0, Units="cm"))
	float FireJutsuVfxRange = 1600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fire Jutsu|VFX", meta=(ClampMin=0, Units="cm"))
	float FireJutsuVfxWidth = 420.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fire Jutsu|VFX", meta=(ClampMin=0.01))
	float FireJutsuVfxEndScaleMultiplier = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fire Jutsu|VFX", meta=(ClampMin=0, Units="s"))
	float FireJutsuVfxDuration = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Mode|Debug", meta = (ClampMin = 0.05, ClampMax = 3.0))
	float WeaponModeMontagePlayRate = 1.0f;

	/** 【Codex新增】拔刀/收刀目标状态，用于Montage结束后落状态 */
	bool bPendingWeaponDrawn = false;

	/** 【Codex新增】拔刀完成后是否继续执行蓄力攻击 */
	bool bPendingComboSheathe = false;

	bool bQueuedWeaponModeChargedAttack = false;

	/** Distance ahead of the character that melee attack sphere collision traces will extend */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Trace", meta = (ClampMin = 0, ClampMax = 500, Units="cm"))
	float MeleeTraceDistance = 75.0f;

	/** Radius of the sphere trace for melee attacks */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Trace", meta = (ClampMin = 0, ClampMax = 200, Units = "cm"))
	float MeleeTraceRadius = 75.0f;

	/** Distance ahead of the character that enemies will be notified of incoming attacks */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Trace", meta = (ClampMin = 0, ClampMax = 500, Units="cm"))
	float DangerTraceDistance = 300.0f;

	/** Radius of the sphere trace to notify enemies of incoming attacks */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Trace", meta = (ClampMin = 0, ClampMax = 200, Units = "cm"))
	float DangerTraceRadius = 100.0f;

	/** Amount of damage a melee attack will deal */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Damage", meta = (ClampMin = 0, ClampMax = 100))
	float MeleeDamage = 1.0f;

	/** Amount of knockback impulse a melee attack will apply */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Damage", meta = (ClampMin = 0, ClampMax = 1000, Units = "cm/s"))
	float MeleeKnockbackImpulse = 250.0f;

	/** Amount of upwards impulse a melee attack will apply */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Damage", meta = (ClampMin = 0, ClampMax = 1000, Units = "cm/s"))
	float MeleeLaunchImpulse = 300.0f;

	/** AnimMontage that will play for combo attacks */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Combo")
	UAnimMontage* ComboAttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Attack|Combo|Debug", meta = (ClampMin = 0.05, ClampMax = 3.0))
	float ComboAttackMontagePlayRate = 1.0f;

	/** Names of the AnimMontage sections that correspond to each stage of the combo attack */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Combo")
	TArray<FName> ComboSectionNames;

	/** 【Codex新增】第1-3段连招没有继续输入时，要跳转到的收刀Section名称 */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Combo")
	TArray<FName> ComboSheatheSectionNames;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Attack|Combo")
	bool bRequireComboInputWindow = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Attack|Combo")
	bool bCheckComboOnInputWindowEnd = true;

	/** Index of the current stage of the melee attack combo */
	int32 ComboCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Melee Attack|Combo")
	bool bComboInputQueued = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Melee Attack|Combo")
	bool bComboAttackHeld = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Melee Attack|Combo")
	bool bComboInputWindowOpen = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Melee Attack|Combo")
	bool bComboInputWindowConsumed = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Melee Attack|Combo")
	float ComboInputWindowElapsedTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Melee Attack|Combo")
	float ComboInputWindowDuration = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Melee Attack|Combo")
	float ComboInputWindowAlpha = 0.0f;

	/** AnimMontage that will play for charged attacks */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Charged")
	UAnimMontage* ChargedAttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Attack|Charged|Debug", meta = (ClampMin = 0.05, ClampMax = 3.0))
	float ChargedAttackMontagePlayRate = 1.0f;

	/** Name of the AnimMontage section that corresponds to the charge loop */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Charged")
	FName ChargeLoopSection;

	/** Name of the AnimMontage section that corresponds to the attack */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Charged")
	FName ChargeAttackSection;

	/** Flag that determines if the player is currently holding the charged attack input */
	bool bIsChargingAttack = false;
	
	/** If true, the charged attack hold check has been tested at least once */
	bool bHasLoopedChargedAttack = false;

	/** Camera boom length while the character is dead */
	UPROPERTY(EditAnywhere, Category="Camera", meta = (ClampMin = 0, ClampMax = 1000, Units = "cm"))
	float DeathCameraDistance = 400.0f;

	/** Camera boom length when the character respawns */
	UPROPERTY(EditAnywhere, Category="Camera", meta = (ClampMin = 0, ClampMax = 1000, Units = "cm"))
	float DefaultCameraDistance = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera|Debug")
	bool bUseFrontDebugCamera = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera|Debug", meta = (ClampMin = 0, ClampMax = 1000, Units = "cm"))
	float FrontDebugCameraDistance = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera|Debug", meta = (ClampMin = -89, ClampMax = 89, Units = "deg"))
	float FrontDebugCameraPitch = -5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera|Debug", meta = (ClampMin = -180, ClampMax = 180, Units = "deg"))
	float FrontDebugCameraYaw = 180.0f;

	/** Time to wait before respawning the character */
	UPROPERTY(EditAnywhere, Category="Respawn", meta = (ClampMin = 0, ClampMax = 10, Units = "s"))
	float RespawnTime = 3.0f;

	/** Attack montage ended delegate */
	FOnMontageEnded OnAttackMontageEnded;

	/** 【Codex新增】拔刀/收刀Montage结束代理 */
	FOnMontageEnded OnWeaponModeMontageEnded;

	/** Character respawn timer */
	FTimerHandle RespawnTimer;

	FTimerHandle DamageReactionTimer;

	FTimerHandle FireJutsuVfxTimer;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> ActiveFireJutsuVfx;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UNiagaraComponent>> ActiveFireJutsuVfxSegments;

	float FireJutsuAttachDirectionEndTime = 0.0f;

	/** Copy of the mesh's transform so we can reset it after ragdoll animations */
	FTransform MeshStartingTransform;

public:
	
	/** Constructor */
	ACombatCharacter();

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** Called for combo attack input */
	void ComboAttackPressed();

	/** Called for combo attack input pressed */
	void ChargedAttackPressed();

	/** Called for combo attack input released */
	void ChargedAttackReleased();

	/** Called for toggle camera side input */
	void ToggleCamera();

	/** 【Codex新增】输入触发的拔刀/收刀切换 */
	void ToggleWeaponModePressed();

	void FireJutsuPressed();

	void ShadowClonePressed();

	/** BP hook to animate the camera side switch */
	UFUNCTION(BlueprintImplementableEvent, Category="Combat")
	void BP_ToggleCamera();

public:

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles combo attack pressed from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoComboAttackStart();

	/** Handles combo attack released from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoComboAttackEnd();

	/** 【Codex新增】允许蓝图或运行时代码动态替换连招Montage */
	UFUNCTION(BlueprintCallable, Category="Combat|Combo")
	void SetComboAttackMontage(UAnimMontage* NewMontage);

	/** 【Codex新增】切换持剑状态；会优先播放拔刀/收刀Montage */
	UFUNCTION(BlueprintCallable, Category="Weapon Mode")
	void SetWeaponDrawn(bool bNewWeaponDrawn);

	/** 【Codex新增】在持剑和收刀之间切换 */
	UFUNCTION(BlueprintCallable, Category="Weapon Mode")
	void ToggleWeaponMode();

	/** 【Codex新增】AnimBP读取：当前是否持剑 */
	UFUNCTION(BlueprintPure, Category="Weapon Mode")
	bool IsWeaponDrawn() const { return bIsWeaponDrawn; }

	/** 【Codex新增】AnimBP读取：当前是否正在拔刀/收刀过渡 */
	UFUNCTION(BlueprintPure, Category="Weapon Mode")
	bool IsWeaponModeChanging() const { return bIsWeaponModeChanging; }

	/** 【Codex新增】允许蓝图或运行时代码动态替换拔刀/收刀Montage */
	UFUNCTION(BlueprintPure, Category="Weapon Mode")
	ECombatWeaponType GetCurrentWeaponType() const { return CurrentWeaponType; }

	UFUNCTION(BlueprintPure, Category="Weapon Mode")
	ECombatWeaponType GetActiveWeaponType() const { return bIsWeaponDrawn ? CurrentWeaponType : ECombatWeaponType::Unarmed; }

	UFUNCTION(BlueprintCallable, Category="Weapon Mode")
	void SetCurrentWeaponType(ECombatWeaponType NewWeaponType);

	UFUNCTION(BlueprintPure, Category="Weapon Mode|Movement")
	float GetGroundSpeed() const;

	UFUNCTION(BlueprintPure, Category="Weapon Mode|Movement")
	float GetRawGroundSpeed() const;

	UFUNCTION(BlueprintPure, Category="Weapon Mode|Movement")
	bool IsMovingOnGround() const;

	UFUNCTION(BlueprintCallable, Category="Weapon Mode")
	void SetWeaponModeMontages(UAnimMontage* NewDrawMontage, UAnimMontage* NewSheatheMontage);

	UFUNCTION(BlueprintCallable, Category="Movement|Back Jump")
	bool DoBackJump();

	UFUNCTION(BlueprintCallable, Category="Summon Test")
	bool DoSummonTest();

	UFUNCTION(BlueprintCallable, Category="Fire Jutsu")
	bool DoFireJutsu();

	UFUNCTION(BlueprintCallable, Category="Shadow Clone")
	bool DoShadowClone();

	UFUNCTION(BlueprintCallable, Category="Fire Jutsu")
	void TriggerFireJutsuVfx();

	UFUNCTION(BlueprintCallable, Category="Fire Jutsu")
	void StopFireJutsuVfx();

	UFUNCTION(BlueprintCallable, Category="Fire Jutsu")
	void AlignFireJutsuAttachedVfxToCharacterDirection();

	UFUNCTION(BlueprintCallable, Category="Weapon Mode|Visual")
	void AttachWeaponToDrawnSocket();

	UFUNCTION(BlueprintCallable, Category="Weapon Mode|Visual")
	void AttachWeaponToSheathedSocket();

	UFUNCTION(BlueprintCallable, Category="Weapon Mode|Visual")
	void AttachWeaponToSocket(FName SocketName);

	UFUNCTION(BlueprintCallable, Category="Combat|Combo")
	void EndComboMontage(float BlendOutTime = 0.05f);

	UFUNCTION(BlueprintCallable, Category="Combat|Combo")
	void BeginComboInputWindow(float WindowDuration = 0.0f);

	UFUNCTION(BlueprintCallable, Category="Combat|Combo")
	void TickComboInputWindow(float DeltaSeconds, float WindowDuration);

	UFUNCTION(BlueprintCallable, Category="Combat|Combo")
	void EndComboInputWindow(FName NoInputSectionName = NAME_None);

	/** Handles charged attack pressed from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoChargedAttackStart();

	/** Handles charged attack released from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoChargedAttackEnd();

protected:

	/** Resets the character's current HP to maximum */
	void ResetHP();

	/** Performs a combo attack */
	void ComboAttack();

	/** 【Codex新增】播放拔刀/收刀Montage；没有Montage时直接落状态 */
	void PlayWeaponModeMontage(bool bDrawWeapon);

	/** 【Codex新增】拔刀/收刀过渡结束后统一落状态 */
	void FinishWeaponModeTransition(bool bInterrupted);

	/** 【Codex新增】拔刀/收刀Montage结束回调 */
	void WeaponModeMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	/** 【Codex新增】跳转到当前连招段对应的收刀Section */
	bool TryJumpToComboSheatheSection(FName OverrideSectionName = NAME_None);

	/** 【Codex新增-连招收刀核心逻辑】判断继续连招，还是进入当前段收刀 */
	void CheckComboOrSheatheSection(FName OverrideSheatheSectionName = NAME_None);

	void QueueComboInputIfAllowed();

	bool IsComboAttackInputHeld() const;

	void ApplyWeaponMovementState();

	void ApplyCameraDebugSettings();

	void UpdateWeaponAttachment();

	bool ShouldAttachWeaponAsDrawn() const;

	void AttachComponentToWeaponSocket(USceneComponent* ComponentToAttach, FName SocketName);

	void AttachNamedWeaponComponents(FName SocketName);

	void AutoFitDefaultWeaponHitBox();

	void RegisterDefaultWeaponHitBox();

	bool CanAutoSheatheFromCombo() const;

	FName GetActiveWeaponCollisionId() const;

	/** Performs a charged attack */
	void ChargedAttack();

	/** Called from a delegate when the attack montage ends */
	void AttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	
public:

	// ~begin CombatAttacker interface

	/** Performs the collision check for an attack */
	virtual void DoAttackTrace(FName DamageSourceBone) override;

	/** Performs the combo string check */
	virtual void CheckCombo() override;

	/** Performs the charged attack hold check */
	virtual void CheckChargedAttack() override;

	// ~end CombatAttacker interface

	// ~begin CombatDamageable interface

	/** Notifies nearby enemies that an attack is coming so they can react */
	void NotifyEnemiesOfIncomingAttack();

	/** Handles damage and knockback events */
	virtual void ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse) override;

	/** Handles death events */
	virtual void HandleDeath() override;

	/** Handles healing events */
	virtual void ApplyHealing(float Healing, AActor* Healer) override;

	/** Allows reaction to incoming attacks */
	virtual void NotifyDanger(const FVector& DangerLocation, AActor* DangerSource) override;

	// ~end CombatDamageable interface

	/** Called from the respawn timer to destroy and re-create the character */
	void RespawnCharacter();

public:

	/** Overrides the default TakeDamage functionality */
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	/** Overrides landing to reset damage ragdoll physics */
	virtual void Landed(const FHitResult& Hit) override;

protected:

	virtual void OnConstruction(const FTransform& Transform) override;

	/** Blueprint handler to play damage dealt effects */
	UFUNCTION(BlueprintImplementableEvent, Category="Combat")
	void DealtDamage(float Damage, const FVector& ImpactPoint);

	UFUNCTION()
	void HandleWeaponDamageDealt(float Damage, const FVector& ImpactPoint);

	void ClearDamageReaction();

	void FindNearestBasketballs(float SearchRadius, int32 MaxTargets, TArray<ACombatFlyingBasketball*>& OutBasketballs) const;

	ACombatBasketballSpawner* FindNearestBasketballSpawner(float SearchRadius) const;

	void SpawnSummonMarkers();

	void FireSummonShotAt(AActor* TargetActor, int32 ShotIndex, int32 ShotCount);

	/** Blueprint handler to play damage received effects */
	UFUNCTION(BlueprintImplementableEvent, Category="Combat")
	void ReceivedDamage(float Damage, const FVector& ImpactPoint, const FVector& DamageDirection);

protected:

	/** Initialization */
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	/** Cleanup */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Handles input bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Handles possessed initialization */
	virtual void NotifyControllerChanged() override;

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};
