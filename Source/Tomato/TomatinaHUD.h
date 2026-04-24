// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "TomatoDirtManager.h"   // FDirtSplat
#include "TomatinaHUD.generated.h"

class UUserWidget;
class UTexture2D;
class UMaterialInterface;
class SWindow;

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

	/** 第二ウィンドウ (スマホ側) に出す UserWidget。
	 *  想定階層:
	 *    Root: CanvasPanel (Fill)
	 *      - IMG_ZoomView (Image, Fill) — C++ が RT_Zoom をバインド
	 *      - PhoneSplatContainer (CanvasPanel, Fill) — 汚れ動的生成先
	 *      - IMG_PhoneCursor (Image, CanvasPanelSlot) — ズーム中のクロスヘア
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Widgets")
	TSubclassOf<UUserWidget> PhoneViewWidgetClass;

	// =========================================================================
	// マテリアル・テクスチャ
	// =========================================================================
	UPROPERTY(EditAnywhere, Category="HUD|Material")
	UMaterialInterface* ZoomDisplayMaterial = nullptr;

	UPROPERTY(EditAnywhere, Category="HUD|Material")
	UMaterialInterface* PhotoDisplayMaterial = nullptr;

	UPROPERTY(EditAnywhere, Category="HUD|Material")
	UTexture2D* DirtTexture = nullptr;

	/**
	 * 汚れ画像のバリエーション配列。
	 * FDirtSplat::TextureIndex で参照される。空ならフォールバックで DirtTexture を使用。
	 * Manager 側の NumDirtVariants とサイズを合わせること。
	 */
	UPROPERTY(EditAnywhere, Category="HUD|Material")
	TArray<UTexture2D*> DirtTextures;

	// =========================================================================
	// 写真表示サイズ（WBP_PhotoResult の SplatContainer サイズと一致させる）
	// =========================================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Photo")
	float PhotoDisplayWidth = 1024.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Photo")
	float PhotoDisplayHeight = 768.f;

	/** 汚れ表示時に確保する内側マージン（ピクセル）。汚れが画面端からさらに内側に寄る */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Dirt")
	float DirtInnerMargin = 30.f;

	/** スマホ側 (PhoneView) に表示する汚れのサイズ倍率。
	 *  物理サイズが小さいので相対的に大きく見えがち → 0.7 等に落とすと自然になる */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Dirt", meta=(ClampMin="0.1", ClampMax="2.0"))
	float DirtSizeScalePhone = 0.7f;

	/** 写真リザルト画面の汚れサイズ倍率 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Dirt", meta=(ClampMin="0.1", ClampMax="2.0"))
	float DirtSizeScalePhoto = 0.5f;

	/** メインモニター側の汚れ表示エリアの割合 (X, Y)。中央寄せで縮小。
	 *  デフォルト 2560x1600 のうち中央 1920x1080 相当
	 *  (1920/2560, 1080/1600) = (0.75, 0.675)。スマホ・写真には影響しない。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Dirt", meta=(ClampMin="0.1", ClampMax="1.0"))
	FVector2D MainDirtAreaRatio = FVector2D(0.75f, 0.675f);

	// =========================================================================
	// 画面サイズ（PlayerPawn から BeginPlay で取得）
	// =========================================================================
	UPROPERTY(BlueprintReadOnly, Category="HUD|Screen")
	float MainWidth = 2560.f;

	UPROPERTY(BlueprintReadOnly, Category="HUD|Screen")
	float MainHeight = 1600.f;

	UPROPERTY(BlueprintReadOnly, Category="HUD|Screen")
	float PhoneWidth = 2024.f;

	UPROPERTY(BlueprintReadOnly, Category="HUD|Screen")
	float PhoneHeight = 1152.f;

	UPROPERTY(BlueprintReadOnly, Category="HUD|Screen")
	bool bTestMode = true;

	/** true なら HUD が第二 SWindow を生成しスマホ側 UI を独立ウィンドウに出す */
	UPROPERTY(BlueprintReadOnly, Category="HUD|Screen")
	bool bUseSeparatePhoneWindow = true;

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
	// タオル表示（LeapMotion の手位置に追従）
	// WBP_DirtOverlay に IMG_Towel（CanvasPanelSlot）を置いておく
	// =========================================================================
	UFUNCTION(BlueprintCallable, Category="HUD|Towel")
	void UpdateTowelPosition(FVector2D Pos);

	UFUNCTION(BlueprintCallable, Category="HUD|Towel")
	void ShowTowel();

	UFUNCTION(BlueprintCallable, Category="HUD|Towel")
	void HideTowel();

	// =========================================================================
	// リザルト
	// =========================================================================
	UFUNCTION(BlueprintCallable, Category="HUD|Result")
	void ShowResult(int32 Score, const FString& Comment, const TArray<FDirtSplat>& Dirts);

	UFUNCTION(BlueprintCallable, Category="HUD|Result")
	void HideResult();

	UFUNCTION(BlueprintCallable, Category="HUD|Result")
	void ShowMissionResult(int32 Score, const FString& Comment);

	UFUNCTION(BlueprintCallable, Category="HUD|Result")
	void HideMissionResult();

	UFUNCTION(BlueprintCallable, Category="HUD|Result")
	void ShowFinalResult(int32 InTotalScore, int32 MissionCount,
		const FString& AverageStylishRank = TEXT("C"), float SyncRate01 = 0.f);

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
	// スタイリッシュランク表示
	// =========================================================================
	UFUNCTION(BlueprintCallable, Category="HUD|Stylish")
	void UpdateStylishDisplay(const FString& RankText, float GaugePercent, int32 ComboCount, bool bDanger);

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

	/** シャッターフラッシュの表示時間（秒）。微調整用 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Flash", meta=(ClampMin="0.01", ClampMax="2.0"))
	float ShutterFlashDuration = 0.2f;

	/** シャッターフラッシュの Z-Order。汚れ等より最前面に出すため大きい値。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Flash")
	int32 ShutterFlashZOrder = 9999;

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
	UPROPERTY() UUserWidget* PhoneViewWidget      = nullptr;

	/** ズームマテリアルの動的インスタンス (PhoneView 用)。RT_Zoom をパラメータ注入して持つ */
	UPROPERTY() class UMaterialInstanceDynamic* PhoneZoomDMI = nullptr;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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

	/**
	 * 指定 CanvasPanel に汚れの UImage を動的生成する共通ヘルパー。
	 * メイン/フォン/写真の 3 箇所から呼ばれる。
	 *
	 * @param OwnerWidget  NewObject の Outer として使う Widget
	 * @param Container    汚れを追加する CanvasPanel
	 * @param Dirts        汚れ配列
	 * @param AreaWidth    汚れ位置・サイズの基準となる領域幅
	 * @param AreaHeight   同、領域高さ
	 * @param OriginX      領域の左上 X（絶対座標。例：メイン=0 / フォン=MainWidth / 写真=0）
	 * @param OriginY      領域の左上 Y
	 */
	void AddDirtSplatsToCanvas(
		UUserWidget* OwnerWidget,
		class UCanvasPanel* Container,
		const TArray<FDirtSplat>& Dirts,
		float AreaWidth,
		float AreaHeight,
		float OriginX,
		float OriginY,
		float SizeScale = 1.0f);

	/** WBP_MissionDisplay に必要なスタイリッシュ UI 名称が揃っているかを確認する */
	void ValidateMissionStylishWidgets();

	/** Widget 内のズーム表示 Image に ZoomDisplayMaterial をバインドする */
	bool BindZoomMaterialToWidget(UUserWidget* Widget, FName PreferredImageName, const TCHAR* WidgetLabel);

	/** iPhone 領域に合わせてズーム表示 Image の位置とサイズを強制レイアウトする */
	void LayoutPhoneZoomImage(UUserWidget* Widget, FName PreferredImageName, const TCHAR* WidgetLabel);

	/** ZoomView 用 Image を Widget から探し、なければ実行時に生成する */
	class UImage* FindOrCreateZoomImage(UUserWidget* Widget, FName PreferredImageName, const TCHAR* WidgetLabel);

	/** ZoomView 用 Image に RT_Zoom（またはマテリアル）を設定する */
	bool ConfigureZoomImageContent(class UImage* ImageWidget, const TCHAR* WidgetLabel);

	/** 第二 SWindow を生成しスマホ側 UI (PhoneViewWidget) をアタッチ */
	void CreatePhoneWindow();

	/** 第二 SWindow を破棄 */
	void DestroyPhoneWindow();

	/** スマホウィンドウの生成済み Slate Window */
	TSharedPtr<SWindow> PhoneWindow;
};
