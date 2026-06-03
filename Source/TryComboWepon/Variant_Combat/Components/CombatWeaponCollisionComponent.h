// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "CombatWeaponCollisionComponent.generated.h"

class UPrimitiveComponent;
class USkeletalMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCombatWeaponDamageDealtSignature, float, Damage, const FVector&, ImpactPoint);

USTRUCT(BlueprintType)
struct FCombatWeaponCollisionBody
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Collision")
	FName WeaponId = TEXT("Katana");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Collision")
	TObjectPtr<UPrimitiveComponent> CollisionComponent = nullptr;
};

USTRUCT(BlueprintType)
struct FCombatWeaponTraceSegment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Collision")
	FName StartSocket = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Collision")
	FName EndSocket = NAME_None;
};

USTRUCT(BlueprintType)
struct FCombatWeaponCollisionProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Collision")
	FName WeaponId = TEXT("Katana");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Collision")
	TArray<FCombatWeaponTraceSegment> TraceSegments;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Collision", meta=(ClampMin=0, ClampMax=200, Units="cm"))
	float TraceRadius = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Collision", meta=(ClampMin=0, ClampMax=500, Units="cm"))
	float FallbackTraceDistance = 75.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Collision", meta=(ClampMin=0, ClampMax=100))
	float Damage = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Collision", meta=(ClampMin=0, ClampMax=1000, Units="cm/s"))
	float KnockbackImpulse = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Collision", meta=(ClampMin=0, ClampMax=1000, Units="cm/s"))
	float LaunchImpulse = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Collision")
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
};

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class UCombatWeaponCollisionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatWeaponCollisionComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Collision")
	FName DefaultWeaponId = TEXT("Katana");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Collision")
	TArray<FCombatWeaponCollisionProfile> WeaponProfiles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Collision")
	bool bDamageActorOnlyOncePerAttackWindow = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Collision|Debug")
	bool bDrawDebugTraces = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Collision|Debug")
	bool bDrawDebugCollisionBodies = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Collision|Debug")
	bool bDrawDebugOnlyWhenCollisionWindowActive = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Collision|Debug", meta=(ClampMin=0, ClampMax=10, Units="s"))
	float DebugCollisionBodyDrawDuration = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Collision|Debug", meta=(ClampMin=0, ClampMax=20))
	float DebugCollisionBodyThickness = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Collision|Debug")
	FColor DebugInactiveCollisionBodyColor = FColor::Cyan;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Collision|Debug")
	FColor DebugActiveCollisionBodyColor = FColor::Red;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Collision|Debug")
	FName DebugFallbackCollisionBodyComponentName = TEXT("KatanaHitBox");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Collision|Debug")
	bool bAutoRegisterDebugFallbackCollisionBody = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Collision|Debug")
	bool bPrintDebugMessages = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Collision|Debug", meta=(ClampMin=0, ClampMax=10, Units="s"))
	float DebugMessageDuration = 2.0f;

	UPROPERTY(BlueprintAssignable, Category="Weapon Collision")
	FCombatWeaponDamageDealtSignature OnDamageDealt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Collision|Bodies")
	TArray<FCombatWeaponCollisionBody> CollisionBodies;

	UFUNCTION(BlueprintCallable, Category="Weapon Collision")
	void ResetHitActors();

	UFUNCTION(BlueprintCallable, Category="Weapon Collision|Bodies")
	void RegisterCollisionBody(UPrimitiveComponent* CollisionComponent, FName WeaponId);

	UFUNCTION(BlueprintCallable, Category="Weapon Collision|Bodies")
	void ConfigureCollisionBody(UPrimitiveComponent* CollisionComponent);

	UFUNCTION(BlueprintCallable, Category="Weapon Collision|Bodies")
	void UnregisterCollisionBody(UPrimitiveComponent* CollisionComponent);

	UFUNCTION(BlueprintCallable, Category="Weapon Collision|Bodies")
	void BeginCollisionWindow(FName WeaponId);

	UFUNCTION(BlueprintCallable, Category="Weapon Collision|Bodies")
	void EndCollisionWindow();

	UFUNCTION(BlueprintCallable, Category="Weapon Collision")
	bool PerformAttackTrace(USkeletalMeshComponent* SourceMesh, FName WeaponId, FName FallbackSourceSocket);

	UFUNCTION(BlueprintCallable, Category="Weapon Collision|Debug")
	void DrawDebugCollisionBodies() const;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	const FCombatWeaponCollisionProfile* FindProfile(FName WeaponId) const;
	const FCombatWeaponCollisionProfile* GetActiveProfile() const;
	bool SweepSegment(TArray<FHitResult>& OutHits, USkeletalMeshComponent* SourceMesh, const FCombatWeaponCollisionProfile& Profile, const FVector& Start, const FVector& End) const;
	bool ApplyHit(const FHitResult& Hit, const FCombatWeaponCollisionProfile& Profile, TSet<TWeakObjectPtr<AActor>>& ActorsHitThisTrace);
	bool ApplyDamageToActor(AActor* HitActor, const FVector& ImpactPoint, const FVector& ImpactNormal, const FCombatWeaponCollisionProfile& Profile, TSet<TWeakObjectPtr<AActor>>& ActorsHitThisTrace);
	void SetCollisionBodiesEnabled(bool bEnabled, FName WeaponId);
	bool HasRegisteredCollisionBody(FName WeaponId) const;
	bool TryAutoRegisterFallbackCollisionBody(FName WeaponId);
	void ProcessActiveCollisionBodyOverlaps();
	void DrawDebugCollisionBody(const FCombatWeaponCollisionBody& Body) const;
	void PrintDebugMessage(const FString& Message, const FColor& Color) const;

	UFUNCTION()
	void HandleCollisionBodyBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	TSet<TWeakObjectPtr<AActor>> HitActorsThisAttackWindow;
	bool bCollisionWindowActive = false;
	FName ActiveCollisionWeaponId = NAME_None;
};
