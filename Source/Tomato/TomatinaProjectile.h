// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TomatinaProjectile.generated.h"

class UStaticMeshComponent;
class ATomatoDirtManager;

// ─────────────────────────────────────────────────────────────────────────────
// 飛行パターン
// ─────────────────────────────────────────────────────────────────────────────
UENUM(BlueprintType)
enum class ETomatoTrajectory : uint8
{
	Straight  UMETA(DisplayName="Straight"),  // 直線（速い）
	Arc       UMETA(DisplayName="Arc"),       // 山なり放物線
	Curve     UMETA(DisplayName="Curve"),     // 横カーブ
};

// ─────────────────────────────────────────────────────────────────────────────
// ATomatinaProjectile
// ─────────────────────────────────────────────────────────────────────────────
UCLASS(Blueprintable)
class TOMATO_API ATomatinaProjectile : public AActor
{
	GENERATED_BODY()

public:
	ATomatinaProjectile();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// =========================================================================
	// 設定プロパティ
	// =========================================================================

	/** 飛行パターン */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomato")
	ETomatoTrajectory Trajectory = ETomatoTrajectory::Straight;

	/** 飛行速度（cm/s） */
	UPROPERTY(EditAnywhere, Category="Tomato")
	float FlightSpeed = 1500.0f;

	/** Arc の最高点の高さ（cm） */
	UPROPERTY(EditAnywhere, Category="Tomato")
	float ArcHeight = 500.0f;

	/** Curve の曲がり量（cm） */
	UPROPERTY(EditAnywhere, Category="Tomato")
	float CurveStrength = 300.0f;

	/** カーブ方向（1.0=右, -1.0=左） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomato")
	float CurveDirection = 1.0f;

	/** トマトのビジュアルメッシュ */
	UPROPERTY(EditAnywhere, Category="Tomato")
	UStaticMeshComponent* TomatoMesh;

	/** 着弾汚れサイズの最小値（正規化座標系） */
	UPROPERTY(EditAnywhere, Category="Tomato")
	float SplatSizeMin = 0.16f;

	/** 着弾汚れサイズの最大値（正規化座標系） */
	UPROPERTY(EditAnywhere, Category="Tomato")
	float SplatSizeMax = 0.30f;

	/** トマトを破棄するまでの最大寿命（秒）。プレイヤーに当たらず消える保険。 */
	UPROPERTY(EditAnywhere, Category="Tomato")
	float MaxLifetime = 6.0f;

	// =========================================================================
	// 初期化
	// =========================================================================

	/**
	 * Spawner からスポーン直後に呼ぶ。飛行先と軌道を設定する。
	 * @param Target       飛行ターゲット（PlayerCamera の位置）
	 * @param InTrajectory 飛行パターン
	 */
	UFUNCTION(BlueprintCallable, Category="Tomato")
	void Initialize(FVector Target, ETomatoTrajectory InTrajectory);

private:
	FVector StartLocation;
	FVector TargetLocation;
	float   FlightProgress = 0.f;
	float   FlightDuration = 1.f;
	float   ElapsedTime    = 0.f;
	bool    bHasHit        = false;

	/** カメラへの着弾処理（汚れ生成 → Destroy） */
	void OnHitCamera();

	/** プレイヤーポーンとのオーバーラップで呼ばれる */
	UFUNCTION()
	void OnMeshOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	/** レベル上の ATomatoDirtManager を取得（キャッシュ付き） */
	ATomatoDirtManager* GetDirtManager();

	UPROPERTY()
	ATomatoDirtManager* CachedDirtManager;
};
