// Fill out your copyright notice in the Description page of Project Settings.

#include "TomatinaTowelSystem.h"

#include "Components/AudioComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Sound/SoundBase.h"

#include "TomatoDirtManager.h"
#include "TomatinaPlayerPawn.h"
#include "TomatinaHUD.h"
#include "TomatinaFunctionLibrary.h"

// =============================================================================
// コンストラクタ
// =============================================================================

ATomatinaTowelSystem::ATomatinaTowelSystem()
	: CachedDirtManager(nullptr)
	, CachedPlayerPawn(nullptr)
{
	PrimaryActorTick.bCanEverTick = true;
}

// =============================================================================
// BeginPlay
// =============================================================================

void ATomatinaTowelSystem::BeginPlay()
{
	Super::BeginPlay();

	CurrentDurability = MaxDurability;

	UE_LOG(LogTemp, Warning,
		TEXT("ATomatinaTowelSystem::BeginPlay: 初期化完了 Durability=%.1f"), CurrentDurability);
}

// =============================================================================
// UpdateHandData（BP から毎フレーム呼ぶ）
// =============================================================================

void ATomatinaTowelSystem::UpdateHandData(bool bDetected, FVector2D ScreenPosition, float Speed)
{
	bHandDetected      = bDetected;
	HandScreenPosition = ScreenPosition;
	HandSpeed          = Speed;
}

// =============================================================================
// Tick
// =============================================================================

void ATomatinaTowelSystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	APlayerController* PC  = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	ATomatinaHUD*       HUD = PC ? Cast<ATomatinaHUD>(PC->GetHUD()) : nullptr;

	// 手が検出されていない
	if (!bHandDetected)
	{
		bTowelVisible    = false;
		bTowelInZoomView = false;

		// 拭き音を止める（手が離れたら即停止）
		UpdateWipeSound(false);

		if (HUD) { HUD->HideCursor(); }
		return;
	}

	// 手検出中：タオルを表示
	bTowelVisible = true;

	if (HUD)
	{
		const FVector2D ScreenPos(
			HandScreenPosition.X * HUD->MainWidth,
			HandScreenPosition.Y * HUD->MainHeight);
		HUD->UpdateCursorPosition(ScreenPos);
		HUD->ShowCursor();
	}

	// タオル交換中
	if (bIsSwapping)
	{
		bTowelInZoomView = false;
		// 交換中は拭けないので音も止める
		UpdateWipeSound(false);

		SwapTimer -= DeltaTime;
		if (SwapTimer <= 0.f)
		{
			bIsSwapping       = false;
			CurrentDurability = MaxDurability;
			UE_LOG(LogTemp, Warning,
				TEXT("ATomatinaTowelSystem: タオル交換完了 Durability=%.1f"), CurrentDurability);
		}
		return;
	}

	// 拭き取り処理
	const bool bIsWiping = (HandSpeed >= MinSpeedToWipe);
	UpdateWipeSound(bIsWiping);

	if (bIsWiping)
	{
		CurrentDurability -= DurabilityDrainRate * DeltaTime;

		const float Amount = WipeEfficiency * HandSpeed * SpeedMultiplier * DeltaTime;
		if (ATomatoDirtManager* DirtMgr = GetDirtManager())
		{
			DirtMgr->WipeDirtAt(HandScreenPosition, WipeRadius, Amount);
		}

		if (CurrentDurability <= 0.f)
		{
			CurrentDurability = 0.f;
			bIsSwapping       = true;
			SwapTimer         = SwapDuration;
			UE_LOG(LogTemp, Warning,
				TEXT("ATomatinaTowelSystem: 耐久値切れ → タオル交換開始 (%.1f 秒)"), SwapDuration);
		}
	}

	// ズーム映像へのタオル映り込み判定
	bTowelInZoomView = CheckTowelInView(HandScreenPosition);
}

// =============================================================================
// CheckTowelInView
// =============================================================================

bool ATomatinaTowelSystem::CheckTowelInView(FVector2D TowelNormPos)
{
	ATomatinaPlayerPawn* Pawn = GetPlayerPawn();
	if (!Pawn || !Pawn->bIsZooming)  { return false; }
	if (!Pawn->SceneCapture_Zoom)    { return false; }

	APlayerController* PC = Pawn->GetController<APlayerController>();
	if (!PC) { return false; }

	const FVector2D ZoomScreenCenter = UTomatinaFunctionLibrary::ProjectZoomToMainScreen(
		PC, Pawn->SceneCapture_Zoom, Pawn->MainWidth, Pawn->MainHeight);

	const FVector2D NormZoomCenter(
		ZoomScreenCenter.X / FMath::Max(Pawn->MainWidth,  1.f),
		ZoomScreenCenter.Y / FMath::Max(Pawn->MainHeight, 1.f));

	const float ZoomRatio    = Pawn->DefaultFOV
	                           / FMath::Max<float>(Pawn->SceneCapture_Zoom->FOVAngle, 1.0f);
	const float ViewHalfSize = 0.5f / ZoomRatio;

	const bool bInView =
		FMath::Abs(TowelNormPos.X - NormZoomCenter.X) < ViewHalfSize &&
		FMath::Abs(TowelNormPos.Y - NormZoomCenter.Y) < ViewHalfSize;

	UE_LOG(LogTemp, Log,
		TEXT("ATomatinaTowelSystem::CheckTowelInView: "
		     "Towel=(%.2f,%.2f) ZoomCenter=(%.2f,%.2f) HalfSize=%.3f → %s"),
		TowelNormPos.X, TowelNormPos.Y,
		NormZoomCenter.X, NormZoomCenter.Y,
		ViewHalfSize,
		bInView ? TEXT("映込") : TEXT("範囲外"));

	return bInView;
}

// =============================================================================
// GetDirtManager（キャッシュ付き）
// =============================================================================

ATomatoDirtManager* ATomatinaTowelSystem::GetDirtManager()
{
	if (!CachedDirtManager)
	{
		AActor* Found = UGameplayStatics::GetActorOfClass(
			GetWorld(), ATomatoDirtManager::StaticClass());
		CachedDirtManager = Cast<ATomatoDirtManager>(Found);

		if (!CachedDirtManager)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("ATomatinaTowelSystem::GetDirtManager: "
				     "ATomatoDirtManager がレベル上に見つかりません"));
		}
	}
	return CachedDirtManager;
}

// =============================================================================
// GetPlayerPawn（キャッシュ付き）
// =============================================================================

ATomatinaPlayerPawn* ATomatinaTowelSystem::GetPlayerPawn()
{
	if (!CachedPlayerPawn)
	{
		if (APlayerController* PC =
			GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
		{
			CachedPlayerPawn = Cast<ATomatinaPlayerPawn>(PC->GetPawn());
		}

		if (!CachedPlayerPawn)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("ATomatinaTowelSystem::GetPlayerPawn: "
				     "ATomatinaPlayerPawn の取得に失敗"));
		}
	}
	return CachedPlayerPawn;
}

// =============================================================================
// UpdateWipeSound — 拭き状態のエッジ検出でループ SE を開始・停止
// =============================================================================

void ATomatinaTowelSystem::UpdateWipeSound(bool bIsWiping)
{
	if (bIsWiping == bWasWiping)
	{
		return; // 状態変化なし
	}
	bWasWiping = bIsWiping;

	if (bIsWiping)
	{
		if (!WipeLoopSound) { return; }
		// 既に再生中なら何もしない
		if (WipeAudioComp && WipeAudioComp->IsPlaying())
		{
			return;
		}
		WipeAudioComp = UGameplayStatics::SpawnSound2D(
			this, WipeLoopSound, WipeLoopVolume, WipeLoopPitch);
	}
	else
	{
		if (WipeAudioComp && WipeAudioComp->IsPlaying())
		{
			WipeAudioComp->Stop();
		}
		WipeAudioComp = nullptr;
	}
}
