// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "TomatoDirtManager.h"   // FDirtSplat
#include "TomatinaHUD.generated.h"

class UUserWidget;

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
};
