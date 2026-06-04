// Copyright Epic Games, Inc. All Rights Reserved.

#include "CombatPhotoTarget.h"
#include "CombatPhotoTargetWidget.h"
#include "Components/BillboardComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "ImageUtils.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ACombatPhotoTarget::ACombatPhotoTarget()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	HitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("HitBox"));
	HitBox->SetupAttachment(Root);
	ConfigureTargetCollision();

	TargetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TargetMesh"));
	TargetMesh->SetupAttachment(Root);
	TargetMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TargetMesh->SetCanEverAffectNavigation(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> TargetMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (TargetMeshAsset.Succeeded())
	{
		TargetMesh->SetStaticMesh(TargetMeshAsset.Object);
	}

	PhotoWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PhotoWidget"));
	PhotoWidget->SetupAttachment(Root);
	PhotoWidget->SetWidgetClass(UCombatPhotoTargetWidget::StaticClass());
	PhotoWidget->SetWidgetSpace(EWidgetSpace::World);
	PhotoWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PhotoWidget->SetTwoSided(true);
	PhotoWidget->SetPivot(FVector2D(0.5f, 0.5f));
	PhotoWidget->SetVisibility(false);
	PhotoWidget->SetHiddenInGame(true);

	PhotoBillboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("PhotoBillboard"));
	PhotoBillboard->SetupAttachment(Root);
	PhotoBillboard->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PhotoBillboard->SetVisibility(false);
	PhotoBillboard->SetHiddenInGame(true);
	PhotoBillboard->SetRelativeLocation(FVector(-6.0f, 0.0f, 0.0f));
	PhotoBillboard->SetRelativeScale3D(FVector(2.0f));

	ImageFilePaths = {
		TEXT("E:/xwechat_files/wxid_m6ymg5am5zwv22_6ef7/temp/RWTemp/2026-06/9e20f478899dc29eb19741386f9343c8/7291b24fde909dbf84bc632077c2b406.jpg"),
		TEXT("E:/xwechat_files/wxid_m6ymg5am5zwv22_6ef7/temp/RWTemp/2026-06/9e20f478899dc29eb19741386f9343c8/6a80bb8c5513aba7dc8dd7346811db5a.jpg"),
		TEXT("E:/xwechat_files/wxid_m6ymg5am5zwv22_6ef7/temp/RWTemp/2026-06/9e20f478899dc29eb19741386f9343c8/a7fe3af3b42aa77f478442b532f6785b.jpg"),
		TEXT("E:/xwechat_files/wxid_m6ymg5am5zwv22_6ef7/temp/RWTemp/2026-06/9e20f478899dc29eb19741386f9343c8/85d0548eba03a639504db856a14105e4.jpg"),
		TEXT("E:/xwechat_files/wxid_m6ymg5am5zwv22_6ef7/temp/RWTemp/2026-06/9e20f478899dc29eb19741386f9343c8/8e77812c546f189fdcdb73057c1a663c.jpg"),
		TEXT("E:/xwechat_files/wxid_m6ymg5am5zwv22_6ef7/temp/RWTemp/2026-06/9e20f478899dc29eb19741386f9343c8/62188bd95bd257691aefc3f04b1ec2d4.jpg"),
		TEXT("E:/xwechat_files/wxid_m6ymg5am5zwv22_6ef7/temp/RWTemp/2026-06/9e20f478899dc29eb19741386f9343c8/e41235d9b58025fa794508b56dc14446.png"),
		TEXT("E:/xwechat_files/wxid_m6ymg5am5zwv22_6ef7/temp/RWTemp/2026-06/9e20f478899dc29eb19741386f9343c8/2d594f53a5c300ffe2ad2ae4c2127cb8.jpg")
	};

	HitPhrases = {
		TEXT("ABSTRACT +10"),
		TEXT("MEME HIT"),
		TEXT("PHOTO SHUFFLE"),
		TEXT("TARGET BROKEN"),
		TEXT("COMBO CHECK OK"),
		TEXT("COLLISION VERIFIED")
	};

	DeathPhrases = {
		TEXT("梗图开奖"),
		TEXT("靶子破防了"),
		TEXT("这一刀有节目效果"),
		TEXT("抽象值爆表"),
		TEXT("连招检测通过"),
		TEXT("下一张准备上场")
	};

	UpdateTargetShape();
}

void ACombatPhotoTarget::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	EnsureDefaultGallery();
	ConfigureTargetCollision();
	UpdateTargetShape();
	ReloadPhoto();
	HideMemePopup();
}

void ACombatPhotoTarget::BeginPlay()
{
	Super::BeginPlay();
	CurrentHP = MaxHP;
	EnsureDefaultGallery();
	ConfigureTargetCollision();
	UpdateTargetShape();
	ReloadPhoto();
	HideMemePopup();
}

#if WITH_EDITOR
void ACombatPhotoTarget::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	ConfigureTargetCollision();
	UpdateTargetShape();
}
#endif

bool ACombatPhotoTarget::ReloadPhoto()
{
	RuntimeTexture = nullptr;

	if (LoadTextureForCurrentIndex())
	{
		ApplyTextureToVisuals();
		PrintDebugMessage(FString::Printf(TEXT("PHOTO TARGET: image %d/%d"), CurrentPhotoIndex + 1, GetPhotoCount()), FColor::Green);
		return true;
	}

	PrintDebugMessage(TEXT("PHOTO TARGET: assign PhotoTexture or ImageFilePath"), FColor::Yellow);
	return false;
}

void ACombatPhotoTarget::EnsureDefaultGallery()
{
	if (ImageFilePaths.Num() == 0 && !ImageFilePath.IsEmpty())
	{
		ImageFilePaths.Add(ImageFilePath);
	}

	if (ImageFilePaths.Num() == 0 && PhotoTextures.Num() == 0 && !PhotoTexture)
	{
		ImageFilePaths = {
			TEXT("E:/xwechat_files/wxid_m6ymg5am5zwv22_6ef7/temp/RWTemp/2026-06/9e20f478899dc29eb19741386f9343c8/7291b24fde909dbf84bc632077c2b406.jpg"),
			TEXT("E:/xwechat_files/wxid_m6ymg5am5zwv22_6ef7/temp/RWTemp/2026-06/9e20f478899dc29eb19741386f9343c8/6a80bb8c5513aba7dc8dd7346811db5a.jpg"),
			TEXT("E:/xwechat_files/wxid_m6ymg5am5zwv22_6ef7/temp/RWTemp/2026-06/9e20f478899dc29eb19741386f9343c8/a7fe3af3b42aa77f478442b532f6785b.jpg"),
			TEXT("E:/xwechat_files/wxid_m6ymg5am5zwv22_6ef7/temp/RWTemp/2026-06/9e20f478899dc29eb19741386f9343c8/85d0548eba03a639504db856a14105e4.jpg"),
			TEXT("E:/xwechat_files/wxid_m6ymg5am5zwv22_6ef7/temp/RWTemp/2026-06/9e20f478899dc29eb19741386f9343c8/8e77812c546f189fdcdb73057c1a663c.jpg"),
			TEXT("E:/xwechat_files/wxid_m6ymg5am5zwv22_6ef7/temp/RWTemp/2026-06/9e20f478899dc29eb19741386f9343c8/62188bd95bd257691aefc3f04b1ec2d4.jpg"),
			TEXT("E:/xwechat_files/wxid_m6ymg5am5zwv22_6ef7/temp/RWTemp/2026-06/9e20f478899dc29eb19741386f9343c8/e41235d9b58025fa794508b56dc14446.png"),
			TEXT("E:/xwechat_files/wxid_m6ymg5am5zwv22_6ef7/temp/RWTemp/2026-06/9e20f478899dc29eb19741386f9343c8/2d594f53a5c300ffe2ad2ae4c2127cb8.jpg")
		};
	}

	if (HitPhrases.Num() == 0)
	{
		HitPhrases = {
			TEXT("ABSTRACT +10"),
			TEXT("MEME HIT"),
			TEXT("PHOTO SHUFFLE"),
			TEXT("TARGET BROKEN"),
			TEXT("COMBO CHECK OK"),
			TEXT("COLLISION VERIFIED")
		};
	}

	if (DeathPhrases.Num() == 0)
	{
		DeathPhrases = {
			TEXT("梗图开奖"),
			TEXT("靶子破防了"),
			TEXT("这一刀有节目效果"),
			TEXT("抽象值爆表"),
			TEXT("连招检测通过"),
			TEXT("下一张准备上场")
		};
	}

	const int32 PhotoCount = GetPhotoCount();
	if (PhotoCount > 0)
	{
		CurrentPhotoIndex = FMath::Clamp(CurrentPhotoIndex, 0, PhotoCount - 1);
	}
}

bool ACombatPhotoTarget::SetPhotoIndex(int32 NewPhotoIndex)
{
	const int32 PhotoCount = GetPhotoCount();
	if (PhotoCount <= 0)
	{
		return false;
	}

	CurrentPhotoIndex = FMath::Clamp(NewPhotoIndex, 0, PhotoCount - 1);
	HitsOnCurrentPhoto = 0;
	return ReloadPhoto();
}

bool ACombatPhotoTarget::AdvancePhoto()
{
	const int32 PhotoCount = GetPhotoCount();
	if (PhotoCount <= 0)
	{
		return false;
	}

	if (bRandomizeNextPhoto && PhotoCount > 1)
	{
		int32 NextIndex = CurrentPhotoIndex;
		while (NextIndex == CurrentPhotoIndex)
		{
			NextIndex = FMath::RandRange(0, PhotoCount - 1);
		}
		return SetPhotoIndex(NextIndex);
	}

	return SetPhotoIndex((CurrentPhotoIndex + 1) % PhotoCount);
}

void ACombatPhotoTarget::SetShowPopupOnHit(bool bNewShowPopupOnHit)
{
	bShowPopupOnHit = bNewShowPopupOnHit;
}

void ACombatPhotoTarget::SetBlockCharacters(bool bNewBlockCharacters)
{
	bBlockCharacters = bNewBlockCharacters;
	ConfigureTargetCollision();
}

void ACombatPhotoTarget::TestPopup()
{
	ShowMemePopup();
}

void ACombatPhotoTarget::TriggerBasketballPopup()
{
	if (bRandomizePhotoOnBasketballHit)
	{
		const int32 PhotoCount = GetPhotoCount();
		if (PhotoCount > 1)
		{
			int32 NextPhotoIndex = CurrentPhotoIndex;
			while (NextPhotoIndex == CurrentPhotoIndex)
			{
				NextPhotoIndex = FMath::RandRange(0, PhotoCount - 1);
			}

			SetPhotoIndex(NextPhotoIndex);
		}
	}

	ShowMemePopup();
}

bool ACombatPhotoTarget::LoadTextureForCurrentIndex()
{
	if (PhotoTextures.IsValidIndex(CurrentPhotoIndex) && PhotoTextures[CurrentPhotoIndex])
	{
		RuntimeTexture = PhotoTextures[CurrentPhotoIndex];
		return true;
	}

	if (PhotoTexture)
	{
		RuntimeTexture = PhotoTexture;
		return true;
	}

	if (ImageFilePaths.IsValidIndex(CurrentPhotoIndex) && !ImageFilePaths[CurrentPhotoIndex].IsEmpty())
	{
		RuntimeTexture = FImageUtils::ImportFileAsTexture2D(ImageFilePaths[CurrentPhotoIndex]);
		if (RuntimeTexture)
		{
			return true;
		}

		UE_LOG(LogTemp, Warning, TEXT("Photo target failed to load image: %s"), *ImageFilePaths[CurrentPhotoIndex]);
		PrintDebugMessage(TEXT("PHOTO TARGET: gallery image load failed"), FColor::Red);
	}

	if (!ImageFilePath.IsEmpty())
	{
		RuntimeTexture = FImageUtils::ImportFileAsTexture2D(ImageFilePath);
		if (RuntimeTexture)
		{
			return true;
		}

		UE_LOG(LogTemp, Warning, TEXT("Photo target failed to load image: %s"), *ImageFilePath);
		PrintDebugMessage(TEXT("PHOTO TARGET: image load failed"), FColor::Red);
	}

	return false;
}

int32 ACombatPhotoTarget::GetPhotoCount() const
{
	int32 PhotoCount = FMath::Max(PhotoTextures.Num(), ImageFilePaths.Num());
	if (PhotoCount <= 0 && (PhotoTexture || !ImageFilePath.IsEmpty()))
	{
		PhotoCount = 1;
	}

	return PhotoCount;
}

FString ACombatPhotoTarget::GetCurrentHitPhrase() const
{
	if (HitPhrases.Num() == 0)
	{
		return TEXT("PHOTO TARGET HIT");
	}

	return HitPhrases[TotalHits % HitPhrases.Num()];
}

FString ACombatPhotoTarget::GetCurrentDeathPhrase() const
{
	if (DeathPhrases.Num() == 0)
	{
		return TEXT("MEME UNLOCKED");
	}

	return DeathPhrases[TotalHits % DeathPhrases.Num()];
}

void ACombatPhotoTarget::ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse)
{
	EnsureDefaultGallery();
	++TotalHits;
	++HitsOnCurrentPhoto;
	Score += FMath::Max(1, FMath::RoundToInt(Damage * 10.0f));
	CurrentHP = FMath::Clamp(CurrentHP - 1.0f, 0.0f, MaxHP);

	const FVector ImpactPoint = DamageLocation.IsNearlyZero() ? GetActorLocation() : DamageLocation;
	UE_LOG(LogTemp, Warning, TEXT("Photo target hit. Damage=%.2f RemainingHP=%.2f Causer=%s Impact=%s"),
		Damage,
		CurrentHP,
		*GetNameSafe(DamageCauser),
		*ImpactPoint.ToCompactString());

	const FString HitPhrase = GetCurrentHitPhrase();
	PrintDebugMessage(FString::Printf(TEXT("%s | SCORE %d | HP %.1f | PIC %d/%d"), *HitPhrase, Score, CurrentHP, CurrentPhotoIndex + 1, GetPhotoCount()), FColor::Green);

	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, ImpactPoint, HitSoundVolume, HitSoundPitch);
	}

	if (bDrawDebugOnHit && GetWorld())
	{
		DrawDebugSphere(GetWorld(), ImpactPoint, 24.0f, 16, FColor::Green, false, DebugDrawDuration, 0, 3.0f);
		DrawDebugCircle(GetWorld(), ImpactPoint, 42.0f, 32, FColor::Yellow, false, DebugDrawDuration, 0, 3.0f, FVector::UpVector, FVector::RightVector, false);
		DrawDebugString(GetWorld(), GetActorLocation() + FVector(0.0f, 0.0f, TargetSize.Y * 0.5f + 25.0f), FString::Printf(TEXT("%s  Score %d"), *HitPhrase, Score), nullptr, FColor::Green, DebugDrawDuration, true);
	}

	OnDamaged.Broadcast(Damage, ImpactPoint, CurrentHP);

	if (CurrentHP <= 0.0f)
	{
		HandleDeath();
		return;
	}

	if (bShowPopupOnHit && !bShowPhotoOnlyOnDeath)
	{
		if (bAdvancePhotoOnHit && HitsOnCurrentPhoto >= HitsPerPhoto)
		{
			if (AdvancePhoto())
			{
				PrintDebugMessage(FString::Printf(TEXT("PHOTO SHUFFLE -> %d/%d"), CurrentPhotoIndex + 1, GetPhotoCount()), FColor::Yellow);
			}
		}

		ShowMemePopup();
	}
}

void ACombatPhotoTarget::HandleDeath()
{
	if (bAdvancePhotoOnDeath)
	{
		AdvancePhoto();
	}

	ShowMemePopup();
	PrintDebugMessage(FString::Printf(TEXT("%s | SCORE %d | MEME %d/%d"), *GetCurrentDeathPhrase(), Score, CurrentPhotoIndex + 1, GetPhotoCount()), FColor::Red);

	if (bResetHealthOnDeath)
	{
		CurrentHP = MaxHP;
	}
}

void ACombatPhotoTarget::ApplyHealing(float Healing, AActor* Healer)
{
	CurrentHP = FMath::Clamp(CurrentHP + Healing, 0.0f, MaxHP);
}

void ACombatPhotoTarget::NotifyDanger(const FVector& DangerLocation, AActor* DangerSource)
{
	// The photo target is a static test object.
}

void ACombatPhotoTarget::ConfigureTargetCollision()
{
	if (!HitBox)
	{
		return;
	}

	HitBox->SetCollisionObjectType(ECC_WorldDynamic);
	HitBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HitBox->SetGenerateOverlapEvents(true);
	HitBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	HitBox->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	HitBox->SetCollisionResponseToChannel(ECC_Pawn, bBlockCharacters ? ECR_Block : ECR_Overlap);
	HitBox->SetCanEverAffectNavigation(false);
}

void ACombatPhotoTarget::UpdateTargetShape()
{
	ConfigureTargetCollision();
	const FVector2D SafeSize(FMath::Max(TargetSize.X, 1.0f), FMath::Max(TargetSize.Y, 1.0f));
	HitBox->SetBoxExtent(FVector(8.0f, SafeSize.X * 0.5f, SafeSize.Y * 0.5f));
	TargetMesh->SetRelativeScale3D(FVector(0.08f, SafeSize.X / 100.0f, SafeSize.Y / 100.0f));
	TargetMesh->SetRelativeLocation(FVector(2.0f, 0.0f, 0.0f));
	PhotoWidget->SetDrawSize(FVector2D(512.0f, 512.0f * (SafeSize.Y / SafeSize.X)));
	PhotoWidget->SetRelativeScale3D(FVector(WidgetWorldScale));
	PhotoWidget->SetRelativeLocation(FVector(-12.0f, 0.0f, SafeSize.Y * 0.15f));
	PhotoBillboard->SetRelativeScale3D(FVector(FMath::Max(WidgetWorldScale * 6.0f, 0.1f)));
}

void ACombatPhotoTarget::ApplyTextureToWidget()
{
	if (!PhotoWidget)
	{
		return;
	}

	PhotoWidget->SetWidgetClass(UCombatPhotoTargetWidget::StaticClass());
	PhotoWidget->InitWidget();

	if (UCombatPhotoTargetWidget* TargetWidget = Cast<UCombatPhotoTargetWidget>(PhotoWidget->GetUserWidgetObject()))
	{
		TargetWidget->SetTargetTexture(RuntimeTexture);
	}
}

void ACombatPhotoTarget::ApplyTextureToBillboard()
{
	if (!PhotoBillboard || !RuntimeTexture)
	{
		return;
	}

	PhotoBillboard->SetSprite(RuntimeTexture);
}

void ACombatPhotoTarget::ApplyTextureToVisuals()
{
	ApplyTextureToWidget();
	ApplyTextureToBillboard();
}

void ACombatPhotoTarget::ShowMemePopup()
{
	const bool bHasPhoto = ReloadPhoto();
	if (!bHasPhoto)
	{
		PrintDebugMessage(TEXT("PHOTO POPUP TRIGGERED, BUT PHOTO LOAD FAILED"), FColor::Red);
	}

	if (PhotoWidget && bHasPhoto)
	{
		PhotoWidget->SetVisibility(true);
		PhotoWidget->SetHiddenInGame(false);
	}

	if (PhotoBillboard && bHasPhoto)
	{
		PhotoBillboard->SetVisibility(true);
		PhotoBillboard->SetHiddenInGame(false);
	}

	if (PopupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PopupSound, GetActorLocation(), PopupSoundVolume, PopupSoundPitch);
	}

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(HideMemePopupTimer);
		GetWorld()->GetTimerManager().SetTimer(HideMemePopupTimer, this, &ACombatPhotoTarget::HideMemePopup, MemePopupDuration, false);
		DrawDebugString(GetWorld(), GetActorLocation() + FVector(0.0f, 0.0f, TargetSize.Y * 0.65f), GetCurrentDeathPhrase(), nullptr, FColor::Red, MemePopupDuration, true);
	}
}

void ACombatPhotoTarget::HideMemePopup()
{
	if (PhotoWidget)
	{
		PhotoWidget->SetVisibility(false);
		PhotoWidget->SetHiddenInGame(true);
	}

	if (PhotoBillboard)
	{
		PhotoBillboard->SetVisibility(false);
		PhotoBillboard->SetHiddenInGame(true);
	}
}

void ACombatPhotoTarget::PrintDebugMessage(const FString& Message, const FColor& Color) const
{
	if (!bPrintHitMessages || !GEngine)
	{
		return;
	}

	GEngine->AddOnScreenDebugMessage(-1, DebugDrawDuration, Color, Message);
}
