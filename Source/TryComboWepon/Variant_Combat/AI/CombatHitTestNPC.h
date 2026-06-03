// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CombatDamageable.h"
#include "CombatHitTestNPC.generated.h"

class UStaticMeshComponent;
class UBoxComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCombatHitTestNPCDamagedSignature, float, Damage, const FVector&, ImpactPoint, float, RemainingHP);

/**
 * Simple standing target used to verify weapon collision windows.
 */
UCLASS(Blueprintable)
class ACombatHitTestNPC : public ACharacter, public ICombatDamageable
{
	GENERATED_BODY()

public:
	ACombatHitTestNPC();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> DebugBody;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UBoxComponent> HitReceiver;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage", meta=(ClampMin=1, ClampMax=1000))
	float MaxHP = 10.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Damage", meta=(ClampMin=0, ClampMax=1000))
	float CurrentHP = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage")
	bool bResetHealthOnDeath = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage")
	bool bLaunchOnDamage = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Debug")
	bool bDrawDebugOnDamage = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Debug", meta=(ClampMin=0, ClampMax=30, Units="s"))
	float DebugDrawDuration = 2.0f;

	UPROPERTY(BlueprintAssignable, Category="Events")
	FCombatHitTestNPCDamagedSignature OnDamaged;

	virtual void Tick(float DeltaSeconds) override;

	virtual void ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse) override;
	virtual void HandleDeath() override;
	virtual void ApplyHealing(float Healing, AActor* Healer) override;
	virtual void NotifyDanger(const FVector& DangerLocation, AActor* DangerSource) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI")
	bool bFacePlayer = true;

	virtual void BeginPlay() override;
};
