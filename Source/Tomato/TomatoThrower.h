// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TomatoThrower.generated.h"

class USkeletalMeshComponent;
class USceneComponent;
class ATomatinaProjectile;
enum class ETomatoTrajectory : uint8;

UCLASS(Blueprintable)
class TOMATO_API ATomatoThrower : public AActor
{
	GENERATED_BODY()

public:
	ATomatoThrower();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// =========================================================================
	// コンポーネント
	// =========================================================================

	/** 人間のスケルタルメッシュ */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Thrower")
	USkeletalMeshComponent* ThrowerMesh;

	/** トマト発射位置（手のあたりに配置） */
	UPROPERTY(EditAnywhere, Category="Thrower")
	USceneComponent* ThrowPoint;

	// =========================================================================
	// 設定
	// =========================================================================

	/** 投げるアニメーション */
	UPROPERTY(EditAnywhere, Category="Thrower")
	UAnimationAsset* ThrowAnimation;

	/** 投げる間隔の基準値（秒） */
	UPROPERTY(EditAnywhere, Category="Thrower")
	float ThrowInterval = 3.0f;

	/** 間隔のランダム幅（±秒） */
	UPROPERTY(EditAnywhere, Category="Thrower")
	float ThrowIntervalVariance = 1.5f;

	/**
	 * アニメーション開始から何秒後にトマトを発射するか。
	 * 振りかぶりのタイミングに合わせて調整する。
	 */
	UPROPERTY(EditAnywhere, Category="Thrower")
	float AnimNotifyTime = 0.4f;

	/** スポーンする発射物クラス */
	UPROPERTY(EditAnywhere, Category="Thrower")
	TSubclassOf<ATomatinaProjectile> ProjectileClass;

	/** Straight 軌道が選ばれる確率（0〜1） */
	UPROPERTY(EditAnywhere, Category="Thrower")
	float StraightChance = 0.4f;

	/** Arc 軌道が選ばれる確率（0〜1） */
	UPROPERTY(EditAnywhere, Category="Thrower")
	float ArcChance = 0.35f;

	/** Curve 軌道が選ばれる確率（0〜1） */
	UPROPERTY(EditAnywhere, Category="Thrower")
	float CurveChance = 0.25f;

private:
	float ThrowTimer      = 0.0f;
	bool  bAnimPlaying    = false;
	float AnimTimer       = 0.0f;
	bool  bTomatoLaunched = false;

	void StartThrow();
	void LaunchTomato();
};
