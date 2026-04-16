// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TomatinaGameMode.generated.h"

class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class ATomatinaTowelSystem;

/**
 * ゲームモード。撮影・スコア計算・ミッション進行を管理する。
 */
UCLASS()
class TOMATO_API ATomatinaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATomatinaGameMode();

	// =========================================================================
	// ミッション変数
	// =========================================================================

	/** 現在のお題 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Mission")
	FName CurrentMission = FName("Gorilla");

	/** 今回の撮影スコア */
	UPROPERTY(BlueprintReadOnly, Category="Tomatina|Score")
	int32 CurrentScore = 0;

	/** 累計スコア */
	UPROPERTY(BlueprintReadOnly, Category="Tomatina|Score")
	int32 TotalScore = 0;

	/** お題リスト（EditAnywhere で BP・エディタから設定可能） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Mission")
	TArray<FName> MissionList;

	/** 現在のミッションインデックス */
	UPROPERTY(BlueprintReadOnly, Category="Tomatina|Mission")
	int32 MissionIndex = 0;

	/** リザルト表示秒数 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Result")
	float ResultDisplayTime = 2.0f;

	/** リザルトタイマーハンドル */
	FTimerHandle ResultTimerHandle;

	/** リザルト表示中フラグ（二重撮影防止） */
	UPROPERTY(BlueprintReadOnly, Category="Tomatina|Result")
	bool bIsShowingResult = false;

	// =========================================================================
	// レンダーターゲット・画面サイズ
	// =========================================================================

	/** 撮影結果を保持する RenderTarget（リザルト画面に表示） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Photo")
	UTextureRenderTarget2D* RT_Photo = nullptr;

	/** iPhone 画面の横幅（ピクセル） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Photo")
	float PhoneWidth = 1024.f;

	/** iPhone 画面の縦幅（ピクセル） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Photo")
	float PhoneHeight = 768.f;

	// =========================================================================
	// 関数
	// =========================================================================

	/**
	 * 撮影処理のエントリポイント。
	 * bIsShowingResult 中は無視（二重撮影防止）。
	 */
	UFUNCTION(BlueprintCallable, Category="Tomatina|Photo")
	void TakePhoto(USceneCaptureComponent2D* ZoomCamera);

	/**
	 * スコアに応じたコメントを返す。
	 *   >= 100 → "全身バッチリ！"
	 *   >=  50 → "上半身のみ！"
	 *   >=  10 → "足だけ..."
	 *   その他 → "映ってない！"
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Tomatina|Score")
	FString GetScoreComment(int32 Score) const;

	/** 次のミッションへ進む */
	UFUNCTION(BlueprintCallable, Category="Tomatina|Mission")
	void AdvanceMission();

protected:
	/** ResultDisplayTime 経過後に呼ばれるタイマーコールバック */
	UFUNCTION()
	void OnResultTimerEnd();
};
