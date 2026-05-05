// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DefaultPawn.h"
#include "InputActionValue.h"
#include "TomatinaSoundCue.h"  // FTomatinaSoundCue
#include "TomatinaPlayerPawn.generated.h"

class UCameraComponent;
class USceneCaptureComponent2D;
class APlayerController;
class UInputMappingContext;
class UInputAction;
class ULeapComponent;
class ATomatinaHUD;

/**
 * カメラマン 1P のポーン。
 * PlayerCamera（メインモニター用）と SceneCapture_Zoom（RT_Zoom 出力）を持つ。
 */
UCLASS()
class TOMATO_API ATomatinaPlayerPawn : public ADefaultPawn
{
	GENERATED_BODY()

public:
	ATomatinaPlayerPawn();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// ──────────────────────────────────────────────
	// コンポーネント
	// ──────────────────────────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
	UCameraComponent* PlayerCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera")
	USceneCaptureComponent2D* SceneCapture_Zoom;

	/**
	 * Deprecated compatibility component for old BP_TomatinaPlayerPawn Leap events.
	 * Towel control now reads Leap input in ATomatinaTowelSystem, so this stays inactive.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LeapMotion|Deprecated")
	ULeapComponent* Leap = nullptr;

	// ──────────────────────────────────────────────
	// 画面サイズ（BP で一元設定する 4 変数）
	// GameMode / HUD は BeginPlay で Pawn から取得
	// ──────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Screen")
	float MainWidth = 2560.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Screen")
	float MainHeight = 1600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Screen")
	float PhoneWidth = 2024.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Screen")
	float PhoneHeight = 1152.f;

	// ──────────────────────────────────────────────
	// テストモード（iPhone なしで PiP 動作確認）
	// ──────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Debug")
	bool bTestMode = true;

	/** PlayerPawn の詳細ログ。通常はOFF。入力/ズーム/画面配置調査時だけON。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Debug")
	bool bDebugPlayerLog = false;

	// ──────────────────────────────────────────────
	// 第二ウィンドウ方式 (スマホ側を独立 SWindow に出す)
	//   true:  メインウィンドウ = 2560x1600 (スパンしない)
	//          HUD が別 SWindow を (MainWidth, 0) に生成しスマホ側 UI を出す
	//   false: 旧仕様の「1枚スパンウィンドウ」
	// ──────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Screen")
	bool bUseSeparatePhoneWindow = true;

	// ──────────────────────────────────────────────
	// ズーム状態
	// ──────────────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, Category="Zoom")
	bool bIsZooming = false;

	UPROPERTY(BlueprintReadOnly, Category="Zoom")
	bool bZoomComplete = false;

	UPROPERTY(BlueprintReadOnly, Category="Zoom")
	bool bCursorCentered = false;

	UPROPERTY(BlueprintReadOnly, Category="Zoom")
	float ZoomAlpha = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zoom")
	float DefaultFOV = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zoom")
	float ZoomFOV = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zoom")
	float ZoomSpeed = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zoom")
	float MoveSpeed = 500.f;

	/** ズーム中の視点移動感度（値が大きいほど少ないマウス移動で大きく回る） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zoom")
	float ZoomLookSensitivity = 3.0f;

	/** ズーム中の Pitch（上下）最大角度（±）。90 近くにすると真上／真下まで向ける */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zoom")
	float ZoomPitchLimit = 89.f;

	/** ズーム中の Yaw（左右）最大角度（±）。180 で一周可。0 以下で制限なし */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zoom")
	float ZoomYawLimit = 0.f;

	/** ズームカメラの位置オフセットスイープ時の球半径 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zoom")
	float ZoomOffsetSweepRadius = 15.f;

	/** スイープヒット時に壁から離しておく余裕距離 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zoom")
	float ZoomOffsetSafetyMargin = 25.f;

	/** SceneCapture の近接クリップ (cm)。壁へ食い込みかけた時にその面を描画しない保険 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zoom")
	float ZoomNearClippingPlane = 20.f;

	/** ゲーム開始直後にスマホ側 RenderTarget を温めるための短時間キャプチャ秒数 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zoom|Capture", meta=(ClampMin="0.0", ClampMax="2.0"))
	float InitialPhoneCaptureSeconds = 0.35f;

	/** ズーム開始/解除前後の見た目を破綻させないための短時間キャプチャ秒数 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zoom|Capture", meta=(ClampMin="0.0", ClampMax="2.0"))
	float ZoomTransitionCaptureSeconds = 0.25f;

	/** 虚空（空）をクリックしたときに仮想ヒット位置として使う距離 (cm)。
	 *  これにより空にいる鳥等にもズームして撮影できる。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zoom")
	float SkyFallbackDistance = 3000.f;

	UPROPERTY(BlueprintReadOnly, Category="Zoom")
	FVector TargetOffset = FVector::ZeroVector;

	/** ズーム開始時（右クリックでズーム確定した瞬間）の SE */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zoom|Audio")
	FTomatinaSoundCue ZoomInSound;

	/** ズーム解除時の SE */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zoom|Audio")
	FTomatinaSoundCue ZoomOutSound;

	// ──────────────────────────────────────────────
	// Enhanced Input
	// ──────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	UInputMappingContext* DefaultMappingContext = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	UInputAction* IA_RightMouse = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	UInputAction* IA_LeftMouse = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	UInputAction* IA_Look = nullptr;

protected:
	void OnRightMousePressed(const FInputActionValue& Value);
	void OnRightMouseReleased(const FInputActionValue& Value);
	void OnLeftMousePressed(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void CenterCursorOnMainScreen();

private:
	UPROPERTY()
	APlayerController* PC = nullptr;

	void EnsureDualScreenWindowLayout();

	bool bWindowLayoutVerified = false;
	float WindowLayoutRetryElapsed = 0.f;
	int32 WindowLayoutRetryCount = 0;

	// Tick で消費されるマウス入力（Triggered で蓄積）
	FVector2D CurrentLookInput = FVector2D::ZeroVector;

	float PhoneCaptureBurstRemaining = 0.f;

	void UpdateDualScreenLayoutRetry(float DeltaTime);
	void UpdateZoomInterpolation(float RealDelta);
	void CenterLegacySpanCursorWhenZoomReady();
	void EnableZoomLookWhenReady();
	void ApplyZoomLookInput(float RealDelta);
	void UpdateZoomHUDCursor(ATomatinaHUD* HUD);
	void ResetZoomViewWhenIdle();
	void ClampMouseCursorToMainScreen();
	void EndZoomInteraction();
	void ClampZoomTargetOffsetAgainstWorld();
	void ApplyZoomNearClipGuard();
	void RequestPhoneCaptureBurst(float Duration);
	void UpdatePhoneSceneCapture(float DeltaTime);
	void CapturePhoneSceneNow();
};
