// Copyright Epic Games, Inc. All Rights Reserved.

#include "CombatSummonMarker.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ACombatSummonMarker::ACombatSummonMarker()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->SetRelativeScale3D(FVector(0.35f, 0.35f, 0.08f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshAsset.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMeshAsset.Object);
	}
}

void ACombatSummonMarker::BeginPlay()
{
	Super::BeginPlay();

	if (GetWorld() && Lifetime > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(DestroyTimer, this, &ACombatSummonMarker::DestroyMarker, Lifetime, false);
	}
}

void ACombatSummonMarker::DestroyMarker()
{
	Destroy();
}
