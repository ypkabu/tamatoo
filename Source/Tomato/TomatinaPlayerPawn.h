// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DefaultPawn.h"
#include "InputActionValue.h"
#include "TomatinaPlayerPawn.generated.h"

class UCameraComponent;
class USceneCaptureComponent2D;
class ATomatinaHUD;
class APlayerController;
class UInputMappingContext;
class UInputAction;

/**
 * プレイヤーが操作するポーン。
 * メインカメラと SceneCapture によるズーム撮影機能を持つ。
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

	// -------------------------------------------------------------------------
	// コンポーネント
	// -------------------------------------------------------------------------

	/** メインモニター用カメラ（固定） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
	UCameraComponent* PlayerCamera;

	/** ズーム用 SceneCapture（PlayerCamera の子） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera")
	USceneCaptureComponent2D* SceneCapture_Zoom;

	// -------------------------------------------------------------------------
	// 画面サイズ
	// -------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Screen")
	float MainWidth = 2560.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Screen")
	float MainHeight = 1600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Screen")
	float PhoneWidth = 1024.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Screen")
	float PhoneHeight = 768.f;

	// -------------------------------------------------------------------------
	// ズーム状態
	// -------------------------------------------------------------------------

	UPROPERTY(BlueprintReadWrite, Category="Zoom")
	bool bIsZooming = false;

	UPROPERTY(BlueprintReadWrite, Category="Zoom")
	bool bZoomComplete = false;

	UPROPERTY(BlueprintReadWrite, Category="Zoom")
	bool bCursorCentered = false;

	/** ズームなし時の FOV */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zoom")
	float DefaultFOV = 90.f;

	/** ズーム時の FOV */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zoom")
	float ZoomFOV = 30.f;

	/** ZoomAlpha の補間速度（1/0.3 ≈ 3.3 で 0.3 秒到達） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zoom")
	float ZoomSpeed = 3.3f;

	/** ズーム進行度（0 = 未ズーム、1 = フルズーム） */
	UPROPERTY(BlueprintReadWrite, Category="Zoom")
	float ZoomAlpha = 0.f;

	/** ズーム完了後のマウスルック速度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zoom")
	float MoveSpeed = 500.f;

	/** クリック地点を中心に持ってくるための SceneCapture ローカルオフセット */
	UPROPERTY(BlueprintReadWrite, Category="Zoom")
	FVector TargetOffset = FVector::ZeroVector;

	// -------------------------------------------------------------------------
	// Enhanced Input
	// -------------------------------------------------------------------------

	/** デフォルト Input Mapping Context（BP_TomatinaPlayerPawn の Detail で設定） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	UInputMappingContext* DefaultMappingContext = nullptr;

	/** 右クリック Input Action（IA_RightMouse） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	UInputAction* IA_RightMouse = nullptr;

	/** 左クリック Input Action（IA_LeftMouse） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	UInputAction* IA_LeftMouse = nullptr;

	// -------------------------------------------------------------------------
	// テストモード
	// -------------------------------------------------------------------------

	/** true: PiP プレビューをメインモニターに表示する（開発用） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Debug")
	bool bTestMode = true;

protected:
	// -------------------------------------------------------------------------
	// 入力ハンドラ
	// -------------------------------------------------------------------------

	/** 右クリック押下：ズーム開始 */
	void OnRightMousePressed(const FInputActionValue& Value);

	/** 右クリック解放：ズーム解除 */
	void OnRightMouseReleased(const FInputActionValue& Value);

	/** 左クリック押下：撮影 */
	void OnLeftMousePressed(const FInputActionValue& Value);

private:
	/** キャッシュ済みプレイヤーコントローラー */
	UPROPERTY()
	APlayerController* PC;
};
