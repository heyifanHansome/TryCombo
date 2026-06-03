// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_AttachWeapon.generated.h"

UENUM(BlueprintType)
enum class ECombatWeaponAttachTarget : uint8
{
	DrawnSocket UMETA(DisplayName="Drawn Socket"),
	SheathedSocket UMETA(DisplayName="Sheathed Socket"),
	CustomSocket UMETA(DisplayName="Custom Socket")
};

/**
 * Moves the visible weapon between hand and sheathed sockets during draw/sheathe montages.
 */
UCLASS()
class UAnimNotify_AttachWeapon : public UAnimNotify
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Attach")
	ECombatWeaponAttachTarget AttachTarget = ECombatWeaponAttachTarget::DrawnSocket;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Attach")
	FName CustomSocketName = NAME_None;

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
