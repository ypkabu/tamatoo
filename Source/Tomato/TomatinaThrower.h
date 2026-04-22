// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TomatinaProjectile.h"   // ETomatoTrajectory
#include "TomatinaThrower.generated.h"

class USkeletalMeshComponent;
class UAnimMontage;
class ATomatinaProjectile;

/**
 * トマトを投げてくる敵キャラ。
 *
 * 動作:
 *   1. ThrowInterval ごとに ThrowMontage を再生
 *   2. ReleaseDelay 秒後（＝振りかぶった瞬間）に HandSocketName からトマトをスポーン
 *   3. プレイヤーカメラへ向かって発射
 *
 * Anim Notify を使えるなら、ReleaseDelay を 0 にして
 * BP のモンタージュに Anim Notify を仕込み、そこから ReleaseTomato() を呼んでもよい。
 */
UCLASS(Blueprintable)
class TOMATO_API ATomatinaThrower : public AActor
{
	GENERATED_BODY()

public:
	ATomatinaThrower();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// =========================================================================
	// コンポーネント
	// =========================================================================

	/** 投げるキャラのビジュアル */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Thrower")
	USkeletalMeshComponent* MeshComp;

	// =========================================================================
	// アニメーション
	// =========================================================================

	/** 投げモーション（AnimMontage） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Thrower|Animation")
	UAnimMontage* ThrowMontage = nullptr;

	/** Montage の再生レート */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Thrower|Animation")
	float ThrowMontagePlayRate = 1.0f;

	/**
	 * Montage 再生開始から、トマトが手から離れるまでの秒数（振りかぶり時間）。
	 * 0 にすると即時発射。Anim Notify を使う場合は 0 にして、
	 * Notify から ReleaseTomato() を呼ぶ。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Thrower|Animation")
	float ReleaseDelay = 0.4f;

	/** トマトをスポーンさせる手のソケット名 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Thrower|Animation")
	FName HandSocketName = TEXT("HandSocket");

	// =========================================================================
	// 投擲
	// =========================================================================

	/** トマト Blueprint */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Thrower|Throw")
	TSubclassOf<ATomatinaProjectile> ProjectileClass;

	/** 投げる間隔（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Thrower|Throw")
	float ThrowInterval = 4.0f;

	/** 投げる間隔のランダム幅（±秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Thrower|Throw")
	float ThrowIntervalVariance = 1.5f;

	/** 開始時の待機時間（出現直後に投げないため） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Thrower|Throw")
	float StartDelay = 1.0f;

	/** プレイヤー方向を狙うか。false ならアクターの正面方向に投げる */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Thrower|Throw")
	bool bAimAtPlayer = true;

	/** プレイヤーを向く回転を発射時に揃えるか */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Thrower|Throw")
	bool bRotateToPlayerOnThrow = true;

	/** Straight 軌道の確率 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Thrower|Throw")
	float StraightChance = 0.4f;

	/** Arc 軌道の確率 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Thrower|Throw")
	float ArcChance = 0.35f;

	/** Curve 軌道の確率 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Thrower|Throw")
	float CurveChance = 0.25f;

	// =========================================================================
	// API
	// =========================================================================

	/** 投げモーションを開始する。ReleaseDelay 後に ReleaseTomato が呼ばれる */
	UFUNCTION(BlueprintCallable, Category="Thrower")
	void StartThrow();

	/** トマトを実際に発射する。Anim Notify からも直接呼べる */
	UFUNCTION(BlueprintCallable, Category="Thrower")
	void ReleaseTomato();

private:
	float ThrowTimer        = 0.f;
	float ReleaseTimer      = -1.f;  // -1 = 待機中、0 以上 = 振りかぶり中
	bool  bStartDelayActive = true;
	float StartDelayTimer   = 0.f;

	/** 軌道を確率で 1 つ選ぶ */
	ETomatoTrajectory PickTrajectory() const;

	/** プレイヤーポーンの位置を取得（取れなければ前方 1500cm） */
	FVector GetAimTargetLocation() const;
};
