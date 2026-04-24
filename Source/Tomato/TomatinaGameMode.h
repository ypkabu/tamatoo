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
class UAudioComponent;
class ATomatinaTargetSpawner;
class ATomatinaHUD;
class ATomatoDirtManager;

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

// -----------------------------------------------------------------------------
// FFanfareTier — スコア閾値ごとのファンファーレ SE
// -----------------------------------------------------------------------------
USTRUCT(BlueprintType)
struct FFanfareTier
{
	GENERATED_BODY()

	/** このスコア以上なら Sound を採用（降順で評価され、最も高い閾値が勝つ） */
	UPROPERTY(EditAnywhere)
	int32 MinScore = 0;

	/** 再生するファンファーレ SE */
	UPROPERTY(EditAnywhere)
	USoundBase* Sound = nullptr;

	/** 音量倍率 */
	UPROPERTY(EditAnywhere, meta=(ClampMin="0.0", ClampMax="4.0"))
	float VolumeMultiplier = 1.0f;

	/** ピッチ倍率 */
	UPROPERTY(EditAnywhere, meta=(ClampMin="0.1", ClampMax="4.0"))
	float PitchMultiplier = 1.0f;
};

UENUM(BlueprintType)
enum class EStylishRank : uint8
{
	C   UMETA(DisplayName="C"),
	B   UMETA(DisplayName="B"),
	A   UMETA(DisplayName="A"),
	S   UMETA(DisplayName="S"),
	SSS UMETA(DisplayName="SSS"),
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

	/** 観客ざわめき（ゲームプレイ中ループ再生。BeginPlay で開始、最終リザルトで停止） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Audio")
	USoundBase* CrowdAmbient = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Audio", meta=(ClampMin="0.0", ClampMax="4.0"))
	float CrowdAmbientVolume = 1.0f;

	/** 撮影後のファンファーレ（写真スコア別）。MinScore 降順で最初に一致したものを再生 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Audio")
	TArray<FFanfareTier> FanfareTiers;

	// ── リザルト表示時間 ──────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Result")
	float ResultDisplayTime = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Result")
	float MissionResultDisplayTime = 2.0f;

	/** 最終リザルト表示前の「溜め」時間（秒）。この間 BGM 以外の音停止・汚れ全削除 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Result", meta=(ClampMin="0.0", ClampMax="10.0"))
	float FinalResultBuildupTime = 1.5f;

	/** 撮影写真に写り込んだ汚れ 1 個あたりの減点量（負の値を推奨） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Score")
	int32 DirtPenaltyPerSplat = -5;

	// ── スタイリッシュランク ───────────────────────────────────
	UPROPERTY(BlueprintReadOnly, Category="Tomatina|Stylish")
	EStylishRank StylishRank = EStylishRank::C;

	UPROPERTY(BlueprintReadOnly, Category="Tomatina|Stylish")
	float StylishGauge = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Tomatina|Stylish")
	int32 StylishComboCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Tomatina|Stylish")
	float DirtCoverage01 = 0.0f;

	/** スタイリッシュゲージの上限 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Stylish")
	float StylishGaugeMax = 100.0f;

	/** 基本減衰量（毎秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Stylish")
	float StylishBaseDecayPerSecond = 3.0f;

	/** 汚れがこの割合を超えると減衰強化 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Stylish", meta=(ClampMin="0.0", ClampMax="1.0"))
	float StylishDirtDangerThreshold = 0.72f;

	/** 汚れがこの割合を超えると大幅減衰 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Stylish", meta=(ClampMin="0.0", ClampMax="1.0"))
	float StylishDirtCriticalThreshold = 0.92f;

	/** Danger 閾値超過時の追加減衰（毎秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Stylish")
	float StylishDirtDangerDecayPerSecond = 12.0f;

	/** Critical 閾値超過時の追加減衰（毎秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Stylish")
	float StylishDirtCriticalDecayPerSecond = 32.0f;

	/** この点数以上の写真を「高得点」とみなしてコンボ対象にする */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Stylish")
	int32 StylishGoodShotThreshold = 60;

	/** 高得点写真の連続扱いになる最大間隔（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Stylish")
	float StylishTempoWindow = 4.0f;

	/** 写真スコア→ゲージ加算の係数（100点で +20 なら 0.2） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Stylish")
	float StylishScoreToGaugeScale = 0.22f;

	/** 高得点連続のテンポボーナス加算値 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Stylish")
	float StylishTempoBonusGauge = 10.0f;

	/** 撮影失敗時のゲージ減少量 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Stylish")
	float StylishMissPenalty = 15.0f;

	/** 失敗時にコンボをリセットするか */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Stylish")
	bool bResetComboOnMiss = true;

	/** B/A/S/SSS への昇格閾値（C は 0） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Stylish")
	float StylishThresholdB = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Stylish")
	float StylishThresholdA = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Stylish")
	float StylishThresholdS = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Stylish")
	float StylishThresholdSSS = 90.0f;

	/** このランク以上を「高ランク状態」とみなし、ターゲット演出を解放 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Stylish")
	EStylishRank HiddenActionMinRank = EStylishRank::S;

	/** スタイリッシュランク変化時の BP フック */
	UFUNCTION(BlueprintImplementableEvent, Category="Tomatina|Stylish")
	void OnStylishRankChanged(EStylishRank NewRank, EStylishRank OldRank, bool bRankUp);

	/** 高ランク状態での撮影成功時フック（演出・SE 用） */
	UFUNCTION(BlueprintImplementableEvent, Category="Tomatina|Stylish")
	void OnHighStylishShot(ATomatinaTargetBase* Target, EStylishRank Rank, int32 ShotScore);

	// ── 内部状態 ──────────────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, Category="Tomatina|State")
	bool bInCountdown = false;

	UPROPERTY(BlueprintReadOnly, Category="Tomatina|State")
	bool bIsShowingResult = false;

	UPROPERTY(BlueprintReadOnly, Category="Tomatina|State")
	bool bIsShowingMissionResult = false;

	/** 最終リザルトの溜め中（画面暗転・音停止・汚れ消去） */
	UPROPERTY(BlueprintReadOnly, Category="Tomatina|State")
	bool bIsBuildingUpFinalResult = false;

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
	float PhoneWidth  = 2024.f;
	float PhoneHeight = 1152.f;

	// ── カウントダウン ───────────────────────────────────────
	float CountdownRemaining = 0.f;
	int32 LastCountdownSecond = -1;

	// ── リザルト計時（FApp::GetDeltaTime 累積） ──────────────
	float ResultElapsed = 0.f;
	float MissionResultElapsed = 0.f;
	float FinalBuildupElapsed = 0.f;

	UPROPERTY()
	ATomatinaTargetSpawner* TargetSpawner = nullptr;

	/** BGM 再生用の AudioComponent。StopAllSoundsExceptBGM で判別に使う */
	UPROPERTY()
	UAudioComponent* BGMAudioComp = nullptr;

	/** 観客ざわめき再生用の AudioComponent */
	UPROPERTY()
	UAudioComponent* CrowdAudioComp = nullptr;

	void ShuffleMissions();
	void BeginFinalResultBuildup();
	void ShowFinalResult();
	void StopAllSoundsExceptBGM();
	void PlayFanfareForShotScore(int32 ShotScore);
	ATomatinaHUD* GetTomatinaHUD() const;
	void CachePlayerPawnSizes();
	ATomatoDirtManager* GetDirtManager();
	float CalculateDirtCoverage01();
	void UpdateStylishGauge(float RealDelta);
	void AddStylishGaugeFromShot(int32 ShotScore);
	void ApplyStylishRankFromGauge();
	FName GetStylishRankName(EStylishRank Rank) const;
	void PushStylishStateToHUD();

	UPROPERTY()
	ATomatoDirtManager* CachedDirtManager = nullptr;

	float LastHighScoreShotTime = -1000.f;

	// =========================================================================
	// リザルト統計 (最終結果画面に表示する「平均アクションランク」と「1P2P シンクロ率」)
	// =========================================================================

	/** 撮影時点のスタイリッシュランクの累計 (float 集計 → 平均) */
	float StylishRankSum = 0.f;

	/** 撮影ごとのサンプル数 */
	int32 StylishRankSampleCount = 0;

	/** 撮影試行の総数 (シンクロ率の分母) */
	int32 TotalPhotoAttempts = 0;

	/** 1P 撮影時に 2P が直近で活動していた回数 (シンクロ率の分子) */
	int32 SyncPhotoCount = 0;

public:
	/** シンクロ判定の猶予時間 (秒)。撮影時刻から過去これだけの間に 2P が拭いていれば同期扱い */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Stats")
	float SyncWindowSeconds = 2.0f;

private:
};
