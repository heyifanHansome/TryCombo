// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CombatSummonShot.generated.h"

class UProjectileMovementComponent;
class UPrimitiveComponent;
class USphereComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class ACombatSummonShot : public AActor
{
	GENERATED_BODY()

public:
	ACombatSummonShot();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Summon Shot", meta=(ClampMin=0, Units="cm/s"))
	float Speed = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Summon Shot")
	bool bUseHoming = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Summon Shot", meta=(ClampMin=0, Units="cm/s^2"))
	float HomingAcceleration = 12000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Summon Shot", meta=(ClampMin=0))
	float Damage = 999.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Summon Shot", meta=(ClampMin=0, Units="s"))
	float Lifetime = 3.0f;

	UFUNCTION(BlueprintCallable, Category="Summon Shot")
	void FireAtTarget(AActor* NewTargetActor);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void DestroyShot();

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> TargetActor;

	FTimerHandle DestroyTimer;
};
