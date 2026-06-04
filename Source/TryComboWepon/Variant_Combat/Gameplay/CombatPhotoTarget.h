// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CombatDamageable.h"
#include "CombatPhotoTarget.generated.h"

class UBoxComponent;
class UBillboardComponent;
class USceneComponent;
class UStaticMeshComponent;
class USoundBase;
class UTexture2D;
class UWidgetComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCombatPhotoTargetDamagedSignature, float, Damage, const FVector&, ImpactPoint, float, RemainingHP);

/**
 * A flat image target that can be hit by weapon collision windows.
 */
UCLASS(Blueprintable)
class ACombatPhotoTarget : public AActor, public ICombatDamageable
{
	GENERATED_BODY()

public:
	ACombatPhotoTarget();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UBoxComponent> HitBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> TargetMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UWidgetComponent> PhotoWidget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UBillboardComponent> PhotoBillboard;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Photo Target")
	TObjectPtr<UTexture2D> PhotoTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Photo Target")
	TArray<TObjectPtr<UTexture2D>> PhotoTextures;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Photo Target")
	FString ImageFilePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Photo Target")
	TArray<FString> ImageFilePaths;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Photo Target", meta=(ClampMin=1, Units="cm"))
	FVector2D TargetSize = FVector2D(140.0f, 230.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Photo Target", meta=(ClampMin=0.001))
	float WidgetWorldScale = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage", meta=(ClampMin=1, ClampMax=1000, DisplayName="Max HP / Hits To Popup"))
	float MaxHP = 20.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Damage", meta=(ClampMin=0, ClampMax=1000))
	float CurrentHP = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage")
	bool bResetHealthOnDeath = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meme Target", meta=(ClampMin=1, ClampMax=50))
	int32 HitsPerPhoto = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meme Target")
	bool bAdvancePhotoOnHit = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meme Target")
	bool bShowPhotoOnlyOnDeath = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meme Target")
	bool bAdvancePhotoOnDeath = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meme Target")
	bool bRandomizeNextPhoto = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meme Target")
	TArray<FString> HitPhrases;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meme Target")
	TArray<FString> DeathPhrases;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meme Target", meta=(ClampMin=0.1, ClampMax=30, Units="s"))
	float MemePopupDuration = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Test")
	bool bShowPopupOnHit = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Test")
	bool bBlockCharacters = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meme Target|Audio")
	TObjectPtr<USoundBase> PopupSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meme Target|Audio", meta=(ClampMin=0))
	float PopupSoundVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meme Target|Audio", meta=(ClampMin=0.01))
	float PopupSoundPitch = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meme Target|Audio")
	TObjectPtr<USoundBase> HitSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meme Target|Audio", meta=(ClampMin=0))
	float HitSoundVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meme Target|Audio", meta=(ClampMin=0.01))
	float HitSoundPitch = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Meme Target")
	int32 CurrentPhotoIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Meme Target")
	int32 HitsOnCurrentPhoto = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Meme Target")
	int32 TotalHits = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Meme Target")
	int32 Score = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Debug")
	bool bPrintHitMessages = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Debug")
	bool bDrawDebugOnHit = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Debug", meta=(ClampMin=0, ClampMax=30, Units="s"))
	float DebugDrawDuration = 2.0f;

	UPROPERTY(BlueprintAssignable, Category="Events")
	FCombatPhotoTargetDamagedSignature OnDamaged;

	UFUNCTION(BlueprintCallable, Category="Photo Target")
	bool ReloadPhoto();

	UFUNCTION(BlueprintCallable, Category="Photo Target")
	bool SetPhotoIndex(int32 NewPhotoIndex);

	UFUNCTION(BlueprintCallable, Category="Photo Target")
	bool AdvancePhoto();

	UFUNCTION(BlueprintCallable, Category="Attack Test")
	void SetShowPopupOnHit(bool bNewShowPopupOnHit);

	UFUNCTION(BlueprintCallable, Category="Attack Test")
	void SetBlockCharacters(bool bNewBlockCharacters);

	UFUNCTION(BlueprintCallable, Category="Attack Test")
	void TestPopup();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual void ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse) override;
	virtual void HandleDeath() override;
	virtual void ApplyHealing(float Healing, AActor* Healer) override;
	virtual void NotifyDanger(const FVector& DangerLocation, AActor* DangerSource) override;

protected:
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> RuntimeTexture;

	FTimerHandle HideMemePopupTimer;

	void EnsureDefaultGallery();
	bool LoadTextureForCurrentIndex();
	int32 GetPhotoCount() const;
	FString GetCurrentHitPhrase() const;
	FString GetCurrentDeathPhrase() const;
	void ConfigureTargetCollision();
	void UpdateTargetShape();
	void ApplyTextureToWidget();
	void ApplyTextureToBillboard();
	void ApplyTextureToVisuals();
	void ShowMemePopup();
	void HideMemePopup();
	void PrintDebugMessage(const FString& Message, const FColor& Color) const;
};
