// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DefaultPawn.h"
#include "InputActionValue.h"
#include "TomatinaPlayerPawn.generated.h"

class UCameraComponent;
class USceneCaptureComponent2D;
class APlayerController;
class UInputMappingContext;
class UInputAction;

/**
 * プレイヤーポーン。メインカメラと SceneCapture によるズーム撮影を持つ。
 * 位置制御は一切行わない（UpdateCursorPosition のみ例外）。
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
	UCameraComponent* PlayerCamera;

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
	// テストモード（PiP プレビュー）
	// -------------------------------------------------------------------------

	/** true: メインモニター上にズーム映像の PiP プレビューを出す */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Test")
	bool bTestMode = true;

	// -------------------------------------------------------------------------
	// ズーム状態
	// -------------------------------------------------------------------------

	UPROPERTY(BlueprintReadWrite, Category="Zoom")
	bool bIsZooming = false;

	UPROPERTY(BlueprintReadWrite, Category="Zoom")
	bool bZoomComplete = false;

	UPROPERTY(BlueprintReadWrite, Category="Zoom")
	bool bCursorCentered = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zoom")
	float DefaultFOV = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zoom")
	float ZoomFOV = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zoom")
	float ZoomSpeed = 3.3f;

	UPROPERTY(BlueprintReadWrite, Category="Zoom")
	float ZoomAlpha = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zoom")
	float MoveSpeed = 500.f;

	UPROPERTY(BlueprintReadWrite, Category="Zoom")
	FVector TargetOffset = FVector::ZeroVector;

	// -------------------------------------------------------------------------
	// Enhanced Input
	// -------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	UInputAction* IA_RightMouse;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	UInputAction* IA_LeftMouse;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	UInputAction* IA_Look;

	UPROPERTY()
	FVector2D CurrentLookInput = FVector2D::ZeroVector;

protected:
	UFUNCTION(BlueprintCallable, Category="Input")
	void OnRightMousePressed();

	UFUNCTION(BlueprintCallable, Category="Input")
	void OnRightMouseReleased();

	UFUNCTION(BlueprintCallable, Category="Input")
	void OnLeftMousePressed();

	void OnLook(const FInputActionValue& Value);

private:
	UPROPERTY()
	APlayerController* PC;
};
