// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimNotify_AttachWeapon.h"
#include "CombatCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

void UAnimNotify_AttachWeapon::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp)
	{
		return;
	}

	ACombatCharacter* CombatCharacter = Cast<ACombatCharacter>(MeshComp->GetOwner());
	if (!CombatCharacter)
	{
		return;
	}

	switch (AttachTarget)
	{
	case ECombatWeaponAttachTarget::DrawnSocket:
		CombatCharacter->AttachWeaponToDrawnSocket();
		break;
	case ECombatWeaponAttachTarget::SheathedSocket:
		CombatCharacter->AttachWeaponToSheathedSocket();
		break;
	case ECombatWeaponAttachTarget::CustomSocket:
		CombatCharacter->AttachWeaponToSocket(CustomSocketName);
		break;
	default:
		break;
	}
}

FString UAnimNotify_AttachWeapon::GetNotifyName_Implementation() const
{
	switch (AttachTarget)
	{
	case ECombatWeaponAttachTarget::DrawnSocket:
		return TEXT("Attach Weapon: Drawn");
	case ECombatWeaponAttachTarget::SheathedSocket:
		return TEXT("Attach Weapon: Sheathed");
	case ECombatWeaponAttachTarget::CustomSocket:
		return FString::Printf(TEXT("Attach Weapon: %s"), *CustomSocketName.ToString());
	default:
		return TEXT("Attach Weapon");
	}
}
