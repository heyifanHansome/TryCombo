// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CombatSummonMarker.generated.h"

class UStaticMeshComponent;

UCLASS(Blueprintable)
class ACombatSummonMarker : public AActor
{
	GENERATED_BODY()

public:
	ACombatSummonMarker();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Summon Marker", meta=(ClampMin=0, Units="s"))
	float Lifetime = 4.0f;

protected:
	virtual void BeginPlay() override;

	void DestroyMarker();

	FTimerHandle DestroyTimer;
};
