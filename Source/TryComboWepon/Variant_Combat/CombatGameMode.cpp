// Copyright Epic Games, Inc. All Rights Reserved.


#include "Variant_Combat/CombatGameMode.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"

ACombatGameMode::ACombatGameMode()
{

}

void ACombatGameMode::SetCombatAIEnabled(bool bEnabled)
{
	bEnableCombatAI = bEnabled;
}

void ACombatGameMode::PlayBackgroundMusicByIndex(int32 MusicIndex)
{
	if (!BackgroundMusicOptions.IsValidIndex(MusicIndex))
	{
		StopBackgroundMusic();
		return;
	}

	USoundBase* SelectedMusic = BackgroundMusicOptions[MusicIndex].Music;
	if (!SelectedMusic)
	{
		StopBackgroundMusic();
		return;
	}

	StopBackgroundMusic();
	ActiveBackgroundMusic = UGameplayStatics::SpawnSound2D(this, SelectedMusic, BackgroundMusicVolume, 1.0f, 0.0f, nullptr, true);
}

void ACombatGameMode::StopBackgroundMusic()
{
	if (ActiveBackgroundMusic)
	{
		ActiveBackgroundMusic->Stop();
		ActiveBackgroundMusic = nullptr;
	}
}
