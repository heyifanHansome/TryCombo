// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CombatDamageable.h"
#include "CombatFlyingBasketball.generated.h"

class ACombatBasketballFragment;
class ACombatBasketballDrop;
class ACombatPhotoTarget;
class UProjectileMovementComponent;
class UPrimitiveComponent;
class UMaterialInstanceDynamic;
class USoundBase;
class USphereComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ECombatBasketballHitType : uint8
{
	Breakable UMETA(DisplayName="Breakable"),
	Deflectable UMETA(DisplayName="Deflectable"),
	Random UMETA(DisplayName="Random")
};

UCLASS(Blueprintable)
class ACombatFlyingBasketball : public AActor, public ICombatDamageable
{
	GENERATED_BODY()

public:
	ACombatFlyingBasketball();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USphereComponent> AttackCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball|Target")
	TObjectPtr<AActor> TargetActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball|Flight", meta=(ClampMin=1, Units="cm/s"))
	float FlightSpeed = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball|Flight", meta=(ClampMin=0, Units="cm/s"))
	float HomingAcceleration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball|Damage", meta=(ClampMin=1, ClampMax=100))
	float MaxHP = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Basketball|Damage")
	float CurrentHP = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball|Damage", meta=(ClampMin=0, ClampMax=100))
	float PlayerDamage = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball|Damage", meta=(ClampMin=0, Units="cm/s"))
	float PlayerKnockback = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball|Type")
	ECombatBasketballHitType HitType = ECombatBasketballHitType::Random;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball|Type")
	FLinearColor BreakableColor = FLinearColor(1.0f, 0.18f, 0.08f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball|Type")
	FLinearColor DeflectableColor = FLinearColor(0.05f, 0.45f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball|Type")
	FLinearColor RandomColor = FLinearColor(1.0f, 0.82f, 0.05f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball|Hit Feel", meta=(ClampMin=1, Units="cm"))
	float AttackHitRadius = 85.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball|Hit Feel", meta=(ClampMin=0, ClampMax=1))
	float BreakChance = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball|Hit Feel", meta=(ClampMin=0, ClampMax=1))
	float DeflectChance = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball|Hit Feel", meta=(ClampMin=0, Units="cm/s"))
	float DeflectSpeed = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball|Hit Feel")
	bool bDeflectTowardPhotoTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball|Hit Feel", meta=(ClampMin=0, Units="cm"))
	float DeflectTargetSearchRadius = 4000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball|Hit Feel", meta=(Units="cm"))
	float DeflectPlaneHeightOffset = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball|Break")
	TSubclassOf<ACombatBasketballFragment> FragmentClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball|Break")
	bool bSplitOnDeath = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball|Break", meta=(ClampMin=0, Units="cm/s"))
	float FragmentImpulse = 700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball|Break", meta=(ClampMin=0.01, ClampMax=1))
	float FragmentScale = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball|Break", meta=(ClampMin=0, Units="cm"))
	float FragmentSpawnOffset = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball|Drop")
	TSubclassOf<ACombatBasketballDrop> DropClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball|Drop", meta=(ClampMin=0, ClampMax=1))
	float DropChance = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball|Audio")
	TObjectPtr<USoundBase> HitSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball|Audio")
	TObjectPtr<USoundBase> BreakSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball|Audio")
	TObjectPtr<USoundBase> PlayerImpactSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball|Audio")
	TObjectPtr<USoundBase> DeflectSound;

	UFUNCTION(BlueprintCallable, Category="Basketball")
	void LaunchAtTarget(AActor* NewTargetActor);

	UFUNCTION(BlueprintCallable, Category="Basketball")
	void SetHitType(ECombatBasketballHitType NewHitType);

	virtual void ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse) override;
	virtual void HandleDeath() override;
	virtual void ApplyHealing(float Healing, AActor* Healer) override;
	virtual void NotifyDanger(const FVector& DangerLocation, AActor* DangerSource) override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void ConfigureCollision();
	void ApplyTypeVisuals();
	FLinearColor GetTypeColor() const;
	ACombatPhotoTarget* FindNearestPhotoTarget() const;
	void DeflectFromHit(const FVector& DamageLocation, const FVector& DamageImpulse, AActor* DamageCauser);
	bool TryTriggerPhotoTarget(AActor* OtherActor);
	void TrySpawnDrop();
	void SplitAndDestroy(const FVector& DamageLocation, const FVector& DamageImpulse, AActor* DamageCauser);
	FVector GetSplitAxis(const FVector& DamageLocation, const FVector& DamageImpulse, AActor* DamageCauser) const;

	bool bDead = false;
	bool bDeflected = false;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;
};
