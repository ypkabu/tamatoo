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

UENUM(BlueprintType)
enum class EHandSmoothingMode : uint8
{
	None    UMETA(DisplayName="None"),
	EMA     UMETA(DisplayName="EMA"),
	OneEuro UMETA(DisplayName="One Euro"),
};

/**
 * タオルの表示・拭き取り・耐久値を管理する。
 *
 * 手のデータは Blueprint から UpdateHandData() で渡す方式。
 * BP 派生クラス側で Ultraleap の OnLeapHandMoved 等を受けて
 * UpdateHandData(bDetected, ScreenPosition, Speed) を呼ぶこと。
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

	/** LeapMotion の視野に手があるか */
	UPROPERTY(BlueprintReadOnly, Category="LeapMotion")
	bool bHandDetected = false;

	/**
	 * 拭き取り処理に使う最終座標（スムージング後、0〜1 に Clamp 済み）。
	 * BP 側の既存参照を壊さないため、従来の HandScreenPosition は処理済み座標として維持する。
	 */
	UPROPERTY(BlueprintReadOnly, Category="LeapMotion")
	FVector2D HandScreenPosition = FVector2D(0.5f, 0.5f);

	/** BP から渡された生の正規化座標。検出範囲端では 0〜1 の外に出ることがある。 */
	UPROPERTY(BlueprintReadOnly, Category="LeapMotion")
	FVector2D RawHandScreenPosition = FVector2D(0.5f, 0.5f);

	/** RawHandScreenPosition にスムージングを適用した座標。速度計算用ではなく拭き取り中心用。 */
	UPROPERTY(BlueprintReadOnly, Category="LeapMotion")
	FVector2D SmoothedHandScreenPosition = FVector2D(0.5f, 0.5f);

	/** SmoothedHandScreenPosition を 0〜1 に Clamp した座標。DirtManager へ渡す安全な座標。 */
	UPROPERTY(BlueprintReadOnly, Category="LeapMotion")
	FVector2D ClampedHandScreenPosition = FVector2D(0.5f, 0.5f);

	/**
	 * 手の移動速度。BP 側でフレーム間差分から計算して渡すこと。
	 * 拭き取り効率に直結する。
	 */
	UPROPERTY(BlueprintReadOnly, Category="LeapMotion")
	float HandSpeed = 0.0f;

	/** 拭き取り/SE判定に使う速度。通常は HandSpeed、入力保持中は短時間だけ減衰させる。 */
	UPROPERTY(BlueprintReadOnly, Category="LeapMotion")
	float ProcessedHandSpeed = 0.0f;

	/** C++ 側で拭き取り処理に使える入力があるか（検出中、または短時間の入力保持中）。 */
	UPROPERTY(BlueprintReadOnly, Category="LeapMotion")
	bool bHasValidInput = false;

	/** 手が一瞬ロストしたため、最後の有効入力を短時間だけ使っているか。 */
	UPROPERTY(BlueprintReadOnly, Category="LeapMotion")
	bool bUsingGraceInput = false;

	// =========================================================================
	// 手データ更新（BP から毎フレーム呼ぶ）
	// =========================================================================

	/**
	 * BP 派生クラスから毎フレーム呼んで手のデータを渡す。
	 * Ultraleap の OnLeapTrackingData 等でデータを受け取り
	 * ここに流し込むことで C++ 側が SDK に依存しない構造になる。
	 *
	 * @param bDetected      手が検出されているか
	 * @param ScreenPosition 手の位置（正規化座標 0〜1）
	 * @param Speed          手の移動速度（正規化座標/秒）
	 */
	UFUNCTION(BlueprintCallable, Category="Towel")
	void UpdateHandData(bool bDetected, FVector2D ScreenPosition, float Speed);

	// =========================================================================
	// Leap Motion 入力安定化
	// =========================================================================

	/** Leap入力の詳細ログ。端ロスト調査時だけ ON にする。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LeapMotion|Stability")
	bool bDebugLeapInput = false;

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

	bool bHasEverValidInput = false;
	bool bHasSmoothedHandPosition = false;
	float LastValidInputTime = -1000.0f;
	float LastValidHandSpeed = 0.0f;
	FVector2D LastValidRawHandScreenPosition = FVector2D(0.5f, 0.5f);
	FVector2D LastValidSmoothedHandScreenPosition = FVector2D(0.5f, 0.5f);
	FVector2D LastValidClampedHandScreenPosition = FVector2D(0.5f, 0.5f);

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
	FVector2D ApplyHandSmoothing(FVector2D RawPosition, float DeltaTime);
	float ApplyOneEuroAxis(float Value, float DeltaTime, FOneEuroAxisState& AxisState) const;
	float CalculateOneEuroAlpha(float Cutoff, float DeltaTime) const;
	float CalculateEdgeAdjustedWipeRadius(FVector2D ClampedPosition) const;
	void ResetHandInputFilter();
	void UpdateWipeSound(bool bIsWiping);
};
