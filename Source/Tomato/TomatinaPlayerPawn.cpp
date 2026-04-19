// Fill out your copyright notice in the Description page of Project Settings.

#include "TomatinaPlayerPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

#include "TomatinaFunctionLibrary.h"
#include "TomatinaGameMode.h"
#include "TomatinaHUD.h"

// =============================================================================
// コンストラクタ
// =============================================================================

ATomatinaPlayerPawn::ATomatinaPlayerPawn()
	: PC(nullptr)
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessPlayer = EAutoReceiveInput::Player0;

	// メインカメラ：ルートコンポーネントにアタッチ
	PlayerCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
	PlayerCamera->SetupAttachment(RootComponent);

	// ズーム用 SceneCapture：PlayerCamera の子
	SceneCapture_Zoom = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture_Zoom"));
	SceneCapture_Zoom->SetupAttachment(PlayerCamera);
	SceneCapture_Zoom->bCaptureEveryFrame = true;
	SceneCapture_Zoom->FOVAngle = 90.f;
}

// =============================================================================
// BeginPlay
// =============================================================================

void ATomatinaPlayerPawn::BeginPlay()
{
	UE_LOG(LogTemp, Warning,
		TEXT("===== ATomatinaPlayerPawn::BeginPlay called ====="));

	Super::BeginPlay();

	UE_LOG(LogTemp, Warning,
		TEXT("PlayerPawn: Main=(%d,%d) Phone=(%d,%d) bTestMode=%d"),
		(int)MainWidth, (int)MainHeight,
		(int)PhoneWidth, (int)PhoneHeight, bTestMode);

	if (GetWorld())
	{
		PC = GetWorld()->GetFirstPlayerController();
	}

	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("ATomatinaPlayerPawn::BeginPlay: PlayerController の取得に失敗"));
		return;
	}

	// Enhanced Input MappingContext を登録
	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer());
	if (Subsystem && DefaultMappingContext)
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
		UE_LOG(LogTemp, Warning, TEXT("PlayerPawn: MappingContext 追加完了"));
	}
	else
	{
		UE_LOG(LogTemp, Error,
			TEXT("PlayerPawn: MappingContext 追加失敗 Subsystem=%s Context=%s"),
			Subsystem          ? TEXT("OK") : TEXT("NULL"),
			DefaultMappingContext ? TEXT("OK") : TEXT("NULL"));
	}
}

// =============================================================================
// SetupPlayerInputComponent
// =============================================================================

void ATomatinaPlayerPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EIC)
	{
		UE_LOG(LogTemp, Error,
			TEXT("PlayerPawn: UEnhancedInputComponent のキャスト失敗 "
			     "（Project Settings > Input で Enhanced Input Plugin が有効か確認）"));
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

	UE_LOG(LogTemp, Warning,
		TEXT("PlayerPawn: Input バインド完了 RightMouse=%s LeftMouse=%s"),
		IA_RightMouse ? TEXT("OK") : TEXT("NULL"),
		IA_LeftMouse  ? TEXT("OK") : TEXT("NULL"));
}

// =============================================================================
// Tick
// =============================================================================

void ATomatinaPlayerPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!PC || !SceneCapture_Zoom)
	{
		return;
	}

	// ------------------------------------------------------------------
	// ZoomAlpha 補間（FInterpTo は速度スケールが大きいほど速い）
	// ------------------------------------------------------------------
	const float InterpSpeed = ZoomSpeed * 10.f;

	if (bIsZooming)
	{
		ZoomAlpha = FMath::FInterpTo(ZoomAlpha, 1.0f, DeltaTime, InterpSpeed);
	}
	else
	{
		ZoomAlpha = FMath::FInterpTo(ZoomAlpha, 0.0f, DeltaTime, InterpSpeed);
	}

	// ------------------------------------------------------------------
	// FOV と SceneCapture 位置を ZoomAlpha で更新
	// ------------------------------------------------------------------
	const float CurrentFOV = FMath::Lerp(DefaultFOV, ZoomFOV, ZoomAlpha);
	SceneCapture_Zoom->FOVAngle = CurrentFOV;

	const FVector CurrentOffset = FMath::Lerp(FVector::ZeroVector, TargetOffset, ZoomAlpha);
	SceneCapture_Zoom->SetRelativeLocation(CurrentOffset);

	// ------------------------------------------------------------------
	// ZoomAlpha >= 0.95：カーソルを iPhone 中央に移動（一度だけ）
	// ------------------------------------------------------------------
	if (ZoomAlpha >= 0.95f && !bCursorCentered && bIsZooming)
	{
		bCursorCentered = true;

		const FVector2D Center = UTomatinaFunctionLibrary::GetZoomScreenCenter(
			MainWidth, PhoneWidth, PhoneHeight);
		PC->SetMouseLocation(static_cast<int32>(Center.X), static_cast<int32>(Center.Y));

		UE_LOG(LogTemp, Warning, TEXT("ATomatinaPlayerPawn::Tick: カーソルを iPhone 中央へ移動 (%.0f, %.0f)"),
			Center.X, Center.Y);
	}

	// ------------------------------------------------------------------
	// ZoomAlpha >= 0.99：ズーム完了（Timeline Finished 相当）
	// ------------------------------------------------------------------
	if (ZoomAlpha >= 0.99f && bIsZooming && !bZoomComplete)
	{
		bZoomComplete = true;
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
		UE_LOG(LogTemp, Warning, TEXT("ATomatinaPlayerPawn::Tick: ズーム完了"));
	}

	// ------------------------------------------------------------------
	// ズーム中：HUD のカーソル位置を更新
	// ------------------------------------------------------------------
	if (bIsZooming)
	{
		const FVector2D CursorPos = UTomatinaFunctionLibrary::ProjectZoomToMainScreen(
			PC, SceneCapture_Zoom, MainWidth, MainHeight);

		ATomatinaHUD* HUD = Cast<ATomatinaHUD>(PC->GetHUD());
		if (HUD)
		{
			HUD->UpdateCursorPosition(CursorPos);
		}
	}

	// ------------------------------------------------------------------
	// ズーム中：マウス入力でパン（TargetOffset に累積）
	// ------------------------------------------------------------------
	if (bIsZooming)
	{
		const float DeltaX = PC->GetInputAxisValue(TEXT("Turn"));
		const float DeltaY = PC->GetInputAxisValue(TEXT("LookUp"));

		if (FMath::Abs(DeltaX) > 0.001f || FMath::Abs(DeltaY) > 0.001f)
		{
			const FVector Offset(
				0.f,
				DeltaX * MoveSpeed * DeltaTime * -1.f,
				DeltaY * MoveSpeed * DeltaTime);

			TargetOffset += Offset;
		}
	}

	// ------------------------------------------------------------------
	// ズーム解除が完了（ZoomAlpha ≈ 0）したらリセット
	// ------------------------------------------------------------------
	if (!bIsZooming && ZoomAlpha < 0.01f)
	{
		SceneCapture_Zoom->SetRelativeLocation(FVector::ZeroVector);
		ZoomAlpha = 0.0f;
	}
}

// =============================================================================
// 右クリック押下
// =============================================================================

void ATomatinaPlayerPawn::OnRightMousePressed(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Warning, TEXT("PlayerPawn: RightMouse Pressed"));

	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("ATomatinaPlayerPawn::OnRightMousePressed: PC が null"));
		return;
	}
	if (!PlayerCamera)
	{
		UE_LOG(LogTemp, Warning, TEXT("ATomatinaPlayerPawn::OnRightMousePressed: PlayerCamera が null"));
		return;
	}

	// マウス位置の取得
	float MouseX = 0.f, MouseY = 0.f;
	if (!PC->GetMousePosition(MouseX, MouseY))
	{
		UE_LOG(LogTemp, Warning, TEXT("ATomatinaPlayerPawn::OnRightMousePressed: マウス位置の取得に失敗"));
		return;
	}

	// スクリーン座標 → ワールドの Ray に変換
	FVector WorldLocation, WorldDirection;
	if (!PC->DeprojectScreenPositionToWorld(MouseX, MouseY, WorldLocation, WorldDirection))
	{
		UE_LOG(LogTemp, Warning, TEXT("ATomatinaPlayerPawn::OnRightMousePressed: DeprojectScreenPositionToWorld に失敗"));
		return;
	}

	// ライントレース（Visibility チャンネル）
	FHitResult HitResult;
	const FVector TraceEnd = WorldLocation + WorldDirection * 100000.f;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		WorldLocation,
		TraceEnd,
		ECC_Visibility,
		QueryParams);

	if (!bHit)
	{
		UE_LOG(LogTemp, Warning, TEXT("ATomatinaPlayerPawn::OnRightMousePressed: ヒットなし → ズームキャンセル"));
		return;
	}

	// ズームオフセット計算
	TargetOffset = UTomatinaFunctionLibrary::CalculateZoomOffset(
		PC, HitResult, PlayerCamera, PlayerCamera->FieldOfView);

	// ズーム開始
	bIsZooming     = true;
	bZoomComplete  = false;
	bCursorCentered = false;
	ZoomAlpha      = 0.0f;

	// カーソル Widget を表示
	ATomatinaHUD* HUD = Cast<ATomatinaHUD>(PC->GetHUD());
	if (HUD)
	{
		HUD->ShowCursor();
	}

	UE_LOG(LogTemp, Warning, TEXT("ATomatinaPlayerPawn::OnRightMousePressed: ズーム開始 Offset=(%.1f, %.1f, %.1f)"),
		TargetOffset.X, TargetOffset.Y, TargetOffset.Z);
}

// =============================================================================
// 右クリック解放
// =============================================================================

void ATomatinaPlayerPawn::OnRightMouseReleased(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Warning, TEXT("PlayerPawn: RightMouse Released"));
	bIsZooming      = false;
	bZoomComplete   = false;
	bCursorCentered = false;

	// ZoomAlpha は Tick で 0 に向かって補間される（逆再生）

	if (PC)
	{
		// マウスカーソルを再表示
		PC->bShowMouseCursor = true;

		// 入力モードを GameAndUI に戻す
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);

		// カーソル Widget を非表示
		ATomatinaHUD* HUD = Cast<ATomatinaHUD>(PC->GetHUD());
		if (HUD)
		{
			HUD->HideCursor();
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("ATomatinaPlayerPawn::OnRightMouseReleased: ズーム解除"));
}

// =============================================================================
// 左クリック押下
// =============================================================================

void ATomatinaPlayerPawn::OnLeftMousePressed(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Warning,
		TEXT("PlayerPawn: LeftMouse Pressed bIsZooming=%d"), bIsZooming);

	if (!bIsZooming) { return; }

	if (!GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("ATomatinaPlayerPawn::OnLeftMousePressed: GetWorld() が null"));
		return;
	}

	ATomatinaGameMode* GameMode = Cast<ATomatinaGameMode>(
		UGameplayStatics::GetGameMode(GetWorld()));

	if (GameMode)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerPawn: TakePhoto 呼び出し"));
		GameMode->TakePhoto(SceneCapture_Zoom);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("PlayerPawn: GameMode 取得失敗"));
	}
}
