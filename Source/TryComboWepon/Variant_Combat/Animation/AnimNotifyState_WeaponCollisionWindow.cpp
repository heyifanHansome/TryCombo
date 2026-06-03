// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimNotifyState_WeaponCollisionWindow.h"
#include "Components/CombatWeaponCollisionComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

void UAnimNotifyState_WeaponCollisionWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp || !MeshComp->GetOwner())
	{
		return;
	}

	if (UCombatWeaponCollisionComponent* WeaponCollision = MeshComp->GetOwner()->FindComponentByClass<UCombatWeaponCollisionComponent>())
	{
		WeaponCollision->BeginCollisionWindow(WeaponId);
	}
}

void UAnimNotifyState_WeaponCollisionWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp || !MeshComp->GetOwner())
	{
		return;
	}

	if (UCombatWeaponCollisionComponent* WeaponCollision = MeshComp->GetOwner()->FindComponentByClass<UCombatWeaponCollisionComponent>())
	{
		WeaponCollision->EndCollisionWindow();
	}
}

FString UAnimNotifyState_WeaponCollisionWindow::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Weapon Collision: %s"), *WeaponId.ToString());
}
