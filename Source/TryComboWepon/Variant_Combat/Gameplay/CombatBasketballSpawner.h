// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CombatFlyingBasketball.h"
#include "CombatBasketballSpawner.generated.h"

UCLASS(Blueprintable)
class ACombatBasketballSpawner : public AActor
{
	GENERATED_BODY()

public:
	ACombatBasketballSpawner();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball Spawner")
	TSubclassOf<ACombatFlyingBasketball> BasketballClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball Spawner|Types")
	bool bRandomizeBasketballTypes = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball Spawner|Types")
	ECombatBasketballHitType FixedBasketballType = ECombatBasketballHitType::Random;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball Spawner|Types", meta=(ClampMin=0))
	float BreakableWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball Spawner|Types", meta=(ClampMin=0))
	float DeflectableWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball Spawner|Types", meta=(ClampMin=0))
	float RandomWeight = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball Spawner")
	TObjectPtr<AActor> TargetActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball Spawner")
	bool bAutoStart = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball Spawner", meta=(ClampMin=0.05, Units="s"))
	float SpawnInterval = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball Spawner", meta=(ClampMin=1, ClampMax=100))
	int32 BasketballsPerSpawn = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball Spawner", meta=(ClampMin=1, Units="cm"))
	float SpawnRadius = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball Spawner", meta=(ClampMin=0, Units="cm"))
	float MinSpawnHeight = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball Spawner", meta=(ClampMin=0, Units="cm"))
	float MaxSpawnHeight = 420.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basketball Spawner", meta=(ClampMin=0, ClampMax=64))
	int32 MaxLiveBasketballs = 8;

	UFUNCTION(BlueprintCallable, Category="Basketball Spawner")
	ACombatFlyingBasketball* SpawnBasketball();

	UFUNCTION(BlueprintCallable, Category="Basketball Spawner")
	void SpawnBasketballWave(int32 Count);

	UFUNCTION(BlueprintCallable, Category="Basketball Spawner")
	void StartSpawning();

	UFUNCTION(BlueprintCallable, Category="Basketball Spawner")
	void StopSpawning();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void SpawnBasketballFromTimer();
	ECombatBasketballHitType PickBasketballType() const;
	void CleanupDeadBasketballs();
	AActor* GetTargetActor() const;

	FTimerHandle SpawnTimer;
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ACombatFlyingBasketball>> LiveBasketballs;
};
