// Fill out your copyright notice in the Description page of Project Settings.

#include "TomatinaTowelSystem.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

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

	UE_LOG(LogTemp, Warning, TEXT("ATomatinaTowelSystem::BeginPlay: 初期化完了 Durability=%.1f"), CurrentDurability);
}

// =============================================================================
// Tick
// =============================================================================

void ATomatinaTowelSystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	ATomatinaHUD* HUD = PC ? Cast<ATomatinaHUD>(PC->GetHUD()) : nullptr;

	// ── 手が検出されていない ────────────────────────────────────────────────
	if (!bHandDetected)
	{
		bTowelVisible    = false;
		bTowelInZoomView = false;

		if (HUD)
		{
			HUD->HideCursor();
		}
		return;
	}

	// ── 手検出中：タオルを表示 ───────────────────────────────────────────────
	bTowelVisible = true;

	if (HUD)
	{
		// HandScreenPosition（正規化）を HUD のカーソル座標（スクリーンピクセル）に変換して渡す
		// 実際の画面サイズは HUD から取得する
		const FVector2D ScreenPos(
			HandScreenPosition.X * HUD->MainWidth,
			HandScreenPosition.Y * HUD->MainHeight);
		HUD->UpdateCursorPosition(ScreenPos);
		HUD->ShowCursor();
	}

	// ── タオル交換中 ─────────────────────────────────────────────────────────
	if (bIsSwapping)
	{
		SwapTimer -= DeltaTime;
		if (SwapTimer <= 0.f)
		{
			bIsSwapping       = false;
			CurrentDurability = MaxDurability;
			UE_LOG(LogTemp, Warning, TEXT("ATomatinaTowelSystem: タオル交換完了 Durability=%.1f"), CurrentDurability);
			// HUD へ「交換完了」を通知（Widget で bIsSwapping を Binding して表示切替）
		}
		return;  // 交換中は拭けない
	}

	// ── 拭き取り処理 ─────────────────────────────────────────────────────────
	const bool bIsWiping = (HandSpeed >= MinSpeedToWipe);

	if (bIsWiping)
	{
		// 耐久値を減らす
		CurrentDurability -= DurabilityDrainRate * DeltaTime;

		// DirtManager に漸進的な拭き取りを依頼
		const float Efficiency = WipeEfficiency * HandSpeed * SpeedMultiplier;
		if (ATomatoDirtManager* DirtMgr = GetDirtManager())
		{
			DirtMgr->WipeDirtAt(HandScreenPosition, WipeRadius, Efficiency * DeltaTime);
		}

		// 耐久値切れ → タオル交換開始
		if (CurrentDurability <= 0.f)
		{
			CurrentDurability = 0.f;
			bIsSwapping       = true;
			SwapTimer         = SwapDuration;
			UE_LOG(LogTemp, Warning, TEXT("ATomatinaTowelSystem: 耐久値切れ → タオル交換開始 (%.1f 秒)"), SwapDuration);
			// HUD へ「タオル交換中」通知
		}
	}

	// ── ズーム映像へのタオル映り込み判定 ────────────────────────────────────
	bTowelInZoomView = CheckTowelInView(HandScreenPosition);
}

// =============================================================================
// CheckTowelInView
// =============================================================================

bool ATomatinaTowelSystem::CheckTowelInView(FVector2D TowelNormPos)
{
	ATomatinaPlayerPawn* Pawn = GetPlayerPawn();
	if (!Pawn || !Pawn->bIsZooming)
	{
		return false;
	}

	if (!Pawn->SceneCapture_Zoom)
	{
		return false;
	}

	APlayerController* PC = Pawn->GetController<APlayerController>();
	if (!PC)
	{
		return false;
	}

	// ズーム視界中心をスクリーン座標で取得し、正規化する
	const FVector2D ZoomScreenCenter = UTomatinaFunctionLibrary::ProjectZoomToMainScreen(
		PC, Pawn->SceneCapture_Zoom, Pawn->MainWidth, Pawn->MainHeight);

	const FVector2D NormZoomCenter(
		ZoomScreenCenter.X / FMath::Max(Pawn->MainWidth,  1.f),
		ZoomScreenCenter.Y / FMath::Max(Pawn->MainHeight, 1.f));

	// 現在の FOV から視界の正規化半径を計算
	// ZoomRatio が大きいほどズームが深く、視界が狭い
	const float ZoomRatio    = Pawn->DefaultFOV
	                           / FMath::Max(Pawn->SceneCapture_Zoom->FOVAngle, 1.f);
	const float ViewHalfSize = 0.5f / ZoomRatio;

	const bool bInView =
		FMath::Abs(TowelNormPos.X - NormZoomCenter.X) < ViewHalfSize &&
		FMath::Abs(TowelNormPos.Y - NormZoomCenter.Y) < ViewHalfSize;

	UE_LOG(LogTemp, Log,
		TEXT("ATomatinaTowelSystem::CheckTowelInView: Towel=(%.2f,%.2f) ZoomCenter=(%.2f,%.2f) HalfSize=%.3f → %s"),
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
		AActor* Found = UGameplayStatics::GetActorOfClass(GetWorld(), ATomatoDirtManager::StaticClass());
		CachedDirtManager = Cast<ATomatoDirtManager>(Found);

		if (!CachedDirtManager)
		{
			UE_LOG(LogTemp, Warning, TEXT("ATomatinaTowelSystem::GetDirtManager: ATomatoDirtManager がレベル上に見つかりません"));
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
		if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
		{
			CachedPlayerPawn = Cast<ATomatinaPlayerPawn>(PC->GetPawn());
		}

		if (!CachedPlayerPawn)
		{
			UE_LOG(LogTemp, Warning, TEXT("ATomatinaTowelSystem::GetPlayerPawn: ATomatinaPlayerPawn の取得に失敗"));
		}
	}
	return CachedPlayerPawn;
}
