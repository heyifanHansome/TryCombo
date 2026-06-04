// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CombatBasketballFragment.generated.h"

class UStaticMeshComponent;

UCLASS(Blueprintable)
class ACombatBasketballFragment : public AActor
{
	GENERATED_BODY()

public:
	ACombatBasketballFragment();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment")
	float Lifetime = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment")
	float SpinImpulse = 50000.0f;

	void LaunchFragment(const FVector& LinearImpulse, const FVector& AngularImpulse);

protected:
	virtual void BeginPlay() override;

	void DestroyFragment();

	FTimerHandle DestroyTimer;
};
