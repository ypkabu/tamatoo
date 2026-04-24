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
	 * 手の位置を 0〜1 の正規化座標に変換したもの。
	 * BP 側で Ultraleap の Hand 座標を変換して渡すこと。
	 */
	UPROPERTY(BlueprintReadOnly, Category="LeapMotion")
	FVector2D HandScreenPosition = FVector2D(0.5f, 0.5f);

	/**
	 * 手の移動速度。BP 側でフレーム間差分から計算して渡すこと。
	 * 拭き取り効率に直結する。
	 */
	UPROPERTY(BlueprintReadOnly, Category="LeapMotion")
	float HandSpeed = 0.0f;

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

	void UpdateWipeSound(bool bIsWiping);
};
