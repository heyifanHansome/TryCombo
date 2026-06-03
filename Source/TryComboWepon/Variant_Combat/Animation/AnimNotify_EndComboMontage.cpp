// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimNotify_EndComboMontage.h"
#include "CombatCharacter.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_EndComboMontage::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp)
	{
		return;
	}

	if (ACombatCharacter* CombatCharacter = Cast<ACombatCharacter>(MeshComp->GetOwner()))
	{
		CombatCharacter->EndComboMontage(BlendOutTime);
	}
}

FString UAnimNotify_EndComboMontage::GetNotifyName_Implementation() const
{
	return TEXT("End Combo Montage");
}
