// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CombatStartMenuWidget.generated.h"

class UButton;
class UCheckBox;
class UComboBoxString;
class UImage;
class UTextBlock;
struct FSlateDynamicImageBrush;
struct FGeometry;
struct FKeyEvent;

UCLASS()
class TRYCOMBOWEPON_API UCombatStartMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	UPROPERTY(Transient)
	TObjectPtr<UCheckBox> EnableAICheckBox;

	UPROPERTY(Transient)
	TObjectPtr<UComboBoxString> MusicComboBox;

	UPROPERTY(Transient)
	TObjectPtr<UButton> StartButton;

	UPROPERTY(Transient)
	TObjectPtr<UImage> CoverImage;

	TSharedPtr<FSlateDynamicImageBrush> CoverBrush;

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UFUNCTION()
	void HandleStartClicked();

	UFUNCTION()
	void HandleAIChanged(bool bIsChecked);

	UFUNCTION()
	void HandleMusicSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	void BuildDefaultMenu();
	void ApplyCurrentOptions();
};

