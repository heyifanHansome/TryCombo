// Copyright Epic Games, Inc. All Rights Reserved.

#include "CombatFlyingBasketball.h"
#include "CombatBasketballDrop.h"
#include "CombatBasketballFragment.h"
#include "CombatPhotoTarget.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Variant_Combat/CombatGameMode.h"

ACombatFlyingBasketball::ACombatFlyingBasketball()
{
	PrimaryActorTick.bCanEverTick = false;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(32.0f);
	ConfigureCollision();

	AttackCollision = CreateDefaultSubobject<USphereComponent>(TEXT("AttackCollision"));
	AttackCollision->SetupAttachment(Collision);
	AttackCollision->InitSphereRadius(AttackHitRadius);
	ConfigureCollision();

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->SetRelativeScale3D(FVector(0.65f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMeshAsset.Succeeded())
	{
		Mesh->SetStaticMesh(SphereMeshAsset.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterialAsset(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BasicMaterialAsset.Succeeded())
	{
		Mesh->SetMaterial(0, BasicMaterialAsset.Object);
	}

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = Collision;
	ProjectileMovement->InitialSpeed = FlightSpeed;
	ProjectileMovement->MaxSpeed = FlightSpeed;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;

	FragmentClass = ACombatBasketballFragment::StaticClass();
	DropClass = ACombatBasketballDrop::StaticClass();
}

void ACombatFlyingBasketball::BeginPlay()
{
	Super::BeginPlay();

	if (const ACombatGameMode* CombatGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACombatGameMode>() : nullptr)
	{
		if (!CombatGameMode->IsCombatAIEnabled())
		{
			Destroy();
			return;
		}
	}

	CurrentHP = MaxHP;
	ConfigureCollision();
	Collision->OnComponentBeginOverlap.AddUniqueDynamic(this, &ACombatFlyingBasketball::HandleOverlap);
	ApplyTypeVisuals();

	LaunchAtTarget(TargetActor);
}

void ACombatFlyingBasketball::ConfigureCollision()
{
	if (!Collision)
	{
		return;
	}

	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetGenerateOverlapEvents(true);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Collision->SetCanEverAffectNavigation(false);

	if (AttackCollision)
	{
		AttackCollision->SetSphereRadius(AttackHitRadius);
		AttackCollision->SetCollisionObjectType(ECC_WorldDynamic);
		AttackCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		AttackCollision->SetGenerateOverlapEvents(true);
		AttackCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
		AttackCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
		AttackCollision->SetCanEverAffectNavigation(false);
	}
}

void ACombatFlyingBasketball::LaunchAtTarget(AActor* NewTargetActor)
{
	if (const ACombatGameMode* CombatGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACombatGameMode>() : nullptr)
	{
		if (!CombatGameMode->IsCombatAIEnabled())
		{
			Destroy();
			return;
		}
	}

	TargetActor = NewTargetActor;
	if (!TargetActor)
	{
		TargetActor = UGameplayStatics::GetPlayerPawn(this, 0);
	}

	if (!ProjectileMovement)
	{
		return;
	}

	ProjectileMovement->InitialSpeed = FlightSpeed;
	ProjectileMovement->MaxSpeed = FlightSpeed;

	if (TargetActor)
	{
		const FVector Direction = (TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		ProjectileMovement->Velocity = Direction * FlightSpeed;

		if (HomingAcceleration > 0.0f && TargetActor->GetRootComponent())
		{
			ProjectileMovement->bIsHomingProjectile = true;
			ProjectileMovement->HomingTargetComponent = TargetActor->GetRootComponent();
			ProjectileMovement->HomingAccelerationMagnitude = HomingAcceleration;
		}
	}
	else
	{
		ProjectileMovement->Velocity = GetActorForwardVector() * FlightSpeed;
	}
}

void ACombatFlyingBasketball::SetHitType(ECombatBasketballHitType NewHitType)
{
	HitType = NewHitType;
	ApplyTypeVisuals();
}

FLinearColor ACombatFlyingBasketball::GetTypeColor() const
{
	switch (HitType)
	{
	case ECombatBasketballHitType::Breakable:
		return BreakableColor;
	case ECombatBasketballHitType::Deflectable:
		return DeflectableColor;
	case ECombatBasketballHitType::Random:
	default:
		return RandomColor;
	}
}

void ACombatFlyingBasketball::ApplyTypeVisuals()
{
	if (!Mesh)
	{
		return;
	}

	if (!DynamicMaterial)
	{
		DynamicMaterial = Mesh->CreateAndSetMaterialInstanceDynamic(0);
	}

	if (DynamicMaterial)
	{
		const FLinearColor TypeColor = GetTypeColor();
		DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), TypeColor);
		DynamicMaterial->SetVectorParameterValue(TEXT("Color"), TypeColor);
		DynamicMaterial->SetVectorParameterValue(TEXT("TintColor"), TypeColor);
		DynamicMaterial->SetVectorParameterValue(TEXT("BodyColor"), TypeColor);
		DynamicMaterial->SetVectorParameterValue(TEXT("BasketballColor"), TypeColor);
	}
}

ACombatPhotoTarget* ACombatFlyingBasketball::FindNearestPhotoTarget() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	ACombatPhotoTarget* BestTarget = nullptr;
	float BestDistanceSquared = FMath::Square(DeflectTargetSearchRadius);

	for (TActorIterator<ACombatPhotoTarget> It(World); It; ++It)
	{
		ACombatPhotoTarget* Candidate = *It;
		if (!Candidate)
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(GetActorLocation(), Candidate->GetActorLocation());
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestTarget = Candidate;
		}
	}

	return BestTarget;
}

void ACombatFlyingBasketball::ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse)
{
	if (bDead)
	{
		return;
	}

	CurrentHP = FMath::Clamp(CurrentHP - Damage, 0.0f, MaxHP);

	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, DamageLocation.IsNearlyZero() ? GetActorLocation() : DamageLocation);
	}

	if (HitType == ECombatBasketballHitType::Breakable)
	{
		SplitAndDestroy(DamageLocation, DamageImpulse, DamageCauser);
		return;
	}

	if (HitType == ECombatBasketballHitType::Deflectable)
	{
		DeflectFromHit(DamageLocation, DamageImpulse, DamageCauser);
		return;
	}

	if (FMath::FRand() <= BreakChance || CurrentHP <= 0.0f)
	{
		SplitAndDestroy(DamageLocation, DamageImpulse, DamageCauser);
		return;
	}

	if (FMath::FRand() <= DeflectChance)
	{
		DeflectFromHit(DamageLocation, DamageImpulse, DamageCauser);
	}
}

void ACombatFlyingBasketball::HandleDeath()
{
	SplitAndDestroy(GetActorLocation(), FVector::ZeroVector, nullptr);
}

void ACombatFlyingBasketball::ApplyHealing(float Healing, AActor* Healer)
{
	CurrentHP = FMath::Clamp(CurrentHP + Healing, 0.0f, MaxHP);
}

void ACombatFlyingBasketball::NotifyDanger(const FVector& DangerLocation, AActor* DangerSource)
{
	// Basketballs do not dodge incoming attacks.
}

void ACombatFlyingBasketball::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bDead || !OtherActor || OtherActor == this)
	{
		return;
	}

	if (const ACombatGameMode* CombatGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACombatGameMode>() : nullptr)
	{
		if (!CombatGameMode->IsCombatAIEnabled())
		{
			Destroy();
			return;
		}
	}

	if (TryTriggerPhotoTarget(OtherActor))
	{
		return;
	}

	if (!Cast<ACharacter>(OtherActor) || OtherComp != OtherActor->GetRootComponent())
	{
		return;
	}

	if (ICombatDamageable* Damageable = Cast<ICombatDamageable>(OtherActor))
	{
		FVector ImpactPoint = FVector(SweepResult.ImpactPoint);
		if (ImpactPoint.IsNearlyZero())
		{
			ImpactPoint = GetActorLocation();
		}

		FVector Direction = (OtherActor->GetActorLocation() - GetActorLocation());
		Direction.Z = 0.0f;
		Damageable->ApplyDamage(PlayerDamage, this, ImpactPoint, Direction.GetSafeNormal() * PlayerKnockback);
	}

	if (PlayerImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PlayerImpactSound, GetActorLocation());
	}

	Destroy();
}

FVector ACombatFlyingBasketball::GetSplitAxis(const FVector& DamageLocation, const FVector& DamageImpulse, AActor* DamageCauser) const
{
	if (!DamageImpulse.IsNearlyZero())
	{
		const FVector Axis = FVector::CrossProduct(DamageImpulse.GetSafeNormal(), FVector::UpVector).GetSafeNormal();
		if (!Axis.IsNearlyZero())
		{
			return Axis;
		}
	}

	if (DamageCauser)
	{
		const FVector FromAttacker = (GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal();
		const FVector Axis = FVector::CrossProduct(FromAttacker, FVector::UpVector).GetSafeNormal();
		if (!Axis.IsNearlyZero())
		{
			return Axis;
		}
	}

	return GetActorRightVector();
}

void ACombatFlyingBasketball::DeflectFromHit(const FVector& DamageLocation, const FVector& DamageImpulse, AActor* DamageCauser)
{
	if (!ProjectileMovement)
	{
		return;
	}

	ProjectileMovement->bIsHomingProjectile = false;
	ProjectileMovement->HomingTargetComponent = nullptr;

	ACombatPhotoTarget* PhotoTarget = bDeflectTowardPhotoTarget ? FindNearestPhotoTarget() : nullptr;
	FVector DeflectDirection = FVector::ZeroVector;
	if (PhotoTarget)
	{
		FVector AdjustedLocation = GetActorLocation();
		AdjustedLocation.Z = PhotoTarget->GetActorLocation().Z + DeflectPlaneHeightOffset;
		SetActorLocation(AdjustedLocation, false, nullptr, ETeleportType::TeleportPhysics);

		DeflectDirection = PhotoTarget->GetActorLocation() - AdjustedLocation;
		DeflectDirection.Z = 0.0f;
	}

	if (DeflectDirection.IsNearlyZero())
	{
		DeflectDirection = DamageImpulse.GetSafeNormal();
	}

	if (DeflectDirection.IsNearlyZero() && DamageCauser)
	{
		DeflectDirection = (GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal();
	}

	if (DeflectDirection.IsNearlyZero())
	{
		DeflectDirection = GetActorForwardVector();
	}

	DeflectDirection.Z = 0.0f;
	if (DeflectDirection.IsNearlyZero())
	{
		DeflectDirection = FVector::ForwardVector;
	}

	ProjectileMovement->Velocity = DeflectDirection.GetSafeNormal() * DeflectSpeed;
	ProjectileMovement->MaxSpeed = FMath::Max(ProjectileMovement->MaxSpeed, DeflectSpeed);
	bDeflected = true;

	if (DeflectSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DeflectSound, DamageLocation.IsNearlyZero() ? GetActorLocation() : DamageLocation);
	}
}

bool ACombatFlyingBasketball::TryTriggerPhotoTarget(AActor* OtherActor)
{
	if (!bDeflected || !OtherActor)
	{
		return false;
	}

	ACombatPhotoTarget* PhotoTarget = Cast<ACombatPhotoTarget>(OtherActor);
	if (!PhotoTarget)
	{
		return false;
	}

	PhotoTarget->TriggerBasketballPopup();
	if (PlayerImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PlayerImpactSound, GetActorLocation());
	}

	Destroy();
	return true;
}

void ACombatFlyingBasketball::TrySpawnDrop()
{
	if (!DropClass || !GetWorld() || FMath::FRand() > DropChance)
	{
		return;
	}

	GetWorld()->SpawnActor<ACombatBasketballDrop>(DropClass, GetActorLocation(), FRotator::ZeroRotator);
}

void ACombatFlyingBasketball::SplitAndDestroy(const FVector& DamageLocation, const FVector& DamageImpulse, AActor* DamageCauser)
{
	if (bDead)
	{
		return;
	}

	bDead = true;

	if (BreakSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BreakSound, GetActorLocation());
	}

	TrySpawnDrop();

	if (bSplitOnDeath && FragmentClass && GetWorld())
	{
		const FVector SplitAxis = GetSplitAxis(DamageLocation, DamageImpulse, DamageCauser);
		const FVector BaseLocation = GetActorLocation();
		const FRotator BaseRotation = GetActorRotation();

		for (int32 Index = 0; Index < 2; ++Index)
		{
			const float Sign = Index == 0 ? 1.0f : -1.0f;
			const FVector SpawnLocation = BaseLocation + (SplitAxis * FragmentSpawnOffset * Sign);
			ACombatBasketballFragment* Fragment = GetWorld()->SpawnActor<ACombatBasketballFragment>(FragmentClass, SpawnLocation, BaseRotation);
			if (!Fragment)
			{
				continue;
			}

			Fragment->SetActorScale3D(GetActorScale3D() * FragmentScale);
			const FVector OutwardImpulse = ((SplitAxis * Sign) + FVector::UpVector * 0.35f).GetSafeNormal() * FragmentImpulse;
			const FVector AngularImpulse = FVector::CrossProduct(SplitAxis * Sign, FVector::UpVector).GetSafeNormal() * Fragment->SpinImpulse;
			Fragment->LaunchFragment(OutwardImpulse, AngularImpulse);
		}
	}

	Destroy();
}
