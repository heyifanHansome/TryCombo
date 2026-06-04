// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CombatBasketballDrop.generated.h"

class UStaticMeshComponent;

UCLASS(Blueprintable)
class ACombatBasketballDrop : public AActor
{
	GENERATED_BODY()

public:
	ACombatBasketballDrop();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drop", meta=(ClampMin=0, Units="s"))
	float Lifetime = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drop", meta=(ClampMin=0, Units="cm/s"))
	float LaunchImpulse = 350.0f;

protected:
	virtual void BeginPlay() override;

	void DestroyDrop();

	FTimerHandle DestroyTimer;
};
