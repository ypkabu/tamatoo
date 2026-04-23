// Fill out your copyright notice in the Description page of Project Settings.

#include "TomatinaHUD.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Styling/SlateColor.h"
#include "Widgets/SWindow.h"

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
		MainWidth               = Pawn->MainWidth;
		MainHeight              = Pawn->MainHeight;
		PhoneWidth              = Pawn->PhoneWidth;
		PhoneHeight             = Pawn->PhoneHeight;
		bTestMode               = Pawn->bTestMode;
		bUseSeparatePhoneWindow = Pawn->bUseSeparatePhoneWindow;
		UE_LOG(LogTemp, Warning,
			TEXT("ATomatinaHUD: サイズ取得 Main=(%.0fx%.0f) Phone=(%.0fx%.0f) bTestMode=%d bUseSeparatePhoneWindow=%d"),
			MainWidth, MainHeight, PhoneWidth, PhoneHeight,
			bTestMode ? 1 : 0, bUseSeparatePhoneWindow ? 1 : 0);

		if (!Pawn->SceneCapture_Zoom)
		{
			UE_LOG(LogTemp, Error, TEXT("ATomatinaHUD: Pawn.SceneCapture_Zoom が null です"));
		}
		else if (!Pawn->SceneCapture_Zoom->TextureTarget)
		{
			UE_LOG(LogTemp, Error,
				TEXT("ATomatinaHUD: SceneCapture_Zoom の TextureTarget が未設定です（RT_Zoom を割り当ててください）"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD: PlayerPawn 取得失敗（デフォルト値使用）"));
	}

	// ── ViewFinder (旧・スパンウィンドウ方式のときだけメインに追加) ──
	if (!bUseSeparatePhoneWindow)
	{
		if (ViewFinderWidgetClass)
		{
			ViewFinderWidget = CreateWidget<UUserWidget>(PC, ViewFinderWidgetClass);
			if (ViewFinderWidget)
			{
				ViewFinderWidget->AddToViewport(100);
				BindZoomMaterialToWidget(ViewFinderWidget, TEXT("IMG_ZoomView"), TEXT("ViewFinder"));
				LayoutPhoneZoomImage(ViewFinderWidget, TEXT("IMG_ZoomView"), TEXT("ViewFinder"));
			}
		}
		else { UE_LOG(LogTemp, Error, TEXT("ATomatinaHUD: ViewFinderWidgetClass 未設定")); }
	}

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
			ValidateMissionStylishWidgets();
		}
	}
	else { UE_LOG(LogTemp, Error, TEXT("ATomatinaHUD: MissionDisplayWidgetClass 未設定")); }

	// ── TestPip（開発用・スパン方式のみ） ─────────────────────────────
	if (!bUseSeparatePhoneWindow && bTestMode && TestPipWidgetClass)
	{
		TestPipWidget = CreateWidget<UUserWidget>(PC, TestPipWidgetClass);
		if (TestPipWidget)
		{
			TestPipWidget->AddToViewport(110);
			TestPipWidget->SetVisibility(ESlateVisibility::Visible);
			BindZoomMaterialToWidget(TestPipWidget, TEXT("IMG_ZoomView"), TEXT("TestPip"));
			LayoutPhoneZoomImage(TestPipWidget, TEXT("IMG_ZoomView"), TEXT("TestPip"));
		}
	}

	// ── 第二ウィンドウ方式：スマホ側独立 SWindow を生成 ─────────────────
	if (bUseSeparatePhoneWindow)
	{
		CreatePhoneWindow();
	}

	UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD::BeginPlay 完了"));
}

// =============================================================================
// EndPlay — 第二ウィンドウのクリーンアップ
// =============================================================================
void ATomatinaHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroyPhoneWindow();
	Super::EndPlay(EndPlayReason);
}

// =============================================================================
// CreatePhoneWindow — スマホ側を独立 SWindow として生成し UMG を貼る
// =============================================================================
void ATomatinaHUD::CreatePhoneWindow()
{
	if (!FSlateApplication::IsInitialized())
	{
		UE_LOG(LogTemp, Error, TEXT("ATomatinaHUD::CreatePhoneWindow: Slate 未初期化"));
		return;
	}

	if (!PhoneViewWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("ATomatinaHUD::CreatePhoneWindow: PhoneViewWidgetClass 未設定 (BP_TomatinaHUD で WBP_PhoneView をセットしてください)"));
		return;
	}

	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		UE_LOG(LogTemp, Error, TEXT("ATomatinaHUD::CreatePhoneWindow: PlayerController 取得失敗"));
		return;
	}

	PhoneViewWidget = CreateWidget<UUserWidget>(PC, PhoneViewWidgetClass);
	if (!PhoneViewWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("ATomatinaHUD::CreatePhoneWindow: PhoneViewWidget 生成失敗"));
		return;
	}

	BindZoomMaterialToWidget(PhoneViewWidget, TEXT("IMG_ZoomView"), TEXT("PhoneView"));

	// WBP 側で Fill アンカーが付いていないと Image のスロットサイズが 0 のまま
	// になり RT を貼っても見えない。C++ から強制的に親 CanvasPanel 全面塗りにする。
	if (UImage* ZoomImg = Cast<UImage>(
			PhoneViewWidget->GetWidgetFromName(TEXT("IMG_ZoomView"))))
	{
		if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(ZoomImg->Slot))
		{
			Slot->SetAutoSize(false);
			Slot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			Slot->SetOffsets(FMargin(0.f, 0.f, 0.f, 0.f));
			Slot->SetAlignment(FVector2D(0.f, 0.f));
			UE_LOG(LogTemp, Warning,
				TEXT("ATomatinaHUD: PhoneView の IMG_ZoomView を全面塗りに強制"));
		}
		else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("ATomatinaHUD: PhoneView の IMG_ZoomView 親が CanvasPanel でない (親を CanvasPanel にしてください)"));
		}
	}

	// PhoneSplatContainer も同様に親全面で覆う (ここに汚れが動的生成される)
	if (UCanvasPanel* SplatC = Cast<UCanvasPanel>(
			PhoneViewWidget->GetWidgetFromName(TEXT("PhoneSplatContainer"))))
	{
		if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(SplatC->Slot))
		{
			Slot->SetAutoSize(false);
			Slot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			Slot->SetOffsets(FMargin(0.f, 0.f, 0.f, 0.f));
			Slot->SetAlignment(FVector2D(0.f, 0.f));
		}
	}

	const FVector2D ScreenPos(MainWidth, 0.f);
	const FVector2D ClientSize(PhoneWidth, PhoneHeight);

	TSharedRef<SWindow> NewWindow = SNew(SWindow)
		.Type(EWindowType::GameWindow)
		.Title(FText::FromString(TEXT("Tomatina Phone")))
		.CreateTitleBar(false)
		.HasCloseButton(false)
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		.UseOSWindowBorder(false)
		.FocusWhenFirstShown(false)
		.ActivationPolicy(EWindowActivationPolicy::Never)
		.SizingRule(ESizingRule::UserSized)
		.ScreenPosition(ScreenPos)
		.ClientSize(ClientSize);

	FSlateApplication::Get().AddWindow(NewWindow, true);
	PhoneWindow = NewWindow;

	// UMG 側を Slate ツリーにアタッチ
	PhoneWindow->SetContent(PhoneViewWidget->TakeWidget());

	// AddWindow 直後は ScreenPosition が反映されないケースがあるので明示的に移動
	PhoneWindow->MoveWindowTo(ScreenPos);
	PhoneWindow->Resize(ClientSize);

	UE_LOG(LogTemp, Warning,
		TEXT("ATomatinaHUD: 第二ウィンドウ(Phone) 生成 Pos=(%.0f,%.0f) Size=(%.0fx%.0f)"),
		ScreenPos.X, ScreenPos.Y, ClientSize.X, ClientSize.Y);
}

// =============================================================================
// DestroyPhoneWindow — 第二ウィンドウ破棄
// =============================================================================
void ATomatinaHUD::DestroyPhoneWindow()
{
	if (PhoneWindow.IsValid())
	{
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().RequestDestroyWindow(PhoneWindow.ToSharedRef());
		}
		PhoneWindow.Reset();
	}
	PhoneViewWidget = nullptr;
}

// =============================================================================
// BindZoomMaterialToWidget
// =============================================================================
bool ATomatinaHUD::BindZoomMaterialToWidget(UUserWidget* Widget, FName PreferredImageName, const TCHAR* WidgetLabel)
{
	if (!Widget)
	{
		UE_LOG(LogTemp, Error, TEXT("ATomatinaHUD::BindZoomMaterialToWidget: %s Widget が null"), WidgetLabel);
		return false;
	}

	UImage* ZoomImage = FindOrCreateZoomImage(Widget, PreferredImageName, WidgetLabel);
	if (!ZoomImage)
	{
		UE_LOG(LogTemp, Error,
			TEXT("ATomatinaHUD: %s にズーム表示用 Image を確保できませんでした"),
			WidgetLabel);
		return false;
	}

	return ConfigureZoomImageContent(ZoomImage, WidgetLabel);
}

// =============================================================================
// LayoutPhoneZoomImage
// =============================================================================
void ATomatinaHUD::LayoutPhoneZoomImage(UUserWidget* Widget, FName PreferredImageName, const TCHAR* WidgetLabel)
{
	if (!Widget)
	{
		return;
	}

	UImage* TargetImage = FindOrCreateZoomImage(Widget, PreferredImageName, WidgetLabel);

	if (!TargetImage)
	{
		return;
	}

	UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(TargetImage->Slot);
	if (!Slot)
	{
		UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD: %s のズーム Image は CanvasPanelSlot ではありません"), WidgetLabel);
		return;
	}

	// メイン右隣に iPhone 描画領域を配置。
	// スマホ実機を繋いだときの Windows 拡張デスクトップでは、スマホは Y=0 から始まる
	// (メインと上辺を揃える配置が前提) ため、Y を中央寄せにすると下にズレてスマホ外に
	// 押し出され、主ビューが透けて見える症状になる。必ず Y=0 に top-align する。
	//
	// CanvasPanelSlot の座標は "widget-space" (DPI スケール適用前)。
	// 実ピクセル位置 = widget-space × DPIScale なので、ここではピクセル値を
	// DPIScale で割って widget-space に変換してから渡す必要がある。これを怠ると
	// 縦 1600 viewport で DPI≈1.48 倍に引き延ばされ、ZoomView が viewport 外へ
	// 飛び出しスマホに描画されなくなる。
	const float DPIScale = UWidgetLayoutLibrary::GetViewportScale(Widget);
	const float Safe = (DPIScale > KINDA_SMALL_NUMBER) ? DPIScale : 1.f;

	const float PhoneX = MainWidth / Safe;
	const float PhoneY = 0.f;
	const float SlotW  = PhoneWidth  / Safe;
	const float SlotH  = PhoneHeight / Safe;

	Slot->SetAutoSize(false);
	Slot->SetPosition(FVector2D(PhoneX, PhoneY));
	Slot->SetSize(FVector2D(SlotW, SlotH));
	Slot->SetAlignment(FVector2D(0.f, 0.f));

	UE_LOG(LogTemp, Warning,
		TEXT("ATomatinaHUD: %s のズーム領域を配置 DPI=%.3f 画面ピクセル=(%.0f,%.0f)+(%.0f,%.0f) widget-space=(%.0f,%.0f)+(%.0f,%.0f)"),
		WidgetLabel, Safe,
		MainWidth, 0.f, PhoneWidth, PhoneHeight,
		PhoneX, PhoneY, SlotW, SlotH);
}

// =============================================================================
// FindOrCreateZoomImage
// =============================================================================
UImage* ATomatinaHUD::FindOrCreateZoomImage(UUserWidget* Widget, FName PreferredImageName, const TCHAR* WidgetLabel)
{
	if (!Widget) { return nullptr; }

	if (UImage* Preferred = Cast<UImage>(Widget->GetWidgetFromName(PreferredImageName)))
	{
		return Preferred;
	}

	UWidgetTree* Tree = Widget->WidgetTree;
	if (!Tree)
	{
		return nullptr;
	}

	TArray<UWidget*> AllWidgets;
	Tree->GetAllWidgets(AllWidgets);

	for (UWidget* W : AllWidgets)
	{
		UImage* Img = Cast<UImage>(W);
		if (!Img) { continue; }

		const FString N = Img->GetName();
		if (N.Contains(TEXT("Zoom"), ESearchCase::IgnoreCase)
			|| N.Contains(TEXT("Phone"), ESearchCase::IgnoreCase)
			|| N.Contains(TEXT("Finder"), ESearchCase::IgnoreCase))
		{
			return Img;
		}
	}

	// どこにも無い場合は CanvasPanel に実行時生成して強制的に表示先を作る
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(Tree->RootWidget);
	if (!RootCanvas)
	{
		for (UWidget* W : AllWidgets)
		{
			RootCanvas = Cast<UCanvasPanel>(W);
			if (RootCanvas) { break; }
		}
	}

	if (!RootCanvas)
	{
		UE_LOG(LogTemp, Error,
			TEXT("ATomatinaHUD: %s で ZoomView 生成先 CanvasPanel が見つかりません"),
			WidgetLabel);
		return nullptr;
	}

	UImage* RuntimeZoomImage = Tree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("IMG_ZoomView_Runtime"));
	if (!RuntimeZoomImage)
	{
		return nullptr;
	}

	if (UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(RuntimeZoomImage))
	{
		// 親 CanvasPanel 全面に塗る (Anchor 0,0〜1,1 / Offset 全0)
		Slot->SetAutoSize(false);
		Slot->SetAlignment(FVector2D(0.f, 0.f));
		Slot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		Slot->SetOffsets(FMargin(0.f, 0.f, 0.f, 0.f));
	}

	UE_LOG(LogTemp, Warning,
		TEXT("ATomatinaHUD: %s に IMG_ZoomView_Runtime を実行時生成しました (親全面塗り)"),
		WidgetLabel);

	return RuntimeZoomImage;
}

// =============================================================================
// ConfigureZoomImageContent
// =============================================================================
bool ATomatinaHUD::ConfigureZoomImageContent(UImage* ImageWidget, const TCHAR* WidgetLabel)
{
	if (!ImageWidget)
	{
		return false;
	}

	APlayerController* PC = GetOwningPlayerController();
	ATomatinaPlayerPawn* Pawn = PC ? Cast<ATomatinaPlayerPawn>(PC->GetPawn()) : nullptr;
	UTextureRenderTarget2D* ZoomRT = (Pawn && Pawn->SceneCapture_Zoom)
		? Pawn->SceneCapture_Zoom->TextureTarget
		: nullptr;

	if (ZoomRT)
	{
		FSlateBrush Brush = ImageWidget->GetBrush();
		Brush.SetResourceObject(ZoomRT);
		// DrawAs = Image にしないと 9-slice 等で扱われて描画されないことがある
		Brush.DrawAs = ESlateBrushDrawType::Image;
		// ImageSize を RT のサイズで埋めないと 0x0 になり、親スロットサイズに
		// 従わないケース (RootCanvas 以外の親など) で何も描画されない
		if (ZoomRT->SizeX > 0 && ZoomRT->SizeY > 0)
		{
			Brush.ImageSize = FVector2D(ZoomRT->SizeX, ZoomRT->SizeY);
		}
		ImageWidget->SetBrush(Brush);
		ImageWidget->SetColorAndOpacity(FLinearColor::White);
		UE_LOG(LogTemp, Warning,
			TEXT("ATomatinaHUD: %s に RT_Zoom を直接バインド (RT=%dx%d)"),
			WidgetLabel, ZoomRT->SizeX, ZoomRT->SizeY);
		return true;
	}

	if (ZoomDisplayMaterial)
	{
		ImageWidget->SetBrushFromMaterial(ZoomDisplayMaterial);
		UE_LOG(LogTemp, Warning,
			TEXT("ATomatinaHUD: %s に ZoomDisplayMaterial をバインドしました（RT 未検出のため）"),
			WidgetLabel);
		return true;
	}

	UE_LOG(LogTemp, Error,
		TEXT("ATomatinaHUD: %s の ZoomView に RT/Material のどちらも設定できません"),
		WidgetLabel);
	return false;
}

// =============================================================================
// ValidateMissionStylishWidgets
// =============================================================================
void ATomatinaHUD::ValidateMissionStylishWidgets()
{
	if (!MissionDisplayWidget)
	{
		return;
	}

	const bool bHasRank = (MissionDisplayWidget->GetWidgetFromName(TEXT("TXT_StylishRank")) != nullptr);
	const bool bHasCombo = (MissionDisplayWidget->GetWidgetFromName(TEXT("TXT_StylishCombo")) != nullptr);
	const bool bHasGauge = (MissionDisplayWidget->GetWidgetFromName(TEXT("PB_StylishGauge")) != nullptr);

	if (!bHasRank)
	{
		UE_LOG(LogTemp, Error, TEXT("WBP_MissionDisplay: TXT_StylishRank が見つかりません（名前完全一致が必要）"));
	}
	if (!bHasCombo)
	{
		UE_LOG(LogTemp, Error, TEXT("WBP_MissionDisplay: TXT_StylishCombo が見つかりません（名前完全一致が必要）"));
	}
	if (!bHasGauge)
	{
		UE_LOG(LogTemp, Error, TEXT("WBP_MissionDisplay: PB_StylishGauge が見つかりません（名前完全一致が必要）"));
	}

	if (bHasRank && bHasCombo && bHasGauge)
	{
		UE_LOG(LogTemp, Warning, TEXT("WBP_MissionDisplay: スタイリッシュ UI 3要素を検出しました"));
		UpdateStylishDisplay(TEXT("C"), 0.f, 0, false);
	}
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
	const float NormX = (MainWidth  > 0.f) ? Pos.X / MainWidth  : 0.f;
	const float NormY = (MainHeight > 0.f) ? Pos.Y / MainHeight : 0.f;

	if (bUseSeparatePhoneWindow && PhoneViewWidget)
	{
		// 第二ウィンドウ方式：スマホウィンドウ内ローカル座標 (0..PhoneWidth, 0..PhoneHeight)
		UImage* PhoneCrosshair = Cast<UImage>(
			PhoneViewWidget->GetWidgetFromName(TEXT("IMG_PhoneCursor")));
		if (!PhoneCrosshair) { return; }

		UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(PhoneCrosshair->Slot);
		if (!Slot) { return; }

		Slot->SetPosition(FVector2D(NormX * PhoneWidth, NormY * PhoneHeight));
		return;
	}

	// 旧・スパンウィンドウ方式：メインウィンドウ内 (MainWidth 以降) に配置
	if (!CursorWidget) { return; }

	UImage* Crosshair = Cast<UImage>(CursorWidget->GetWidgetFromName(TEXT("IMG_Crosshair")));
	if (!Crosshair) { return; }

	UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Crosshair->Slot);
	if (!Slot) { return; }

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

	if (bUseSeparatePhoneWindow && PhoneViewWidget)
	{
		if (UImage* PhoneCrosshair = Cast<UImage>(
				PhoneViewWidget->GetWidgetFromName(TEXT("IMG_PhoneCursor"))))
		{
			PhoneCrosshair->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		return;
	}

	if (CursorWidget) { CursorWidget->SetVisibility(ESlateVisibility::Visible); }
}

void ATomatinaHUD::HideCursor()
{
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaHUD::HideCursor"));

	if (bUseSeparatePhoneWindow && PhoneViewWidget)
	{
		if (UImage* PhoneCrosshair = Cast<UImage>(
				PhoneViewWidget->GetWidgetFromName(TEXT("IMG_PhoneCursor"))))
		{
			PhoneCrosshair->SetVisibility(ESlateVisibility::Hidden);
		}
		return;
	}

	if (CursorWidget) { CursorWidget->SetVisibility(ESlateVisibility::Hidden); }
}

// =============================================================================
// UpdateTowelPosition — DirtOverlay 内の IMG_Towel をメイン画面のピクセル座標に動かす
// 入力 Pos: 正規化座標 (0〜1)
// =============================================================================
void ATomatinaHUD::UpdateTowelPosition(FVector2D Pos)
{
	if (!DirtOverlayWidget) { return; }

	UImage* Towel = Cast<UImage>(DirtOverlayWidget->GetWidgetFromName(TEXT("IMG_Towel")));
	if (!Towel) { return; }

	UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Towel->Slot);
	if (!Slot) { return; }

	// 正規化 → メイン画面ピクセル
	const float MainX = Pos.X * MainWidth;
	const float MainY = Pos.Y * MainHeight;

	// タオル画像の中心をその座標に合わせる
	const FVector2D Size = Slot->GetSize();
	Slot->SetPosition(FVector2D(MainX - Size.X * 0.5f, MainY - Size.Y * 0.5f));
}

// =============================================================================
// ShowTowel / HideTowel — IMG_Towel の表示切替
// =============================================================================
void ATomatinaHUD::ShowTowel()
{
	if (!DirtOverlayWidget) { return; }
	if (UImage* Towel = Cast<UImage>(DirtOverlayWidget->GetWidgetFromName(TEXT("IMG_Towel"))))
	{
		Towel->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void ATomatinaHUD::HideTowel()
{
	if (!DirtOverlayWidget) { return; }
	if (UImage* Towel = Cast<UImage>(DirtOverlayWidget->GetWidgetFromName(TEXT("IMG_Towel"))))
	{
		Towel->SetVisibility(ESlateVisibility::Hidden);
	}
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
// UpdateStylishDisplay — MissionDisplay のスタイリッシュ UI を更新
// 対象 Widget 名:
//   TXT_StylishRank, TXT_StylishCombo, PB_StylishGauge
// =============================================================================
void ATomatinaHUD::UpdateStylishDisplay(const FString& RankText, float GaugePercent, int32 ComboCount, bool bDanger)
{
	if (!MissionDisplayWidget) { return; }

	const float Gauge01 = FMath::Clamp(GaugePercent, 0.f, 1.f);
	const FLinearColor RankColor = bDanger
		? FLinearColor(1.0f, 0.25f, 0.1f, 1.0f)
		: FLinearColor(1.0f, 0.9f, 0.2f, 1.0f);

	if (UTextBlock* TxtRank = Cast<UTextBlock>(
			MissionDisplayWidget->GetWidgetFromName(TEXT("TXT_StylishRank"))))
	{
		TxtRank->SetText(FText::FromString(FString::Printf(TEXT("RANK %s"), *RankText)));
		TxtRank->SetColorAndOpacity(FSlateColor(RankColor));
	}

	if (UTextBlock* TxtCombo = Cast<UTextBlock>(
			MissionDisplayWidget->GetWidgetFromName(TEXT("TXT_StylishCombo"))))
	{
		if (ComboCount > 1)
		{
			TxtCombo->SetText(FText::FromString(FString::Printf(TEXT("x%d COMBO"), ComboCount)));
		}
		else
		{
			TxtCombo->SetText(FText::FromString(TEXT("COMBO 0")));
		}
		TxtCombo->SetColorAndOpacity(FSlateColor(RankColor));
	}

	if (UProgressBar* GaugeBar = Cast<UProgressBar>(
			MissionDisplayWidget->GetWidgetFromName(TEXT("PB_StylishGauge"))))
	{
		GaugeBar->SetPercent(Gauge01);
		GaugeBar->SetFillColorAndOpacity(RankColor);
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
	// ── メインウィンドウ側 ─────────────────────────────
	if (DirtOverlayWidget)
	{
		if (UCanvasPanel* Container = Cast<UCanvasPanel>(
				DirtOverlayWidget->GetWidgetFromName(TEXT("SplatContainer"))))
		{
			Container->ClearChildren();

			// メインモニター領域 [0, MainWidth] × [0, MainHeight]
			AddDirtSplatsToCanvas(DirtOverlayWidget, Container, Dirts,
				MainWidth, MainHeight, 0.f, 0.f);

			// 旧スパンウィンドウ方式でのみスマホ側もここに乗せる
			if (!bUseSeparatePhoneWindow)
			{
				AddDirtSplatsToCanvas(DirtOverlayWidget, Container, Dirts,
					PhoneWidth, PhoneHeight, MainWidth, 0.f);
			}
		}
	}

	// ── 第二ウィンドウ (Phone) 側 ─────────────────────────────
	// スマホ側は独立 Widget 内の PhoneSplatContainer に原点 (0,0) で配置
	if (bUseSeparatePhoneWindow && PhoneViewWidget)
	{
		if (UCanvasPanel* PhoneContainer = Cast<UCanvasPanel>(
				PhoneViewWidget->GetWidgetFromName(TEXT("PhoneSplatContainer"))))
		{
			PhoneContainer->ClearChildren();
			AddDirtSplatsToCanvas(PhoneViewWidget, PhoneContainer, Dirts,
				PhoneWidth, PhoneHeight, 0.f, 0.f);
		}
	}
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
