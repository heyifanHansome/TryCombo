// Copyright Epic Games, Inc. All Rights Reserved.

#include "Variant_Combat/UI/CombatStartMenuWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateDynamicImageBrush.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Misc/Paths.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Variant_Combat/CombatGameMode.h"
#include "Variant_Combat/CombatPlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogCombatStartMenuWidget, Log, All);

TSharedRef<SWidget> UCombatStartMenuWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UE_LOG(LogCombatStartMenuWidget, Warning, TEXT("RebuildWidget: root is null, building native fallback menu before Slate rebuild."));
		BuildDefaultMenu();
	}

	return Super::RebuildWidget();
}

void UCombatStartMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);

	UE_LOG(LogCombatStartMenuWidget, Warning, TEXT("NativeConstruct: Widget=%s Class=%s WidgetTree=%s RootBefore=%s"),
		*GetNameSafe(this),
		*GetClass()->GetName(),
		WidgetTree ? TEXT("valid") : TEXT("null"),
		WidgetTree && WidgetTree->RootWidget ? *WidgetTree->RootWidget->GetName() : TEXT("null"));

	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		UE_LOG(LogCombatStartMenuWidget, Error, TEXT("NativeConstruct: Root is still null after RebuildWidget. Building late fallback."));
		BuildDefaultMenu();
	}
	else
	{
		UE_LOG(LogCombatStartMenuWidget, Warning, TEXT("Using existing widget tree root. StartButton=%s EnableAI=%s MusicCombo=%s"),
			*GetNameSafe(StartButton),
			*GetNameSafe(EnableAICheckBox),
			*GetNameSafe(MusicComboBox));
	}

	if (StartButton)
	{
		StartButton->OnClicked.RemoveAll(this);
		StartButton->OnClicked.AddDynamic(this, &UCombatStartMenuWidget::HandleStartClicked);
		UE_LOG(LogCombatStartMenuWidget, Warning, TEXT("StartButton bound successfully: %s"), *GetNameSafe(StartButton));
	}
	else
	{
		UE_LOG(LogCombatStartMenuWidget, Error, TEXT("StartButton is null after menu build. No Start Game button can be clicked."));
	}

	if (EnableAICheckBox)
	{
		EnableAICheckBox->OnCheckStateChanged.RemoveAll(this);
		EnableAICheckBox->OnCheckStateChanged.AddDynamic(this, &UCombatStartMenuWidget::HandleAIChanged);
	}

	if (MusicComboBox)
	{
		MusicComboBox->OnSelectionChanged.RemoveAll(this);
		MusicComboBox->OnSelectionChanged.AddDynamic(this, &UCombatStartMenuWidget::HandleMusicSelectionChanged);
	}

	ApplyCurrentOptions();
	SetKeyboardFocus();

	UE_LOG(LogCombatStartMenuWidget, Warning, TEXT("NativeConstruct done: RootAfter=%s StartButton=%s EnableAI=%s MusicCombo=%s"),
		WidgetTree && WidgetTree->RootWidget ? *WidgetTree->RootWidget->GetName() : TEXT("null"),
		*GetNameSafe(StartButton),
		*GetNameSafe(EnableAICheckBox),
		*GetNameSafe(MusicComboBox));
}

FReply UCombatStartMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Escape)
	{
		HandleStartClicked();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UCombatStartMenuWidget::BuildDefaultMenu()
{
	UE_LOG(LogCombatStartMenuWidget, Warning, TEXT("BuildDefaultMenu: constructing native fallback menu."));

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("StartMenuRoot"));
	WidgetTree->RootWidget = RootCanvas;

	const FString CoverPath = FPaths::ProjectContentDir() / TEXT("UI/Covers/ChickenCover.png");
	if (FPaths::FileExists(CoverPath))
	{
		CoverBrush = MakeShared<FSlateDynamicImageBrush>(FName(*CoverPath), FVector2D(1024.0f, 1536.0f));
		UScaleBox* CoverScaleBox = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("ChickenCoverScaleBox"));
		CoverScaleBox->SetStretch(EStretch::ScaleToFill);
		CoverScaleBox->SetStretchDirection(EStretchDirection::Both);
		CoverScaleBox->SetClipping(EWidgetClipping::ClipToBounds);

		CoverImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ChickenCoverImage"));
		CoverImage->SetBrush(*CoverBrush);
		CoverScaleBox->AddChild(CoverImage);

		UCanvasPanelSlot* CoverSlot = RootCanvas->AddChildToCanvas(CoverScaleBox);
		CoverSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		CoverSlot->SetOffsets(FMargin(0.0f));
	}
	else
	{
		UE_LOG(LogCombatStartMenuWidget, Warning, TEXT("Cover image not found: %s"), *CoverPath);
	}

	UBorder* DimBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DimBackground"));
	DimBackground->SetBrushColor(FLinearColor(0.01f, 0.012f, 0.016f, 0.30f));
	UCanvasPanelSlot* BackgroundSlot = RootCanvas->AddChildToCanvas(DimBackground);
	BackgroundSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	BackgroundSlot->SetOffsets(FMargin(0.0f));

	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MenuPanelSize"));
	PanelSize->SetWidthOverride(430.0f);
	PanelSize->SetHeightOverride(560.0f);
	UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(PanelSize);
	PanelSlot->SetAnchors(FAnchors(0.0f, 0.5f));
	PanelSlot->SetAlignment(FVector2D(0.0f, 0.5f));
	PanelSlot->SetPosition(FVector2D(72.0f, 0.0f));
	PanelSlot->SetAutoSize(true);

	UBorder* GoldFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MenuGoldFrame"));
	GoldFrame->SetPadding(FMargin(4.0f));
	GoldFrame->SetBrushColor(FLinearColor(1.0f, 0.64f, 0.05f, 0.92f));
	PanelSize->AddChild(GoldFrame);

	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MenuPanel"));
	PanelBorder->SetPadding(FMargin(30.0f, 34.0f));
	PanelBorder->SetBrushColor(FLinearColor(0.025f, 0.026f, 0.032f, 0.88f));
	GoldFrame->SetContent(PanelBorder);

	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuContent"));
	PanelBorder->SetContent(RootBox);

	UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	TitleText->SetText(FText::FromString(TEXT("CHICKEN ARENA")));
	FSlateFontInfo TitleFont = TitleText->GetFont();
	TitleFont.Size = 38;
	TitleText->SetFont(TitleFont);
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.78f, 0.08f, 1.0f)));
	TitleText->SetShadowOffset(FVector2D(3.0f, 3.0f));
	TitleText->SetShadowColorAndOpacity(FLinearColor::Black);
	TitleText->SetJustification(ETextJustify::Center);
	UVerticalBoxSlot* TitleSlot = RootBox->AddChildToVerticalBox(TitleText);
	TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 2.0f));

	UTextBlock* SubtitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SubtitleText"));
	SubtitleText->SetText(FText::FromString(TEXT("LAUGH BRAWL EDITION")));
	FSlateFontInfo SubtitleFont = SubtitleText->GetFont();
	SubtitleFont.Size = 18;
	SubtitleText->SetFont(SubtitleFont);
	SubtitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.86f, 1.0f, 1.0f)));
	SubtitleText->SetJustification(ETextJustify::Center);
	UVerticalBoxSlot* SubtitleSlot = RootBox->AddChildToVerticalBox(SubtitleText);
	SubtitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 30.0f));

	UBorder* OptionPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("OptionPanel"));
	OptionPanel->SetPadding(FMargin(18.0f, 18.0f));
	OptionPanel->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.42f));
	UVerticalBoxSlot* OptionPanelSlot = RootBox->AddChildToVerticalBox(OptionPanel);
	OptionPanelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 26.0f));

	UVerticalBox* OptionBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("OptionBox"));
	OptionPanel->SetContent(OptionBox);

	UHorizontalBox* AIRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("AIRow"));
	UVerticalBoxSlot* AIRowSlot = OptionBox->AddChildToVerticalBox(AIRow);
	AIRowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 22.0f));

	UTextBlock* AIText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AIText"));
	AIText->SetText(FText::FromString(TEXT("AI OPPONENTS")));
	FSlateFontInfo OptionFont = AIText->GetFont();
	OptionFont.Size = 23;
	AIText->SetFont(OptionFont);
	AIText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	AIText->SetShadowOffset(FVector2D(2.0f, 2.0f));
	AIText->SetShadowColorAndOpacity(FLinearColor::Black);
	UHorizontalBoxSlot* AITextSlot = AIRow->AddChildToHorizontalBox(AIText);
	AITextSlot->SetPadding(FMargin(0.0f, 0.0f, 18.0f, 0.0f));

	EnableAICheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), TEXT("EnableAICheckBox"));
	AIRow->AddChildToHorizontalBox(EnableAICheckBox);

	UTextBlock* MusicLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MusicLabel"));
	MusicLabel->SetText(FText::FromString(TEXT("BATTLE MUSIC")));
	MusicLabel->SetFont(OptionFont);
	MusicLabel->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.78f, 0.08f, 1.0f)));
	MusicLabel->SetShadowOffset(FVector2D(2.0f, 2.0f));
	MusicLabel->SetShadowColorAndOpacity(FLinearColor::Black);
	UVerticalBoxSlot* MusicLabelSlot = OptionBox->AddChildToVerticalBox(MusicLabel);
	MusicLabelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));

	MusicComboBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("MusicComboBox"));
	USizeBox* MusicSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MusicSize"));
	MusicSize->SetHeightOverride(46.0f);
	MusicSize->AddChild(MusicComboBox);
	UVerticalBoxSlot* MusicSlot = OptionBox->AddChildToVerticalBox(MusicSize);
	MusicSlot->SetPadding(FMargin(0.0f));

	USpacer* MenuSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("MenuSpacer"));
	UVerticalBoxSlot* SpacerSlot = RootBox->AddChildToVerticalBox(MenuSpacer);
	SpacerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	StartButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("StartButton"));
	StartButton->SetBackgroundColor(FLinearColor(1.0f, 0.38f, 0.02f, 1.0f));
	USizeBox* StartSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("StartButtonSize"));
	StartSize->SetHeightOverride(70.0f);
	StartSize->AddChild(StartButton);
	UVerticalBoxSlot* StartSlot = RootBox->AddChildToVerticalBox(StartSize);
	StartSlot->SetPadding(FMargin(0.0f));

	UTextBlock* StartText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StartText"));
	StartText->SetText(FText::FromString(TEXT("START BRAWL")));
	FSlateFontInfo StartFont = StartText->GetFont();
	StartFont.Size = 30;
	StartText->SetFont(StartFont);
	StartText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	StartText->SetShadowOffset(FVector2D(2.0f, 2.0f));
	StartText->SetShadowColorAndOpacity(FLinearColor::Black);
	StartText->SetJustification(ETextJustify::Center);
	StartButton->AddChild(StartText);
}

void UCombatStartMenuWidget::ApplyCurrentOptions()
{
	ACombatGameMode* CombatGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACombatGameMode>() : nullptr;
	if (!CombatGameMode)
	{
		UE_LOG(LogCombatStartMenuWidget, Error, TEXT("ApplyCurrentOptions: CombatGameMode not found. Actual GameMode=%s"),
			*GetNameSafe(GetWorld() ? GetWorld()->GetAuthGameMode() : nullptr));
		return;
	}

	if (EnableAICheckBox)
	{
		EnableAICheckBox->SetIsChecked(true);
	}

	if (MusicComboBox)
	{
		MusicComboBox->ClearOptions();

		const TArray<FCombatMusicOption>& MusicOptions = CombatGameMode->GetBackgroundMusicOptions();
		for (int32 Index = 0; Index < MusicOptions.Num(); ++Index)
		{
			const FText& DisplayName = MusicOptions[Index].DisplayName;
			const FString OptionName = DisplayName.IsEmpty()
				? FString::Printf(TEXT("Music %d"), Index + 1)
				: DisplayName.ToString();
			MusicComboBox->AddOption(OptionName);
		}

		if (MusicOptions.Num() == 0)
		{
			MusicComboBox->AddOption(TEXT("No Music"));
			MusicComboBox->SetSelectedIndex(0);
			UE_LOG(LogCombatStartMenuWidget, Warning, TEXT("No background music options configured on %s."), *GetNameSafe(CombatGameMode));
			return;
		}

		const int32 DefaultIndex = FMath::Clamp(CombatGameMode->GetDefaultBackgroundMusicIndex(), 0, MusicOptions.Num() - 1);
		MusicComboBox->SetSelectedIndex(DefaultIndex);
		UE_LOG(LogCombatStartMenuWidget, Warning, TEXT("Loaded %d background music options. Default=%d."), MusicOptions.Num(), DefaultIndex);
	}
}

void UCombatStartMenuWidget::HandleStartClicked()
{
	const bool bEnableAI = EnableAICheckBox ? EnableAICheckBox->IsChecked() : true;
	const int32 MusicIndex = MusicComboBox ? MusicComboBox->GetSelectedIndex() : 0;
	UE_LOG(LogCombatStartMenuWidget, Warning, TEXT("HandleStartClicked: starting combat. AI=%s MusicIndex=%d"),
		bEnableAI ? TEXT("true") : TEXT("false"),
		MusicIndex);

	APlayerController* OwningPlayer = GetOwningPlayer();
	if (ACombatPlayerController* CombatPlayerController = Cast<ACombatPlayerController>(OwningPlayer))
	{
		CombatPlayerController->StartCombatFromMenu(bEnableAI, MusicIndex);
	}
	else if (OwningPlayer)
	{
		UGameplayStatics::SetGamePaused(OwningPlayer, false);
		HandleAIChanged(bEnableAI);
		if (ACombatGameMode* CombatGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACombatGameMode>() : nullptr)
		{
			CombatGameMode->StartCombatGame(bEnableAI, MusicIndex);
		}
		OwningPlayer->SetInputMode(FInputModeGameOnly());
		OwningPlayer->SetShowMouseCursor(false);
		RemoveFromParent();
	}
}

void UCombatStartMenuWidget::HandleAIChanged(bool bIsChecked)
{
	if (ACombatGameMode* CombatGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACombatGameMode>() : nullptr)
	{
		CombatGameMode->SetCombatAIEnabled(bIsChecked);
	}
}

void UCombatStartMenuWidget::HandleMusicSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (!MusicComboBox)
	{
		return;
	}

	if (ACombatGameMode* CombatGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACombatGameMode>() : nullptr)
	{
		CombatGameMode->PlayBackgroundMusicByIndex(MusicComboBox->GetSelectedIndex());
	}
}

