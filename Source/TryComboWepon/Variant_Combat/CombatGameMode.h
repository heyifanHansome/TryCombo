// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CombatGameMode.generated.h"

class UAudioComponent;
class USoundBase;

USTRUCT(BlueprintType)
struct FCombatMusicOption
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Music")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Music")
	TObjectPtr<USoundBase> Music = nullptr;
};

/**
 *  Simple GameMode for a third person combat game
 */
UCLASS(abstract)
class ACombatGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:

	ACombatGameMode();

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Game Start")
	bool bEnableCombatAI = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Game Start")
	bool bCombatGameStarted = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Game Start|Music")
	TArray<FCombatMusicOption> BackgroundMusicOptions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Game Start|Music")
	int32 DefaultBackgroundMusicIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Game Start|Music", meta=(ClampMin=0.0, UIMin=0.0, UIMax=2.0))
	float BackgroundMusicVolume = 0.75f;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ActiveBackgroundMusic;

public:

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category="Game Start")
	void StartCombatGame(bool bEnableAI, int32 MusicIndex);

	UFUNCTION(BlueprintPure, Category="Game Start")
	bool HasCombatGameStarted() const { return bCombatGameStarted; }

	UFUNCTION(BlueprintCallable, Category="Game Start|AI")
	void SetCombatAIEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category="Game Start|AI")
	bool IsCombatAIEnabled() const { return bEnableCombatAI; }

	UFUNCTION(BlueprintCallable, Category="Game Start|Music")
	void PlayBackgroundMusicByIndex(int32 MusicIndex);

	UFUNCTION(BlueprintCallable, Category="Game Start|Music")
	void StopBackgroundMusic();

	UFUNCTION(BlueprintPure, Category="Game Start|Music")
	const TArray<FCombatMusicOption>& GetBackgroundMusicOptions() const { return BackgroundMusicOptions; }

	UFUNCTION(BlueprintPure, Category="Game Start|Music")
	int32 GetDefaultBackgroundMusicIndex() const { return DefaultBackgroundMusicIndex; }

protected:

	void EnsureDefaultMusicOptions();
	void ApplyCombatAIStateToWorld();
};
