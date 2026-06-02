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
struct FInputActionValue;
class UCombatLifeBar;
class UWidgetComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogCombatCharacter, Log, All);

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

	/** Max amount of HP the character will have on respawn */
	UPROPERTY(EditAnywhere, Category="Damage", meta = (ClampMin = 0, ClampMax = 100))
	float MaxHP = 5.0f;

	/** Current amount of HP the character has */
	UPROPERTY(VisibleAnywhere, Category="Damage")
	float CurrentHP = 0.0f;

	/** Life bar widget fill color */
	UPROPERTY(EditAnywhere, Category="Damage")
	FLinearColor LifeBarColor;

	/** Name of the pelvis bone, for damage ragdoll physics */
	UPROPERTY(EditAnywhere, Category="Damage")
	FName PelvisBoneName;

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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon Mode")
	bool bIsWeaponModeChanging = false;

	/** 【Codex新增】攻击时如果还没有持剑，是否先自动拔刀 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Mode")
	bool bAutoDrawWeaponOnAttack = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Mode")
	bool bStartWithWeaponDrawn = false;

	/** 【Codex新增】拔刀Montage */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Mode|Movement", meta = (ClampMin = 0, ClampMax = 1000, Units = "cm/s"))
	float AutoSheatheMaxGroundSpeed = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Mode|Movement", meta = (ClampMin = 0, ClampMax = 2000, Units = "cm/s"))
	float SheathedMaxWalkSpeed = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Mode|Movement", meta = (ClampMin = 0, ClampMax = 2000, Units = "cm/s"))
	float WeaponDrawnMaxWalkSpeed = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Mode|Movement", meta = (ClampMin = 0, ClampMax = 4000, Units = "cm/s^2"))
	float SheathedMaxAcceleration = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Mode|Movement", meta = (ClampMin = 0, ClampMax = 4000, Units = "cm/s^2"))
	float WeaponDrawnMaxAcceleration = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Mode|Movement", meta = (ClampMin = 0, ClampMax = 8000, Units = "cm/s^2"))
	float SheathedBrakingDeceleration = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Mode|Movement", meta = (ClampMin = 0, ClampMax = 8000, Units = "cm/s^2"))
	float WeaponDrawnBrakingDeceleration = 1800.0f;

	UPROPERTY(EditAnywhere, Category="Weapon Mode")
	UAnimMontage* DrawWeaponMontage;

	/** 【Codex新增】收刀Montage */
	UPROPERTY(EditAnywhere, Category="Weapon Mode")
	UAnimMontage* SheatheWeaponMontage;

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

	/** Names of the AnimMontage sections that correspond to each stage of the combo attack */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Combo")
	TArray<FName> ComboSectionNames;

	/** 【Codex新增】第1-3段连招没有继续输入时，要跳转到的收刀Section名称 */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Combo")
	TArray<FName> ComboSheatheSectionNames;

	/** Max amount of time that may elapse for a combo attack input to not be considered stale */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Combo", meta = (ClampMin = 0, ClampMax = 5, Units = "s"))
	float ComboInputCacheTimeTolerance = 0.45f;

	/** Index of the current stage of the melee attack combo */
	int32 ComboCount = 0;

	/** AnimMontage that will play for charged attacks */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Charged")
	UAnimMontage* ChargedAttackMontage;

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

	/** Time to wait before respawning the character */
	UPROPERTY(EditAnywhere, Category="Respawn", meta = (ClampMin = 0, ClampMax = 10, Units = "s"))
	float RespawnTime = 3.0f;

	/** Attack montage ended delegate */
	FOnMontageEnded OnAttackMontageEnded;

	/** 【Codex新增】拔刀/收刀Montage结束代理 */
	FOnMontageEnded OnWeaponModeMontageEnded;

	/** Character respawn timer */
	FTimerHandle RespawnTimer;

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
	UFUNCTION(BlueprintPure, Category="Weapon Mode|Movement")
	float GetGroundSpeed() const;

	UFUNCTION(BlueprintPure, Category="Weapon Mode|Movement")
	bool IsMovingOnGround() const;

	UFUNCTION(BlueprintCallable, Category="Weapon Mode")
	void SetWeaponModeMontages(UAnimMontage* NewDrawMontage, UAnimMontage* NewSheatheMontage);

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
	bool TryJumpToComboSheatheSection();

	/** 【Codex新增-连招收刀核心逻辑】判断继续连招，还是进入当前段收刀 */
	void CheckComboOrSheatheSection();

	void ApplyWeaponMovementState();

	bool CanAutoSheatheFromCombo() const;

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

	/** Blueprint handler to play damage dealt effects */
	UFUNCTION(BlueprintImplementableEvent, Category="Combat")
	void DealtDamage(float Damage, const FVector& ImpactPoint);

	/** Blueprint handler to play damage received effects */
	UFUNCTION(BlueprintImplementableEvent, Category="Combat")
	void ReceivedDamage(float Damage, const FVector& ImpactPoint, const FVector& DamageDirection);

protected:

	/** Initialization */
	virtual void BeginPlay() override;

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
