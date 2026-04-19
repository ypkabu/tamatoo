// Fill out your copyright notice in the Description page of Project Settings.

#include "TomatinaHUD.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

#include "TomatinaPlayerPawn.h"
#include "TomatoDirtManager.h"

// =============================================================================
// コンストラクタ
// =============================================================================

ATomatinaHUD::ATomatinaHUD()
	: DirtTexture(nullptr)
	, ViewFinderWidget(nullptr)
	, CursorWidget(nullptr)
	, DirtOverlayWidget(nullptr)
	, ResultWidget(nullptr)
	, MissionWidget(nullptr)
	, MissionResultWidget(nullptr)
	, FinalResultWidget(nullptr)
	, FlashWidget(nullptr)
	, TestPipWidget(nullptr)
	, CachedDirtManager(nullptr)
{
	PrimaryActorTick.bCanEverTick = true;
}

// =============================================================================
// ApplyZoomMaterial
// =============================================================================

void ATomatinaHUD::ApplyZoomMaterial(UUserWidget* Widget)
{
	if (!Widget || !ZoomDisplayMaterial) { return; }

	UImage* Img = Cast<UImage>(Widget->GetWidgetFromName(TEXT("IMG_ZoomView")));
	if (Img)
	{
		Img->SetBrushFromMaterial(ZoomDisplayMaterial);
	}
}

// =============================================================================
// BeginPlay
// =============================================================================

void ATomatinaHUD::BeginPlay()
{
	Super::BeginPlay();

	// PlayerPawn から画面サイズ・bTestMode を取得
	bool bTestMode = true;
	if (APlayerController* PC = GetOwningPlayerController())
	{
		if (ATomatinaPlayerPawn* Pawn = Cast<ATomatinaPlayerPawn>(PC->GetPawn()))
		{
			MainWidth   = Pawn->MainWidth;
			MainHeight  = Pawn->MainHeight;
			PhoneWidth  = Pawn->PhoneWidth;
			PhoneHeight = Pawn->PhoneHeight;
			bTestMode   = Pawn->bTestMode;
		}
	}

	APlayerController* PC = GetOwningPlayerController();
	if (!PC) { return; }

	// ── ViewFinder ──────────────────────────────────────────────────────
	if (ViewFinderWidgetClass)
	{
		ViewFinderWidget = CreateWidget<UUserWidget>(PC, ViewFinderWidgetClass);
		if (ViewFinderWidget)
		{
			ViewFinderWidget->AddToViewport(100);
			ViewFinderWidget->SetVisibility(ESlateVisibility::Visible);
			ApplyZoomMaterial(ViewFinderWidget);
		}
	}

	// ── DirtOverlay ─────────────────────────────────────────────────────
	if (DirtOverlayWidgetClass)
	{
		DirtOverlayWidget = CreateWidget<UUserWidget>(PC, DirtOverlayWidgetClass);
		if (DirtOverlayWidget)
		{
			DirtOverlayWidget->AddToViewport(150);
			DirtOverlayWidget->SetVisibility(ESlateVisibility::Visible);
		}
	}

	// ── Cursor（初期は Hidden） ─────────────────────────────────────────
	if (CursorWidgetClass)
	{
		CursorWidget = CreateWidget<UUserWidget>(PC, CursorWidgetClass);
		if (CursorWidget)
		{
			CursorWidget->AddToViewport(200);
			CursorWidget->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	// ── Mission（常設表示） ─────────────────────────────────────────────
	if (MissionWidgetClass)
	{
		MissionWidget = CreateWidget<UUserWidget>(PC, MissionWidgetClass);
		if (MissionWidget)
		{
			MissionWidget->AddToViewport(250);
			MissionWidget->SetVisibility(ESlateVisibility::Visible);
		}
	}

	// ── FinalResult（最終リザルト、初期 Hidden） ────────────────────────
	if (FinalResultWidgetClass)
	{
		FinalResultWidget = CreateWidget<UUserWidget>(PC, FinalResultWidgetClass);
		if (FinalResultWidget)
		{
			FinalResultWidget->AddToViewport(350);
			FinalResultWidget->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	// ── TestPip（bTestMode=true のときのみ表示） ────────────────────────
	if (TestPipWidgetClass)
	{
		TestPipWidget = CreateWidget<UUserWidget>(PC, TestPipWidgetClass);
		if (TestPipWidget)
		{
			TestPipWidget->AddToViewport(50);
			TestPipWidget->SetVisibility(bTestMode
				? ESlateVisibility::Visible
				: ESlateVisibility::Hidden);
			ApplyZoomMaterial(TestPipWidget);
		}
	}

	UE_LOG(LogTemp, Warning,
		TEXT("ATomatinaHUD: HUD BeginPlay, Widgets created (bTestMode=%d)"),
		bTestMode ? 1 : 0);
}

// =============================================================================
// Tick
// =============================================================================

void ATomatinaHUD::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 汚れ Widget を DirtManager と同期
	if (ATomatoDirtManager* Mgr = GetDirtManager())
	{
		UpdateDirtDisplay(Mgr->GetActiveDirts());
	}
}

// =============================================================================
// カーソル
// =============================================================================

void ATomatinaHUD::UpdateCursorPosition(FVector2D Pos)
{
	CurrentCursorPosition = Pos;

	if (!CursorWidget) { return; }

	// CursorWidget は画面全体を覆う Canvas Panel を持ち、
	// その中の IMG_Crosshair を Pos に移動させる。
	UImage* Crosshair = Cast<UImage>(
		CursorWidget->GetWidgetFromName(TEXT("IMG_Crosshair")));
	if (!Crosshair) { return; }

	if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Crosshair->Slot))
	{
		Slot->SetPosition(Pos);
	}
}

void ATomatinaHUD::ShowCursor()
{
	if (CursorWidget)
	{
		CursorWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void ATomatinaHUD::HideCursor()
{
	if (CursorWidget)
	{
		CursorWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

// =============================================================================
// リザルト
// =============================================================================

void ATomatinaHUD::ShowResult(int32 Score, const FString& Comment)
{
	LastScore   = Score;
	LastComment = Comment;

	UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD: ShowResult %d"), Score);

	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !ResultWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ShowResult: PC または ResultWidgetClass が null"));
		return;
	}

	if (ResultWidget)
	{
		ResultWidget->RemoveFromParent();
		ResultWidget = nullptr;
	}

	ResultWidget = CreateWidget<UUserWidget>(PC, ResultWidgetClass);
	if (!ResultWidget) { return; }

	ResultWidget->AddToViewport(300);

	// IMG_Photo へマテリアル適用
	if (PhotoDisplayMaterial)
	{
		if (UImage* PhotoImg = Cast<UImage>(
				ResultWidget->GetWidgetFromName(TEXT("IMG_Photo"))))
		{
			PhotoImg->SetBrushFromMaterial(PhotoDisplayMaterial);
		}
	}

	// TXT_Score
	if (UTextBlock* TxtScore = Cast<UTextBlock>(
			ResultWidget->GetWidgetFromName(TEXT("TXT_Score"))))
	{
		TxtScore->SetText(FText::AsNumber(Score));
	}

	// TXT_Comment
	if (UTextBlock* TxtComment = Cast<UTextBlock>(
			ResultWidget->GetWidgetFromName(TEXT("TXT_Comment"))))
	{
		TxtComment->SetText(FText::FromString(Comment));
	}
}

void ATomatinaHUD::HideResult()
{
	if (ResultWidget)
	{
		ResultWidget->RemoveFromParent();
		ResultWidget = nullptr;
	}
}

// =============================================================================
// ミッション
// =============================================================================

void ATomatinaHUD::ShowMissionDisplay(const FText& MissionText, UTexture2D* TargetImage)
{
	CurrentMissionText = MissionText;

	UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD: ShowMissionDisplay %s"),
		*MissionText.ToString());

	if (!MissionWidget) { return; }

	MissionWidget->SetVisibility(ESlateVisibility::Visible);

	if (UTextBlock* Txt = Cast<UTextBlock>(
			MissionWidget->GetWidgetFromName(TEXT("TXT_Mission"))))
	{
		Txt->SetText(MissionText);
	}

	if (TargetImage)
	{
		if (UImage* Img = Cast<UImage>(
				MissionWidget->GetWidgetFromName(TEXT("IMG_TargetPreview"))))
		{
			Img->SetBrushFromTexture(TargetImage);
		}
	}
}

void ATomatinaHUD::HideMissionText()
{
	if (MissionWidget)
	{
		MissionWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

void ATomatinaHUD::ShowMissionResult(int32 Score, const FString& Comment)
{
	MissionResultScore   = Score;
	MissionResultComment = Comment;

	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !MissionResultWidgetClass) { return; }

	if (!MissionResultWidget)
	{
		MissionResultWidget = CreateWidget<UUserWidget>(PC, MissionResultWidgetClass);
		if (MissionResultWidget)
		{
			MissionResultWidget->AddToViewport(275);
		}
	}

	if (!MissionResultWidget) { return; }
	MissionResultWidget->SetVisibility(ESlateVisibility::Visible);

	if (UTextBlock* TxtScore = Cast<UTextBlock>(
			MissionResultWidget->GetWidgetFromName(TEXT("TXT_Score"))))
	{
		TxtScore->SetText(FText::AsNumber(Score));
	}
	if (UTextBlock* TxtComment = Cast<UTextBlock>(
			MissionResultWidget->GetWidgetFromName(TEXT("TXT_Comment"))))
	{
		TxtComment->SetText(FText::FromString(Comment));
	}
}

void ATomatinaHUD::HideMissionResult()
{
	if (MissionResultWidget)
	{
		MissionResultWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

// =============================================================================
// タイマー／スコア
// =============================================================================

void ATomatinaHUD::UpdateTimer(float RemainingSeconds)
{
	if (!MissionWidget) { return; }

	if (UTextBlock* TxtTimer = Cast<UTextBlock>(
			MissionWidget->GetWidgetFromName(TEXT("TXT_Timer"))))
	{
		TxtTimer->SetText(FText::FromString(
			FString::Printf(TEXT("残り %.1f秒"), FMath::Max(0.f, RemainingSeconds))));
	}
}

void ATomatinaHUD::UpdateTotalScore(int32 TotalScore)
{
	CurrentTotalScore = TotalScore;

	if (!MissionWidget) { return; }

	if (UTextBlock* TxtTotal = Cast<UTextBlock>(
			MissionWidget->GetWidgetFromName(TEXT("TXT_TotalScore"))))
	{
		TxtTotal->SetText(FText::FromString(FString::FromInt(TotalScore)));
	}
}

// =============================================================================
// シャッターフラッシュ
// =============================================================================

void ATomatinaHUD::PlayShutterFlash()
{
	if (!FlashWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayShutterFlash: FlashWidgetClass 未設定"));
		return;
	}

	APlayerController* PC = GetOwningPlayerController();
	if (!PC) { return; }

	if (FlashWidget)
	{
		GetWorld()->GetTimerManager().ClearTimer(FlashTimerHandle);
		FlashWidget->RemoveFromParent();
		FlashWidget = nullptr;
	}

	FlashWidget = CreateWidget<UUserWidget>(PC, FlashWidgetClass);
	if (!FlashWidget) { return; }

	FlashAlpha = 1.0f;
	FlashWidget->SetRenderOpacity(FlashAlpha);
	FlashWidget->AddToViewport(500);

	GetWorld()->GetTimerManager().SetTimer(
		FlashTimerHandle,
		[this]()
		{
			FlashAlpha -= 0.1f;
			if (FlashWidget)
			{
				FlashWidget->SetRenderOpacity(FMath::Max(0.0f, FlashAlpha));
			}
			if (FlashAlpha <= 0.0f)
			{
				GetWorld()->GetTimerManager().ClearTimer(FlashTimerHandle);
				if (FlashWidget)
				{
					FlashWidget->RemoveFromParent();
					FlashWidget = nullptr;
				}
			}
		},
		0.02f, /*bLoop=*/true);
}

// =============================================================================
// 最終リザルト
// =============================================================================

void ATomatinaHUD::ShowFinalResult(int32 InTotalScore, int32 MissionCount)
{
	if (!FinalResultWidget) { return; }

	FinalResultWidget->SetVisibility(ESlateVisibility::Visible);

	if (UTextBlock* TxtScore = Cast<UTextBlock>(
			FinalResultWidget->GetWidgetFromName(TEXT("TXT_FinalScore"))))
	{
		TxtScore->SetText(FText::FromString(
			FString::Printf(TEXT("合計スコア: %d"), InTotalScore)));
	}

	const float Avg = (MissionCount > 0)
		? static_cast<float>(InTotalScore) / static_cast<float>(MissionCount)
		: 0.f;

	FString Rank;
	if      (Avg >= 90.f) Rank = TEXT("S");
	else if (Avg >= 75.f) Rank = TEXT("A");
	else if (Avg >= 50.f) Rank = TEXT("B");
	else                  Rank = TEXT("C");

	if (UTextBlock* TxtRank = Cast<UTextBlock>(
			FinalResultWidget->GetWidgetFromName(TEXT("TXT_Rank"))))
	{
		TxtRank->SetText(FText::FromString(Rank));
	}

	UE_LOG(LogTemp, Warning,
		TEXT("ATomatinaHUD::ShowFinalResult: Total=%d Avg=%.1f Rank=%s"),
		InTotalScore, Avg, *Rank);
}

// =============================================================================
// 汚れ
// =============================================================================

void ATomatinaHUD::UpdateDirtDisplay(const TArray<FDirtSplat>& Dirts)
{
	CachedDirts = Dirts;

	if (!DirtOverlayWidget) { return; }

	UCanvasPanel* CanvasPanel = Cast<UCanvasPanel>(
		DirtOverlayWidget->GetWidgetFromName(TEXT("DirtCanvas")));
	if (!CanvasPanel)
	{
		// 後方互換：CanvasPanel_0 も見る
		CanvasPanel = Cast<UCanvasPanel>(
			DirtOverlayWidget->GetWidgetFromName(TEXT("CanvasPanel_0")));
	}
	if (!CanvasPanel) { return; }

	CanvasPanel->ClearChildren();

	for (const FDirtSplat& Dirt : Dirts)
	{
		if (!Dirt.bActive) { continue; }

		// メインモニター側
		UImage* MainDirt = NewObject<UImage>(DirtOverlayWidget);
		if (DirtTexture) { MainDirt->SetBrushFromTexture(DirtTexture); }
		MainDirt->SetRenderOpacity(Dirt.Opacity);
		if (UCanvasPanelSlot* S = CanvasPanel->AddChildToCanvas(MainDirt))
		{
			S->SetPosition(FVector2D(
				Dirt.NormalizedPosition.X * MainWidth,
				Dirt.NormalizedPosition.Y * MainHeight));
			S->SetSize(FVector2D(Dirt.Size * MainWidth, Dirt.Size * MainWidth));
		}

		// iPhone 側（メインモニター右に配置）
		UImage* PhoneDirt = NewObject<UImage>(DirtOverlayWidget);
		if (DirtTexture) { PhoneDirt->SetBrushFromTexture(DirtTexture); }
		PhoneDirt->SetRenderOpacity(Dirt.Opacity);
		if (UCanvasPanelSlot* S = CanvasPanel->AddChildToCanvas(PhoneDirt))
		{
			S->SetPosition(FVector2D(
				MainWidth + Dirt.NormalizedPosition.X * PhoneWidth,
				Dirt.NormalizedPosition.Y * PhoneHeight));
			S->SetSize(FVector2D(Dirt.Size * PhoneWidth, Dirt.Size * PhoneWidth));
		}
	}
}

// =============================================================================
// タオル
// =============================================================================

void ATomatinaHUD::UpdateTowelStatus(float DurabilityPercent, bool bSwapping)
{
	if (!DirtOverlayWidget) { return; }

	if (UProgressBar* PB = Cast<UProgressBar>(
			DirtOverlayWidget->GetWidgetFromName(TEXT("PB_TowelStamina"))))
	{
		PB->SetPercent(DurabilityPercent);
		PB->SetFillColorAndOpacity(bSwapping ? FLinearColor::Red : FLinearColor::Green);
	}

	if (UTextBlock* TxtSwap = Cast<UTextBlock>(
			DirtOverlayWidget->GetWidgetFromName(TEXT("TXT_SwapMessage"))))
	{
		if (bSwapping)
		{
			TxtSwap->SetText(FText::FromString(TEXT("タオル交換中...")));
			TxtSwap->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			TxtSwap->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

// =============================================================================
// DirtManager ヘルパー
// =============================================================================

ATomatoDirtManager* ATomatinaHUD::GetDirtManager()
{
	if (!CachedDirtManager)
	{
		if (UWorld* World = GetWorld())
		{
			CachedDirtManager = Cast<ATomatoDirtManager>(
				UGameplayStatics::GetActorOfClass(World, ATomatoDirtManager::StaticClass()));
		}
	}
	return CachedDirtManager;
}
