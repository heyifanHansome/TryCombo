// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimNotifyState_ComboInputWindow.h"
#include "CombatCharacter.h"
#include "Animation/AnimNotifyQueue.h"
#include "Components/SkeletalMeshComponent.h"

namespace
{
float GetNotifyWindowDuration(const FAnimNotifyEventReference& EventReference)
{
	if (const FAnimNotifyEvent* NotifyEvent = EventReference.GetNotify())
	{
		return NotifyEvent->GetDuration();
	}

	return 0.0f;
}
}

void UAnimNotifyState_ComboInputWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp)
	{
		return;
	}

	if (ACombatCharacter* CombatCharacter = Cast<ACombatCharacter>(MeshComp->GetOwner()))
	{
		CombatCharacter->BeginComboInputWindow(TotalDuration);
	}
}

void UAnimNotifyState_ComboInputWindow::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp)
	{
		return;
	}

	if (ACombatCharacter* CombatCharacter = Cast<ACombatCharacter>(MeshComp->GetOwner()))
	{
		CombatCharacter->TickComboInputWindow(FrameDeltaTime, GetNotifyWindowDuration(EventReference));
	}
}

void UAnimNotifyState_ComboInputWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp)
	{
		return;
	}

	if (ACombatCharacter* CombatCharacter = Cast<ACombatCharacter>(MeshComp->GetOwner()))
	{
		CombatCharacter->EndComboInputWindow(NoInputSectionName);
	}
}

FString UAnimNotifyState_ComboInputWindow::GetNotifyName_Implementation() const
{
	return FString("Combo Input Window");
}
