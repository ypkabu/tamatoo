// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TomatinaSoundCue.h"  // FTomatinaSoundCue
#include "TomatinaTowelSystem.generated.h"

class ATomatoDirtManager;
class ATomatinaPlayerPawn;
class USoundBase;
class UAudioComponent;
class ULeapComponent;

UENUM(BlueprintType)
enum class EHandSmoothingMode : uint8
{
	None    UMETA(DisplayName="None"),
	EMA     UMETA(DisplayName="EMA"),
	OneEuro UMETA(DisplayName="One Euro"),
};

UENUM(BlueprintType)
enum class ELeapTowelHandSelection : uint8
{
	Any   UMETA(DisplayName="Any"),
	Left  UMETA(DisplayName="Left"),
	Right UMETA(DisplayName="Right"),
};

UENUM(BlueprintType)
enum class ELeapTowelAxis : uint8
{
	X         UMETA(DisplayName="X"),
	Y         UMETA(DisplayName="Y"),
	Z         UMETA(DisplayName="Z"),
	NegativeX UMETA(DisplayName="-X"),
	NegativeY UMETA(DisplayName="-Y"),
	NegativeZ UMETA(DisplayName="-Z"),
};

/**
 * タオルの表示・拭き取り・耐久値を管理する。
 *
 * 手のデータは C++ の LeapComponent から直接読む方式と、
 * Blueprint から UpdateHandData() で渡す方式の両方に対応する。
 */
UCLASS(Blueprintable)
class TOMATO_API ATomatinaTowelSystem : public AActor
{
	GENERATED_BODY()

public:
	ATomatinaTowelSystem();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// =========================================================================
	// 手の状態（BP から UpdateHandData で更新）
	// =========================================================================

	/** Details 調整だけで Leap 入力を使えるようにするための Ultraleap コンポーネント。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LeapMotion|C++ Input")
	ULeapComponent* LeapComponent = nullptr;

	/** LeapMotion の視野に手があるか */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LeapMotion|Debug")
	bool bHandDetected = false;

	/**
	 * 拭き取り処理に使う最終座標（スムージング後、0〜1 に Clamp 済み）。
	 * BP 側の既存参照を壊さないため、従来の HandScreenPosition は処理済み座標として維持する。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LeapMotion|Debug")
	FVector2D HandScreenPosition = FVector2D(0.5f, 0.5f);

	/** BP から渡された生の正規化座標。検出範囲端では 0〜1 の外に出ることがある。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LeapMotion|Debug")
	FVector2D RawHandScreenPosition = FVector2D(0.5f, 0.5f);

	/** RawHandScreenPosition にスムージングを適用した座標。速度計算用ではなく拭き取り中心用。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LeapMotion|Debug")
	FVector2D SmoothedHandScreenPosition = FVector2D(0.5f, 0.5f);

	/** SmoothedHandScreenPosition を 0〜1 に Clamp した座標。DirtManager へ渡す安全な座標。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LeapMotion|Debug")
	FVector2D ClampedHandScreenPosition = FVector2D(0.5f, 0.5f);

	/**
	 * 手の移動速度。BP 側でフレーム間差分から計算して渡すこと。
	 * 拭き取り効率に直結する。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LeapMotion|Debug")
	float HandSpeed = 0.0f;

	/** 拭き取り/SE判定に使う速度。通常は HandSpeed、入力保持中は短時間だけ減衰させる。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LeapMotion|Debug")
	float ProcessedHandSpeed = 0.0f;

	/** C++ 側で拭き取り処理に使える入力があるか（検出中、または短時間の入力保持中）。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LeapMotion|Debug")
	bool bHasValidInput = false;

	/** 手が一瞬ロストしたため、最後の有効入力を短時間だけ使っているか。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LeapMotion|Debug")
	bool bUsingGraceInput = false;

	// =========================================================================
	// 手データ更新（C++ 直接入力 / BP 互換入力）
	// =========================================================================

	/**
	 * BP 派生クラスから毎フレーム呼んで手のデータを渡す互換用入口。
	 * bReadLeapInputInCpp が true の場合は Tick 冒頭で C++ 入力が上書きする。
	 *
	 * @param bDetected      手が検出されているか
	 * @param ScreenPosition 手の位置（正規化座標 0〜1）
	 * @param Speed          手の移動速度（正規化座標/秒）
	 */
	UFUNCTION(BlueprintCallable, Category="Towel")
	void UpdateHandData(bool bDetected, FVector2D ScreenPosition, float Speed);

	// =========================================================================
	// Leap Motion C++ 直接入力
	// =========================================================================

	/** true のとき、BP グラフなしで LeapComponent から手座標を直接読み取る。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LeapMotion|C++ Input")
	bool bReadLeapInputInCpp = true;

	/** 操作に使う手。Any は最初に見つかった手を使う。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LeapMotion|C++ Input")
	ELeapTowelHandSelection LeapHandSelection = ELeapTowelHandSelection::Any;

	/** true のとき Ultraleap の stabilized palm position を使う。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LeapMotion|C++ Input")
	bool bUseStabilizedPalmPosition = true;

	/** LeapComponent の GetLatestFrameData に ApplyDeviceOrigin=true を渡すか。通常は false。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LeapMotion|C++ Input")
	bool bApplyLeapDeviceOrigin = false;

	/** LeapComponent から手が取れない場合、Ultraleap plugin のグローバル取得も試す。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LeapMotion|C++ Input")
	bool bUseUltraleapPluginFallback = true;

	/** 特定デバイスを使う場合のシリアル。空ならプラグイン既定デバイス。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LeapMotion|C++ Input")
	FString LeapDeviceSerial;

	/** 手の左右移動に使う Leap 座標軸。環境により向きが違うため Details で調整する。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LeapMotion|Mapping")
	ELeapTowelAxis LeapHorizontalAxis = ELeapTowelAxis::Y;

	/** 手の上下移動に使う Leap 座標軸。環境により向きが違うため Details で調整する。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LeapMotion|Mapping")
	ELeapTowelAxis LeapVerticalAxis = ELeapTowelAxis::Z;

	/** 正規化座標 (0.5, 0.5) に対応する Leap 座標値。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LeapMotion|Mapping")
	FVector2D LeapInputCenter = FVector2D(0.0f, 25.0f);

	/** Leap 座標で画面半分に相当する移動量。小さいほどカーソルが大きく動く。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LeapMotion|Mapping", meta=(ClampMin="1.0"))
	FVector2D LeapInputHalfRange = FVector2D(35.0f, 25.0f);

	/** C++ 側で正規化座標差分から作る HandSpeed の倍率。MinSpeedToWipe との合わせ込み用。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LeapMotion|Mapping", meta=(ClampMin="0.0", ClampMax="1000.0"))
	float LeapInputSpeedScale = 100.0f;

	/** 手がLeap Motionに近すぎるとき、画面に「手を離してね」警告を出す。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LeapMotion|Too Close Warning")
	bool bEnableLeapTooCloseWarning = true;

	/** 近づきすぎ判定に使うLeap座標軸。机置き上向きでは通常Z。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LeapMotion|Too Close Warning")
	ELeapTowelAxis LeapTooCloseAxis = ELeapTowelAxis::Z;

	/** この値以下なら近づきすぎ警告を表示する。単位は現在のLeap座標系に合わせる。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LeapMotion|Too Close Warning")
	float LeapTooCloseThreshold = -5.0f;

	/** 警告解除用のしきい値。表示しきい値より少し大きくして点滅を防ぐ。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LeapMotion|Too Close Warning")
	float LeapTooCloseClearThreshold = 0.0f;

	/** 最後に読んだ近づきすぎ判定軸の値。実機調整用。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LeapMotion|Debug")
	float LastLeapDistanceValue = 0.0f;

	/** 現在、Leap Motionに手が近づきすぎていると判定しているか。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LeapMotion|Debug")
	bool bLeapTooCloseToDevice = false;

	// =========================================================================
	// Leap Motion 入力安定化
	// =========================================================================

	/** Leap入力の詳細ログ。端ロスト調査時だけ ON にする。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LeapMotion|Stability")
	bool bDebugLeapInput = false;

	/** C++ Leap 入力の現在状態。ログを出さずに Details から確認できる。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LeapMotion|Debug")
	FString LastLeapInputStatus = TEXT("Not started");

	/** LeapComponent がデバイスを掴んでいるか。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LeapMotion|Debug")
	bool bLeapComponentHasDevice = false;

	/** 最後に取得した Leap フレーム内の手数。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LeapMotion|Debug")
	int32 LastLeapFrameHandCount = 0;

	/** 最後に取得した Leap フレームID。変化しない場合、フレーム更新自体が止まっている。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LeapMotion|Debug")
	int32 LastLeapFrameId = 0;

	/** 最後に取得した Leap タイムスタンプ。FrameId と合わせてフレーム更新確認に使う。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LeapMotion|Debug")
	int64 LastLeapFrameTimeStamp = 0;

	/** 最後に取得した Leap フレームの左手可視状態。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LeapMotion|Debug")
	bool bLastLeapLeftVisible = false;

	/** 最後に取得した Leap フレームの右手可視状態。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LeapMotion|Debug")
	bool bLastLeapRightVisible = false;

	/** 最後に取得した非安定化の掌座標。手を動かしてここが変わるか確認する。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LeapMotion|Debug")
	FVector LastSelectedLeapPalmRawPosition = FVector::ZeroVector;

	/** 最後に取得した安定化済みの掌座標。プラグイン/デバイスによってはRawより更新が鈍い。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LeapMotion|Debug")
	FVector LastSelectedLeapPalmStabilizedPosition = FVector::ZeroVector;

	/** 最後に採用した掌座標。座標軸調整用。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LeapMotion|Debug")
	FVector LastSelectedLeapPalmPosition = FVector::ZeroVector;

	/** 手が一瞬ロストしたとき、最後の有効座標を保持する秒数。長時間ロストでは拭き取り停止。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LeapMotion|Stability", meta=(ClampMin="0.0", ClampMax="0.5"))
	float InputGraceTime = 0.12f;

	/** 手座標スムージング方式。拭き取り中心にだけ適用し、速度は BP 由来の値を使う。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LeapMotion|Smoothing")
	EHandSmoothingMode SmoothingMode = EHandSmoothingMode::EMA;

	/** EMA の追従率。1.0 に近いほど Raw に近く、0.0 に近いほどなめらか。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LeapMotion|Smoothing", meta=(ClampMin="0.0", ClampMax="1.0"))
	float EMAAlpha = 0.45f;

	/** One Euro Filter の最小カットオフ。低いほど静止時が安定する。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LeapMotion|Smoothing", meta=(ClampMin="0.001", ClampMax="30.0"))
	float OneEuroMinCutoff = 1.0f;

	/** One Euro Filter の速度追従係数。高いほど速い動きで遅延が減る。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LeapMotion|Smoothing", meta=(ClampMin="0.0", ClampMax="10.0"))
	float OneEuroBeta = 0.02f;

	/** One Euro Filter の微分値カットオフ。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LeapMotion|Smoothing", meta=(ClampMin="0.001", ClampMax="30.0"))
	float OneEuroDCutoff = 1.0f;

	/** 画面端からこの距離以内で拭き取り半径を少し広げる（正規化座標）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wipe|Edge", meta=(ClampMin="0.0", ClampMax="0.5"))
	float EdgeThreshold = 0.06f;

	/** 画面端で追加する最大半径（正規化座標）。通常中央付近では加算しない。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wipe|Edge", meta=(ClampMin="0.0", ClampMax="0.5"))
	float EdgeRadiusBoost = 0.03f;

	/** 端補正後の半径倍率上限。EdgeRadiusBoost が大きすぎてもここで止める。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wipe|Edge", meta=(ClampMin="1.0", ClampMax="3.0"))
	float MaxEdgeRadiusMultiplier = 1.35f;

	// =========================================================================
	// タオル設定
	// =========================================================================

	/** タオルの最大耐久値 */
	UPROPERTY(EditAnywhere, Category="Towel")
	float MaxDurability = 100.0f;

	/** 現在の耐久値 */
	UPROPERTY(BlueprintReadOnly, Category="Towel")
	float CurrentDurability = 100.0f;

	/** 拭いている間に 1 秒あたり消費する耐久値 */
	UPROPERTY(EditAnywhere, Category="Towel")
	float DurabilityDrainRate = 10.0f;

	/** タオル交換にかかる秒数 */
	UPROPERTY(EditAnywhere, Category="Towel")
	float SwapDuration = 3.0f;

	/** タオル交換中フラグ */
	UPROPERTY(BlueprintReadOnly, Category="Towel")
	bool bIsSwapping = false;

	/** 交換アニメーションの残り秒数 */
	UPROPERTY(BlueprintReadOnly, Category="Towel")
	float SwapTimer = 0.0f;

	/** タオルが画面に表示されているか（手が検出されているか） */
	UPROPERTY(BlueprintReadOnly, Category="Towel")
	bool bTowelVisible = false;

	// =========================================================================
	// 拭き取り設定
	// =========================================================================

	/** 正規化座標での拭き取り範囲（0.1 = 画面の 10%） */
	UPROPERTY(EditAnywhere, Category="Wipe")
	float WipeRadius = 0.1f;

	/** 基本拭き取り効率（HandSpeed で乗算される） */
	UPROPERTY(EditAnywhere, Category="Wipe")
	float WipeEfficiency = 1.0f;

	/** この速度未満では拭き取りが発生しない */
	UPROPERTY(EditAnywhere, Category="Wipe")
	float MinSpeedToWipe = 50.0f;

	/** HandSpeed にかける係数（速く振るほど効率 UP） */
	UPROPERTY(EditAnywhere, Category="Wipe")
	float SpeedMultiplier = 0.01f;

	/** 自動検索で違う DirtManager を拾う場合に、レベル上の正しい DirtManager を Details で指定する。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wipe|References")
	ATomatoDirtManager* DirtManagerOverride = nullptr;

	/** このフレームで WipeDirtAt() を呼ぶ条件まで到達したか。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Wipe|Debug")
	bool bWipeAttemptedThisFrame = false;

	/** このフレームの拭き取りで有効な DirtManager が見つかったか。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Wipe|Debug")
	bool bWipeHadDirtManagerThisFrame = false;

	/** 最後に DirtManager へ渡した拭き取り座標（0〜1 Clamp 済み）。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Wipe|Debug")
	FVector2D LastWipePosition = FVector2D(0.5f, 0.5f);

	/** 最後に DirtManager へ渡した端補正後の拭き取り半径。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Wipe|Debug")
	float LastAdjustedWipeRadius = 0.0f;

	/** 最後に DirtManager へ渡した拭き取り量。0 の場合は汚れが消えない。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Wipe|Debug")
	float LastWipeAmount = 0.0f;

	// =========================================================================
	// 拭く音（ループ SE）
	// =========================================================================

	/** 拭いている間ループ再生する SE（ループ設定の SoundWave/SoundCue を推奨） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wipe|Audio")
	USoundBase* WipeLoopSound = nullptr;

	/** 拭く音のボリューム倍率 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wipe|Audio", meta=(ClampMin="0.0", ClampMax="4.0"))
	float WipeLoopVolume = 1.0f;

	/** 拭く音のピッチ倍率 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wipe|Audio", meta=(ClampMin="0.1", ClampMax="4.0"))
	float WipeLoopPitch = 1.0f;

	/** タオル耐久が 0 になった瞬間の SE */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Towel|Audio")
	FTomatinaSoundCue TowelBreakSound;

	/** タオル交換完了（新しいタオルが使える）瞬間の SE */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Towel|Audio")
	FTomatinaSoundCue TowelReadySound;

	// =========================================================================
	// 撮影判定
	// =========================================================================

	/** タオルがズーム映像に映っているか（true なら減点対象） */
	UPROPERTY(BlueprintReadOnly, Category="Towel")
	bool bTowelInZoomView = false;

	/** タオルが映り込んだときの減点（負の値） */
	UPROPERTY(EditAnywhere, Category="Towel")
	int32 TowelPenalty = -20;

	// =========================================================================
	// 公開関数
	// =========================================================================

	/**
	 * タオルの正規化座標がズーム視界内に入っているか判定する。
	 * TakePhoto 直前に GameMode から呼ばれる（bTowelInZoomView も更新）。
	 */
	UFUNCTION(BlueprintCallable, Category="Towel")
	bool CheckTowelInView(FVector2D TowelNormPos);

private:
	ATomatoDirtManager* GetDirtManager();
	ATomatinaPlayerPawn* GetPlayerPawn();

	UPROPERTY()
	ATomatoDirtManager* CachedDirtManager;

	UPROPERTY()
	ATomatinaPlayerPawn* CachedPlayerPawn;

	/** 拭き SE の AudioComponent（ループ再生中に保持） */
	UPROPERTY()
	UAudioComponent* WipeAudioComp = nullptr;

	/** Tick 間の拭き状態を保持（エッジ検出用） */
	bool bWasWiping = false;
	bool bTowelShownOnHUD = false;

	bool bHasEverValidInput = false;
	bool bHasSmoothedHandPosition = false;
	bool bHasLastCppLeapScreenPosition = false;
	bool bHasLoggedBlueprintInputState = false;
	bool bLastLoggedBlueprintInputDetected = false;
	bool bLastCppLeapHadSelectedHand = false;
	bool bWarnedMissingDirtManager = false;
	bool bWarnedMissingPlayerPawn = false;
	float LastValidInputTime = -1000.0f;
	float LastValidHandSpeed = 0.0f;
	FVector2D LastValidRawHandScreenPosition = FVector2D(0.5f, 0.5f);
	FVector2D LastValidSmoothedHandScreenPosition = FVector2D(0.5f, 0.5f);
	FVector2D LastValidClampedHandScreenPosition = FVector2D(0.5f, 0.5f);
	FVector2D LastCppLeapScreenPosition = FVector2D(0.5f, 0.5f);

	struct FOneEuroAxisState
	{
		bool bInitialized = false;
		float PreviousRaw = 0.0f;
		float PreviousFiltered = 0.0f;
		float PreviousDerivative = 0.0f;
	};

	FOneEuroAxisState OneEuroX;
	FOneEuroAxisState OneEuroY;

	FVector2D ClampNormalizedHandPosition(FVector2D Position) const;
	void PollLeapInputFromCpp(float DeltaTime);
	void SetLeapInputStatus(const FString& NewStatus);
	bool TryGetSelectedLeapHand(const struct FLeapFrameData& Frame, struct FLeapHandData& OutHand) const;
	FVector2D ConvertLeapPositionToScreen(FVector LeapPosition) const;
	float ReadLeapAxis(FVector LeapPosition, ELeapTowelAxis Axis) const;
	void UpdateLeapTooCloseState(FVector LeapPosition, bool bHasSelectedHand);
	FVector2D ApplyHandSmoothing(FVector2D RawPosition, float DeltaTime);
	float ApplyOneEuroAxis(float Value, float DeltaTime, FOneEuroAxisState& AxisState) const;
	float CalculateOneEuroAlpha(float Cutoff, float DeltaTime) const;
	float CalculateEdgeAdjustedWipeRadius(FVector2D ClampedPosition) const;
	void ResetHandInputFilter();
	void UpdateWipeSound(bool bIsWiping);
};
