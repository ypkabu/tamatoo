// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TomatinaTargetBase.h"
#include "TomatinaGameMode.generated.h"

class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class ATomatinaTowelSystem;
class ATomatinaTargetSpawner;
class ATomatinaHUD;

// ─────────────────────────────────────────────────────────────────────────────
// FMissionData — 1 ミッション分の設定
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FMissionData
{
	GENERATED_BODY()

	/** お題タグ（ATomatinaTargetBase::MyType と照合） */
	UPROPERTY(EditAnywhere)
	FName TargetType;

	/** 画面に表示するお題テキスト（「ゴリラを撮れ！」） */
	UPROPERTY(EditAnywhere)
	FText DisplayText;

	/** 制限時間（秒）。0 なら無制限 */
	UPROPERTY(EditAnywhere)
	float TimeLimit = 15.0f;

	/** スポーンするターゲットの Blueprint クラス */
	UPROPERTY(EditAnywhere)
	TSubclassOf<ATomatinaTargetBase> TargetClass;

	/** 同時に何体出すか */
	UPROPERTY(EditAnywhere)
	int32 SpawnCount = 1;

	/**
	 * 使用するスポーン設定の名前。
	 * ATomatinaTargetSpawner::SpawnProfiles から検索する。
	 */
	UPROPERTY(EditAnywhere)
	FName SpawnProfileName;
};

// ─────────────────────────────────────────────────────────────────────────────
// ATomatinaGameMode
// ─────────────────────────────────────────────────────────────────────────────
UCLASS()
class TOMATO_API ATomatinaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATomatinaGameMode();

	virtual void BeginPlay() override;

	// =========================================================================
	// ミッション
	// =========================================================================

	/** エディタで設定するミッション一覧 */
	UPROPERTY(EditAnywhere, Category="Tomatina|Mission")
	TArray<FMissionData> Missions;

	/**
	 * true: BeginPlay で Missions をシャッフルしてランダム順に出題する。
	 * false: 配列の順番通りに出題する。
	 */
	UPROPERTY(EditAnywhere, Category="Tomatina|Mission")
	bool bRandomOrder = true;

	/** 現在のミッションインデックス */
	UPROPERTY(BlueprintReadOnly, Category="Tomatina|Mission")
	int32 CurrentMissionIndex = 0;

	/** 現在のお題タグ（CalculatePhotoScore に渡す） */
	UPROPERTY(BlueprintReadOnly, Category="Tomatina|Mission")
	FName CurrentMission;

	// =========================================================================
	// スコア
	// =========================================================================

	UPROPERTY(BlueprintReadOnly, Category="Tomatina|Score")
	int32 CurrentScore = 0;

	UPROPERTY(BlueprintReadOnly, Category="Tomatina|Score")
	int32 TotalScore = 0;

	// =========================================================================
	// レンダーターゲット・画面サイズ
	// =========================================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Photo")
	UTextureRenderTarget2D* RT_Photo = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Photo")
	float PhoneWidth = 1024.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Photo")
	float PhoneHeight = 768.f;

	// =========================================================================
	// リザルト
	// =========================================================================

	/** リザルト表示秒数 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Result")
	float ResultDisplayTime = 2.0f;

	/** リザルト表示中フラグ（二重撮影防止） */
	UPROPERTY(BlueprintReadOnly, Category="Tomatina|Result")
	bool bIsShowingResult = false;

	// =========================================================================
	// 関数
	// =========================================================================

	/**
	 * 撮影処理のエントリポイント。
	 * bIsShowingResult 中は無視（二重撮影防止）。
	 */
	UFUNCTION(BlueprintCallable, Category="Tomatina|Photo")
	void TakePhoto(USceneCaptureComponent2D* ZoomCamera);

	/** 指定インデックスのミッションを開始する */
	UFUNCTION(BlueprintCallable, Category="Tomatina|Mission")
	void StartMission(int32 Index);

protected:
	/** ResultDisplayTime 経過後のタイマーコールバック */
	UFUNCTION()
	void OnResultTimerEnd();

	/** 制限時間切れのタイマーコールバック */
	UFUNCTION()
	void OnMissionTimeUp();

private:
	FTimerHandle ResultTimerHandle;
	FTimerHandle MissionTimerHandle;

	UPROPERTY()
	ATomatinaTargetSpawner* TargetSpawner = nullptr;

	/** 全ミッション完了時の最終リザルト表示 */
	void ShowFinalResult();

	/**
	 * Missions を Fisher-Yates アルゴリズムでシャッフルし、
	 * 同じ TargetType が連続しないように調整する。
	 */
	void ShuffleMissions();

	/** HUD を取得するヘルパー */
	ATomatinaHUD* GetTomatinaHUD() const;
};
