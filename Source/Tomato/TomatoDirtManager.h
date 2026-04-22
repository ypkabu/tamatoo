// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TomatoDirtManager.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// 汚れ１つ分のデータ
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FDirtSplat
{
	GENERATED_BODY()

	/** 画面上の正規化座標（0〜1） */
	UPROPERTY(BlueprintReadWrite, Category="Dirt")
	FVector2D NormalizedPosition = FVector2D::ZeroVector;

	/** 不透明度（0 で非表示） */
	UPROPERTY(BlueprintReadWrite, Category="Dirt")
	float Opacity = 1.0f;

	/** 汚れのサイズ（正規化スケール 0〜1。1.0 で領域幅と同じサイズ） */
	UPROPERTY(BlueprintReadWrite, Category="Dirt")
	float Size = 0.2f;

	/** 自然減衰速度（0 = 減衰なし） */
	UPROPERTY(BlueprintReadWrite, Category="Dirt")
	float FadeSpeed = 0.0f;

	/** アクティブフラグ（false になると削除対象） */
	UPROPERTY(BlueprintReadWrite, Category="Dirt")
	bool bActive = true;
};

// ─────────────────────────────────────────────────────────────────────────────
// ATomatoDirtManager
// ─────────────────────────────────────────────────────────────────────────────
UCLASS()
class TOMATO_API ATomatoDirtManager : public AActor
{
	GENERATED_BODY()

public:
	ATomatoDirtManager();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// =========================================================================
	// 設定プロパティ
	// =========================================================================

	/** タイマー方式の自動生成（true でレベル開始からランダム汚れが湧く）。トマト命中で湧かせるなら false */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dirt|Config")
	bool bEnableAutoSpawn = false;

	/** 自動生成の基本間隔（秒）。bEnableAutoSpawn が true のときだけ使う */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dirt|Config")
	float SpawnInterval = 3.0f;

	/** 同時に存在できる最大汚れ数。超過時は AddDirt が無視される（既存の汚れは削除しない） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dirt|Config")
	int32 MaxDirts = 50;

	/** SpawnDirt（自動生成）で使う汚れサイズの最小値（正規化スケール 0〜1） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dirt|Config", meta=(ClampMin="0.01", ClampMax="1.0"))
	float SpawnSizeMin = 0.16f;

	/** SpawnDirt（自動生成）で使う汚れサイズの最大値（正規化スケール 0〜1） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dirt|Config", meta=(ClampMin="0.01", ClampMax="1.0"))
	float SpawnSizeMax = 0.30f;

	/** AddDirt の Size 上限（暴走防止のセーフティクランプ。0 以下で無効） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dirt|Config")
	float MaxDirtSize = 0.6f;

	// =========================================================================
	// 操作関数
	// =========================================================================

	/**
	 * ランダムな汚れを１つ生成し DirtSplats に追加する。
	 * 追加後に HUD へ通知して Widget 更新を促す。
	 */
	UFUNCTION(BlueprintCallable, Category="Dirt")
	void SpawnDirt();

	/**
	 * 座標・サイズを指定して汚れを追加する。
	 * ATomatinaProjectile の着弾時（OnHitCamera）から呼ぶ。
	 * @param NormPos  正規化座標（0〜1）
	 * @param Size     汚れサイズ（正規化スケール）
	 */
	UFUNCTION(BlueprintCallable, Category="Dirt")
	void AddDirt(FVector2D NormPos, float Size);

	/**
	 * 指定座標の半径 Radius 以内にある汚れの Opacity を 0 にする。
	 * 2P の LeapMotion 拭き取り処理から呼ぶ。
	 * @param NormPos  正規化座標（0〜1）
	 * @param Radius   判定半径（正規化座標系）
	 */
	UFUNCTION(BlueprintCallable, Category="Dirt")
	void ClearDirtAt(FVector2D NormPos, float Radius);

	/**
	 * 指定座標の半径 Radius 以内にある汚れの Opacity を EfficiencyDelta だけ減らす。
	 * タオルシステムの漸進的な拭き取りに使う（ClearDirtAt は即時消去）。
	 * @param NormPos  正規化座標（0〜1）
	 * @param Radius   判定半径（正規化座標系）。汚れの Size * 0.5f も加算した範囲が実効半径。
	 * @param Amount   1フレームの拭き取り量。中心ほどフォールオフが大きく効果大。
	 */
	UFUNCTION(BlueprintCallable, Category="Dirt")
	void WipeDirtAt(FVector2D NormPos, float Radius, float Amount);

	/**
	 * アクティブな汚れの配列を返す（HUD / Widget が読み取り用）。
	 */
	UFUNCTION(BlueprintCallable, Category="Dirt")
	TArray<FDirtSplat> GetActiveDirts() const;

	// =========================================================================
	// 汚れデータ
	// =========================================================================

	/** 現在の汚れリスト（Blueprint からも参照可） */
	UPROPERTY(BlueprintReadOnly, Category="Dirt")
	TArray<FDirtSplat> DirtSplats;

private:
	/** SpawnDirt までの残り時間 */
	float SpawnTimer = 0.0f;

	/** HUD に汚れ更新を通知する内部ヘルパー */
	void NotifyHUD();
};
