// Copyright Epic Games, Inc. All Rights Reserved.

#include "CombatWeaponCollisionComponent.h"
#include "CombatDamageable.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"

UCombatWeaponCollisionComponent::UCombatWeaponCollisionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	FCombatWeaponCollisionProfile KatanaProfile;
	KatanaProfile.WeaponId = TEXT("Katana");
	KatanaProfile.TraceRadius = 75.0f;
	KatanaProfile.FallbackTraceDistance = 75.0f;
	KatanaProfile.ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
	KatanaProfile.ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));
	WeaponProfiles.Add(KatanaProfile);
}

void UCombatWeaponCollisionComponent::ResetHitActors()
{
	HitActorsThisAttackWindow.Reset();
}

void UCombatWeaponCollisionComponent::RegisterCollisionBody(UPrimitiveComponent* CollisionComponent, FName WeaponId)
{
	if (!CollisionComponent)
	{
		return;
	}

	for (FCombatWeaponCollisionBody& Body : CollisionBodies)
	{
		if (Body.CollisionComponent == CollisionComponent)
		{
			Body.WeaponId = WeaponId == NAME_None ? DefaultWeaponId : WeaponId;
			return;
		}
	}

	FCombatWeaponCollisionBody NewBody;
	NewBody.CollisionComponent = CollisionComponent;
	NewBody.WeaponId = WeaponId == NAME_None ? DefaultWeaponId : WeaponId;
	CollisionBodies.Add(NewBody);

	ConfigureCollisionBody(CollisionComponent);
	CollisionComponent->OnComponentBeginOverlap.AddUniqueDynamic(this, &UCombatWeaponCollisionComponent::HandleCollisionBodyBeginOverlap);
}

void UCombatWeaponCollisionComponent::ConfigureCollisionBody(UPrimitiveComponent* CollisionComponent)
{
	if (!CollisionComponent)
	{
		return;
	}

	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CollisionComponent->SetGenerateOverlapEvents(true);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	CollisionComponent->SetCanEverAffectNavigation(false);
}

void UCombatWeaponCollisionComponent::UnregisterCollisionBody(UPrimitiveComponent* CollisionComponent)
{
	if (!CollisionComponent)
	{
		return;
	}

	CollisionComponent->OnComponentBeginOverlap.RemoveAll(this);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CollisionBodies.RemoveAll([CollisionComponent](const FCombatWeaponCollisionBody& Body)
	{
		return Body.CollisionComponent == CollisionComponent;
	});
}

void UCombatWeaponCollisionComponent::BeginCollisionWindow(FName WeaponId)
{
	ActiveCollisionWeaponId = WeaponId == NAME_None ? DefaultWeaponId : WeaponId;
	bCollisionWindowActive = true;
	ResetHitActors();
	SetCollisionBodiesEnabled(true, ActiveCollisionWeaponId);
}

void UCombatWeaponCollisionComponent::EndCollisionWindow()
{
	SetCollisionBodiesEnabled(false, ActiveCollisionWeaponId);
	bCollisionWindowActive = false;
	ActiveCollisionWeaponId = NAME_None;
}

bool UCombatWeaponCollisionComponent::PerformAttackTrace(USkeletalMeshComponent* SourceMesh, FName WeaponId, FName FallbackSourceSocket)
{
	if (!SourceMesh || !SourceMesh->GetOwner() || !GetWorld())
	{
		return false;
	}

	const FCombatWeaponCollisionProfile* Profile = FindProfile(WeaponId);
	if (!Profile)
	{
		Profile = FindProfile(DefaultWeaponId);
	}

	if (!Profile)
	{
		return false;
	}

	TArray<FHitResult> Hits;
	TSet<TWeakObjectPtr<AActor>> ActorsHitThisTrace;
	bool bHitAnyDamageable = false;
	bool bUsedConfiguredSegment = false;

	for (const FCombatWeaponTraceSegment& Segment : Profile->TraceSegments)
	{
		if (Segment.StartSocket == NAME_None || Segment.EndSocket == NAME_None)
		{
			continue;
		}

		bUsedConfiguredSegment = true;
		SweepSegment(Hits, SourceMesh, *Profile, SourceMesh->GetSocketLocation(Segment.StartSocket), SourceMesh->GetSocketLocation(Segment.EndSocket));
	}

	if (!bUsedConfiguredSegment)
	{
		const FVector TraceStart = FallbackSourceSocket != NAME_None ? SourceMesh->GetSocketLocation(FallbackSourceSocket) : SourceMesh->GetOwner()->GetActorLocation();
		const FVector TraceEnd = TraceStart + (SourceMesh->GetOwner()->GetActorForwardVector() * Profile->FallbackTraceDistance);
		SweepSegment(Hits, SourceMesh, *Profile, TraceStart, TraceEnd);
	}

	for (const FHitResult& Hit : Hits)
	{
		bHitAnyDamageable |= ApplyHit(Hit, *Profile, ActorsHitThisTrace);
	}

	return bHitAnyDamageable;
}

void UCombatWeaponCollisionComponent::BeginPlay()
{
	Super::BeginPlay();

	for (FCombatWeaponCollisionBody& Body : CollisionBodies)
	{
		if (!Body.CollisionComponent)
		{
			continue;
		}

		if (Body.WeaponId == NAME_None)
		{
			Body.WeaponId = DefaultWeaponId;
		}

		ConfigureCollisionBody(Body.CollisionComponent);
		Body.CollisionComponent->OnComponentBeginOverlap.AddUniqueDynamic(this, &UCombatWeaponCollisionComponent::HandleCollisionBodyBeginOverlap);
	}
}

const FCombatWeaponCollisionProfile* UCombatWeaponCollisionComponent::FindProfile(FName WeaponId) const
{
	const FName ProfileId = WeaponId == NAME_None ? DefaultWeaponId : WeaponId;
	for (const FCombatWeaponCollisionProfile& Profile : WeaponProfiles)
	{
		if (Profile.WeaponId == ProfileId)
		{
			return &Profile;
		}
	}

	return nullptr;
}

const FCombatWeaponCollisionProfile* UCombatWeaponCollisionComponent::GetActiveProfile() const
{
	const FCombatWeaponCollisionProfile* Profile = FindProfile(ActiveCollisionWeaponId);
	return Profile ? Profile : FindProfile(DefaultWeaponId);
}

bool UCombatWeaponCollisionComponent::SweepSegment(TArray<FHitResult>& OutHits, USkeletalMeshComponent* SourceMesh, const FCombatWeaponCollisionProfile& Profile, const FVector& Start, const FVector& End) const
{
	if (!SourceMesh || !SourceMesh->GetOwner())
	{
		return false;
	}

	FCollisionObjectQueryParams ObjectParams;
	for (const TEnumAsByte<EObjectTypeQuery>& ObjectType : Profile.ObjectTypes)
	{
		const ECollisionChannel Channel = UEngineTypes::ConvertToCollisionChannel(ObjectType.GetValue());
		if (Channel != ECC_MAX)
		{
			ObjectParams.AddObjectTypesToQuery(Channel);
		}
	}

	if (!ObjectParams.IsValid())
	{
		ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
		ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CombatWeaponCollision), false, SourceMesh->GetOwner());
	QueryParams.AddIgnoredActor(SourceMesh->GetOwner());

	FCollisionShape CollisionShape;
	CollisionShape.SetSphere(Profile.TraceRadius);

	TArray<FHitResult> SegmentHits;
	const bool bHit = GetWorld()->SweepMultiByObjectType(SegmentHits, Start, End, FQuat::Identity, ObjectParams, CollisionShape, QueryParams);
	OutHits.Append(SegmentHits);

	if (bDrawDebugTraces)
	{
		const FColor DebugColor = bHit ? FColor::Red : FColor::Green;
		DrawDebugLine(GetWorld(), Start, End, DebugColor, false, 1.0f, 0, 1.5f);
		DrawDebugSphere(GetWorld(), Start, Profile.TraceRadius, 12, DebugColor, false, 1.0f);
		DrawDebugSphere(GetWorld(), End, Profile.TraceRadius, 12, DebugColor, false, 1.0f);
	}

	return bHit;
}

bool UCombatWeaponCollisionComponent::ApplyHit(const FHitResult& Hit, const FCombatWeaponCollisionProfile& Profile, TSet<TWeakObjectPtr<AActor>>& ActorsHitThisTrace)
{
	return ApplyDamageToActor(Hit.GetActor(), Hit.ImpactPoint, Hit.ImpactNormal, Profile, ActorsHitThisTrace);
}

bool UCombatWeaponCollisionComponent::ApplyDamageToActor(AActor* HitActor, const FVector& ImpactPoint, const FVector& ImpactNormal, const FCombatWeaponCollisionProfile& Profile, TSet<TWeakObjectPtr<AActor>>& ActorsHitThisTrace)
{
	if (!HitActor || HitActor == GetOwner())
	{
		return false;
	}

	TWeakObjectPtr<AActor> HitActorPtr(HitActor);
	if (ActorsHitThisTrace.Contains(HitActorPtr))
	{
		return false;
	}

	if (bDamageActorOnlyOncePerAttackWindow && HitActorsThisAttackWindow.Contains(HitActorPtr))
	{
		return false;
	}

	ICombatDamageable* Damageable = Cast<ICombatDamageable>(HitActor);
	if (!Damageable)
	{
		return false;
	}

	ActorsHitThisTrace.Add(HitActorPtr);
	HitActorsThisAttackWindow.Add(HitActorPtr);

	const FVector SafeImpactNormal = ImpactNormal.IsNearlyZero() ? (HitActor->GetActorLocation() - GetOwner()->GetActorLocation()).GetSafeNormal() : ImpactNormal;
	const FVector SafeImpactPoint = ImpactPoint.IsNearlyZero() ? HitActor->GetActorLocation() : ImpactPoint;
	const FVector Impulse = (SafeImpactNormal * -Profile.KnockbackImpulse) + (FVector::UpVector * Profile.LaunchImpulse);
	Damageable->ApplyDamage(Profile.Damage, GetOwner(), SafeImpactPoint, Impulse);
	OnDamageDealt.Broadcast(Profile.Damage, SafeImpactPoint);

	return true;
}

void UCombatWeaponCollisionComponent::SetCollisionBodiesEnabled(bool bEnabled, FName WeaponId)
{
	for (const FCombatWeaponCollisionBody& Body : CollisionBodies)
	{
		if (!Body.CollisionComponent || Body.WeaponId != WeaponId)
		{
			continue;
		}

		if (bEnabled)
		{
			ConfigureCollisionBody(Body.CollisionComponent);
		}

		Body.CollisionComponent->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	}
}

void UCombatWeaponCollisionComponent::HandleCollisionBodyBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bCollisionWindowActive || !OverlappedComponent || !OtherActor || OtherActor == GetOwner())
	{
		return;
	}

	bool bRegisteredActiveBody = false;
	for (const FCombatWeaponCollisionBody& Body : CollisionBodies)
	{
		if (Body.CollisionComponent == OverlappedComponent && Body.WeaponId == ActiveCollisionWeaponId)
		{
			bRegisteredActiveBody = true;
			break;
		}
	}

	if (!bRegisteredActiveBody)
	{
		return;
	}

	const FCombatWeaponCollisionProfile* Profile = GetActiveProfile();
	if (!Profile)
	{
		return;
	}

	TSet<TWeakObjectPtr<AActor>> ActorsHitThisOverlap;
	const bool bHasSweepImpact = bFromSweep || SweepResult.bBlockingHit;
	const FVector ImpactPoint = bHasSweepImpact ? FVector(SweepResult.ImpactPoint) : OverlappedComponent->GetComponentLocation();
	const FVector ImpactNormal = bHasSweepImpact ? FVector(SweepResult.ImpactNormal) : FVector::ZeroVector;
	ApplyDamageToActor(OtherActor, ImpactPoint, ImpactNormal, *Profile, ActorsHitThisOverlap);
}
