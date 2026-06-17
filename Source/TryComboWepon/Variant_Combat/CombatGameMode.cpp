// Copyright Epic Games, Inc. All Rights Reserved.


#include "Variant_Combat/CombatGameMode.h"
#include "AIController.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "BrainComponent.h"
#include "Components/AudioComponent.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Variant_Combat/AI/CombatEnemy.h"
#include "Variant_Combat/AI/CombatEnemySpawner.h"
#include "Variant_Combat/Gameplay/CombatBasketballSpawner.h"
#include "Variant_Combat/Gameplay/CombatFlyingBasketball.h"

DEFINE_LOG_CATEGORY_STATIC(LogCombatGameStart, Log, All);

ACombatGameMode::ACombatGameMode()
{

}

void ACombatGameMode::BeginPlay()
{
	Super::BeginPlay();

	EnsureDefaultMusicOptions();
	bCombatGameStarted = false;
	bEnableCombatAI = false;
	StopBackgroundMusic();
	ApplyCombatAIStateToWorld();
}

void ACombatGameMode::StartCombatGame(bool bEnableAI, int32 MusicIndex)
{
	EnsureDefaultMusicOptions();
	bCombatGameStarted = true;
	SetCombatAIEnabled(bEnableAI);
	UE_LOG(LogCombatGameStart, Warning, TEXT("StartCombatGame: AI=%s MusicIndex=%d MusicOptions=%d"),
		bEnableAI ? TEXT("true") : TEXT("false"),
		MusicIndex,
		BackgroundMusicOptions.Num());
	PlayBackgroundMusicByIndex(MusicIndex);
}

void ACombatGameMode::SetCombatAIEnabled(bool bEnabled)
{
	bEnableCombatAI = bEnabled;
	ApplyCombatAIStateToWorld();
}

void ACombatGameMode::PlayBackgroundMusicByIndex(int32 MusicIndex)
{
	EnsureDefaultMusicOptions();

	if (!BackgroundMusicOptions.IsValidIndex(MusicIndex))
	{
		UE_LOG(LogCombatGameStart, Warning, TEXT("PlayBackgroundMusicByIndex: invalid index %d, stopping music."), MusicIndex);
		StopBackgroundMusic();
		return;
	}

	USoundBase* SelectedMusic = BackgroundMusicOptions[MusicIndex].Music;
	if (!SelectedMusic)
	{
		UE_LOG(LogCombatGameStart, Warning, TEXT("PlayBackgroundMusicByIndex: music entry %d is null, stopping music."), MusicIndex);
		StopBackgroundMusic();
		return;
	}

	StopBackgroundMusic();
	ActiveBackgroundMusic = UGameplayStatics::SpawnSound2D(this, SelectedMusic, BackgroundMusicVolume, 1.0f, 0.0f, nullptr, true);
	UE_LOG(LogCombatGameStart, Warning, TEXT("Playing background music: %s"), *GetNameSafe(SelectedMusic));
}

void ACombatGameMode::StopBackgroundMusic()
{
	if (ActiveBackgroundMusic)
	{
		ActiveBackgroundMusic->Stop();
		ActiveBackgroundMusic = nullptr;
	}
}

void ACombatGameMode::EnsureDefaultMusicOptions()
{
	if (BackgroundMusicOptions.Num() > 0)
	{
		return;
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> SoundAssets;
	FARFilter Filter;
	Filter.PackagePaths.Add(FName(TEXT("/Game")));
	Filter.ClassPaths.Add(USoundBase::StaticClass()->GetClassPathName());
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;
	AssetRegistry.GetAssets(Filter, SoundAssets);

	SoundAssets.Sort([](const FAssetData& Left, const FAssetData& Right)
	{
		return Left.GetObjectPathString() < Right.GetObjectPathString();
	});

	for (const FAssetData& SoundAsset : SoundAssets)
	{
		UObject* AssetObject = SoundAsset.GetAsset();
		USoundBase* LoadedMusic = Cast<USoundBase>(AssetObject);
		if (!LoadedMusic)
		{
			continue;
		}

		FCombatMusicOption Option;
		Option.DisplayName = FText::FromString(SoundAsset.AssetName.ToString());
		Option.Music = LoadedMusic;
		BackgroundMusicOptions.Add(Option);
		UE_LOG(LogCombatGameStart, Warning, TEXT("Loaded music option: %s -> %s"), *SoundAsset.GetObjectPathString(), *GetNameSafe(LoadedMusic));
	}

	UE_LOG(LogCombatGameStart, Warning, TEXT("Total music options loaded from AssetRegistry: %d"), BackgroundMusicOptions.Num());
}

void ACombatGameMode::ApplyCombatAIStateToWorld()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	int32 ControllerCount = 0;
	for (TActorIterator<AAIController> It(World); It; ++It)
	{
		AAIController* AIController = *It;
		if (!AIController)
		{
			continue;
		}

		++ControllerCount;
		if (UBrainComponent* Brain = AIController->GetBrainComponent())
		{
			if (bEnableCombatAI)
			{
				Brain->RestartLogic();
			}
			else
			{
				Brain->StopLogic(TEXT("Combat start menu disabled AI"));
			}
		}
	}

	int32 EnemyCount = 0;
	for (TActorIterator<ACombatEnemy> It(World); It; ++It)
	{
		ACombatEnemy* Enemy = *It;
		if (!Enemy)
		{
			continue;
		}

		++EnemyCount;
		Enemy->SetActorTickEnabled(bEnableCombatAI);
		Enemy->SetActorHiddenInGame(!bEnableCombatAI);
		Enemy->SetActorEnableCollision(bEnableCombatAI);

		if (UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement())
		{
			if (bEnableCombatAI)
			{
				Movement->SetMovementMode(MOVE_Walking);
			}
			else
			{
				Movement->StopMovementImmediately();
				Movement->DisableMovement();
			}
		}
	}

	int32 BasketballSpawnerCount = 0;
	for (TActorIterator<ACombatBasketballSpawner> It(World); It; ++It)
	{
		ACombatBasketballSpawner* Spawner = *It;
		if (!Spawner)
		{
			continue;
		}

		++BasketballSpawnerCount;
		if (bEnableCombatAI && Spawner->bAutoStart)
		{
			Spawner->StartSpawning();
		}
		else
		{
			Spawner->StopSpawning();
		}
	}

	int32 BasketballCount = 0;
	if (!bEnableCombatAI)
	{
		for (TActorIterator<ACombatFlyingBasketball> It(World); It; ++It)
		{
			ACombatFlyingBasketball* Basketball = *It;
			if (!Basketball)
			{
				continue;
			}

			++BasketballCount;
			Basketball->Destroy();
		}
	}

	UE_LOG(LogCombatGameStart, Warning, TEXT("Applied AI state: Enabled=%s Controllers=%d Enemies=%d BasketballSpawners=%d DestroyedBasketballs=%d"),
		bEnableCombatAI ? TEXT("true") : TEXT("false"),
		ControllerCount,
		EnemyCount,
		BasketballSpawnerCount,
		BasketballCount);
}
