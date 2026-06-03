// Copyright Epic Games, Inc. All Rights Reserved.

#include "CombatHitTestNPC.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogCombatHitTestNPC, Log, All);

ACombatHitTestNPC::ACombatHitTestNPC()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
	GetCapsuleComponent()->SetCollisionObjectType(ECC_Pawn);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCapsuleComponent()->SetGenerateOverlapEvents(true);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Block);

	HitReceiver = CreateDefaultSubobject<UBoxComponent>(TEXT("HitReceiver"));
	HitReceiver->SetupAttachment(GetCapsuleComponent());
	HitReceiver->SetBoxExtent(FVector(55.0f, 55.0f, 95.0f));
	HitReceiver->SetRelativeLocation(FVector::ZeroVector);
	HitReceiver->SetCollisionObjectType(ECC_Pawn);
	HitReceiver->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HitReceiver->SetGenerateOverlapEvents(true);
	HitReceiver->SetCollisionResponseToAllChannels(ECR_Ignore);
	HitReceiver->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);

	DebugBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DebugBody"));
	DebugBody->SetupAttachment(GetCapsuleComponent());
	DebugBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DebugBody->SetSimulatePhysics(false);
	DebugBody->SetRelativeLocation(FVector(0.0f, 0.0f, -10.0f));
	DebugBody->SetRelativeScale3D(FVector(0.65f, 0.65f, 1.65f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> BodyMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (BodyMesh.Succeeded())
	{
		DebugBody->SetStaticMesh(BodyMesh.Object);
	}

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->MaxWalkSpeed = 0.0f;
	GetCharacterMovement()->GravityScale = 0.0f;
	GetCharacterMovement()->SetMovementMode(MOVE_None);
}

void ACombatHitTestNPC::BeginPlay()
{
	Super::BeginPlay();
	CurrentHP = MaxHP;
	GetCharacterMovement()->GravityScale = 0.0f;
	GetCharacterMovement()->SetMovementMode(MOVE_None);
}

void ACombatHitTestNPC::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bFacePlayer)
	{
		return;
	}

	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn)
	{
		return;
	}

	const FVector ToPlayer = PlayerPawn->GetActorLocation() - GetActorLocation();
	if (ToPlayer.SizeSquared2D() <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	SetActorRotation(FRotator(0.0f, ToPlayer.Rotation().Yaw, 0.0f));
}

void ACombatHitTestNPC::ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse)
{
	CurrentHP = FMath::Clamp(CurrentHP - Damage, 0.0f, MaxHP);

	const FVector SafeImpactPoint = DamageLocation.IsNearlyZero() ? GetActorLocation() : DamageLocation;
	const FVector SafeImpulse = DamageImpulse.IsNearlyZero() ? FVector::ZeroVector : DamageImpulse;

	UE_LOG(LogCombatHitTestNPC, Warning, TEXT("HitTestNPC damaged. Damage=%.2f RemainingHP=%.2f Causer=%s Impact=%s Impulse=%s"),
		Damage,
		CurrentHP,
		*GetNameSafe(DamageCauser),
		*SafeImpactPoint.ToCompactString(),
		*SafeImpulse.ToCompactString());

	if (bDrawDebugOnDamage && GetWorld())
	{
		DrawDebugSphere(GetWorld(), SafeImpactPoint, 18.0f, 16, FColor::Red, false, DebugDrawDuration, 0, 2.5f);
		DrawDebugDirectionalArrow(GetWorld(), SafeImpactPoint, SafeImpactPoint + SafeImpulse.GetClampedToMaxSize(120.0f), 24.0f, FColor::Yellow, false, DebugDrawDuration, 0, 2.5f);
		DrawDebugString(GetWorld(), GetActorLocation() + FVector(0.0f, 0.0f, 130.0f), FString::Printf(TEXT("Hit %.1f / HP %.1f"), Damage, CurrentHP), nullptr, FColor::Red, DebugDrawDuration, true);
	}

	if (bLaunchOnDamage && !SafeImpulse.IsNearlyZero())
	{
		LaunchCharacter(SafeImpulse, true, true);
	}

	OnDamaged.Broadcast(Damage, SafeImpactPoint, CurrentHP);

	if (CurrentHP <= 0.0f)
	{
		HandleDeath();
	}
}

void ACombatHitTestNPC::HandleDeath()
{
	UE_LOG(LogCombatHitTestNPC, Warning, TEXT("HitTestNPC reached 0 HP."));

	if (bResetHealthOnDeath)
	{
		CurrentHP = MaxHP;
	}
}

void ACombatHitTestNPC::ApplyHealing(float Healing, AActor* Healer)
{
	CurrentHP = FMath::Clamp(CurrentHP + Healing, 0.0f, MaxHP);
}

void ACombatHitTestNPC::NotifyDanger(const FVector& DangerLocation, AActor* DangerSource)
{
	// Test NPC does not evade; it only gives collision feedback.
}
