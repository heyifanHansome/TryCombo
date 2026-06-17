// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChickenRainSpawner.generated.h"

USTRUCT()
struct FChickenRainEntry
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AActor> Chicken = nullptr;

	float SpawnTime = 0.0f;
	float NextEggTime = 0.0f;
};

UCLASS()
class TRYCOMBOWEPON_API AChickenRainSpawner : public AActor
{
	GENERATED_BODY()

public:
	AChickenRainSpawner();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Chicken Rain")
	TSubclassOf<AActor> ChickenClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Chicken Rain")
	TSubclassOf<AActor> EggClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Chicken Rain")
	TObjectPtr<AActor> ChickenTemplateActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Chicken Rain")
	TObjectPtr<AActor> EggTemplateActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Chicken Rain")
	bool bHideTemplateActorsOnStart = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Chicken Rain", meta=(ClampMin=1, ClampMax=1000))
	int32 MaxChickenCount = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Chicken Rain", meta=(ClampMin=0.01, Units="s"))
	float ChickenSpawnInterval = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Chicken Rain", meta=(ClampMin=0.0, Units="cm"))
	float SpawnRadius = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Chicken Rain", meta=(ClampMin=0.0, Units="cm"))
	float SpawnHeight = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Chicken Rain", meta=(ClampMin=0.0, Units="s"))
	float EggDropDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Chicken Rain", meta=(ClampMin=0.01, Units="s"))
	float EggDropInterval = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Chicken Rain", meta=(ClampMin=0.0, Units="cm"))
	float EggDropZOffset = -35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Chicken Rain")
	bool bAutoStart = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Chicken Rain|Physics")
	bool bForcePhysicsOnSpawnedActors = true;

	UFUNCTION(BlueprintCallable, Category="Chicken Rain")
	void StartChickenRain();

	UFUNCTION(BlueprintCallable, Category="Chicken Rain")
	void StopChickenRain();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	FTimerHandle ChickenSpawnTimer;
	FTimerHandle EggDropTimer;

	int32 SpawnedChickenCount = 0;

	UPROPERTY()
	TArray<FChickenRainEntry> FallingChickens;

	void SpawnChicken();
	void DropEggs();
	FVector PickSpawnLocation() const;
	TSubclassOf<AActor> GetChickenSpawnClass() const;
	TSubclassOf<AActor> GetEggSpawnClass() const;
	void EnablePhysics(AActor* Actor) const;
};
