// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "TomatoDirtManager.h"   // FDirtSplat
#include "TomatinaHUD.generated.h"

class UUserWidget;
class UTexture2D;
class UTextBlock;
class UImage;
class UCanvasPanel;
class UMaterialInterface;

/**
 * ゲーム HUD。
 * C++ は位置・サイズ制御を行わない。
 * Widget の生成・AddToViewport・中身テキスト/画像更新・Visibility 切替のみ行う。
 *
 * 例外:
 *  - UpdateCursorPosition（IMG_Crosshair のスロット位置を更新）
 *  - UpdateDirtDisplay（汚れは動的生成で座標管理が必須）
 */
UCLASS()
class TOMATO_API ATomatinaHUD : public AHUD
{
	GENERATED_BODY()

public:
	ATomatinaHUD();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// =========================================================================
	// Widget クラス参照（BP の Details で設定）
	// =========================================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Widgets")
	TSubclassOf<UUserWidget> ViewFinderWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Widgets")
	TSubclassOf<UUserWidget> CursorWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Widgets")
	TSubclassOf<UUserWidget> DirtOverlayWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Widgets")
	TSubclassOf<UUserWidget> ResultWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Widgets")
	TSubclassOf<UUserWidget> MissionWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Widgets")
	TSubclassOf<UUserWidget> MissionResultWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Widgets")
	TSubclassOf<UUserWidget> FinalResultWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Widgets")
	TSubclassOf<UUserWidget> FlashWidgetClass;

	/** テストモード用 PiP Widget（WBP_TestPip） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Widgets")
	TSubclassOf<UUserWidget> TestPipWidgetClass;

	// =========================================================================
	// マテリアル（BP の Details で設定）
	// =========================================================================

	/** ResultWidget の IMG_Photo に適用するマテリアル（M_PhotoDisplay） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Materials")
	UMaterialInterface* PhotoDisplayMaterial;

	/** ViewFinder/TestPip の IMG_ZoomView に適用するマテリアル（M_ZoomDisplay） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Materials")
	UMaterialInterface* ZoomDisplayMaterial;

	/** 汚れテクスチャ */
	UPROPERTY(EditAnywhere, Category="HUD|Dirt")
	UTexture2D* DirtTexture;

	// =========================================================================
	// 画面サイズ（PlayerPawn から取得。TowelSystem 等が参照する）
	// =========================================================================

	UPROPERTY(BlueprintReadOnly, Category="HUD|Screen")
	float MainWidth = 2560.f;

	UPROPERTY(BlueprintReadOnly, Category="HUD|Screen")
	float MainHeight = 1600.f;

	UPROPERTY(BlueprintReadOnly, Category="HUD|Screen")
	float PhoneWidth = 1024.f;

	UPROPERTY(BlueprintReadOnly, Category="HUD|Screen")
	float PhoneHeight = 768.f;

	// =========================================================================
	// カーソル
	// =========================================================================

	UFUNCTION(BlueprintCallable, Category="HUD|Cursor")
	void UpdateCursorPosition(FVector2D Pos);

	UFUNCTION(BlueprintCallable, Category="HUD|Cursor")
	void ShowCursor();

	UFUNCTION(BlueprintCallable, Category="HUD|Cursor")
	void HideCursor();

	UPROPERTY(BlueprintReadOnly, Category="HUD|Cursor")
	FVector2D CurrentCursorPosition;

	// =========================================================================
	// リザルト
	// =========================================================================

	UFUNCTION(BlueprintCallable, Category="HUD|Result")
	void ShowResult(int32 Score, const FString& Comment);

	UFUNCTION(BlueprintCallable, Category="HUD|Result")
	void HideResult();

	UPROPERTY(BlueprintReadOnly, Category="HUD|Result")
	int32 LastScore = 0;

	UPROPERTY(BlueprintReadOnly, Category="HUD|Result")
	FString LastComment;

	// =========================================================================
	// ミッション
	// =========================================================================

	UFUNCTION(BlueprintCallable, Category="HUD|Mission")
	void ShowMissionDisplay(const FText& MissionText, UTexture2D* TargetImage);

	UFUNCTION(BlueprintCallable, Category="HUD|Mission")
	void HideMissionText();

	UFUNCTION(BlueprintCallable, Category="HUD|Mission")
	void ShowMissionResult(int32 Score, const FString& Comment);

	UFUNCTION(BlueprintCallable, Category="HUD|Mission")
	void HideMissionResult();

	UPROPERTY(BlueprintReadOnly, Category="HUD|Mission")
	FText CurrentMissionText;

	UPROPERTY(BlueprintReadOnly, Category="HUD|Mission")
	int32 MissionResultScore = 0;

	UPROPERTY(BlueprintReadOnly, Category="HUD|Mission")
	FString MissionResultComment;

	// =========================================================================
	// タイマー／スコア
	// =========================================================================

	UFUNCTION(BlueprintCallable, Category="HUD|Timer")
	void UpdateTimer(float RemainingSeconds);

	UFUNCTION(BlueprintCallable, Category="HUD|Score")
	void UpdateTotalScore(int32 TotalScore);

	UPROPERTY(BlueprintReadOnly, Category="HUD|Score")
	int32 CurrentTotalScore = 0;

	// =========================================================================
	// シャッターフラッシュ
	// =========================================================================

	UFUNCTION(BlueprintCallable, Category="HUD|Flash")
	void PlayShutterFlash();

	// =========================================================================
	// 最終リザルト
	// =========================================================================

	UFUNCTION(BlueprintCallable, Category="HUD|Result")
	void ShowFinalResult(int32 TotalScore, int32 MissionCount);

	// =========================================================================
	// 汚れ
	// =========================================================================

	UFUNCTION(BlueprintCallable, Category="HUD|Dirt")
	void UpdateDirtDisplay(const TArray<FDirtSplat>& Dirts);

	UPROPERTY(BlueprintReadOnly, Category="HUD|Dirt")
	TArray<FDirtSplat> CachedDirts;

	// =========================================================================
	// タオル
	// =========================================================================

	UFUNCTION(BlueprintCallable, Category="HUD|Towel")
	void UpdateTowelStatus(float DurabilityPercent, bool bSwapping);

protected:
	UPROPERTY() UUserWidget* ViewFinderWidget;
	UPROPERTY() UUserWidget* CursorWidget;
	UPROPERTY() UUserWidget* DirtOverlayWidget;
	UPROPERTY() UUserWidget* ResultWidget;
	UPROPERTY() UUserWidget* MissionWidget;
	UPROPERTY() UUserWidget* MissionResultWidget;
	UPROPERTY() UUserWidget* FinalResultWidget;
	UPROPERTY() UUserWidget* FlashWidget;
	UPROPERTY() UUserWidget* TestPipWidget;

private:
	FTimerHandle FlashTimerHandle;
	float        FlashAlpha = 0.0f;

	ATomatoDirtManager* GetDirtManager();
	UPROPERTY() ATomatoDirtManager* CachedDirtManager;

	/** Widget 内の Image/Material 初期化（IMG_ZoomView に ZoomDisplayMaterial を設定） */
	void ApplyZoomMaterial(UUserWidget* Widget);
};
