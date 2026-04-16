// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TomatinaTowelSystem.generated.h"

class ATomatoDirtManager;
class ATomatinaPlayerPawn;

/**
 * LeapMotion の手の入力を受け取り、タオルの表示・拭き取り・耐久値を管理する。
 *
 * LeapMotion との接続（推奨：方法A）
 *   BP 派生クラスで LeapMotion プラグインのイベントを受けて
 *   bHandDetected / HandScreenPosition / HandSpeed を Set する。
 *     OnHandTracked  → bHandDetected = true
 *     OnHandLost     → bHandDetected = false
 *     OnHandMoved    → HandScreenPosition, HandSpeed を更新
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
	// LeapMotion 入力（BP から Set する）
	// =========================================================================

	/** LeapMotion の視野に手があるか */
	UPROPERTY(BlueprintReadWrite, Category="LeapMotion")
	bool bHandDetected = false;

	/**
	 * 手の位置を 0〜1 の正規化座標に変換したもの。
	 * LeapMotion の手の XY 座標を検出範囲で割って正規化して設定する。
	 */
	UPROPERTY(BlueprintReadWrite, Category="LeapMotion")
	FVector2D HandScreenPosition = FVector2D(0.5f, 0.5f);

	/**
	 * 手の移動速度。LeapMotion のフレーム間差分から計算して設定する。
	 * 拭き取り効率に直結する。
	 */
	UPROPERTY(BlueprintReadWrite, Category="LeapMotion")
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

	/** キャッシュ済み DirtManager */
	UPROPERTY()
	ATomatoDirtManager* CachedDirtManager;

	/** キャッシュ済み PlayerPawn */
	UPROPERTY()
	ATomatinaPlayerPawn* CachedPlayerPawn;
};
