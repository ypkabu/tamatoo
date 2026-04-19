// Fill out your copyright notice in the Description page of Project Settings.

#include "TomatinaHUD.h"

#include "Blueprint/UserWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"
#include "TomatinaPlayerPawn.h"

// ─────────────────────────────────────────────────────────────────────────────
ATomatinaHUD::ATomatinaHUD()
	: ViewFinderWidget(nullptr)
	, CursorWidget(nullptr)
	, DirtOverlayWidget(nullptr)
	, ResultWidget(nullptr)
	, MissionWidget(nullptr)
	, MissionResultWidget(nullptr)
	, TestPipWidget(nullptr)
{
}

// ─────────────────────────────────────────────────────────────────────────────
void ATomatinaHUD::BeginPlay()
{
	Super::BeginPlay();

	// ── PlayerPawn から画面サイズを取得 ──────────────────────────────────
	if (APlayerController* PC = GetOwningPlayerController())
	{
		if (ATomatinaPlayerPawn* Pawn = Cast<ATomatinaPlayerPawn>(PC->GetPawn()))
		{
			MainWidth   = Pawn->MainWidth;
			MainHeight  = Pawn->MainHeight;
			PhoneWidth  = Pawn->PhoneWidth;
			PhoneHeight = Pawn->PhoneHeight;
			bTestMode   = Pawn->bTestMode;
			UE_LOG(LogTemp, Warning,
				TEXT("ATomatinaHUD::BeginPlay: サイズ取得 Main=(%.0fx%.0f) Phone=(%.0fx%.0f) bTestMode=%d"),
				MainWidth, MainHeight, PhoneWidth, PhoneHeight, bTestMode);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD::BeginPlay: ATomatinaPlayerPawn へのキャストに失敗（デフォルト値を使用）"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD::BeginPlay: PlayerController の取得に失敗"));
	}

	APlayerController* PC = GetOwningPlayerController();
	if (!PC) { return; }

	// ── ViewFinder Widget ────────────────────────────────────────────────
	if (ViewFinderWidgetClass)
	{
		ViewFinderWidget = CreateWidget<UUserWidget>(PC, ViewFinderWidgetClass);
		if (ViewFinderWidget)
		{
			ViewFinderWidget->AddToViewport(100);
			UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD: ViewFinderWidget を生成しました"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD: ViewFinderWidgetClass が未設定"));
	}

	// ── DirtOverlay Widget ───────────────────────────────────────────────
	if (DirtOverlayWidgetClass)
	{
		DirtOverlayWidget = CreateWidget<UUserWidget>(PC, DirtOverlayWidgetClass);
		if (DirtOverlayWidget)
		{
			DirtOverlayWidget->AddToViewport(150);
			UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD: DirtOverlayWidget を生成しました"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD: DirtOverlayWidgetClass が未設定"));
	}

	// ── Cursor Widget ────────────────────────────────────────────────────
	if (CursorWidgetClass)
	{
		CursorWidget = CreateWidget<UUserWidget>(PC, CursorWidgetClass);
		if (CursorWidget)
		{
			CursorWidget->AddToViewport(200);
			CursorWidget->SetVisibility(ESlateVisibility::Hidden);
			UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD: CursorWidget を生成しました"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD: CursorWidgetClass が未設定"));
	}

	// ── TestPip Widget （bTestMode のときのみ）────────────────────────────
	if (bTestMode && TestPipWidgetClass)
	{
		TestPipWidget = CreateWidget<UUserWidget>(PC, TestPipWidgetClass);
		if (TestPipWidget)
		{
			TestPipWidget->AddToViewport(100);
			TestPipWidget->SetVisibility(ESlateVisibility::Visible);
			UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD: TestPipWidget 生成成功"));

			UImage* ZoomImg = Cast<UImage>(
				TestPipWidget->GetWidgetFromName(TEXT("IMG_ZoomView")));
			if (ZoomImg && ZoomDisplayMaterial)
			{
				ZoomImg->SetBrushFromMaterial(ZoomDisplayMaterial);
				UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD: TestPip ZoomMaterial 設定完了"));
			}
			else
			{
				UE_LOG(LogTemp, Error,
					TEXT("ATomatinaHUD: TestPip IMG_ZoomView=%s Material=%s"),
					ZoomImg            ? TEXT("OK") : TEXT("NULL"),
					ZoomDisplayMaterial ? TEXT("OK") : TEXT("NULL"));
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error,
			TEXT("ATomatinaHUD: TestPip 生成失敗 bTestMode=%d Class=%s"),
			bTestMode,
			TestPipWidgetClass ? TEXT("OK") : TEXT("NULL"));
	}

	// ResultWidget は ShowResult() で動的生成するため BeginPlay では作らない
}

// ─────────────────────────────────────────────────────────────────────────────
void ATomatinaHUD::UpdateCursorPosition(FVector2D Pos)
{
	CurrentCursorPosition = Pos;

	if (!CursorWidget) return;

	UImage* Crosshair = Cast<UImage>(CursorWidget->GetWidgetFromName(TEXT("IMG_Crosshair")));
	if (!Crosshair) return;

	UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Crosshair->Slot);
	if (!Slot) return;

	const float NormX  = (MainWidth  > 0.f) ? Pos.X / MainWidth  : 0.f;
	const float NormY  = (MainHeight > 0.f) ? Pos.Y / MainHeight : 0.f;
	const float PhoneX = MainWidth + NormX * PhoneWidth;
	const float PhoneY = NormY * PhoneHeight;
	Slot->SetPosition(FVector2D(PhoneX, PhoneY));
}

// ─────────────────────────────────────────────────────────────────────────────
void ATomatinaHUD::ShowCursor()
{
	if (!CursorWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD::ShowCursor: CursorWidget が null"));
		return;
	}
	CursorWidget->SetVisibility(ESlateVisibility::Visible);
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD::ShowCursor: カーソル表示"));
}

// ─────────────────────────────────────────────────────────────────────────────
void ATomatinaHUD::HideCursor()
{
	if (!CursorWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD::HideCursor: CursorWidget が null"));
		return;
	}
	CursorWidget->SetVisibility(ESlateVisibility::Hidden);
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD::HideCursor: カーソル非表示"));
}

// ─────────────────────────────────────────────────────────────────────────────
void ATomatinaHUD::ShowResult(int32 Score, const FString& Comment)
{
	LastScore   = Score;
	LastComment = Comment;

	if (!ResultWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD::ShowResult: ResultWidgetClass が未設定"));
		return;
	}

	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD::ShowResult: PlayerController が null"));
		return;
	}

	// 既存リザルトが残っていれば先に破棄
	if (ResultWidget)
	{
		ResultWidget->RemoveFromParent();
		ResultWidget = nullptr;
	}

	ResultWidget = CreateWidget<UUserWidget>(PC, ResultWidgetClass);
	if (!ResultWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD::ShowResult: ResultWidget の生成に失敗"));
		return;
	}

	// 位置・サイズをメイン画面全体に合わせる
	ResultWidget->SetPositionInViewport(FVector2D::ZeroVector, /*bRemoveDPIScale=*/false);
	ResultWidget->SetDesiredSizeInViewport(FVector2D(MainWidth, MainHeight));

	ResultWidget->AddToViewport(300);

	// Score / Comment の反映は UMG の Binding（LastScore / LastComment）で行う。
	// IMG_Photo への M_PhotoDisplay セット、TXT_Score / TXT_Comment の更新は
	// Widget Blueprint の Event Construct / Binding で実装すること。

	UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD::ShowResult: Score=%d Comment=%s"), Score, *Comment);
}

// ─────────────────────────────────────────────────────────────────────────────
void ATomatinaHUD::HideResult()
{
	if (!ResultWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD::HideResult: ResultWidget が null"));
		return;
	}
	ResultWidget->RemoveFromParent();
	ResultWidget = nullptr;
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD::HideResult: リザルト破棄"));
}

// ─────────────────────────────────────────────────────────────────────────────
void ATomatinaHUD::UpdateDirtDisplay(const TArray<FDirtSplat>& Dirts)
{
	CachedDirts = Dirts;

	if (!DirtOverlayWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("UpdateDirtDisplay: DirtOverlayWidget NULL"));
		return;
	}

	UCanvasPanel* SplatContainer = Cast<UCanvasPanel>(
		DirtOverlayWidget->GetWidgetFromName(TEXT("SplatContainer")));
	if (!SplatContainer)
	{
		UE_LOG(LogTemp, Error, TEXT("UpdateDirtDisplay: SplatContainer NULL"));
		return;
	}

	SplatContainer->ClearChildren();

	int32 ActiveCount = 0;
	for (const FDirtSplat& Dirt : Dirts)
	{
		if (!Dirt.bActive) { continue; }
		++ActiveCount;

		// ── メイン画面側 ──────────────────────────────────────────────────
		UImage* MainImg = NewObject<UImage>(DirtOverlayWidget);
		if (DirtTexture)
		{
			MainImg->SetBrushFromTexture(DirtTexture);
		}
		else
		{
			FSlateBrush Brush;
			Brush.TintColor = FSlateColor(FLinearColor::Red);
			Brush.DrawAs   = ESlateBrushDrawType::Image;
			MainImg->SetBrush(Brush);
		}
		MainImg->SetRenderOpacity(Dirt.Opacity);

		UCanvasPanelSlot* MainSlot = SplatContainer->AddChildToCanvas(MainImg);
		if (MainSlot)
		{
			const float MainSize = Dirt.Size * MainWidth;
			MainSlot->SetPosition(FVector2D(
				Dirt.NormalizedPosition.X * MainWidth  - MainSize * 0.5f,
				Dirt.NormalizedPosition.Y * MainHeight - MainSize * 0.5f));
			MainSlot->SetSize(FVector2D(MainSize, MainSize));
		}

		// ── iPhone 画面側 ──────────────────────────────────────────────────
		UImage* PhoneImg = NewObject<UImage>(DirtOverlayWidget);
		if (DirtTexture)
		{
			PhoneImg->SetBrushFromTexture(DirtTexture);
		}
		else
		{
			FSlateBrush Brush;
			Brush.TintColor = FSlateColor(FLinearColor::Red);
			Brush.DrawAs   = ESlateBrushDrawType::Image;
			PhoneImg->SetBrush(Brush);
		}
		PhoneImg->SetRenderOpacity(Dirt.Opacity);

		UCanvasPanelSlot* PhoneSlot = SplatContainer->AddChildToCanvas(PhoneImg);
		if (PhoneSlot)
		{
			const float PhoneSize = Dirt.Size * PhoneWidth;
			PhoneSlot->SetPosition(FVector2D(
				MainWidth + Dirt.NormalizedPosition.X * PhoneWidth  - PhoneSize * 0.5f,
				Dirt.NormalizedPosition.Y * PhoneHeight             - PhoneSize * 0.5f));
			PhoneSlot->SetSize(FVector2D(PhoneSize, PhoneSize));
		}
	}

	if (ActiveCount > 0)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UpdateDirtDisplay: %d 個の汚れを描画"), ActiveCount);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
void ATomatinaHUD::ShowMissionDisplay(const FText& MissionText, UTexture2D* TargetImage)
{
	CurrentMissionText = MissionText;

	APlayerController* PC = GetOwningPlayerController();
	if (!PC) { return; }

	if (!MissionWidgetClass)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("ATomatinaHUD::ShowMissionDisplay: MissionWidgetClass が未設定"));
		return;
	}

	// Widget がなければ生成
	if (!MissionWidget)
	{
		MissionWidget = CreateWidget<UUserWidget>(PC, MissionWidgetClass);
		if (!MissionWidget)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("ATomatinaHUD::ShowMissionDisplay: Widget 生成に失敗"));
			return;
		}

		MissionWidget->AddToViewport(250);
	}

	MissionWidget->SetVisibility(ESlateVisibility::Visible);

	// TXT_Mission にお題テキストをセット
	UTextBlock* TxtMission = Cast<UTextBlock>(
		MissionWidget->GetWidgetFromName(TEXT("TXT_Mission")));
	if (TxtMission)
	{
		TxtMission->SetText(MissionText);
		UE_LOG(LogTemp, Warning,
			TEXT("ATomatinaHUD: TXT_Mission 更新 '%s'"), *MissionText.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ATomatinaHUD: TXT_Mission が見つかりません"));
	}

	// IMG_TargetPreview にプレビュー画像をセット
	if (TargetImage)
	{
		UImage* PreviewImg = Cast<UImage>(
			MissionWidget->GetWidgetFromName(TEXT("IMG_TargetPreview")));
		if (PreviewImg)
		{
			PreviewImg->SetBrushFromTexture(TargetImage);
		}
	}

	UE_LOG(LogTemp, Log,
		TEXT("ATomatinaHUD::ShowMissionDisplay: '%s' Image=%s"),
		*MissionText.ToString(),
		TargetImage ? *TargetImage->GetName() : TEXT("none"));
}

// ─────────────────────────────────────────────────────────────────────────────
void ATomatinaHUD::ShowMissionText(const FText& Text)
{
	ShowMissionDisplay(Text, nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────
void ATomatinaHUD::HideMissionText()
{
	if (MissionWidget)
	{
		MissionWidget->SetVisibility(ESlateVisibility::Hidden);
		UE_LOG(LogTemp, Log, TEXT("ATomatinaHUD::HideMissionText: 非表示"));
	}
}

// ─────────────────────────────────────────────────────────────────────────────
void ATomatinaHUD::ShowMissionResult(int32 Score, const FString& Comment)
{
	MissionResultScore   = Score;
	MissionResultComment = Comment;

	APlayerController* PC = GetOwningPlayerController();
	if (!PC) { return; }

	if (!MissionResultWidgetClass)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("ATomatinaHUD::ShowMissionResult: MissionResultWidgetClass が未設定"));
		return;
	}

	// 既存インスタンスがあれば再利用
	if (!MissionResultWidget)
	{
		MissionResultWidget = CreateWidget<UUserWidget>(PC, MissionResultWidgetClass);
		if (MissionResultWidget) { MissionResultWidget->AddToViewport(275); }
	}

	if (MissionResultWidget)
	{
		MissionResultWidget->SetVisibility(ESlateVisibility::Visible);
		UE_LOG(LogTemp, Log,
			TEXT("ATomatinaHUD::ShowMissionResult: Score=%d Comment=%s"), Score, *Comment);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
void ATomatinaHUD::HideMissionResult()
{
	if (MissionResultWidget)
	{
		MissionResultWidget->SetVisibility(ESlateVisibility::Hidden);
		UE_LOG(LogTemp, Log, TEXT("ATomatinaHUD::HideMissionResult: 非表示"));
	}
}

// ─────────────────────────────────────────────────────────────────────────────
void ATomatinaHUD::PlayShutterFlash()
{
	if (!FlashWidgetClass) return;
	APlayerController* PC = GetOwningPlayerController();
	if (!PC) return;

	UUserWidget* Flash = CreateWidget<UUserWidget>(PC, FlashWidgetClass);
	if (!Flash) return;

	Flash->AddToViewport(999);

	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle,
		FTimerDelegate::CreateLambda([Flash]()
		{
			if (IsValid(Flash)) { Flash->RemoveFromParent(); }
		}),
		0.1f, false);
}

// ─────────────────────────────────────────────────────────────────────────────
void ATomatinaHUD::UpdateTimer(float RemainingSeconds)
{
	if (!MissionWidget) return;
	UTextBlock* Txt = Cast<UTextBlock>(MissionWidget->GetWidgetFromName(TEXT("TXT_Timer")));
	if (Txt)
	{
		FString Str = FString::Printf(TEXT("残り %.1f 秒"), RemainingSeconds);
		Txt->SetText(FText::FromString(Str));
	}
}

// ─────────────────────────────────────────────────────────────────────────────
void ATomatinaHUD::ShowCountdown(int32 Seconds)
{
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD::ShowCountdown: %d"), Seconds);
	if (!MissionWidget) return;
	UTextBlock* Txt = Cast<UTextBlock>(MissionWidget->GetWidgetFromName(TEXT("TXT_Timer")));
	if (Txt)
	{
		Txt->SetText(FText::AsNumber(Seconds));
		MissionWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
void ATomatinaHUD::HideCountdown()
{
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD::HideCountdown"));
	if (MissionWidget)
	{
		UTextBlock* Txt = Cast<UTextBlock>(MissionWidget->GetWidgetFromName(TEXT("TXT_Timer")));
		if (Txt) { Txt->SetText(FText::GetEmpty()); }
	}
}

// ─────────────────────────────────────────────────────────────────────────────
void ATomatinaHUD::UpdateTotalScore(int32 InTotalScore)
{
	CurrentTotalScore = InTotalScore;
	if (!MissionWidget) return;
	UTextBlock* Txt = Cast<UTextBlock>(MissionWidget->GetWidgetFromName(TEXT("TXT_TotalScore")));
	if (Txt)
	{
		FString Str = FString::Printf(TEXT("Score: %d"), CurrentTotalScore);
		Txt->SetText(FText::FromString(Str));
	}
}

// ─────────────────────────────────────────────────────────────────────────────
void ATomatinaHUD::UpdateTowelStatus(float DurabilityPercent, bool bSwapping)
{
	if (!DirtOverlayWidget) return;

	UProgressBar* PbTowel = Cast<UProgressBar>(
		DirtOverlayWidget->GetWidgetFromName(TEXT("PB_TowelStamina")));
	if (PbTowel) { PbTowel->SetPercent(DurabilityPercent); }

	UTextBlock* TxtSwap = Cast<UTextBlock>(
		DirtOverlayWidget->GetWidgetFromName(TEXT("TXT_SwapMessage")));
	if (TxtSwap)
	{
		TxtSwap->SetVisibility(bSwapping ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
void ATomatinaHUD::ShowFinalResult(int32 InTotalScore, int32 MissionCount)
{
	if (!FinalResultWidgetClass) return;
	APlayerController* PC = GetOwningPlayerController();
	if (!PC) return;

	UUserWidget* FinalWidget = CreateWidget<UUserWidget>(PC, FinalResultWidgetClass);
	if (!FinalWidget) return;

	FinalWidget->AddToViewport(500);

	UTextBlock* TxtScore = Cast<UTextBlock>(
		FinalWidget->GetWidgetFromName(TEXT("TXT_FinalScore")));
	if (TxtScore) { TxtScore->SetText(FText::AsNumber(InTotalScore)); }

	UTextBlock* TxtMissions = Cast<UTextBlock>(
		FinalWidget->GetWidgetFromName(TEXT("TXT_MissionCount")));
	if (TxtMissions) { TxtMissions->SetText(FText::AsNumber(MissionCount)); }

	UE_LOG(LogTemp, Warning,
		TEXT("ATomatinaHUD::ShowFinalResult: Score=%d Missions=%d"), InTotalScore, MissionCount);
}
