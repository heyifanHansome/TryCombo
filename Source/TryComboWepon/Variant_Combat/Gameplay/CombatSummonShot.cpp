// Copyright Epic Games, Inc. All Rights Reserved.

#include "CombatSummonShot.h"
#include "CombatDamageable.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ACombatSummonShot::ACombatSummonShot()
{
	PrimaryActorTick.bCanEverTick = false;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(28.0f);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetGenerateOverlapEvents(true);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Collision->SetCanEverAffectNavigation(false);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->SetRelativeScale3D(FVector(0.18f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMeshAsset.Succeeded())
	{
		Mesh->SetStaticMesh(SphereMeshAsset.Object);
	}

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = Collision;
	ProjectileMovement->InitialSpeed = Speed;
	ProjectileMovement->MaxSpeed = Speed;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
}

void ACombatSummonShot::BeginPlay()
{
	Super::BeginPlay();

	Collision->OnComponentBeginOverlap.AddUniqueDynamic(this, &ACombatSummonShot::HandleOverlap);

	if (GetWorld() && Lifetime > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(DestroyTimer, this, &ACombatSummonShot::DestroyShot, Lifetime, false);
	}
}

void ACombatSummonShot::FireAtTarget(AActor* NewTargetActor)
{
	TargetActor = NewTargetActor;
	if (!ProjectileMovement)
	{
		return;
	}

	ProjectileMovement->InitialSpeed = Speed;
	ProjectileMovement->MaxSpeed = Speed;

	const FVector TargetLocation = NewTargetActor ? NewTargetActor->GetActorLocation() : (GetActorLocation() + GetActorForwardVector() * 100.0f);
	const FVector Direction = (TargetLocation - GetActorLocation()).GetSafeNormal();
	ProjectileMovement->Velocity = Direction * Speed;

	if (bUseHoming && NewTargetActor && NewTargetActor->GetRootComponent())
	{
		ProjectileMovement->bIsHomingProjectile = true;
		ProjectileMovement->HomingTargetComponent = NewTargetActor->GetRootComponent();
		ProjectileMovement->HomingAccelerationMagnitude = HomingAcceleration;
	}
}

void ACombatSummonShot::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == this)
	{
		return;
	}

	if (TargetActor.IsValid() && OtherActor != TargetActor.Get())
	{
		return;
	}

	if (ICombatDamageable* Damageable = Cast<ICombatDamageable>(OtherActor))
	{
		const FVector ImpactPoint = SweepResult.ImpactPoint.IsNearlyZero() ? GetActorLocation() : FVector(SweepResult.ImpactPoint);
		Damageable->ApplyDamage(Damage, GetOwner(), ImpactPoint, FVector::ZeroVector);
	}

	Destroy();
}

void ACombatSummonShot::DestroyShot()
{
	Destroy();
}
