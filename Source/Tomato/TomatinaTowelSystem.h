// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TomatinaTowelSystem.generated.h"

class ATomatoDirtManager;
class ATomatinaPlayerPawn;
class ULeapComponent;

/**
 * Ultraleap LeapMotion の手入力を受け取り、タオルの表示・拭き取り・耐久値を管理する。
 *
 * LeapComponent を内包し、Tick で GetLatestFrameData() をポーリングして
 * 手の位置を毎フレーム取得する。
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
	// LeapMotion コンポーネント
	// =========================================================================

	/** Ultraleap LeapComponent。フレームデータのポーリングに使用 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LeapMotion")
	ULeapComponent* LeapComp;

	// =========================================================================
	// LeapMotion 座標範囲（EditAnywhere で調整可能）
	// =========================================================================

	/** LeapMotion X 軸の最小値（左端、mm 単位） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LeapMotion|Range")
	float LeapRangeXMin = -150.0f;

	/** LeapMotion X 軸の最大値（右端、mm 単位） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LeapMotion|Range")
	float LeapRangeXMax = 150.0f;

	/**
	 * LeapMotion Y 軸の最小値（下端、mm 単位）。
	 * SCREENTOP モードでは Y が奥行き・Z が高さになる場合があるため
	 * LeapHeightAxisMin/Max と合わせて調整すること。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LeapMotion|Range")
	float LeapRangeYMin = 50.0f;

	/** LeapMotion Y 軸の最大値（上端、mm 単位） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LeapMotion|Range")
	float LeapRangeYMax = 350.0f;

	// =========================================================================
	// 手の状態（LeapMotion が自動更新。BP からも参照可能）
	// =========================================================================

	/** LeapMotion の視野に手があるか */
	UPROPERTY(BlueprintReadOnly, Category="LeapMotion")
	bool bHandDetected = false;

	/**
	 * 手の位置を 0〜1 の正規化座標に変換したもの。
	 * Tick 内で LeapComponent の Palm.Position から自動更新される。
	 */
	UPROPERTY(BlueprintReadOnly, Category="LeapMotion")
	FVector2D HandScreenPosition = FVector2D(0.5f, 0.5f);

	/**
	 * 手の移動速度（正規化座標/秒）。
	 * 前フレームからの移動距離 / DeltaTime で算出。
	 * 拭き取り効率に直結する。
	 */
	UPROPERTY(BlueprintReadOnly, Category="LeapMotion")
	float HandSpeed = 0.0f;

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

	/** タオルが画面に表示されているか */
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

	/** この速度未満では拭き取りが発生しない（正規化座標/秒） */
	UPROPERTY(EditAnywhere, Category="Wipe")
	float MinSpeedToWipe = 0.05f;

	/** HandSpeed にかける係数（速く振るほど効率 UP） */
	UPROPERTY(EditAnywhere, Category="Wipe")
	float SpeedMultiplier = 0.5f;

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
	/** レベル上の ATomatoDirtManager を取得（キャッシュ付き） */
	ATomatoDirtManager* GetDirtManager();

	/** 1P の PlayerPawn を取得（キャッシュ付き） */
	ATomatinaPlayerPawn* GetPlayerPawn();

	UPROPERTY()
	ATomatoDirtManager* CachedDirtManager;

	UPROPERTY()
	ATomatinaPlayerPawn* CachedPlayerPawn;

	/** 前フレームの正規化座標（速度計算用） */
	FVector2D PrevHandPos = FVector2D(0.5f, 0.5f);
};
