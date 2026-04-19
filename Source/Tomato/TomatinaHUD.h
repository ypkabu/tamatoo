// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "TomatoDirtManager.h"   // FDirtSplat
#include "TomatinaHUD.generated.h"

class UUserWidget;
class UTexture2D;
class UMaterialInterface;

/**
 * Tomatina の HUD。
 * 全 Widget の生成・表示制御を一括で管理する。
 * 位置・サイズは原則 C++ で触らない。
 *   例外1: UpdateCursorPosition   → IMG_Crosshair の CanvasPanelSlot を移動
 *   例外2: UpdateDirtDisplay      → SplatContainer 内に UImage を動的生成
 */
UCLASS()
class TOMATO_API ATomatinaHUD : public AHUD
{
	GENERATED_BODY()

public:
	ATomatinaHUD();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// =========================================================================
	// Widget クラス参照（BP で設定）
	// =========================================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Widgets")
	TSubclassOf<UUserWidget> ViewFinderWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Widgets")
	TSubclassOf<UUserWidget> CursorWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Widgets")
	TSubclassOf<UUserWidget> DirtOverlayWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Widgets")
	TSubclassOf<UUserWidget> PhotoResultWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Widgets")
	TSubclassOf<UUserWidget> MissionDisplayWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Widgets")
	TSubclassOf<UUserWidget> MissionResultWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Widgets")
	TSubclassOf<UUserWidget> FinalResultWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Widgets")
	TSubclassOf<UUserWidget> ShutterFlashWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Widgets")
	TSubclassOf<UUserWidget> CountdownWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Widgets")
	TSubclassOf<UUserWidget> TestPipWidgetClass;

	// =========================================================================
	// マテリアル・テクスチャ
	// =========================================================================
	UPROPERTY(EditAnywhere, Category="HUD|Material")
	UMaterialInterface* ZoomDisplayMaterial = nullptr;

	UPROPERTY(EditAnywhere, Category="HUD|Material")
	UMaterialInterface* PhotoDisplayMaterial = nullptr;

	UPROPERTY(EditAnywhere, Category="HUD|Material")
	UTexture2D* DirtTexture = nullptr;

	// =========================================================================
	// 画面サイズ（PlayerPawn から BeginPlay で取得）
	// =========================================================================
	UPROPERTY(BlueprintReadOnly, Category="HUD|Screen")
	float MainWidth = 2560.f;

	UPROPERTY(BlueprintReadOnly, Category="HUD|Screen")
	float MainHeight = 1600.f;

	UPROPERTY(BlueprintReadOnly, Category="HUD|Screen")
	float PhoneWidth = 1024.f;

	UPROPERTY(BlueprintReadOnly, Category="HUD|Screen")
	float PhoneHeight = 768.f;

	UPROPERTY(BlueprintReadOnly, Category="HUD|Screen")
	bool bTestMode = true;

	// =========================================================================
	// カーソル追従（C++ で位置操作する例外1）
	// =========================================================================
	UFUNCTION(BlueprintCallable, Category="HUD|Cursor")
	void UpdateCursorPosition(FVector2D Pos);

	UFUNCTION(BlueprintCallable, Category="HUD|Cursor")
	void ShowCursor();

	UFUNCTION(BlueprintCallable, Category="HUD|Cursor")
	void HideCursor();

	// =========================================================================
	// リザルト
	// =========================================================================
	UFUNCTION(BlueprintCallable, Category="HUD|Result")
	void ShowResult(int32 Score, const FString& Comment);

	UFUNCTION(BlueprintCallable, Category="HUD|Result")
	void HideResult();

	UFUNCTION(BlueprintCallable, Category="HUD|Result")
	void ShowMissionResult(int32 Score, const FString& Comment);

	UFUNCTION(BlueprintCallable, Category="HUD|Result")
	void HideMissionResult();

	UFUNCTION(BlueprintCallable, Category="HUD|Result")
	void ShowFinalResult(int32 InTotalScore, int32 MissionCount);

	// =========================================================================
	// ミッション表示
	// =========================================================================
	UFUNCTION(BlueprintCallable, Category="HUD|Mission")
	void ShowMissionDisplay(const FText& MissionText, UTexture2D* TargetImage);

	UFUNCTION(BlueprintCallable, Category="HUD|Mission")
	void HideMissionDisplay();

	UFUNCTION(BlueprintCallable, Category="HUD|Mission")
	void UpdateTimer(float RemainingSeconds);

	UFUNCTION(BlueprintCallable, Category="HUD|Mission")
	void UpdateTotalScore(int32 TotalScore);

	// =========================================================================
	// カウントダウン
	// =========================================================================
	UFUNCTION(BlueprintCallable, Category="HUD|Countdown")
	void ShowCountdown(int32 Seconds);

	UFUNCTION(BlueprintCallable, Category="HUD|Countdown")
	void HideCountdown();

	// =========================================================================
	// シャッターフラッシュ
	// =========================================================================
	UFUNCTION(BlueprintCallable, Category="HUD|Flash")
	void PlayShutterFlash();

	// =========================================================================
	// 汚れ（C++ で位置操作する例外2）
	// =========================================================================
	UFUNCTION(BlueprintCallable, Category="HUD|Dirt")
	void UpdateDirtDisplay(const TArray<FDirtSplat>& Dirts);

	// =========================================================================
	// タオル（Blueprint から呼ばれる）
	// =========================================================================
	UFUNCTION(BlueprintCallable, Category="HUD|Towel")
	void UpdateTowelStatus(float DurabilityPercent, bool bSwapping);

protected:
	// ── 永続 Widget ─────────────────────────────
	UPROPERTY() UUserWidget* ViewFinderWidget     = nullptr;
	UPROPERTY() UUserWidget* CursorWidget         = nullptr;
	UPROPERTY() UUserWidget* DirtOverlayWidget    = nullptr;
	UPROPERTY() UUserWidget* MissionDisplayWidget = nullptr;
	UPROPERTY() UUserWidget* TestPipWidget        = nullptr;

	// ── 都度生成・破棄する Widget ─────────────────
	UPROPERTY() UUserWidget* PhotoResultWidget    = nullptr;
	UPROPERTY() UUserWidget* MissionResultWidget  = nullptr;
	UPROPERTY() UUserWidget* FinalResultWidget    = nullptr;
	UPROPERTY() UUserWidget* CountdownWidget      = nullptr;
	UPROPERTY() UUserWidget* ShutterFlashWidget   = nullptr;

private:
	// シャッターフラッシュの実時間タイマー（TimeDilation=0 でも動く）
	float FlashElapsed = 0.f;
	bool  bFlashActive = false;
};
