// Copyright Epic Games, Inc. All Rights Reserved.

#include "CombatBasketballSpawner.h"
#include "CombatFlyingBasketball.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ACombatBasketballSpawner::ACombatBasketballSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	BasketballClass = ACombatFlyingBasketball::StaticClass();
}

void ACombatBasketballSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoStart)
	{
		StartSpawning();
	}
}

void ACombatBasketballSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopSpawning();
	Super::EndPlay(EndPlayReason);
}

void ACombatBasketballSpawner::StartSpawning()
{
	if (!GetWorld())
	{
		return;
	}

	GetWorld()->GetTimerManager().SetTimer(SpawnTimer, this, &ACombatBasketballSpawner::SpawnBasketballFromTimer, SpawnInterval, true, 0.0f);
}

void ACombatBasketballSpawner::StopSpawning()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(SpawnTimer);
	}
}

AActor* ACombatBasketballSpawner::GetTargetActor() const
{
	return TargetActor ? TargetActor.Get() : UGameplayStatics::GetPlayerPawn(this, 0);
}

void ACombatBasketballSpawner::CleanupDeadBasketballs()
{
	LiveBasketballs.RemoveAll([](const TWeakObjectPtr<ACombatFlyingBasketball>& Basketball)
	{
		return !Basketball.IsValid();
	});
}

void ACombatBasketballSpawner::SpawnBasketballFromTimer()
{
	SpawnBasketball();
}

ECombatBasketballHitType ACombatBasketballSpawner::PickBasketballType() const
{
	if (!bRandomizeBasketballTypes)
	{
		return FixedBasketballType;
	}

	const float SafeBreakableWeight = FMath::Max(0.0f, BreakableWeight);
	const float SafeDeflectableWeight = FMath::Max(0.0f, DeflectableWeight);
	const float SafeRandomWeight = FMath::Max(0.0f, RandomWeight);
	const float TotalWeight = SafeBreakableWeight + SafeDeflectableWeight + SafeRandomWeight;
	if (TotalWeight <= 0.0f)
	{
		return ECombatBasketballHitType::Random;
	}

	const float Pick = FMath::FRandRange(0.0f, TotalWeight);
	if (Pick < SafeBreakableWeight)
	{
		return ECombatBasketballHitType::Breakable;
	}

	if (Pick < SafeBreakableWeight + SafeDeflectableWeight)
	{
		return ECombatBasketballHitType::Deflectable;
	}

	return ECombatBasketballHitType::Random;
}

ACombatFlyingBasketball* ACombatBasketballSpawner::SpawnBasketball()
{
	if (!GetWorld() || !BasketballClass)
	{
		return nullptr;
	}

	CleanupDeadBasketballs();
	if (MaxLiveBasketballs > 0 && LiveBasketballs.Num() >= MaxLiveBasketballs)
	{
		return nullptr;
	}

	AActor* SpawnTarget = GetTargetActor();
	const FVector TargetLocation = SpawnTarget ? SpawnTarget->GetActorLocation() : GetActorLocation();
	const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
	const FVector HorizontalOffset(FMath::Cos(Angle) * SpawnRadius, FMath::Sin(Angle) * SpawnRadius, 0.0f);
	const FVector SpawnLocation = TargetLocation + HorizontalOffset + FVector(0.0f, 0.0f, FMath::FRandRange(MinSpawnHeight, MaxSpawnHeight));
	const FRotator SpawnRotation = (TargetLocation - SpawnLocation).Rotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;

	ACombatFlyingBasketball* Basketball = GetWorld()->SpawnActor<ACombatFlyingBasketball>(BasketballClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (Basketball)
	{
		Basketball->SetHitType(PickBasketballType());
		Basketball->LaunchAtTarget(SpawnTarget);
		LiveBasketballs.Add(Basketball);
	}

	return Basketball;
}
