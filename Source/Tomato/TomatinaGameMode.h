// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TomatinaTargetBase.h"
#include "TomatinaGameMode.generated.h"

class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class UTexture2D;
class USoundBase;
class ATomatinaTargetSpawner;
class ATomatinaHUD;

// -----------------------------------------------------------------------------
// FMissionData — 1 ミッション分の設定
// -----------------------------------------------------------------------------
USTRUCT(BlueprintType)
struct FMissionData
{
	GENERATED_BODY()

	/** お題タグ。ATomatinaTargetBase::MyType と照合 */
	UPROPERTY(EditAnywhere)
	FName TargetType;

	/** お題テキスト（例：「ゴリラを撮れ！」） */
	UPROPERTY(EditAnywhere)
	FText DisplayText;

	/** 制限時間（秒）。0 以下なら無制限 */
	UPROPERTY(EditAnywhere)
	float TimeLimit = 15.0f;

	/** スポーンするターゲット BP クラス */
	UPROPERTY(EditAnywhere)
	TSubclassOf<ATomatinaTargetBase> TargetClass;

	/** 同時スポーン数 */
	UPROPERTY(EditAnywhere)
	int32 SpawnCount = 1;

	/** ATomatinaTargetSpawner::SpawnProfiles から検索するプロファイル名 */
	UPROPERTY(EditAnywhere)
	FName SpawnProfileName;

	/** ターゲットのプレビュー画像（HUD の IMG_TargetPreview に表示） */
	UPROPERTY(EditAnywhere)
	UTexture2D* TargetImage = nullptr;
};

UCLASS()
class TOMATO_API ATomatinaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATomatinaGameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// ── ミッション一覧 ───────────────────────────────────────
	UPROPERTY(EditAnywhere, Category="Tomatina|Mission")
	TArray<FMissionData> Missions;

	UPROPERTY(EditAnywhere, Category="Tomatina|Mission")
	bool bRandomOrder = true;

	UPROPERTY(BlueprintReadOnly, Category="Tomatina|Mission")
	int32 CurrentMissionIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category="Tomatina|Mission")
	FName CurrentMission;

	// ── スコア ───────────────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, Category="Tomatina|Score")
	int32 CurrentScore = 0;

	UPROPERTY(BlueprintReadOnly, Category="Tomatina|Score")
	int32 TotalScore = 0;

	// ── レンダーターゲット・音 ──────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Photo")
	UTextureRenderTarget2D* RT_Photo = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Audio")
	USoundBase* ShutterSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Audio")
	USoundBase* BGM = nullptr;

	// ── リザルト表示時間 ──────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Result")
	float ResultDisplayTime = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Result")
	float MissionResultDisplayTime = 2.0f;

	/** 撮影写真に写り込んだ汚れ 1 個あたりの減点量（負の値を推奨） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Score")
	int32 DirtPenaltyPerSplat = -5;

	// ── 内部状態 ──────────────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, Category="Tomatina|State")
	bool bInCountdown = false;

	UPROPERTY(BlueprintReadOnly, Category="Tomatina|State")
	bool bIsShowingResult = false;

	UPROPERTY(BlueprintReadOnly, Category="Tomatina|State")
	bool bIsShowingMissionResult = false;

	UPROPERTY(BlueprintReadOnly, Category="Tomatina|State")
	float RemainingTime = -1.f;

	// ── 撮影（PlayerPawn から呼ばれる） ────────────────────────
	UFUNCTION(BlueprintCallable, Category="Tomatina|Photo")
	void TakePhoto(USceneCaptureComponent2D* ZoomCamera);

	UFUNCTION(BlueprintCallable, Category="Tomatina|Mission")
	void StartMission(int32 Index);

private:
	// ── 画面サイズ（PlayerPawn から取得してキャッシュ） ────────
	float MainWidth   = 2560.f;
	float MainHeight  = 1600.f;
	float PhoneWidth  = 1024.f;
	float PhoneHeight = 768.f;

	// ── カウントダウン ───────────────────────────────────────
	float CountdownRemaining = 0.f;
	int32 LastCountdownSecond = -1;

	// ── リザルト計時（FApp::GetDeltaTime 累積） ──────────────
	float ResultElapsed = 0.f;
	float MissionResultElapsed = 0.f;

	UPROPERTY()
	ATomatinaTargetSpawner* TargetSpawner = nullptr;

	void ShuffleMissions();
	void ShowFinalResult();
	ATomatinaHUD* GetTomatinaHUD() const;
	void CachePlayerPawnSizes();
};
