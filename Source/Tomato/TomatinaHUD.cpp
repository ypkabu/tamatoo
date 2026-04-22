// Fill out your copyright notice in the Description page of Project Settings.

#include "TomatinaHUD.h"

#include "Blueprint/UserWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"

#include "TomatinaPlayerPawn.h"

// =============================================================================
// コンストラクタ
// =============================================================================
ATomatinaHUD::ATomatinaHUD()
{
	PrimaryActorTick.bCanEverTick = true;
}

// =============================================================================
// BeginPlay — 永続 Widget を生成
// =============================================================================
void ATomatinaHUD::BeginPlay()
{
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD::BeginPlay 開始"));
	Super::BeginPlay();

	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		UE_LOG(LogTemp, Error, TEXT("ATomatinaHUD: PlayerController 取得失敗"));
		return;
	}

	// ── PlayerPawn から 4 サイズと bTestMode を取得 ──
	if (ATomatinaPlayerPawn* Pawn = Cast<ATomatinaPlayerPawn>(PC->GetPawn()))
	{
		MainWidth   = Pawn->MainWidth;
		MainHeight  = Pawn->MainHeight;
		PhoneWidth  = Pawn->PhoneWidth;
		PhoneHeight = Pawn->PhoneHeight;
		bTestMode   = Pawn->bTestMode;
		UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD: サイズ取得 Main=(%.0fx%.0f) Phone=(%.0fx%.0f) bTestMode=%d"),
			MainWidth, MainHeight, PhoneWidth, PhoneHeight, bTestMode ? 1 : 0);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD: PlayerPawn 取得失敗（デフォルト値使用）"));
	}

	// ── ViewFinder ─────────────────────────────
	if (ViewFinderWidgetClass)
	{
		ViewFinderWidget = CreateWidget<UUserWidget>(PC, ViewFinderWidgetClass);
		if (ViewFinderWidget) { ViewFinderWidget->AddToViewport(100); }
	}
	else { UE_LOG(LogTemp, Error, TEXT("ATomatinaHUD: ViewFinderWidgetClass 未設定")); }

	// ── DirtOverlay ─────────────────────────────
	if (DirtOverlayWidgetClass)
	{
		DirtOverlayWidget = CreateWidget<UUserWidget>(PC, DirtOverlayWidgetClass);
		if (DirtOverlayWidget) { DirtOverlayWidget->AddToViewport(150); }
	}
	else { UE_LOG(LogTemp, Error, TEXT("ATomatinaHUD: DirtOverlayWidgetClass 未設定")); }

	// ── Cursor ─────────────────────────────
	if (CursorWidgetClass)
	{
		CursorWidget = CreateWidget<UUserWidget>(PC, CursorWidgetClass);
		if (CursorWidget)
		{
			CursorWidget->AddToViewport(200);
			CursorWidget->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	else { UE_LOG(LogTemp, Error, TEXT("ATomatinaHUD: CursorWidgetClass 未設定")); }

	// ── MissionDisplay（HUD 常時表示：お題・タイマー・スコア） ──
	if (MissionDisplayWidgetClass)
	{
		MissionDisplayWidget = CreateWidget<UUserWidget>(PC, MissionDisplayWidgetClass);
		if (MissionDisplayWidget)
		{
			MissionDisplayWidget->AddToViewport(250);
			MissionDisplayWidget->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	else { UE_LOG(LogTemp, Error, TEXT("ATomatinaHUD: MissionDisplayWidgetClass 未設定")); }

	// ── TestPip（開発用） ─────────────────────────────
	if (bTestMode && TestPipWidgetClass)
	{
		TestPipWidget = CreateWidget<UUserWidget>(PC, TestPipWidgetClass);
		if (TestPipWidget)
		{
			TestPipWidget->AddToViewport(110);
			TestPipWidget->SetVisibility(ESlateVisibility::Visible);

			UImage* Img = Cast<UImage>(TestPipWidget->GetWidgetFromName(TEXT("IMG_ZoomView")));
			if (Img && ZoomDisplayMaterial)
			{
				Img->SetBrushFromMaterial(ZoomDisplayMaterial);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("ATomatinaHUD: TestPip IMG_ZoomView=%s Material=%s"),
					Img ? TEXT("OK") : TEXT("NULL"),
					ZoomDisplayMaterial ? TEXT("OK") : TEXT("NULL"));
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD::BeginPlay 完了"));
}

// =============================================================================
// Tick — シャッターフラッシュの実時間制御
// =============================================================================
void ATomatinaHUD::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bFlashActive && ShutterFlashWidget)
	{
		FlashElapsed += FApp::GetDeltaTime();
		if (FlashElapsed >= 0.1f)
		{
			ShutterFlashWidget->RemoveFromParent();
			ShutterFlashWidget = nullptr;
			bFlashActive = false;
			FlashElapsed = 0.f;
		}
	}
}

// =============================================================================
// UpdateCursorPosition（例外1）
// IMG_Crosshair の CanvasPanelSlot をメイン座標 → iPhone 座標にマップ
// =============================================================================
void ATomatinaHUD::UpdateCursorPosition(FVector2D Pos)
{
	if (!CursorWidget) { return; }

	UImage* Crosshair = Cast<UImage>(CursorWidget->GetWidgetFromName(TEXT("IMG_Crosshair")));
	if (!Crosshair) { return; }

	UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Crosshair->Slot);
	if (!Slot) { return; }

	const float NormX  = (MainWidth  > 0.f) ? Pos.X / MainWidth  : 0.f;
	const float NormY  = (MainHeight > 0.f) ? Pos.Y / MainHeight : 0.f;
	const float PhoneX = MainWidth + NormX * PhoneWidth;
	const float PhoneY = NormY * PhoneHeight;
	Slot->SetPosition(FVector2D(PhoneX, PhoneY));
}

// =============================================================================
// ShowCursor / HideCursor
// =============================================================================
void ATomatinaHUD::ShowCursor()
{
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD::ShowCursor"));
	if (CursorWidget) { CursorWidget->SetVisibility(ESlateVisibility::Visible); }
}

void ATomatinaHUD::HideCursor()
{
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD::HideCursor"));
	if (CursorWidget) { CursorWidget->SetVisibility(ESlateVisibility::Hidden); }
}

// =============================================================================
// ShowResult — 撮影後の結果（WBP_PhotoResult）
// IMG_Photo の上にある SplatContainer に撮影時の汚れを同じ正規化座標で重ねる
// =============================================================================
void ATomatinaHUD::ShowResult(int32 Score, const FString& Comment, const TArray<FDirtSplat>& Dirts)
{
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD::ShowResult Score=%d Dirts=%d"),
		Score, Dirts.Num());

	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !PhotoResultWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("ShowResult: PC=%s Class=%s"),
			PC ? TEXT("OK") : TEXT("NULL"),
			PhotoResultWidgetClass ? TEXT("OK") : TEXT("NULL"));
		return;
	}

	if (PhotoResultWidget)
	{
		PhotoResultWidget->RemoveFromParent();
		PhotoResultWidget = nullptr;
	}

	PhotoResultWidget = CreateWidget<UUserWidget>(PC, PhotoResultWidgetClass);
	if (!PhotoResultWidget) { return; }
	PhotoResultWidget->AddToViewport(300);

	// TXT_Score
	if (UTextBlock* TxtScore = Cast<UTextBlock>(
			PhotoResultWidget->GetWidgetFromName(TEXT("TXT_Score"))))
	{
		TxtScore->SetText(FText::FromString(FString::Printf(TEXT("%d 点"), Score)));
	}

	// TXT_Comment
	if (UTextBlock* TxtComment = Cast<UTextBlock>(
			PhotoResultWidget->GetWidgetFromName(TEXT("TXT_Comment"))))
	{
		TxtComment->SetText(FText::FromString(Comment));
	}

	// IMG_Photo（M_PhotoDisplay をマテリアルとしてセット）
	if (UImage* ImgPhoto = Cast<UImage>(
			PhotoResultWidget->GetWidgetFromName(TEXT("IMG_Photo"))))
	{
		if (PhotoDisplayMaterial) { ImgPhoto->SetBrushFromMaterial(PhotoDisplayMaterial); }
	}

	// SplatContainer 上に同じ正規化座標で汚れを重ねる
	// （IMG_Photo に被せる位置で WBP 側に UCanvasPanel "SplatContainer" を配置すること）
	if (UCanvasPanel* PhotoSplat = Cast<UCanvasPanel>(
			PhotoResultWidget->GetWidgetFromName(TEXT("SplatContainer"))))
	{
		AddDirtSplatsToCanvas(
			PhotoResultWidget, PhotoSplat, Dirts,
			PhotoDisplayWidth, PhotoDisplayHeight,
			0.f, 0.f);
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("ShowResult: WBP_PhotoResult に SplatContainer が見つかりません（汚れ非表示）"));
	}
}

void ATomatinaHUD::HideResult()
{
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD::HideResult"));
	if (PhotoResultWidget)
	{
		PhotoResultWidget->RemoveFromParent();
		PhotoResultWidget = nullptr;
	}
}

// =============================================================================
// ShowMissionResult — 時間切れ表示（WBP_MissionResult）
// =============================================================================
void ATomatinaHUD::ShowMissionResult(int32 Score, const FString& Comment)
{
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD::ShowMissionResult Score=%d"), Score);

	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !MissionResultWidgetClass) { return; }

	if (MissionResultWidget)
	{
		MissionResultWidget->RemoveFromParent();
		MissionResultWidget = nullptr;
	}

	MissionResultWidget = CreateWidget<UUserWidget>(PC, MissionResultWidgetClass);
	if (!MissionResultWidget) { return; }
	MissionResultWidget->AddToViewport(275);

	if (UTextBlock* TxtScore = Cast<UTextBlock>(
			MissionResultWidget->GetWidgetFromName(TEXT("TXT_Score"))))
	{
		TxtScore->SetText(FText::FromString(FString::Printf(TEXT("%d 点"), Score)));
	}
	if (UTextBlock* TxtComment = Cast<UTextBlock>(
			MissionResultWidget->GetWidgetFromName(TEXT("TXT_Comment"))))
	{
		TxtComment->SetText(FText::FromString(Comment));
	}
}

void ATomatinaHUD::HideMissionResult()
{
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD::HideMissionResult"));
	if (MissionResultWidget)
	{
		MissionResultWidget->RemoveFromParent();
		MissionResultWidget = nullptr;
	}
}

// =============================================================================
// ShowFinalResult — 全ミッション終了（WBP_FinalResult）
// =============================================================================
void ATomatinaHUD::ShowFinalResult(int32 InTotalScore, int32 MissionCount)
{
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD::ShowFinalResult Total=%d Missions=%d"),
		InTotalScore, MissionCount);

	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !FinalResultWidgetClass) { return; }

	if (FinalResultWidget)
	{
		FinalResultWidget->RemoveFromParent();
		FinalResultWidget = nullptr;
	}

	FinalResultWidget = CreateWidget<UUserWidget>(PC, FinalResultWidgetClass);
	if (!FinalResultWidget) { return; }
	FinalResultWidget->AddToViewport(500);

	if (UTextBlock* TxtScore = Cast<UTextBlock>(
			FinalResultWidget->GetWidgetFromName(TEXT("TXT_FinalScore"))))
	{
		TxtScore->SetText(FText::AsNumber(InTotalScore));
	}

	// Rank 判定（平均点基準）
	const float Avg = (MissionCount > 0) ? static_cast<float>(InTotalScore) / MissionCount : 0.f;
	FString Rank;
	if      (Avg >= 80.f) { Rank = TEXT("S"); }
	else if (Avg >= 60.f) { Rank = TEXT("A"); }
	else if (Avg >= 40.f) { Rank = TEXT("B"); }
	else                  { Rank = TEXT("C"); }

	if (UTextBlock* TxtRank = Cast<UTextBlock>(
			FinalResultWidget->GetWidgetFromName(TEXT("TXT_Rank"))))
	{
		TxtRank->SetText(FText::FromString(Rank));
	}
}

// =============================================================================
// ShowMissionDisplay — お題テキスト・プレビュー画像
// =============================================================================
void ATomatinaHUD::ShowMissionDisplay(const FText& MissionText, UTexture2D* TargetImage)
{
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD::ShowMissionDisplay '%s'"),
		*MissionText.ToString());

	if (!MissionDisplayWidget) { return; }
	MissionDisplayWidget->SetVisibility(ESlateVisibility::Visible);

	if (UTextBlock* TxtMission = Cast<UTextBlock>(
			MissionDisplayWidget->GetWidgetFromName(TEXT("TXT_Mission"))))
	{
		TxtMission->SetText(MissionText);
	}

	if (TargetImage)
	{
		if (UImage* Preview = Cast<UImage>(
				MissionDisplayWidget->GetWidgetFromName(TEXT("IMG_TargetPreview"))))
		{
			Preview->SetBrushFromTexture(TargetImage);
		}
	}
}

void ATomatinaHUD::HideMissionDisplay()
{
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD::HideMissionDisplay"));
	if (MissionDisplayWidget)
	{
		MissionDisplayWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

// =============================================================================
// UpdateTimer — MissionDisplay の TXT_Timer を更新
// =============================================================================
void ATomatinaHUD::UpdateTimer(float RemainingSeconds)
{
	if (!MissionDisplayWidget) { return; }
	if (UTextBlock* Txt = Cast<UTextBlock>(
			MissionDisplayWidget->GetWidgetFromName(TEXT("TXT_Timer"))))
	{
		Txt->SetText(FText::FromString(
			FString::Printf(TEXT("残り %.1f 秒"), RemainingSeconds)));
	}
}

// =============================================================================
// UpdateTotalScore — MissionDisplay の TXT_TotalScore を更新
// =============================================================================
void ATomatinaHUD::UpdateTotalScore(int32 InTotalScore)
{
	if (!MissionDisplayWidget) { return; }
	if (UTextBlock* Txt = Cast<UTextBlock>(
			MissionDisplayWidget->GetWidgetFromName(TEXT("TXT_TotalScore"))))
	{
		Txt->SetText(FText::FromString(
			FString::Printf(TEXT("Score: %d"), InTotalScore)));
	}
}

// =============================================================================
// ShowCountdown / HideCountdown — WBP_CountdownDisplay
// =============================================================================
void ATomatinaHUD::ShowCountdown(int32 Seconds)
{
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD::ShowCountdown %d"), Seconds);

	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !CountdownWidgetClass) { return; }

	if (!CountdownWidget)
	{
		CountdownWidget = CreateWidget<UUserWidget>(PC, CountdownWidgetClass);
		if (CountdownWidget) { CountdownWidget->AddToViewport(400); }
	}
	if (!CountdownWidget) { return; }

	CountdownWidget->SetVisibility(ESlateVisibility::Visible);
	if (UTextBlock* Txt = Cast<UTextBlock>(
			CountdownWidget->GetWidgetFromName(TEXT("TXT_Countdown"))))
	{
		Txt->SetText(FText::AsNumber(Seconds));
	}
}

void ATomatinaHUD::HideCountdown()
{
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD::HideCountdown"));
	if (CountdownWidget)
	{
		CountdownWidget->RemoveFromParent();
		CountdownWidget = nullptr;
	}
}

// =============================================================================
// PlayShutterFlash — WBP_ShutterFlash を 0.1 秒だけ表示
// =============================================================================
void ATomatinaHUD::PlayShutterFlash()
{
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD::PlayShutterFlash"));

	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !ShutterFlashWidgetClass) { return; }

	if (ShutterFlashWidget)
	{
		ShutterFlashWidget->RemoveFromParent();
		ShutterFlashWidget = nullptr;
	}

	ShutterFlashWidget = CreateWidget<UUserWidget>(PC, ShutterFlashWidgetClass);
	if (!ShutterFlashWidget) { return; }
	ShutterFlashWidget->AddToViewport(999);

	bFlashActive = true;
	FlashElapsed = 0.f;
}

// =============================================================================
// UpdateDirtDisplay（例外2）
// SplatContainer 内に汚れ UImage を動的生成。メイン側・iPhone 側の 2 箇所に配置。
// =============================================================================
void ATomatinaHUD::UpdateDirtDisplay(const TArray<FDirtSplat>& Dirts)
{
	if (!DirtOverlayWidget) { return; }

	UCanvasPanel* Container = Cast<UCanvasPanel>(
		DirtOverlayWidget->GetWidgetFromName(TEXT("SplatContainer")));
	if (!Container) { return; }

	Container->ClearChildren();

	// メインモニター側（領域 [0, MainWidth] × [0, MainHeight]）
	AddDirtSplatsToCanvas(DirtOverlayWidget, Container, Dirts,
		MainWidth, MainHeight, 0.f, 0.f);

	// iPhone 側（領域 [MainWidth, MainWidth+PhoneWidth] × [0, PhoneHeight]）
	AddDirtSplatsToCanvas(DirtOverlayWidget, Container, Dirts,
		PhoneWidth, PhoneHeight, MainWidth, 0.f);
}

// =============================================================================
// AddDirtSplatsToCanvas — 汚れ UImage を動的生成する共通ヘルパー
// =============================================================================
void ATomatinaHUD::AddDirtSplatsToCanvas(
	UUserWidget* OwnerWidget,
	UCanvasPanel* Container,
	const TArray<FDirtSplat>& Dirts,
	float AreaWidth,
	float AreaHeight,
	float OriginX,
	float OriginY)
{
	if (!OwnerWidget || !Container) { return; }

	for (const FDirtSplat& Dirt : Dirts)
	{
		if (!Dirt.bActive) { continue; }

		UImage* Img = NewObject<UImage>(OwnerWidget);

		// TextureIndex で DirtTextures から画像を選択。範囲外なら DirtTexture にフォールバック
		UTexture2D* UseTex = nullptr;
		if (DirtTextures.IsValidIndex(Dirt.TextureIndex))
		{
			UseTex = DirtTextures[Dirt.TextureIndex];
		}
		if (!UseTex) { UseTex = DirtTexture; }
		if (UseTex) { Img->SetBrushFromTexture(UseTex); }

		Img->SetRenderOpacity(Dirt.Opacity);

		UCanvasPanelSlot* Slot = Container->AddChildToCanvas(Img);
		if (!Slot) { continue; }

		// Size は正規化スケール（0〜1）想定。1.0 を超える異常値は領域幅にクランプ
		const float ClampedNormSize = FMath::Clamp(Dirt.Size, 0.01f, 1.0f);
		const float Size     = ClampedNormSize * AreaWidth;
		const float HalfSize = Size * 0.5f;

		// 中心座標を領域内に計算 → 汚れ全体が領域＋内側マージンに収まるようクランプ
		float CenterX = Dirt.NormalizedPosition.X * AreaWidth;
		float CenterY = Dirt.NormalizedPosition.Y * AreaHeight;

		const float MinX = HalfSize + DirtInnerMargin;
		const float MaxX = AreaWidth  - HalfSize - DirtInnerMargin;
		const float MinY = HalfSize + DirtInnerMargin;
		const float MaxY = AreaHeight - HalfSize - DirtInnerMargin;

		// 汚れが領域より大きい場合はクランプできないので中央に寄せる
		CenterX = (MinX <= MaxX) ? FMath::Clamp(CenterX, MinX, MaxX) : AreaWidth  * 0.5f;
		CenterY = (MinY <= MaxY) ? FMath::Clamp(CenterY, MinY, MaxY) : AreaHeight * 0.5f;

		Slot->SetPosition(FVector2D(
			OriginX + CenterX - HalfSize,
			OriginY + CenterY - HalfSize));
		Slot->SetSize(FVector2D(Size, Size));
	}
}

// =============================================================================
// UpdateTowelStatus — DirtOverlay 内の PB_TowelStamina / TXT_SwapMessage を更新
// =============================================================================
void ATomatinaHUD::UpdateTowelStatus(float DurabilityPercent, bool bSwapping)
{
	if (!DirtOverlayWidget) { return; }

	if (UProgressBar* Bar = Cast<UProgressBar>(
			DirtOverlayWidget->GetWidgetFromName(TEXT("PB_TowelStamina"))))
	{
		Bar->SetPercent(DurabilityPercent);
	}

	if (UTextBlock* Txt = Cast<UTextBlock>(
			DirtOverlayWidget->GetWidgetFromName(TEXT("TXT_SwapMessage"))))
	{
		Txt->SetVisibility(bSwapping ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}
