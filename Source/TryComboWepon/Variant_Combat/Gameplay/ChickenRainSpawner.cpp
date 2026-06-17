// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/ChickenRainSpawner.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

AChickenRainSpawner::AChickenRainSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AChickenRainSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoStart)
	{
		if (bHideTemplateActorsOnStart)
		{
			if (ChickenTemplateActor)
			{
				ChickenTemplateActor->SetActorHiddenInGame(true);
				ChickenTemplateActor->SetActorEnableCollision(false);
			}

			if (EggTemplateActor)
			{
				EggTemplateActor->SetActorHiddenInGame(true);
				EggTemplateActor->SetActorEnableCollision(false);
			}
		}

		StartChickenRain();
	}
}

void AChickenRainSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopChickenRain();

	Super::EndPlay(EndPlayReason);
}

void AChickenRainSpawner::StartChickenRain()
{
	UWorld* World = GetWorld();
	if (!World || !GetChickenSpawnClass())
	{
		return;
	}

	SpawnedChickenCount = 0;
	FallingChickens.Reset();

	World->GetTimerManager().SetTimer(ChickenSpawnTimer, this, &AChickenRainSpawner::SpawnChicken, ChickenSpawnInterval, true, 0.0f);

	if (GetEggSpawnClass())
	{
		World->GetTimerManager().SetTimer(EggDropTimer, this, &AChickenRainSpawner::DropEggs, EggDropInterval, true, EggDropInterval);
	}
}

void AChickenRainSpawner::StopChickenRain()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ChickenSpawnTimer);
		World->GetTimerManager().ClearTimer(EggDropTimer);
	}
}

void AChickenRainSpawner::SpawnChicken()
{
	UWorld* World = GetWorld();
	const TSubclassOf<AActor> SpawnClass = GetChickenSpawnClass();
	if (!World || !SpawnClass)
	{
		StopChickenRain();
		return;
	}

	if (SpawnedChickenCount >= MaxChickenCount)
	{
		World->GetTimerManager().ClearTimer(ChickenSpawnTimer);
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FRotator SpawnRotation(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f);
	AActor* Chicken = World->SpawnActor<AActor>(SpawnClass, PickSpawnLocation(), SpawnRotation, SpawnParams);
	if (!Chicken)
	{
		return;
	}

	EnablePhysics(Chicken);

	FChickenRainEntry Entry;
	Entry.Chicken = Chicken;
	Entry.SpawnTime = World->GetTimeSeconds();
	Entry.NextEggTime = Entry.SpawnTime;
	FallingChickens.Add(Entry);

	++SpawnedChickenCount;
}

void AChickenRainSpawner::DropEggs()
{
	UWorld* World = GetWorld();
	const TSubclassOf<AActor> SpawnClass = GetEggSpawnClass();
	if (!World || !SpawnClass)
	{
		return;
	}

	const float Now = World->GetTimeSeconds();

	for (int32 Index = FallingChickens.Num() - 1; Index >= 0; --Index)
	{
		FChickenRainEntry& Entry = FallingChickens[Index];
		AActor* Chicken = Entry.Chicken.Get();
		if (!Chicken || Now - Entry.SpawnTime > EggDropDuration)
		{
			FallingChickens.RemoveAtSwap(Index);
			continue;
		}

		if (Now < Entry.NextEggTime)
		{
			continue;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		const FVector EggLocation = Chicken->GetActorLocation() + FVector(0.0f, 0.0f, EggDropZOffset);
		AActor* Egg = World->SpawnActor<AActor>(SpawnClass, EggLocation, Chicken->GetActorRotation(), SpawnParams);
		EnablePhysics(Egg);

		Entry.NextEggTime = Now + EggDropInterval;
	}

	if (SpawnedChickenCount >= MaxChickenCount && FallingChickens.Num() == 0)
	{
		World->GetTimerManager().ClearTimer(EggDropTimer);
	}
}

FVector AChickenRainSpawner::PickSpawnLocation() const
{
	const FVector Origin = GetActorLocation();
	const FVector2D Offset2D = FMath::RandPointInCircle(SpawnRadius);
	return Origin + FVector(Offset2D.X, Offset2D.Y, SpawnHeight);
}

TSubclassOf<AActor> AChickenRainSpawner::GetChickenSpawnClass() const
{
	if (ChickenClass)
	{
		return ChickenClass;
	}

	return ChickenTemplateActor ? TSubclassOf<AActor>(ChickenTemplateActor->GetClass()) : nullptr;
}

TSubclassOf<AActor> AChickenRainSpawner::GetEggSpawnClass() const
{
	if (EggClass)
	{
		return EggClass;
	}

	return EggTemplateActor ? TSubclassOf<AActor>(EggTemplateActor->GetClass()) : nullptr;
}

void AChickenRainSpawner::EnablePhysics(AActor* Actor) const
{
	if (!Actor || !bForcePhysicsOnSpawnedActors)
	{
		return;
	}

	TArray<UPrimitiveComponent*> PrimitiveComponents;
	Actor->GetComponents<UPrimitiveComponent>(PrimitiveComponents);

	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (!PrimitiveComponent)
		{
			continue;
		}

		PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		PrimitiveComponent->SetEnableGravity(true);
		PrimitiveComponent->SetSimulatePhysics(true);
	}
}
