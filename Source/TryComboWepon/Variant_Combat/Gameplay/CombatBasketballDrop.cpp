// Copyright Epic Games, Inc. All Rights Reserved.

#include "CombatBasketballDrop.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ACombatBasketballDrop::ACombatBasketballDrop()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	Mesh->SetSimulatePhysics(true);
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->SetRelativeScale3D(FVector(0.25f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshAsset.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMeshAsset.Object);
	}
}

void ACombatBasketballDrop::BeginPlay()
{
	Super::BeginPlay();

	if (Mesh && LaunchImpulse > 0.0f)
	{
		const FVector Direction = (FMath::VRand() + FVector::UpVector * 1.5f).GetSafeNormal();
		Mesh->AddImpulse(Direction * LaunchImpulse, NAME_None, true);
	}

	if (GetWorld() && Lifetime > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(DestroyTimer, this, &ACombatBasketballDrop::DestroyDrop, Lifetime, false);
	}
}

void ACombatBasketballDrop::DestroyDrop()
{
	Destroy();
}
