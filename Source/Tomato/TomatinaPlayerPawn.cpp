// Fill out your copyright notice in the Description page of Project Settings.

#include "TomatinaPlayerPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Framework/Application/SlateApplication.h"

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

	PlayerCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
	PlayerCamera->SetupAttachment(RootComponent);

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
	Super::BeginPlay();

	// モニター解像度を自動検出（PhoneWidth/PhoneHeight は副モニターがある場合のみ上書き）
	FDisplayMetrics DM;
	FSlateApplication::Get().GetDisplayMetrics(DM);
	for (const FMonitorInfo& Monitor : DM.MonitorInfo)
	{
		if (Monitor.bIsPrimary)
		{
			MainWidth  = static_cast<float>(Monitor.NativeWidth);
			MainHeight = static_cast<float>(Monitor.NativeHeight);
		}
		else
		{
			PhoneWidth  = static_cast<float>(Monitor.NativeWidth);
			PhoneHeight = static_cast<float>(Monitor.NativeHeight);
		}
	}
	UE_LOG(LogTemp, Warning,
		TEXT("ATomatinaPlayerPawn::BeginPlay: Main=(%.0fx%.0f) Phone=(%.0fx%.0f) bTestMode=%d"),
		MainWidth, MainHeight, PhoneWidth, PhoneHeight, bTestMode ? 1 : 0);

	PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("ATomatinaPlayerPawn::BeginPlay: PlayerController の取得に失敗"));
		return;
	}

	// カスタムカーソル運用のためシステムカーソルは常時非表示
	PC->bShowMouseCursor = false;

	// GameAndUI：マウス位置トラッキングを有効化
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(InputMode);

	// Enhanced Input
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ATomatinaPlayerPawn::BeginPlay: DefaultMappingContext 未設定"));
		}
	}

	// 初期化
	ZoomAlpha       = 0.f;
	bIsZooming      = false;
	bZoomComplete   = false;
	bCursorCentered = false;
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
		UE_LOG(LogTemp, Warning, TEXT("ATomatinaPlayerPawn::SetupPlayerInputComponent: EIC が取得できません"));
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
}

// =============================================================================
// Tick
// =============================================================================

void ATomatinaPlayerPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!PC || !SceneCapture_Zoom) { return; }

	// ZoomAlpha 補間
	const float InterpSpeed = ZoomSpeed * 10.f;
	ZoomAlpha = FMath::FInterpTo(ZoomAlpha, bIsZooming ? 1.0f : 0.0f, DeltaTime, InterpSpeed);

	// FOV と位置
	SceneCapture_Zoom->FOVAngle = FMath::Lerp(DefaultFOV, ZoomFOV, ZoomAlpha);
	SceneCapture_Zoom->SetRelativeLocation(
		FMath::Lerp(FVector::ZeroVector, TargetOffset, ZoomAlpha));

	// 0.95：カーソル中央移動
	if (ZoomAlpha >= 0.95f && !bCursorCentered && bIsZooming)
	{
		bCursorCentered = true;

		FVector2D Center;
		if (bTestMode)
		{
			// テストモード：メインモニター中央
			Center = FVector2D(MainWidth * 0.5f, MainHeight * 0.5f);
		}
		else
		{
			// 本番：iPhone 画面中央（GetZoomScreenCenter は Phone 画面中心を返す）
			Center = UTomatinaFunctionLibrary::GetZoomScreenCenter(MainWidth, PhoneWidth, PhoneHeight);
		}
		PC->SetMouseLocation(static_cast<int32>(Center.X), static_cast<int32>(Center.Y));

		UE_LOG(LogTemp, Warning, TEXT("ATomatinaPlayerPawn::Tick: カーソル中央へ (%.0f, %.0f)"),
			Center.X, Center.Y);
	}

	// 0.99：ズーム完了
	if (ZoomAlpha >= 0.99f && bIsZooming && !bZoomComplete)
	{
		bZoomComplete = true;
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
		UE_LOG(LogTemp, Warning, TEXT("ATomatinaPlayerPawn::Tick: ズーム完了"));
	}

	// ズーム中：HUD のカーソル位置を更新
	if (bIsZooming)
	{
		const FVector2D CursorPos = UTomatinaFunctionLibrary::ProjectZoomToMainScreen(
			PC, SceneCapture_Zoom, MainWidth, MainHeight);

		if (ATomatinaHUD* HUD = Cast<ATomatinaHUD>(PC->GetHUD()))
		{
			HUD->UpdateCursorPosition(CursorPos);
		}
	}

	// ズーム完了後：マウスルック
	if (bZoomComplete)
	{
		const FVector Offset(
			0.f,
			CurrentLookInput.X * MoveSpeed * DeltaTime * -1.f,
			CurrentLookInput.Y * MoveSpeed * DeltaTime);
		SceneCapture_Zoom->AddLocalOffset(Offset);
		CurrentLookInput = FVector2D::ZeroVector;
	}

	// ズーム解除完了でリセット
	if (!bIsZooming && ZoomAlpha < 0.01f)
	{
		SceneCapture_Zoom->SetRelativeLocation(FVector::ZeroVector);
		ZoomAlpha = 0.0f;
	}
}

// =============================================================================
// 右クリック押下：ズーム開始
// =============================================================================

void ATomatinaPlayerPawn::OnRightMousePressed()
{
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaPlayerPawn: RightMouse Pressed"));

	if (!PC || !PlayerCamera) { return; }

	float MouseX = 0.f, MouseY = 0.f;
	if (!PC->GetMousePosition(MouseX, MouseY))
	{
		UE_LOG(LogTemp, Warning, TEXT("OnRightMousePressed: マウス位置取得失敗"));
		return;
	}

	FVector WorldLocation, WorldDirection;
	if (!PC->DeprojectScreenPositionToWorld(MouseX, MouseY, WorldLocation, WorldDirection))
	{
		UE_LOG(LogTemp, Warning, TEXT("OnRightMousePressed: Deproject 失敗"));
		return;
	}

	FHitResult HitResult;
	FCollisionQueryParams QP;
	QP.AddIgnoredActor(this);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult, WorldLocation, WorldLocation + WorldDirection * 100000.f, ECC_Visibility, QP);

	if (!bHit)
	{
		UE_LOG(LogTemp, Warning, TEXT("OnRightMousePressed: ヒットなし → ズームキャンセル"));
		return;
	}

	TargetOffset = UTomatinaFunctionLibrary::CalculateZoomOffset(
		PC, HitResult, PlayerCamera, PlayerCamera->FieldOfView);

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
// 右クリック解放：ズーム解除
// =============================================================================

void ATomatinaPlayerPawn::OnRightMouseReleased()
{
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaPlayerPawn: RightMouse Released"));

	bIsZooming      = false;
	bZoomComplete   = false;
	bCursorCentered = false;

	if (PC)
	{
		PC->bShowMouseCursor = true;
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);

		if (ATomatinaHUD* HUD = Cast<ATomatinaHUD>(PC->GetHUD()))
		{
			HUD->HideCursor();
		}
	}
}

// =============================================================================
// 左クリック押下：撮影
// =============================================================================

void ATomatinaPlayerPawn::OnLeftMousePressed()
{
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaPlayerPawn: LeftMouse Pressed, bIsZooming=%d"), bIsZooming ? 1 : 0);

	if (!bIsZooming) { return; }
	if (!GetWorld()) { return; }

	ATomatinaGameMode* GameMode = Cast<ATomatinaGameMode>(
		UGameplayStatics::GetGameMode(GetWorld()));

	if (!GameMode)
	{
		UE_LOG(LogTemp, Warning, TEXT("OnLeftMousePressed: GameMode の取得に失敗"));
		return;
	}
	if (!SceneCapture_Zoom)
	{
		UE_LOG(LogTemp, Warning, TEXT("OnLeftMousePressed: SceneCapture_Zoom が null"));
		return;
	}

	GameMode->TakePhoto(SceneCapture_Zoom);
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaPlayerPawn: TakePhoto called"));
}

// =============================================================================
// マウス Look
// =============================================================================

void ATomatinaPlayerPawn::OnLook(const FInputActionValue& Value)
{
	CurrentLookInput = Value.Get<FVector2D>();
}
