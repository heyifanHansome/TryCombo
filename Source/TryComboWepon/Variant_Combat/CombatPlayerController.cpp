// Copyright Epic Games, Inc. All Rights Reserved.


#include "Variant_Combat/CombatPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "CombatCharacter.h"
#include "Variant_Combat/UI/CombatStartMenuWidget.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Blueprint/UserWidget.h"
#include "TryComboWepon.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "UObject/ConstructorHelpers.h"

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

	if (bShowStartMenuOnBeginPlay && IsLocalPlayerController() && StartMenuWidgetClass)
	{
		StartMenuWidget = CreateWidget<UCombatStartMenuWidget>(this, StartMenuWidgetClass);
		if (StartMenuWidget)
		{
			StartMenuWidget->AddToPlayerScreen(10);
			SetInputMode(FInputModeUIOnly());
			SetShowMouseCursor(true);
			UGameplayStatics::SetGamePaused(this, true);
		}
	}
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
