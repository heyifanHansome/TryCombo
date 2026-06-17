// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/CombatShadowClone.h"
#include "CombatCharacter.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Interfaces/CombatDamageable.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"

ACombatShadowClone::ACombatShadowClone()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	CloneMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CloneMesh"));
	CloneMesh->SetupAttachment(Root);
	CloneMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CloneMesh->SetGenerateOverlapEvents(false);
	CloneMesh->SetCanEverAffectNavigation(false);
}

void ACombatShadowClone::BeginPlay()
{
	Super::BeginPlay();

	if (DamageDelay >= 0.0f)
	{
		GetWorldTimerManager().SetTimer(DamageTimer, this, &ACombatShadowClone::PerformCloneDamage, DamageDelay, false);
	}

	GetWorldTimerManager().SetTimer(LifeTimer, this, &ACombatShadowClone::DestroyClone, LifeSeconds, false);
}

void ACombatShadowClone::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(DamageTimer);
	GetWorldTimerManager().ClearTimer(LifeTimer);

	Super::EndPlay(EndPlayReason);
}

void ACombatShadowClone::InitializeFromCharacter(ACombatCharacter* SourceCharacter)
{
	if (!SourceCharacter || !SourceCharacter->GetMesh() || !CloneMesh)
	{
		return;
	}

	SourceCharacterPtr = SourceCharacter;
	USkeletalMeshComponent* SourceMesh = SourceCharacter->GetMesh();

	CloneMesh->SetSkeletalMesh(SourceMesh->GetSkeletalMeshAsset());
	CloneMesh->SetAnimInstanceClass(SourceMesh->GetAnimClass());
	CloneMesh->SetRelativeTransform(SourceMesh->GetRelativeTransform());
	CloneMesh->SetLeaderPoseComponent(SourceMesh);

	for (int32 MaterialIndex = 0; MaterialIndex < SourceMesh->GetNumMaterials(); ++MaterialIndex)
	{
		CloneMesh->SetMaterial(MaterialIndex, SourceMesh->GetMaterial(MaterialIndex));
	}

	CopyWeaponVisuals(SourceCharacter);
	ApplyCloneVisuals();
}

void ACombatShadowClone::CopyWeaponVisuals(ACombatCharacter* SourceCharacter)
{
	TArray<UStaticMeshComponent*> SourceStaticMeshes;
	SourceCharacter->GetComponents<UStaticMeshComponent>(SourceStaticMeshes);

	for (UStaticMeshComponent* SourceStaticMesh : SourceStaticMeshes)
	{
		if (!SourceStaticMesh || !SourceStaticMesh->GetStaticMesh())
		{
			continue;
		}

		const FString ComponentName = SourceStaticMesh->GetName();
		if (!ComponentName.Contains(TEXT("Katana")) && !ComponentName.Contains(TEXT("Weapon")) && !ComponentName.Contains(TEXT("Sword")))
		{
			continue;
		}

		UStaticMeshComponent* CloneStaticMesh = NewObject<UStaticMeshComponent>(this);
		CloneStaticMesh->RegisterComponent();
		CloneStaticMesh->SetStaticMesh(SourceStaticMesh->GetStaticMesh());
		CloneStaticMesh->SetWorldTransform(SourceStaticMesh->GetComponentTransform());
		CloneStaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CloneStaticMesh->SetGenerateOverlapEvents(false);
		CloneStaticMesh->SetCanEverAffectNavigation(false);
		CloneStaticMesh->AttachToComponent(Root, FAttachmentTransformRules::KeepWorldTransform);

		for (int32 MaterialIndex = 0; MaterialIndex < SourceStaticMesh->GetNumMaterials(); ++MaterialIndex)
		{
			CloneStaticMesh->SetMaterial(MaterialIndex, SourceStaticMesh->GetMaterial(MaterialIndex));
		}

		CopiedWeaponComponents.Add(CloneStaticMesh);
	}
}

void ACombatShadowClone::ApplyCloneVisuals()
{
	SetActorScale3D(FVector(1.0f));

	if (CloneMesh)
	{
		CloneMesh->SetRenderCustomDepth(true);
	}

	for (USceneComponent* Component : CopiedWeaponComponents)
	{
		if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component))
		{
			Primitive->SetRenderCustomDepth(true);
		}
	}
}

void ACombatShadowClone::PerformCloneDamage()
{
	UWorld* World = GetWorld();
	ACombatCharacter* SourceCharacter = SourceCharacterPtr.Get();
	if (!World || !SourceCharacter)
	{
		return;
	}

	const FVector DamageCenter = GetActorLocation() + GetActorForwardVector() * DamageForwardOffset;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);
	ActorsToIgnore.Add(SourceCharacter);

	TArray<AActor*> HitActors;
	UKismetSystemLibrary::SphereOverlapActors(
		this,
		DamageCenter,
		DamageRadius,
		TArray<TEnumAsByte<EObjectTypeQuery>>(),
		nullptr,
		ActorsToIgnore,
		HitActors);

	for (AActor* HitActor : HitActors)
	{
		if (!HitActor || HitActor == SourceCharacter || HitActor->ActorHasTag(FName("Player")))
		{
			continue;
		}

		if (ICombatDamageable* Damageable = Cast<ICombatDamageable>(HitActor))
		{
			const FVector Direction = (HitActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
			Damageable->ApplyDamage(Damage, SourceCharacter, DamageCenter, Direction * 250.0f);
		}
	}
}

void ACombatShadowClone::DestroyClone()
{
	Destroy();
}
