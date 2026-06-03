// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CombatPhotoTargetWidget.generated.h"

class UImage;
class UTexture2D;

/**
 * Runtime widget used by photo targets to display an imported image.
 */
UCLASS()
class UCombatPhotoTargetWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Photo Target")
	void SetTargetTexture(UTexture2D* NewTexture);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(Transient)
	TObjectPtr<UImage> PhotoImage;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> TargetTexture;

	void RefreshImage();
};
