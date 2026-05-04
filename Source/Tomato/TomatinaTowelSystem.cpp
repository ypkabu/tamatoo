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
#include "TomatinaFunctionLibrary.h"  // PlayTomatinaCue2D

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
		TEXT("ATomatinaTowelSystem::BeginPlay: 初期化完了 Durability=%.1f MinSpeedToWipe=%.2f WipeRadius=%.3f"),
		CurrentDurability, MinSpeedToWipe, WipeRadius);

	// [DIAG] パッケージ確認用：自分が生きていることのマーカー
	UE_LOG(LogTemp, Warning,
		TEXT("[TowelDiag] ATomatinaTowelSystem started — もし以後 [TowelDiag] UpdateHandData が出なければ BP 側が UpdateHandData を呼んでいない"));
}

// =============================================================================
// UpdateHandData（BP から毎フレーム呼ぶ）
// =============================================================================

void ATomatinaTowelSystem::UpdateHandData(bool bDetected, FVector2D ScreenPosition, float Speed)
{
	bHandDetected         = bDetected;
	RawHandScreenPosition = ScreenPosition;
	ClampedHandScreenPosition = ClampNormalizedHandPosition(ScreenPosition);
	HandScreenPosition = ClampedHandScreenPosition;
	HandSpeed             = FMath::Max(0.0f, Speed);

	// [DIAG] パッケージ版でも残るように Warning レベル。1秒に1回だけ出力（毎フレームはスパム）
	static double LastDiagTime = 0.0;
	const double Now = FPlatformTime::Seconds();
	if (bDebugLeapInput && Now - LastDiagTime > 1.0)
	{
		LastDiagTime = Now;
		UE_LOG(LogTemp, Warning,
			TEXT("[TowelDiag] UpdateHandData Detected=%d Raw=(%.3f,%.3f) Speed=%.2f (MinSpeedToWipe=%.1f)"),
			bDetected ? 1 : 0,
			ScreenPosition.X, ScreenPosition.Y,
			Speed, MinSpeedToWipe);
	}
}

// =============================================================================
// Tick
// =============================================================================

void ATomatinaTowelSystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	APlayerController* PC  = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	ATomatinaHUD*       HUD = PC ? Cast<ATomatinaHUD>(PC->GetHUD()) : nullptr;

	const float NowSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	bHasValidInput = false;
	bUsingGraceInput = false;
	ProcessedHandSpeed = 0.0f;

	if (bHandDetected)
	{
		bHasValidInput = true;
		SmoothedHandScreenPosition = ApplyHandSmoothing(RawHandScreenPosition, DeltaTime);
		ClampedHandScreenPosition = ClampNormalizedHandPosition(SmoothedHandScreenPosition);
		HandScreenPosition = ClampedHandScreenPosition;

		// 速度は Clamp 後の座標差分ではなく BP 由来の生速度を使う。
		// 画面端で座標が 0/1 に張り付いても、拭き取り/SE の速度判定を不自然に落とさないため。
		ProcessedHandSpeed = HandSpeed;

		bHasEverValidInput = true;
		LastValidInputTime = NowSeconds;
		LastValidHandSpeed = ProcessedHandSpeed;
		LastValidRawHandScreenPosition = RawHandScreenPosition;
		LastValidSmoothedHandScreenPosition = SmoothedHandScreenPosition;
		LastValidClampedHandScreenPosition = ClampedHandScreenPosition;
	}
	else if (bHasEverValidInput && InputGraceTime > 0.0f)
	{
		const float LostTime = NowSeconds - LastValidInputTime;
		if (LostTime <= InputGraceTime)
		{
			bHasValidInput = true;
			bUsingGraceInput = true;

			// 短時間ロスト中は最後のスムージング済み座標を保持し、速度だけ線形に落とす。
			// これにより端で一瞬外れても拭き取り中心は途切れにくく、長く拭き続けることはない。
			const float GraceAlpha = 1.0f - FMath::Clamp(LostTime / FMath::Max(InputGraceTime, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
			SmoothedHandScreenPosition = LastValidSmoothedHandScreenPosition;
			ClampedHandScreenPosition = LastValidClampedHandScreenPosition;
			HandScreenPosition = ClampedHandScreenPosition;
			ProcessedHandSpeed = LastValidHandSpeed * GraceAlpha;
		}
	}

	if (!bHasValidInput)
	{
		bTowelVisible    = false;
		bTowelInZoomView = false;
		ProcessedHandSpeed = 0.0f;
		ResetHandInputFilter();

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
			ClampedHandScreenPosition.X * HUD->MainWidth,
			ClampedHandScreenPosition.Y * HUD->MainHeight);
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

			// 交換完了 SE
			UTomatinaFunctionLibrary::PlayTomatinaCue2D(this, TowelReadySound);
		}
		return;
	}

	// 拭き取り処理
	const bool bIsWiping = (ProcessedHandSpeed >= MinSpeedToWipe);
	UpdateWipeSound(bIsWiping);

	// [DIAG] 拭き状態が変化した瞬間だけログ
	{
		static bool bDiagWasWiping = false;
		if (bDebugLeapInput && bIsWiping != bDiagWasWiping)
		{
			bDiagWasWiping = bIsWiping;
			UE_LOG(LogTemp, Warning,
				TEXT("[TowelDiag] WipingState=%s ProcessedSpeed=%.2f RawSpeed=%.2f Min=%.2f"),
				bIsWiping ? TEXT("ON") : TEXT("OFF"),
				ProcessedHandSpeed, HandSpeed, MinSpeedToWipe);
		}
	}

	if (bIsWiping)
	{
		CurrentDurability -= DurabilityDrainRate * DeltaTime;

		const float Amount = WipeEfficiency * ProcessedHandSpeed * SpeedMultiplier * DeltaTime;
		const float AdjustedWipeRadius = CalculateEdgeAdjustedWipeRadius(ClampedHandScreenPosition);
		if (ATomatoDirtManager* DirtMgr = GetDirtManager())
		{
			DirtMgr->WipeDirtAt(ClampedHandScreenPosition, AdjustedWipeRadius, Amount);

			// [DIAG] 1秒に1回だけ拭き量と DirtMgr 取得状態を出す
			static double LastWipeDiagTime = 0.0;
			const double Now2 = FPlatformTime::Seconds();
			if (bDebugLeapInput && Now2 - LastWipeDiagTime > 1.0)
			{
				LastWipeDiagTime = Now2;
				UE_LOG(LogTemp, Warning,
					TEXT("[TowelDiag] Wipe Amount=%.4f Raw=(%.3f,%.3f) Smooth=(%.3f,%.3f) Clamp=(%.3f,%.3f) Valid=%d Grace=%d Speed=%.2f Radius=%.3f DirtMgr=OK"),
					Amount,
					RawHandScreenPosition.X, RawHandScreenPosition.Y,
					SmoothedHandScreenPosition.X, SmoothedHandScreenPosition.Y,
					ClampedHandScreenPosition.X, ClampedHandScreenPosition.Y,
					bHasValidInput ? 1 : 0,
					bUsingGraceInput ? 1 : 0,
					ProcessedHandSpeed,
					AdjustedWipeRadius);
			}
		}
		else
		{
			static double LastNoMgrLogTime = 0.0;
			const double Now3 = FPlatformTime::Seconds();
			if (bDebugLeapInput && Now3 - LastNoMgrLogTime > 2.0)
			{
				LastNoMgrLogTime = Now3;
				UE_LOG(LogTemp, Warning,
					TEXT("[TowelDiag] Wiping but DirtManager=NULL（レベルに ATomatoDirtManager 配置されてない可能性）"));
			}
		}

		if (CurrentDurability <= 0.f)
		{
			CurrentDurability = 0.f;
			bIsSwapping       = true;
			SwapTimer         = SwapDuration;
			UE_LOG(LogTemp, Warning,
				TEXT("ATomatinaTowelSystem: 耐久値切れ → タオル交換開始 (%.1f 秒)"), SwapDuration);

			// タオル破損 SE
			UTomatinaFunctionLibrary::PlayTomatinaCue2D(this, TowelBreakSound);
		}
	}

	// ズーム映像へのタオル映り込み判定
	bTowelInZoomView = CheckTowelInView(ClampedHandScreenPosition);
}

// =============================================================================
// Leap Motion 入力安定化
// =============================================================================

FVector2D ATomatinaTowelSystem::ClampNormalizedHandPosition(FVector2D Position) const
{
	return FVector2D(
		FMath::Clamp(Position.X, 0.0f, 1.0f),
		FMath::Clamp(Position.Y, 0.0f, 1.0f));
}

FVector2D ATomatinaTowelSystem::ApplyHandSmoothing(FVector2D RawPosition, float DeltaTime)
{
	if (!bHasSmoothedHandPosition)
	{
		bHasSmoothedHandPosition = true;
		OneEuroX = FOneEuroAxisState();
		OneEuroY = FOneEuroAxisState();
		return RawPosition;
	}

	switch (SmoothingMode)
	{
	case EHandSmoothingMode::EMA:
	{
		const float Alpha = FMath::Clamp(EMAAlpha, 0.0f, 1.0f);
		return FMath::Lerp(SmoothedHandScreenPosition, RawPosition, Alpha);
	}
	case EHandSmoothingMode::OneEuro:
		return FVector2D(
			ApplyOneEuroAxis(RawPosition.X, DeltaTime, OneEuroX),
			ApplyOneEuroAxis(RawPosition.Y, DeltaTime, OneEuroY));
	case EHandSmoothingMode::None:
	default:
		return RawPosition;
	}
}

float ATomatinaTowelSystem::ApplyOneEuroAxis(float Value, float DeltaTime, FOneEuroAxisState& AxisState) const
{
	const float SafeDeltaTime = FMath::Max(DeltaTime, KINDA_SMALL_NUMBER);
	if (!AxisState.bInitialized)
	{
		AxisState.bInitialized = true;
		AxisState.PreviousRaw = Value;
		AxisState.PreviousFiltered = Value;
		AxisState.PreviousDerivative = 0.0f;
		return Value;
	}

	const float RawDerivative = (Value - AxisState.PreviousRaw) / SafeDeltaTime;
	const float DerivativeAlpha = CalculateOneEuroAlpha(OneEuroDCutoff, SafeDeltaTime);
	const float FilteredDerivative = FMath::Lerp(AxisState.PreviousDerivative, RawDerivative, DerivativeAlpha);
	const float DynamicCutoff = FMath::Max(0.001f, OneEuroMinCutoff + OneEuroBeta * FMath::Abs(FilteredDerivative));
	const float PositionAlpha = CalculateOneEuroAlpha(DynamicCutoff, SafeDeltaTime);
	const float FilteredValue = FMath::Lerp(AxisState.PreviousFiltered, Value, PositionAlpha);

	AxisState.PreviousRaw = Value;
	AxisState.PreviousFiltered = FilteredValue;
	AxisState.PreviousDerivative = FilteredDerivative;
	return FilteredValue;
}

float ATomatinaTowelSystem::CalculateOneEuroAlpha(float Cutoff, float DeltaTime) const
{
	const float SafeCutoff = FMath::Max(Cutoff, 0.001f);
	const float Tau = 1.0f / (2.0f * PI * SafeCutoff);
	return 1.0f / (1.0f + Tau / FMath::Max(DeltaTime, KINDA_SMALL_NUMBER));
}

float ATomatinaTowelSystem::CalculateEdgeAdjustedWipeRadius(FVector2D ClampedPosition) const
{
	const float BaseRadius = FMath::Max(0.0f, WipeRadius);
	if (BaseRadius <= 0.0f || EdgeThreshold <= 0.0f || EdgeRadiusBoost <= 0.0f)
	{
		return BaseRadius;
	}

	const float DistanceToEdge = FMath::Min(
		FMath::Min(ClampedPosition.X, 1.0f - ClampedPosition.X),
		FMath::Min(ClampedPosition.Y, 1.0f - ClampedPosition.Y));
	const float EdgeAlpha = 1.0f - FMath::Clamp(DistanceToEdge / EdgeThreshold, 0.0f, 1.0f);
	const float BoostedRadius = BaseRadius + EdgeRadiusBoost * EdgeAlpha;
	const float MaxRadius = BaseRadius * FMath::Max(1.0f, MaxEdgeRadiusMultiplier);
	return FMath::Clamp(BoostedRadius, BaseRadius, MaxRadius);
}

void ATomatinaTowelSystem::ResetHandInputFilter()
{
	bHasSmoothedHandPosition = false;
	OneEuroX = FOneEuroAxisState();
	OneEuroY = FOneEuroAxisState();
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
