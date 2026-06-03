// Copyright Epic Games, Inc. All Rights Reserved.

#include "CombatPhotoTargetWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

void UCombatPhotoTargetWidget::SetTargetTexture(UTexture2D* NewTexture)
{
	TargetTexture = NewTexture;
	RefreshImage();
}

void UCombatPhotoTargetWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!PhotoImage && WidgetTree)
	{
		PhotoImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PhotoImage"));
		WidgetTree->RootWidget = PhotoImage;
	}

	RefreshImage();
}

void UCombatPhotoTargetWidget::RefreshImage()
{
	if (!PhotoImage || !TargetTexture)
	{
		return;
	}

	PhotoImage->SetBrushFromTexture(TargetTexture, true);
	PhotoImage->SetColorAndOpacity(FLinearColor::White);
}
