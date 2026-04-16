// Fill out your copyright notice in the Description page of Project Settings.

#include "TomatinaTowelSystem.h"

#include "LeapComponent.h"
#include "UltraleapTrackingData.h"
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

	LeapComp = CreateDefaultSubobject<ULeapComponent>(TEXT("LeapComponent"));
}

// =============================================================================
// BeginPlay
// =============================================================================

void ATomatinaTowelSystem::BeginPlay()
{
	Super::BeginPlay();

	CurrentDurability = MaxDurability;
	PrevHandPos = HandScreenPosition;

	// デスクトップ設置想定（デバイスが上向き）
	// スクリーントップ設置の場合は LEAP_MODE_SCREENTOP に変更すること
	LeapComp->SetTrackingMode(ELeapMode::LEAP_MODE_DESKTOP);

	UE_LOG(LogTemp, Warning,
		TEXT("ATomatinaTowelSystem::BeginPlay: 初期化完了 Durability=%.1f"), CurrentDurability);
}

// =============================================================================
// Tick
// =============================================================================

void ATomatinaTowelSystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// ── LeapComponent からフレームデータを取得 ──────────────────────────────
	FLeapFrameData Frame;
	LeapComp->GetLatestFrameData(Frame);

	if (Frame.Hands.Num() > 0)
	{
		// 最初の手を使用（2P は片手操作想定）
		const FLeapHandData& Hand = Frame.Hands[0];

		bHandDetected = true;

		// 手のひらの位置（プラグインが mm→cm 変換済み。範囲変数は mm 基準なので 10 倍してスケール）
		// ※ Palm.Position の単位はプラグインが cm に変換している
		//   EditAnywhere の LeapRange 変数は mm 基準で入力するため
		//   比較には Hand.Palm.Position をそのまま使い、範囲だけ cm に合わせる
		const FVector PalmPos = Hand.Palm.Position;

		// 左右（X 軸）を 0〜1 に正規化
		const float NormX = FMath::GetMappedRangeValueClamped(
			FVector2D(LeapRangeXMin, LeapRangeXMax),
			FVector2D(0.0f, 1.0f),
			PalmPos.X);

		// 高さ（Y 軸）を 0〜1 に正規化（Y 反転：上が 0）
		const float NormY = FMath::GetMappedRangeValueClamped(
			FVector2D(LeapRangeYMin, LeapRangeYMax),
			FVector2D(1.0f, 0.0f),
			PalmPos.Y);

		const FVector2D NewPos(NormX, NormY);

		// 前フレームとの差分で速度算出（正規化座標/秒）
		HandSpeed = (DeltaTime > 0.0f)
			? FVector2D::Distance(PrevHandPos, NewPos) / DeltaTime
			: 0.0f;

		PrevHandPos        = HandScreenPosition;
		HandScreenPosition = NewPos;
	}
	else
	{
		bHandDetected = false;
		HandSpeed     = 0.0f;
	}

	// ── 以降は既存の拭き取り・HUD 更新処理 ──────────────────────────────────

	APlayerController* PC  = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	ATomatinaHUD*       HUD = PC ? Cast<ATomatinaHUD>(PC->GetHUD()) : nullptr;

	// 手が検出されていない
	if (!bHandDetected)
	{
		bTowelVisible    = false;
		bTowelInZoomView = false;

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
	                           / FMath::Max(Pawn->SceneCapture_Zoom->FOVAngle, 1.f);
	const float ViewHalfSize = 0.5f / ZoomRatio;

	const bool bInView =
		FMath::Abs(TowelNormPos.X - NormZoomCenter.X) < ViewHalfSize &&
		FMath::Abs(TowelNormPos.Y - NormZoomCenter.Y) < ViewHalfSize;

	UE_LOG(LogTemp, Log,
		TEXT("ATomatinaTowelSystem::CheckTowelInView: Towel=(%.2f,%.2f) "
		     "ZoomCenter=(%.2f,%.2f) HalfSize=%.3f → %s"),
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
