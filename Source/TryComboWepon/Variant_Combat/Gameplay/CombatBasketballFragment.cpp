// Copyright Epic Games, Inc. All Rights Reserved.

#include "CombatBasketballFragment.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ACombatBasketballFragment::ACombatBasketballFragment()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	Mesh->SetSimulatePhysics(true);
	Mesh->SetCanEverAffectNavigation(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMeshAsset.Succeeded())
	{
		Mesh->SetStaticMesh(SphereMeshAsset.Object);
	}
}

void ACombatBasketballFragment::BeginPlay()
{
	Super::BeginPlay();

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(DestroyTimer, this, &ACombatBasketballFragment::DestroyFragment, Lifetime, false);
	}
}

void ACombatBasketballFragment::DestroyFragment()
{
	Destroy();
}

void ACombatBasketballFragment::LaunchFragment(const FVector& LinearImpulse, const FVector& AngularImpulse)
{
	if (!Mesh)
	{
		return;
	}

	Mesh->AddImpulse(LinearImpulse, NAME_None, true);
	Mesh->AddAngularImpulseInDegrees(AngularImpulse, NAME_None, true);
}
