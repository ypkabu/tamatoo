// Fill out your copyright notice in the Description page of Project Settings.

#include "TomatinaPlayerPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/SWindow.h"

#include "TomatinaFunctionLibrary.h"
#include "TomatinaGameMode.h"
#include "TomatinaHUD.h"

ATomatinaPlayerPawn::ATomatinaPlayerPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessPlayer = EAutoReceiveInput::Player0;

	// DefaultPawn の組み込み WASD / マウスルックをすべて無効化
	// （入力はすべて Enhanced Input で制御する）
	bAddDefaultMovementBindings = false;

	PlayerCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
	PlayerCamera->SetupAttachment(RootComponent);

	SceneCapture_Zoom = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture_Zoom"));
	SceneCapture_Zoom->SetupAttachment(PlayerCamera);
	SceneCapture_Zoom->bCaptureEveryFrame = true;
	SceneCapture_Zoom->FOVAngle = 90.f;
}

void ATomatinaPlayerPawn::BeginPlay()
{
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaPlayerPawn::BeginPlay 開始 Main=(%.0fx%.0f) Phone=(%.0fx%.0f) bTestMode=%d"),
		MainWidth, MainHeight, PhoneWidth, PhoneHeight, bTestMode ? 1 : 0);

	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		PC = World->GetFirstPlayerController();
	}

	if (!PC)
	{
		UE_LOG(LogTemp, Error, TEXT("ATomatinaPlayerPawn::BeginPlay: PlayerController 取得失敗"));
		return;
	}

	// Enhanced Input Mapping Context を登録
	if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
				ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
				UE_LOG(LogTemp, Warning, TEXT("ATomatinaPlayerPawn: MappingContext 追加完了"));
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("ATomatinaPlayerPawn: DefaultMappingContext が未設定"));
			}
		}
	}

	// カーソル表示・GameAndUI モード
	PC->bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	PC->SetInputMode(InputMode);

	// ── SceneCapture_Zoom 診断ログ ─────────────────────────────────────
	//   bCaptureEveryFrame が false のままなら RT は更新されない。
	//   BP で意図せず false になっていないかを可視化する。
	if (SceneCapture_Zoom)
	{
		UTextureRenderTarget2D* RT = SceneCapture_Zoom->TextureTarget;
		UE_LOG(LogTemp, Warning,
			TEXT("SceneCapture 診断: bCaptureEveryFrame=%d bCaptureOnMovement=%d CaptureSource=%d FOV=%.1f RT=%s RTSize=%dx%d"),
			SceneCapture_Zoom->bCaptureEveryFrame ? 1 : 0,
			SceneCapture_Zoom->bCaptureOnMovement ? 1 : 0,
			static_cast<int32>(SceneCapture_Zoom->CaptureSource),
			SceneCapture_Zoom->FOVAngle,
			*GetNameSafe(RT),
			RT ? RT->SizeX : 0, RT ? RT->SizeY : 0);

		// 安全側：毎フレームキャプチャを強制 ON (BP で false に戻されても起動時に正す)
		if (!SceneCapture_Zoom->bCaptureEveryFrame)
		{
			SceneCapture_Zoom->bCaptureEveryFrame = true;
			UE_LOG(LogTemp, Warning, TEXT("SceneCapture_Zoom: bCaptureEveryFrame を強制 ON"));
		}

		// CaptureSource が未設定(0 = HDR) のままだと UMG 上で真っ黒になりがち。
		// Final Color (LDR) in RGB (= 2) を強制。BP で別の値に設定済みなら上書きしない。
		if (SceneCapture_Zoom->CaptureSource == ESceneCaptureSource::SCS_SceneColorHDR)
		{
			SceneCapture_Zoom->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
			UE_LOG(LogTemp, Warning,
				TEXT("SceneCapture_Zoom: CaptureSource を SCS_FinalColorLDR に自動補正"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("SceneCapture_Zoom が null です"));
	}

	if (!bTestMode)
	{
		EnsureDualScreenWindowLayout();
	}
}

void ATomatinaPlayerPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaPlayerPawn::SetupPlayerInputComponent"));

	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EIC)
	{
		UE_LOG(LogTemp, Error,
			TEXT("ATomatinaPlayerPawn: EnhancedInputComponent キャスト失敗（Project Settings > Input を確認）"));
		return;
	}

	if (IA_RightMouse)
	{
		EIC->BindAction(IA_RightMouse, ETriggerEvent::Started,   this, &ATomatinaPlayerPawn::OnRightMousePressed);
		EIC->BindAction(IA_RightMouse, ETriggerEvent::Completed, this, &ATomatinaPlayerPawn::OnRightMouseReleased);
	}
	if (IA_LeftMouse)
	{
		EIC->BindAction(IA_LeftMouse, ETriggerEvent::Started, this, &ATomatinaPlayerPawn::OnLeftMousePressed);
	}
	if (IA_Look)
	{
		EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &ATomatinaPlayerPawn::OnLook);
	}

	UE_LOG(LogTemp, Warning, TEXT("ATomatinaPlayerPawn: Input バインド R=%s L=%s Look=%s"),
		IA_RightMouse ? TEXT("OK") : TEXT("NULL"),
		IA_LeftMouse  ? TEXT("OK") : TEXT("NULL"),
		IA_Look       ? TEXT("OK") : TEXT("NULL"));
}

void ATomatinaPlayerPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!PC || !SceneCapture_Zoom) { return; }

	// 手動 CaptureScene セーフティ。
	// bCaptureEveryFrame=true が設定されていても UE 5.7 でなぜか自動キャプチャが
	// 走らないケースが観測されているため、Tick で毎フレーム明示的に呼ぶ。
	// 二重呼び出しになってもパフォ影響は軽微。
	if (SceneCapture_Zoom->TextureTarget)
	{
		SceneCapture_Zoom->CaptureScene();
	}

	if (!bTestMode && !bWindowLayoutVerified)
	{
		WindowLayoutRetryElapsed += DeltaTime;
		if (WindowLayoutRetryElapsed >= 0.5f && WindowLayoutRetryCount < 6)
		{
			WindowLayoutRetryElapsed = 0.f;
			EnsureDualScreenWindowLayout();
		}
	}

	// ── ZoomAlpha の補間（実時間ベース） ───────────────────────────────
	const float RealDelta = FApp::GetDeltaTime();
	const float Target    = bIsZooming ? 1.f : 0.f;
	ZoomAlpha = FMath::FInterpTo(ZoomAlpha, Target, RealDelta, ZoomSpeed);

	// ── FOV と相対位置を ZoomAlpha で更新 ─────────────────────────────
	const float CurrentFOV = FMath::Lerp(DefaultFOV, ZoomFOV, ZoomAlpha);
	SceneCapture_Zoom->FOVAngle = CurrentFOV;

	const FVector CurrentOffset = FMath::Lerp(FVector::ZeroVector, TargetOffset, ZoomAlpha);
	SceneCapture_Zoom->SetRelativeLocation(CurrentOffset);

	ATomatinaHUD* HUD = Cast<ATomatinaHUD>(PC->GetHUD());

	// ── ZoomAlpha >= 0.95：カーソルを iPhone 中央に一度だけ飛ばす ──────
	// 旧スパンウィンドウ方式のときのみ。第二ウィンドウ方式ではスマホ側は独立 SWindow
	// なので OS カーソルを動かす必要がなく、IMG_PhoneCursor(UMG) で視覚的に示す。
	if (bIsZooming && ZoomAlpha >= 0.95f && !bCursorCentered)
	{
		bCursorCentered = true;
		if (!bUseSeparatePhoneWindow)
		{
			const FVector2D Center =
				UTomatinaFunctionLibrary::GetZoomScreenCenter(MainWidth, PhoneWidth, PhoneHeight);
			PC->SetMouseLocation(static_cast<int32>(Center.X), static_cast<int32>(Center.Y));
			UE_LOG(LogTemp, Warning, TEXT("ATomatinaPlayerPawn::Tick: カーソルを iPhone 中央(%.0f,%.0f)へ移動"),
				Center.X, Center.Y);
		}
	}

	// ── ZoomAlpha >= 0.99：カーソル非表示 / マウスルック有効化 ──────────
	if (bIsZooming && ZoomAlpha >= 0.99f && !bZoomComplete)
	{
		bZoomComplete = true;
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
		UE_LOG(LogTemp, Warning, TEXT("ATomatinaPlayerPawn::Tick: ズーム完了（マウスルック有効）"));
	}

	// ── ズーム中：マウスルックを SceneCapture_Zoom のローカル回転に適用 ──
	if (bZoomComplete && !CurrentLookInput.IsNearlyZero())
	{
		FRotator Rel = SceneCapture_Zoom->GetRelativeRotation();

		// 感度 = MoveSpeed * ZoomLookSensitivity（両方 BP でチューニング可）
		const float Delta = MoveSpeed * ZoomLookSensitivity * RealDelta * 0.01f;

		Rel.Yaw   += CurrentLookInput.X * Delta;
		Rel.Pitch -= CurrentLookInput.Y * Delta;

		// Pitch 上限（デフォルト ±89 度 = ほぼ真上／真下まで）
		Rel.Pitch = FMath::ClampAngle(Rel.Pitch, -ZoomPitchLimit, ZoomPitchLimit);

		// Yaw は 0 以下なら制限なし、正の値で ± 制限
		if (ZoomYawLimit > 0.f)
		{
			Rel.Yaw = FMath::ClampAngle(Rel.Yaw, -ZoomYawLimit, ZoomYawLimit);
		}

		SceneCapture_Zoom->SetRelativeRotation(Rel);
	}
	CurrentLookInput = FVector2D::ZeroVector;

	// ── ズーム中：HUD にカーソル位置を通知（iPhone 側に表示） ────────
	if (bIsZooming && HUD)
	{
		const FVector2D MainPos = UTomatinaFunctionLibrary::ProjectZoomToMainScreen(
			PC, SceneCapture_Zoom, MainWidth, MainHeight);
		HUD->UpdateCursorPosition(MainPos);
	}

	// ── ズーム解除が完了したらリセット ─────────────────────────────
	if (!bIsZooming && ZoomAlpha < 0.01f)
	{
		ZoomAlpha = 0.f;
		SceneCapture_Zoom->SetRelativeLocation(FVector::ZeroVector);
		SceneCapture_Zoom->SetRelativeRotation(FRotator::ZeroRotator);
	}

	// ── 非ズーム時：カーソルをメインモニター内に閉じ込める ────────────
	// ウィンドウは Main + iPhone の横並びなので、右側 iPhone 領域に抜けていた
	// カーソルを戻す。ズーム中はカーソル非表示 or iPhone 側に飛ばしてるので対象外。
	if (!bIsZooming && PC->bShowMouseCursor)
	{
		float MX = 0.f, MY = 0.f;
		if (PC->GetMousePosition(MX, MY))
		{
			const float ClampedX = FMath::Clamp(MX, 0.f, MainWidth  - 1.f);
			const float ClampedY = FMath::Clamp(MY, 0.f, MainHeight - 1.f);
			if (!FMath::IsNearlyEqual(ClampedX, MX) || !FMath::IsNearlyEqual(ClampedY, MY))
			{
				PC->SetMouseLocation(static_cast<int32>(ClampedX), static_cast<int32>(ClampedY));
			}
		}
	}
}

// =============================================================================
// OnRightMousePressed — ズーム開始
// =============================================================================
void ATomatinaPlayerPawn::OnRightMousePressed(const FInputActionValue& /*Value*/)
{
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaPlayerPawn::OnRightMousePressed"));

	if (!PC || !PlayerCamera || !GetWorld()) { return; }

	float MouseX = 0.f, MouseY = 0.f;
	if (!PC->GetMousePosition(MouseX, MouseY))
	{
		UE_LOG(LogTemp, Warning, TEXT("OnRightMousePressed: マウス位置取得失敗"));
		return;
	}

	FVector WorldLoc, WorldDir;
	if (!PC->DeprojectScreenPositionToWorld(MouseX, MouseY, WorldLoc, WorldDir))
	{
		UE_LOG(LogTemp, Warning, TEXT("OnRightMousePressed: Deproject 失敗"));
		return;
	}

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	const FVector TraceEnd = WorldLoc + WorldDir * 100000.f;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit, WorldLoc, TraceEnd, ECC_Visibility, Params);

	if (!bHit)
	{
		UE_LOG(LogTemp, Warning, TEXT("OnRightMousePressed: ヒットなし（ズームキャンセル）"));
		return;
	}

	TargetOffset = UTomatinaFunctionLibrary::CalculateZoomOffset(
		PC, Hit, PlayerCamera, PlayerCamera->FieldOfView);

	// ── 壁/床めり込み防止: カメラ位置→オフセット先をスイープして安全距離にクランプ ──
	// TargetOffset はカメラローカル座標なのでワールドに変換してからスイープする。
	if (GetWorld() && !TargetOffset.IsNearlyZero())
	{
		const FTransform CamXf = PlayerCamera->GetComponentTransform();
		const FVector StartLoc = CamXf.GetLocation();
		const FVector WorldOffset = CamXf.TransformVectorNoScale(TargetOffset);
		const FVector EndLoc = StartLoc + WorldOffset;

		FCollisionQueryParams SweepParams(TEXT("ZoomOffsetSweep"), false, this);
		SweepParams.AddIgnoredActor(this);

		FHitResult SweepHit;
		const bool bBlocked = GetWorld()->SweepSingleByChannel(
			SweepHit, StartLoc, EndLoc, FQuat::Identity,
			ECC_Visibility,
			FCollisionShape::MakeSphere(FMath::Max(1.f, ZoomOffsetSweepRadius)),
			SweepParams);

		if (bBlocked)
		{
			const float FullDist = WorldOffset.Size();
			const float HitDist  = (SweepHit.Location - StartLoc).Size();
			const float SafeDist = FMath::Max(0.f, HitDist - ZoomOffsetSafetyMargin);

			if (FullDist > KINDA_SMALL_NUMBER)
			{
				const float Scale = FMath::Clamp(SafeDist / FullDist, 0.f, 1.f);
				TargetOffset *= Scale;
				UE_LOG(LogTemp, Warning,
					TEXT("OnRightMousePressed: 壁検出 FullDist=%.1f HitDist=%.1f SafeDist=%.1f -> Scale=%.2f"),
					FullDist, HitDist, SafeDist, Scale);
			}
			else
			{
				TargetOffset = FVector::ZeroVector;
			}
		}
	}

	// 近接クリップを広めに設定しておく。スイープで防ぎきれないケース (回転でめり込む等)
	// でも壁の内側面が描画されず、ズームビューが黒く潰れない。
	if (SceneCapture_Zoom && ZoomNearClippingPlane > 0.f)
	{
		SceneCapture_Zoom->CustomNearClippingPlane = ZoomNearClippingPlane;
		SceneCapture_Zoom->bOverride_CustomNearClippingPlane = true;
	}

	bIsZooming      = true;
	bZoomComplete   = false;
	bCursorCentered = false;

	if (ATomatinaHUD* HUD = Cast<ATomatinaHUD>(PC->GetHUD()))
	{
		HUD->ShowCursor();
	}

	UE_LOG(LogTemp, Warning, TEXT("OnRightMousePressed: ズーム開始 Offset=(%.1f,%.1f,%.1f)"),
		TargetOffset.X, TargetOffset.Y, TargetOffset.Z);
}

// =============================================================================
// OnRightMouseReleased — ズーム解除（逆再生）
// =============================================================================
void ATomatinaPlayerPawn::OnRightMouseReleased(const FInputActionValue& /*Value*/)
{
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaPlayerPawn::OnRightMouseReleased"));

	bIsZooming      = false;
	bZoomComplete   = false;
	bCursorCentered = false;

	if (PC)
	{
		PC->bShowMouseCursor = true;
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);

		if (ATomatinaHUD* HUD = Cast<ATomatinaHUD>(PC->GetHUD()))
		{
			HUD->HideCursor();
		}
	}
}

// =============================================================================
// OnLeftMousePressed — 撮影（ズーム中のみ）
// =============================================================================
void ATomatinaPlayerPawn::OnLeftMousePressed(const FInputActionValue& /*Value*/)
{
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaPlayerPawn::OnLeftMousePressed bIsZooming=%d"),
		bIsZooming ? 1 : 0);

	if (!bIsZooming) { return; }
	if (!GetWorld())  { return; }

	if (ATomatinaGameMode* GameMode = Cast<ATomatinaGameMode>(
			UGameplayStatics::GetGameMode(GetWorld())))
	{
		GameMode->TakePhoto(SceneCapture_Zoom);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("OnLeftMousePressed: GameMode 取得失敗"));
	}

	// ── 撮影直後：ズーム解除（カーソル位置は Tick のクランプ処理に任せる） ─
	bIsZooming      = false;
	bZoomComplete   = false;
	bCursorCentered = false;

	if (PC)
	{
		PC->bShowMouseCursor = true;
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);

		if (ATomatinaHUD* HUD = Cast<ATomatinaHUD>(PC->GetHUD()))
		{
			HUD->HideCursor();
		}
	}
}

// =============================================================================
// OnLook — マウス移動を蓄積
// =============================================================================
void ATomatinaPlayerPawn::OnLook(const FInputActionValue& Value)
{
	CurrentLookInput += Value.Get<FVector2D>();
}

void ATomatinaPlayerPawn::EnsureDualScreenWindowLayout()
{
	if (!PC)
	{
		return;
	}

	// 第二ウィンドウ方式ではメインウィンドウは MainWidth × MainHeight 単独。
	// 旧スパン方式のときのみ横に PhoneWidth を足した幅にする。
	const int32 DesiredW = bUseSeparatePhoneWindow
		? FMath::Max(1, FMath::RoundToInt(MainWidth))
		: FMath::Max(1, FMath::RoundToInt(MainWidth + PhoneWidth));
	const int32 DesiredH = FMath::Max(1, FMath::RoundToInt(MainHeight));

	int32 ViewW = 0;
	int32 ViewH = 0;
	PC->GetViewportSize(ViewW, ViewH);

	if (ViewW >= DesiredW && ViewH >= DesiredH)
	{
		if (!bWindowLayoutVerified)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("ATomatinaPlayerPawn: ウィンドウサイズ確認 OK Viewport=(%dx%d) Required=(%dx%d)"),
				ViewW, ViewH, DesiredW, DesiredH);
		}
		bWindowLayoutVerified = true;
		return;
	}

	if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
	{
		Settings->SetFullscreenMode(EWindowMode::Windowed);
		Settings->SetScreenResolution(FIntPoint(DesiredW, DesiredH));
		Settings->ApplyResolutionSettings(false);
		Settings->ApplySettings(false);
	}

	PC->ConsoleCommand(FString::Printf(TEXT("r.SetRes %dx%dw"), DesiredW, DesiredH), true);

	// ウィンドウを (0,0) に移動してメインモニタ左上から右にスパンさせる。
	// スタンドアロン起動時のみ有効 (PIE ではエディタがウィンドウ位置を管理しており、
	// FSlateApplication::GetActiveTopLevelWindow() がエディタウィンドウを返してしまう場合がある)。
	if (FSlateApplication::IsInitialized())
	{
		if (TSharedPtr<SWindow> GameWindow = GEngine && GEngine->GameViewport
			? GEngine->GameViewport->GetWindow()
			: nullptr)
		{
			GameWindow->MoveWindowTo(FVector2D(0.f, 0.f));
		}
	}

	WindowLayoutRetryCount++;
	UE_LOG(LogTemp, Warning,
		TEXT("ATomatinaPlayerPawn: ウィンドウサイズ不足 Viewport=(%dx%d) Required=(%dx%d) -> r.SetRes 適用試行 %d/6"),
		ViewW, ViewH, DesiredW, DesiredH, WindowLayoutRetryCount);
}
