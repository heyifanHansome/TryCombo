// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CombatShadowClone.generated.h"

class ACombatCharacter;
class UMaterialInterface;
class USceneComponent;
class USkeletalMeshComponent;

UCLASS(Blueprintable)
class TRYCOMBOWEPON_API ACombatShadowClone : public AActor
{
	GENERATED_BODY()

public:
	ACombatShadowClone();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USkeletalMeshComponent> CloneMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow Clone", meta=(ClampMin=0.1, Units="s"))
	float LifeSeconds = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow Clone", meta=(ClampMin=0, Units="s"))
	float DamageDelay = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow Clone", meta=(ClampMin=0, ClampMax=100))
	float Damage = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow Clone", meta=(ClampMin=1, Units="cm"))
	float DamageRadius = 135.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow Clone", meta=(ClampMin=1, Units="cm"))
	float DamageForwardOffset = 135.0f;

	UFUNCTION(BlueprintCallable, Category="Shadow Clone")
	void InitializeFromCharacter(ACombatCharacter* SourceCharacter);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	void CopyWeaponVisuals(ACombatCharacter* SourceCharacter);
	void ApplyCloneVisuals();
	void PerformCloneDamage();
	void DestroyClone();

	UPROPERTY(Transient)
	TWeakObjectPtr<ACombatCharacter> SourceCharacterPtr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USceneComponent>> CopiedWeaponComponents;

	FTimerHandle DamageTimer;
	FTimerHandle LifeTimer;
};
