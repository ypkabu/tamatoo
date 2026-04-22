// Fill out your copyright notice in the Description page of Project Settings.

#include "TomatinaPlayerPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

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
	if (bIsZooming && ZoomAlpha >= 0.95f && !bCursorCentered)
	{
		bCursorCentered = true;
		const FVector2D Center =
			UTomatinaFunctionLibrary::GetZoomScreenCenter(MainWidth, PhoneWidth, PhoneHeight);
		PC->SetMouseLocation(static_cast<int32>(Center.X), static_cast<int32>(Center.Y));
		UE_LOG(LogTemp, Warning, TEXT("ATomatinaPlayerPawn::Tick: カーソルを iPhone 中央(%.0f,%.0f)へ移動"),
			Center.X, Center.Y);
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
