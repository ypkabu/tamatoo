// Fill out your copyright notice in the Description page of Project Settings.

#include "TomatinaTowelSystem.h"

#include "Components/AudioComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "IUltraleapTrackingPlugin.h"
#include "LeapComponent.h"
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
	LeapComponent = CreateDefaultSubobject<ULeapComponent>(TEXT("LeapComponent"));
}

// =============================================================================
// BeginPlay
// =============================================================================

void ATomatinaTowelSystem::BeginPlay()
{
	Super::BeginPlay();

	CurrentDurability = MaxDurability;

	SetLeapInputStatus(bReadLeapInputInCpp ? TEXT("Waiting for Leap frame") : TEXT("C++ Leap input disabled"));

	if (bDebugLeapInput)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[TowelDiag] BeginPlay LeapCppInput=%d Durability=%.1f MinSpeedToWipe=%.2f WipeRadius=%.3f"),
			bReadLeapInputInCpp ? 1 : 0,
			CurrentDurability, MinSpeedToWipe, WipeRadius);
	}
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

	if (bDebugLeapInput && (!bHasLoggedBlueprintInputState || bDetected != bLastLoggedBlueprintInputDetected))
	{
		bHasLoggedBlueprintInputState = true;
		bLastLoggedBlueprintInputDetected = bDetected;
		UE_LOG(LogTemp, Warning,
			TEXT("[TowelDiag] UpdateHandData Detected=%d Raw=(%.3f,%.3f) Speed=%.2f"),
			bDetected ? 1 : 0,
			ScreenPosition.X, ScreenPosition.Y,
			Speed);
	}
}

// =============================================================================
// Tick
// =============================================================================

void ATomatinaTowelSystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	PollLeapInputFromCpp(DeltaTime);

	APlayerController* PC  = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	ATomatinaHUD*       HUD = PC ? Cast<ATomatinaHUD>(PC->GetHUD()) : nullptr;
	if (HUD)
	{
		HUD->UpdateLeapDistanceWarning(bEnableLeapTooCloseWarning && bLeapTooCloseToDevice);
	}

	const float NowSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	bHasValidInput = false;
	bUsingGraceInput = false;
	ProcessedHandSpeed = 0.0f;
	bWipeAttemptedThisFrame = false;
	bWipeHadDirtManagerThisFrame = false;
	LastWipeAmount = 0.0f;
	LastAdjustedWipeRadius = 0.0f;

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

	// タオル交換は入力が無い間も進める。先に no-input return すると、
	// 手を離した瞬間に交換タイマーと「交換中」UIが止まってしまう。
	if (bIsSwapping)
	{
		bTowelVisible = false;
		bTowelInZoomView = false;
		UpdateWipeSound(false);

		if (HUD && bTowelShownOnHUD)
		{
			HUD->HideTowel();
			bTowelShownOnHUD = false;
		}

		if (!bHasValidInput)
		{
			ProcessedHandSpeed = 0.0f;
			ResetHandInputFilter();
		}

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

		if (HUD)
		{
			HUD->UpdateTowelStatus(MaxDurability > KINDA_SMALL_NUMBER ? CurrentDurability / MaxDurability : 0.0f, bIsSwapping);
		}
		return;
	}

	if (!bHasValidInput)
	{
		bTowelVisible    = false;
		bTowelInZoomView = false;
		ProcessedHandSpeed = 0.0f;
		ResetHandInputFilter();

		// 拭き音を止める（手が離れたら即停止）
		UpdateWipeSound(false);

		if (HUD && bTowelShownOnHUD)
		{
			HUD->HideTowel();
			bTowelShownOnHUD = false;
		}
		if (HUD)
		{
			HUD->UpdateTowelStatus(MaxDurability > KINDA_SMALL_NUMBER ? CurrentDurability / MaxDurability : 0.0f, bIsSwapping);
		}
		return;
	}

	// 手検出中：タオルを表示
	bTowelVisible = true;

	if (HUD)
	{
		// Leapタオルはスマホ用カーソルではなく、メイン画面DirtOverlay上のIMG_Towelを動かす。
		HUD->UpdateTowelPosition(ClampedHandScreenPosition);
		if (!bTowelShownOnHUD)
		{
			HUD->ShowTowel();
			bTowelShownOnHUD = true;
		}
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
		bWipeAttemptedThisFrame = true;
		LastWipePosition = ClampedHandScreenPosition;
		LastAdjustedWipeRadius = AdjustedWipeRadius;
		LastWipeAmount = Amount;
		if (ATomatoDirtManager* DirtMgr = GetDirtManager())
		{
			bWipeHadDirtManagerThisFrame = true;
			DirtMgr->WipeDirtAt(ClampedHandScreenPosition, AdjustedWipeRadius, Amount);
		}
		else
		{
			if (bDebugLeapInput && !bWarnedMissingDirtManager)
			{
				bWarnedMissingDirtManager = true;
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

	if (HUD)
	{
		HUD->UpdateTowelStatus(MaxDurability > KINDA_SMALL_NUMBER ? CurrentDurability / MaxDurability : 0.0f, bIsSwapping);
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

void ATomatinaTowelSystem::PollLeapInputFromCpp(float DeltaTime)
{
	if (!bReadLeapInputInCpp)
	{
		SetLeapInputStatus(TEXT("C++ Leap input disabled"));
		UpdateLeapTooCloseState(FVector::ZeroVector, false);
		return;
	}

	if (!LeapComponent)
	{
		SetLeapInputStatus(TEXT("LeapComponent is null"));
		UpdateHandData(false, RawHandScreenPosition, 0.0f);
		UpdateLeapTooCloseState(FVector::ZeroVector, false);
		bHasLastCppLeapScreenPosition = false;
		bLastCppLeapHadSelectedHand = false;
		return;
	}

	FLeapFrameData Frame;
	LeapComponent->GetLatestFrameData(Frame, bApplyLeapDeviceOrigin);
	bLeapComponentHasDevice = LeapComponent->IsActiveDevicePluggedIn();
	LeapComponent->AreHandsVisible(bLastLeapLeftVisible, bLastLeapRightVisible);

	bool bUsedPluginFallback = false;
	if (Frame.Hands.Num() == 0 && bUseUltraleapPluginFallback && IUltraleapTrackingPlugin::IsAvailable())
	{
		FLeapFrameData FallbackFrame;
		IUltraleapTrackingPlugin::Get().GetLatestFrameData(FallbackFrame, LeapDeviceSerial);
		if (FallbackFrame.Hands.Num() > 0 || FallbackFrame.NumberOfHandsVisible > 0)
		{
			Frame = FallbackFrame;
			bUsedPluginFallback = true;
		}
	}

	LastLeapFrameHandCount = FMath::Max(Frame.Hands.Num(), Frame.NumberOfHandsVisible);
	LastLeapFrameId = Frame.FrameId;
	LastLeapFrameTimeStamp = Frame.TimeStamp;
	bLastLeapLeftVisible = Frame.LeftHandVisible;
	bLastLeapRightVisible = Frame.RightHandVisible;

	FLeapHandData SelectedHand;
	if (!TryGetSelectedLeapHand(Frame, SelectedHand))
	{
		if (Frame.Hands.Num() > 0)
		{
			SetLeapInputStatus(TEXT("Leap frame received, but selected hand was not found"));
		}
		else if (!bLeapComponentHasDevice && !bUsedPluginFallback)
		{
			SetLeapInputStatus(TEXT("Leap device is not active on LeapComponent"));
		}
		else
		{
			SetLeapInputStatus(TEXT("Leap frame has no hands"));
		}
		UpdateHandData(false, RawHandScreenPosition, 0.0f);
		UpdateLeapTooCloseState(FVector::ZeroVector, false);
		bHasLastCppLeapScreenPosition = false;
		bLastCppLeapHadSelectedHand = false;
		return;
	}

	LastSelectedLeapPalmRawPosition = SelectedHand.Palm.Position;
	LastSelectedLeapPalmStabilizedPosition = SelectedHand.Palm.StabilizedPosition;

	const bool bHasStabilizedPosition = !SelectedHand.Palm.StabilizedPosition.IsNearlyZero();
	const FVector PalmPosition = (bUseStabilizedPalmPosition && bHasStabilizedPosition)
		? SelectedHand.Palm.StabilizedPosition
		: SelectedHand.Palm.Position;
	LastSelectedLeapPalmPosition = PalmPosition;
	UpdateLeapTooCloseState(PalmPosition, true);
	const FVector2D ScreenPosition = ConvertLeapPositionToScreen(PalmPosition);

	float Speed = 0.0f;
	const float SafeDeltaTime = FMath::Max(DeltaTime, KINDA_SMALL_NUMBER);
	if (bHasLastCppLeapScreenPosition)
	{
		// C++ 直接入力では BP 由来の速度がないため、Raw 正規化座標差分から速度を作る。
		// Clamp/スムージング後では端張り付きや遅延で速度が鈍るので、ここでは未加工座標を使う。
		Speed = FVector2D::Distance(ScreenPosition, LastCppLeapScreenPosition) / SafeDeltaTime * LeapInputSpeedScale;
	}

	LastCppLeapScreenPosition = ScreenPosition;
	bHasLastCppLeapScreenPosition = true;
	UpdateHandData(true, ScreenPosition, Speed);
	SetLeapInputStatus(bUsedPluginFallback ? TEXT("Tracking hand via plugin fallback") : TEXT("Tracking hand via LeapComponent"));

	if (bDebugLeapInput && !bLastCppLeapHadSelectedHand)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[TowelDiag] CppLeap selected Hand=%d Palm=(%.2f,%.2f,%.2f) ScreenRaw=(%.3f,%.3f) Fallback=%d"),
			static_cast<int32>(SelectedHand.HandType.GetValue()),
			PalmPosition.X, PalmPosition.Y, PalmPosition.Z,
			ScreenPosition.X, ScreenPosition.Y,
			bUsedPluginFallback ? 1 : 0);
	}
	bLastCppLeapHadSelectedHand = true;
}

void ATomatinaTowelSystem::SetLeapInputStatus(const FString& NewStatus)
{
	if (LastLeapInputStatus == NewStatus)
	{
		return;
	}

	LastLeapInputStatus = NewStatus;
	if (bDebugLeapInput)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TowelDiag] %s"), *LastLeapInputStatus);
	}
}

bool ATomatinaTowelSystem::TryGetSelectedLeapHand(const FLeapFrameData& Frame, FLeapHandData& OutHand) const
{
	for (const FLeapHandData& Hand : Frame.Hands)
	{
		const EHandType HandType = Hand.HandType.GetValue();
		const bool bMatchesSelection =
			LeapHandSelection == ELeapTowelHandSelection::Any ||
			(LeapHandSelection == ELeapTowelHandSelection::Left && HandType == LEAP_HAND_LEFT) ||
			(LeapHandSelection == ELeapTowelHandSelection::Right && HandType == LEAP_HAND_RIGHT);

		if (bMatchesSelection)
		{
			OutHand = Hand;
			return true;
		}
	}

	return false;
}

FVector2D ATomatinaTowelSystem::ConvertLeapPositionToScreen(FVector LeapPosition) const
{
	const float Horizontal = ReadLeapAxis(LeapPosition, LeapHorizontalAxis);
	const float Vertical = ReadLeapAxis(LeapPosition, LeapVerticalAxis);
	const FVector2D SafeHalfRange(
		FMath::Max(LeapInputHalfRange.X, 1.0f),
		FMath::Max(LeapInputHalfRange.Y, 1.0f));

	return FVector2D(
		0.5f + (Horizontal - LeapInputCenter.X) / (SafeHalfRange.X * 2.0f),
		0.5f - (Vertical - LeapInputCenter.Y) / (SafeHalfRange.Y * 2.0f));
}

float ATomatinaTowelSystem::ReadLeapAxis(FVector LeapPosition, ELeapTowelAxis Axis) const
{
	switch (Axis)
	{
	case ELeapTowelAxis::X:
		return LeapPosition.X;
	case ELeapTowelAxis::Y:
		return LeapPosition.Y;
	case ELeapTowelAxis::Z:
		return LeapPosition.Z;
	case ELeapTowelAxis::NegativeX:
		return -LeapPosition.X;
	case ELeapTowelAxis::NegativeY:
		return -LeapPosition.Y;
	case ELeapTowelAxis::NegativeZ:
		return -LeapPosition.Z;
	default:
		return LeapPosition.Y;
	}
}

void ATomatinaTowelSystem::UpdateLeapTooCloseState(FVector LeapPosition, bool bHasSelectedHand)
{
	if (!bEnableLeapTooCloseWarning || !bHasSelectedHand)
	{
		bLeapTooCloseToDevice = false;
		LastLeapDistanceValue = 0.0f;
		return;
	}

	LastLeapDistanceValue = ReadLeapAxis(LeapPosition, LeapTooCloseAxis);
	const float ShowThreshold = LeapTooCloseThreshold;
	const float ClearThreshold = FMath::Max(LeapTooCloseClearThreshold, ShowThreshold);

	// ヒステリシスを持たせ、閾値付近で警告が点滅しないようにする。
	if (bLeapTooCloseToDevice)
	{
		bLeapTooCloseToDevice = LastLeapDistanceValue <= ClearThreshold;
	}
	else
	{
		bLeapTooCloseToDevice = LastLeapDistanceValue <= ShowThreshold;
	}
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

	return bInView;
}

// =============================================================================
// GetDirtManager（キャッシュ付き）
// =============================================================================

ATomatoDirtManager* ATomatinaTowelSystem::GetDirtManager()
{
	if (IsValid(DirtManagerOverride))
	{
		CachedDirtManager = DirtManagerOverride;
		return CachedDirtManager;
	}

	if (!IsValid(CachedDirtManager))
	{
		AActor* Found = UGameplayStatics::GetActorOfClass(
			GetWorld(), ATomatoDirtManager::StaticClass());
		CachedDirtManager = Cast<ATomatoDirtManager>(Found);

		if (!CachedDirtManager && bDebugLeapInput && !bWarnedMissingDirtManager)
		{
			bWarnedMissingDirtManager = true;
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

		if (!CachedPlayerPawn && bDebugLeapInput && !bWarnedMissingPlayerPawn)
		{
			bWarnedMissingPlayerPawn = true;
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
