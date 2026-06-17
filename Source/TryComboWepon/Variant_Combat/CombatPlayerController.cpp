// Copyright Epic Games, Inc. All Rights Reserved.


#include "Variant_Combat/CombatPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "CombatCharacter.h"
#include "Variant_Combat/CombatGameMode.h"
#include "Variant_Combat/UI/CombatStartMenuWidget.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Blueprint/UserWidget.h"
#include "TryComboWepon.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogCombatStartFlow, Log, All);

ACombatPlayerController::ACombatPlayerController()
{
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> DefaultInputContext(TEXT("/Game/Input/IMC_Default.IMC_Default"));
	if (DefaultInputContext.Succeeded())
	{
		DefaultMappingContexts.AddUnique(DefaultInputContext.Object);
	}

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> CombatInputContext(TEXT("/Game/Variant_Combat/Input/IMC_Combat.IMC_Combat"));
	if (CombatInputContext.Succeeded())
	{
		DefaultMappingContexts.AddUnique(CombatInputContext.Object);
	}

	StartMenuWidgetClass = UCombatStartMenuWidget::StaticClass();
}

void ACombatPlayerController::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogCombatStartFlow, Warning, TEXT("BeginPlay: Controller=%s Class=%s Local=%s Pawn=%s GameMode=%s ShowMenu=%s WidgetClass=%s Paused=%s"),
		*GetNameSafe(this),
		*GetClass()->GetName(),
		IsLocalPlayerController() ? TEXT("true") : TEXT("false"),
		*GetNameSafe(GetPawn()),
		*GetNameSafe(GetWorld() ? GetWorld()->GetAuthGameMode() : nullptr),
		bShowStartMenuOnBeginPlay ? TEXT("true") : TEXT("false"),
		*GetNameSafe(StartMenuWidgetClass.Get()),
		UGameplayStatics::IsGamePaused(this) ? TEXT("true") : TEXT("false"));

	// only spawn touch controls on local player controllers
	if (ShouldUseTouchControls() && IsLocalPlayerController())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogTryComboWepon, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}

	if (!IsLocalPlayerController())
	{
		UE_LOG(LogCombatStartFlow, Warning, TEXT("Skip start menu: controller is not local."));
		return;
	}

	if (!StartMenuWidgetClass)
	{
		UE_LOG(LogCombatStartFlow, Error, TEXT("StartMenuWidgetClass was null, using native UCombatStartMenuWidget fallback."));
		StartMenuWidgetClass = UCombatStartMenuWidget::StaticClass();
	}

	if (!bShowStartMenuOnBeginPlay)
	{
		UE_LOG(LogCombatStartFlow, Error, TEXT("bShowStartMenuOnBeginPlay is false, likely overridden by BP_CombatPlayerController. Forcing menu for diagnosis."));
	}

	if (StartMenuWidgetClass)
	{
		StartMenuWidget = CreateWidget<UCombatStartMenuWidget>(this, StartMenuWidgetClass);
		if (StartMenuWidget)
		{
			if (ACombatGameMode* CombatGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACombatGameMode>() : nullptr)
			{
				CombatGameMode->SetCombatAIEnabled(false);
				CombatGameMode->StopBackgroundMusic();
			}

			UE_LOG(LogCombatStartFlow, Warning, TEXT("Start menu widget created: Widget=%s Class=%s"),
				*GetNameSafe(StartMenuWidget),
				*StartMenuWidget->GetClass()->GetName());

			StartMenuWidget->AddToPlayerScreen(10);
			StartMenuWidget->SetKeyboardFocus();

			FInputModeGameAndUI InputMode;
			InputMode.SetWidgetToFocus(StartMenuWidget->TakeWidget());
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			InputMode.SetHideCursorDuringCapture(false);
			SetInputMode(InputMode);
			SetShowMouseCursor(true);

			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Green, TEXT("Combat start menu created. Check Saved/Logs/TryComboWepon.log for LogCombatStartFlow."));
			}

			UGameplayStatics::SetGamePaused(this, true);
		}
		else
		{
			UE_LOG(LogCombatStartFlow, Error, TEXT("CreateWidget failed for StartMenuWidgetClass=%s"), *GetNameSafe(StartMenuWidgetClass.Get()));
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Red, TEXT("Start menu CreateWidget failed. See LogCombatStartFlow."));
			}
		}
	}
}

void ACombatPlayerController::StartCombatFromMenu(bool bEnableAI, int32 MusicIndex)
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	if (ACombatGameMode* CombatGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACombatGameMode>() : nullptr)
	{
		CombatGameMode->StartCombatGame(bEnableAI, MusicIndex);
	}

	if (StartMenuWidget)
	{
		StartMenuWidget->RemoveFromParent();
		StartMenuWidget = nullptr;
	}

	UGameplayStatics::SetGamePaused(this, false);
	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(false);
}

void ACombatPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// add the input mapping context
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

void ACombatPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// subscribe to the pawn's OnDestroyed delegate
	InPawn->OnDestroyed.AddDynamic(this, &ACombatPlayerController::OnPawnDestroyed);
}

void ACombatPlayerController::SetRespawnTransform(const FTransform& NewRespawn)
{
	// save the new respawn transform
	RespawnTransform = NewRespawn;
}

void ACombatPlayerController::OnPawnDestroyed(AActor* DestroyedActor)
{
	// spawn a new character at the respawn transform
	if (ACombatCharacter* RespawnedCharacter = GetWorld()->SpawnActor<ACombatCharacter>(CharacterClass, RespawnTransform))
	{
		// possess the character
		Possess(RespawnedCharacter);
	}
}

bool ACombatPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
