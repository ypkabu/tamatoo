// Fill out your copyright notice in the Description page of Project Settings.

#include "TomatinaHUD.h"

#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "TomatinaPlayerPawn.h"

// ─────────────────────────────────────────────────────────────────────────────
ATomatinaHUD::ATomatinaHUD()
	: ViewFinderWidget(nullptr)
	, CursorWidget(nullptr)
	, DirtOverlayWidget(nullptr)
	, ResultWidget(nullptr)
	, MissionWidget(nullptr)
	, MissionResultWidget(nullptr)
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

	// DirtOverlayWidget は CachedDirts を Binding で参照し、
	// 各要素の NormalizedPosition・Opacity・Size に基づいてスプラットを描画すること。

	UE_LOG(LogTemp, Log, TEXT("ATomatinaHUD::UpdateDirtDisplay: 汚れ数=%d"), CachedDirts.Num());
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

	// 右上に配置（MainWidth - 500, 30）、サイズ 450×100
	const FVector2D Pos(MainWidth - 500.f, 30.f);
	const FVector2D Size(450.f, 100.f);
	MissionWidget->SetPositionInViewport(Pos, /*bRemoveDPIScale=*/false);
	MissionWidget->SetDesiredSizeInViewport(Size);
	MissionWidget->SetVisibility(ESlateVisibility::Visible);

	// IMG_TargetPreview にプレビュー画像をセット
	UImage* PreviewImg = Cast<UImage>(
		MissionWidget->GetWidgetFromName(TEXT("IMG_TargetPreview")));
	if (PreviewImg && TargetImage)
	{
		PreviewImg->SetBrushFromTexture(TargetImage);
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
