// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "TomatoDirtManager.h"   // FDirtSplat
#include "TomatinaHUD.generated.h"

class UUserWidget;
class UTexture2D;

/**
 * ゲーム HUD。
 * 全 Widget の生成・配置・更新を C++ で管理する。
 * Widget Blueprint 自体はエディタで見た目を作るが、
 * いつ・どこに表示するかは全て C++ で制御。
 */
UCLASS()
class TOMATO_API ATomatinaHUD : public AHUD
{
	GENERATED_BODY()

public:
	ATomatinaHUD();

	virtual void BeginPlay() override;

	// =========================================================================
	// Widget クラス参照（BP で設定）
	// =========================================================================

	/** ビューファインダー Widget */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Widgets")
	TSubclassOf<UUserWidget> ViewFinderWidgetClass;

	/** ズームカーソル Widget */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Widgets")
	TSubclassOf<UUserWidget> CursorWidgetClass;

	/** 汚れオーバーレイ Widget */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Widgets")
	TSubclassOf<UUserWidget> DirtOverlayWidgetClass;

	/** リザルト Widget */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Widgets")
	TSubclassOf<UUserWidget> ResultWidgetClass;

	// =========================================================================
	// 画面サイズ（PlayerPawn から取得）
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
	// カーソル操作
	// =========================================================================

	/**
	 * ズームカーソルの画面座標を更新する。
	 * Widget は CurrentCursorPosition を参照して描画位置を決める。
	 */
	UFUNCTION(BlueprintCallable, Category="HUD|Cursor")
	void UpdateCursorPosition(FVector2D Pos);

	/** カーソル Widget を表示する */
	UFUNCTION(BlueprintCallable, Category="HUD|Cursor")
	void ShowCursor();

	/** カーソル Widget を非表示にする */
	UFUNCTION(BlueprintCallable, Category="HUD|Cursor")
	void HideCursor();

	/** 現在のカーソル座標（Widget の Binding 用） */
	UPROPERTY(BlueprintReadOnly, Category="HUD|Cursor")
	FVector2D CurrentCursorPosition;

	// =========================================================================
	// リザルト
	// =========================================================================

	/**
	 * リザルト Widget を動的生成して表示する。
	 * @param Score    スコア（0〜100）
	 * @param Comment  スコアコメント文字列
	 */
	UFUNCTION(BlueprintCallable, Category="HUD|Result")
	void ShowResult(int32 Score, const FString& Comment);

	/** リザルト Widget を非表示にして破棄する */
	UFUNCTION(BlueprintCallable, Category="HUD|Result")
	void HideResult();

	/** 直近スコア（Widget Binding 用） */
	UPROPERTY(BlueprintReadOnly, Category="HUD|Result")
	int32 LastScore = 0;

	/** 直近コメント（Widget Binding 用） */
	UPROPERTY(BlueprintReadOnly, Category="HUD|Result")
	FString LastComment;

	// =========================================================================
	// ミッション表示
	// =========================================================================

	/** ミッションテキスト Widget のクラス（WBP_MissionDisplay を設定） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Widgets")
	TSubclassOf<UUserWidget> MissionWidgetClass;

	/** ミッション結果（時間切れなど）の一時表示 Widget のクラス */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Widgets")
	TSubclassOf<UUserWidget> MissionResultWidgetClass;

	/**
	 * ミッション開始時にお題テキストとターゲット画像を表示する。
	 * MissionWidget の TXT_Mission テキストと IMG_TargetPreview 画像を更新し、
	 * メインモニター右上（MainWidth - 500, 30）にサイズ (450×100) で配置する。
	 *
	 * @param MissionText  「〇〇を撮れ！」などのお題テキスト
	 * @param TargetImage  ターゲットのプレビュー画像（nullptr 可）
	 */
	UFUNCTION(BlueprintCallable, Category="HUD|Mission")
	void ShowMissionDisplay(const FText& MissionText, UTexture2D* TargetImage);

	/**
	 * ミッション開始時に「〇〇を撮れ！」テキストのみを表示する。
	 * 画像不要なときの後方互換用。内部で ShowMissionDisplay(Text, nullptr) を呼ぶ。
	 */
	UFUNCTION(BlueprintCallable, Category="HUD|Mission")
	void ShowMissionText(const FText& Text);

	/** ミッションテキスト Widget をフェードアウトして非表示にする */
	UFUNCTION(BlueprintCallable, Category="HUD|Mission")
	void HideMissionText();

	/**
	 * 制限時間切れ・次ミッション移行時のスコアとコメントを一時表示する。
	 * @param Score    スコア（0 なら「時間切れ！」）
	 * @param Comment  表示コメント文字列
	 */
	UFUNCTION(BlueprintCallable, Category="HUD|Mission")
	void ShowMissionResult(int32 Score, const FString& Comment);

	/** ミッション結果の一時表示 Widget を非表示にする */
	UFUNCTION(BlueprintCallable, Category="HUD|Mission")
	void HideMissionResult();

	/** 現在のミッションテキスト（MissionWidget の Binding 用） */
	UPROPERTY(BlueprintReadOnly, Category="HUD|Mission")
	FText CurrentMissionText;

	/** ミッション結果スコア（MissionResultWidget の Binding 用） */
	UPROPERTY(BlueprintReadOnly, Category="HUD|Mission")
	int32 MissionResultScore = 0;

	/** ミッション結果コメント（MissionResultWidget の Binding 用） */
	UPROPERTY(BlueprintReadOnly, Category="HUD|Mission")
	FString MissionResultComment;

	// =========================================================================
	// 汚れ表示
	// =========================================================================

	/**
	 * 汚れ Widget を最新の汚れリストで更新する。
	 * 汚れの正規化座標からメイン・Phone 両画面の実座標を計算して配置する。
	 * DirtOverlayWidget は Blueprint 側で CachedDirts を参照して描画する。
	 * @param Dirts  ATomatoDirtManager::GetActiveDirts() の戻り値
	 */
	UFUNCTION(BlueprintCallable, Category="HUD|Dirt")
	void UpdateDirtDisplay(const TArray<FDirtSplat>& Dirts);

	/** 最新の汚れデータ（DirtOverlay Widget の Binding 用） */
	UPROPERTY(BlueprintReadOnly, Category="HUD|Dirt")
	TArray<FDirtSplat> CachedDirts;

protected:
	/** ビューファインダー Widget インスタンス */
	UPROPERTY()
	UUserWidget* ViewFinderWidget;

	/** カーソル Widget インスタンス */
	UPROPERTY()
	UUserWidget* CursorWidget;

	/** 汚れオーバーレイ Widget インスタンス */
	UPROPERTY()
	UUserWidget* DirtOverlayWidget;

	/** リザルト Widget インスタンス（ShowResult で生成、HideResult で破棄） */
	UPROPERTY()
	UUserWidget* ResultWidget;

	/** ミッションテキスト Widget インスタンス（ShowMissionText で生成） */
	UPROPERTY()
	UUserWidget* MissionWidget;

	/** ミッション結果 Widget インスタンス（ShowMissionResult で生成、HideMissionResult で破棄） */
	UPROPERTY()
	UUserWidget* MissionResultWidget;
};
