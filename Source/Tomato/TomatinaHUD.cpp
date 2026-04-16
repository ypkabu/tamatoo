// Fill out your copyright notice in the Description page of Project Settings.

#include "TomatinaHUD.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "TomatinaPlayerPawn.h"

// ─────────────────────────────────────────────────────────────────────────────
ATomatinaHUD::ATomatinaHUD()
	: ViewFinderWidget(nullptr)
	, CursorWidget(nullptr)
	, DirtOverlayWidget(nullptr)
	, ResultWidget(nullptr)
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
			UE_LOG(LogTemp, Warning,
				TEXT("ATomatinaHUD::BeginPlay: サイズ取得 Main=(%.0fx%.0f) Phone=(%.0fx%.0f)"),
				MainWidth, MainHeight, PhoneWidth, PhoneHeight);
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

	// ResultWidget は ShowResult() で動的生成するため BeginPlay では作らない
}

// ─────────────────────────────────────────────────────────────────────────────
void ATomatinaHUD::UpdateCursorPosition(FVector2D Pos)
{
	CurrentCursorPosition = Pos;

	// CursorWidget 全体をビューポート上の Pos へ移動する。
	// Widget 内部の Image を動かしたい場合は Blueprint 側で
	// CurrentCursorPosition を Binding して処理すること。
	if (CursorWidget)
	{
		CursorWidget->SetPositionInViewport(Pos, /*bRemoveDPIScale=*/false);
	}
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

	// 正規化座標 → 実座標の変換例（DirtOverlayWidget の Blueprint で使用）:
	//   メイン画面座標 = NormalizedPosition * FVector2D(MainWidth, MainHeight)
	//   Phone 画面座標 = NormalizedPosition * FVector2D(PhoneWidth, PhoneHeight)
	//     + PhoneOrigin（Phone 表示左上のオフセット）
	//
	// DirtOverlayWidget は CachedDirts を Binding で参照し、
	// 各要素の NormalizedPosition・Opacity・Size に基づいてスプラットを描画すること。

	UE_LOG(LogTemp, Log, TEXT("ATomatinaHUD::UpdateDirtDisplay: 汚れ数=%d"), CachedDirts.Num());
}
