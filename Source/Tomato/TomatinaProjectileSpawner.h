// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TomatinaProjectileSpawner.generated.h"

class ATomatinaProjectile;
class ATomatoThrower;

// ─────────────────────────────────────────────────────────────────────────────
// スポーンモード
// ─────────────────────────────────────────────────────────────────────────────
UENUM(BlueprintType)
enum class ETomatoSpawnMode : uint8
{
	IntervalRandom   UMETA(DisplayName="Interval Random"),    // 一定間隔ランダム（従来方式）
	ThrowerAnimation UMETA(DisplayName="Thrower Animation"),  // 投げる人のアニメーション連動
};

/**
 * トマトを定期的にスポーンする。レベルに 1 個配置して使う。
 * SpawnPoints のいずれかからランダムに発射し、
 * Straight / Arc / Curve の比率は各 Chance プロパティで制御する。
 */
UCLASS(Blueprintable)
class TOMATO_API ATomatinaProjectileSpawner : public AActor
{
	GENERATED_BODY()

public:
	ATomatinaProjectileSpawner();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// =========================================================================
	// スポーンモード
	// =========================================================================

	/** スポーンモード */
	UPROPERTY(EditAnywhere, Category="Spawner")
	ETomatoSpawnMode SpawnMode = ETomatoSpawnMode::IntervalRandom;

	/**
	 * ThrowerAnimation モード用。
	 * レベルに配置した ATomatoThrower への参照（管理用）。
	 * Thrower 自身が自分のタイミングで投げるため、
	 * Spawner はこの配列で参照を保持するだけでよい。
	 */
	UPROPERTY(EditAnywhere, Category="Spawner|Thrower")
	TArray<ATomatoThrower*> Throwers;

	// =========================================================================
	// スポーン設定
	// =========================================================================

	/** スポーンする発射物の Blueprint クラス */
	UPROPERTY(EditAnywhere, Category="Spawner")
	TSubclassOf<ATomatinaProjectile> ProjectileClass;

	/** スポーン間隔の基準値（秒） */
	UPROPERTY(EditAnywhere, Category="Spawner")
	float SpawnInterval = 2.0f;

	/** スポーン間隔のランダム幅（±秒） */
	UPROPERTY(EditAnywhere, Category="Spawner")
	float SpawnIntervalVariance = 1.0f;

	/** 発射位置の候補リスト（ワールド座標）。エディタで設定する */
	UPROPERTY(EditAnywhere, Category="Spawner")
	TArray<FVector> SpawnPoints;

	/** Straight 軌道が選ばれる確率（0〜1） */
	UPROPERTY(EditAnywhere, Category="Spawner")
	float StraightChance = 0.4f;

	/** Arc 軌道が選ばれる確率（0〜1） */
	UPROPERTY(EditAnywhere, Category="Spawner")
	float ArcChance = 0.35f;

	/** Curve 軌道が選ばれる確率（0〜1） */
	UPROPERTY(EditAnywhere, Category="Spawner")
	float CurveChance = 0.25f;

private:
	/** 次のスポーンまでの残り秒数 */
	float SpawnTimer = 0.0f;

	/** トマトを 1 個スポーンして Initialize を呼ぶ */
	void SpawnTomato();
};
