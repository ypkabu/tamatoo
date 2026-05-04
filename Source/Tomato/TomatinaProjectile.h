// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TomatinaProjectile.generated.h"

class UStaticMeshComponent;
class ATomatoDirtManager;
class USoundBase;

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

UENUM(BlueprintType)
enum class ETomatoImpactVariant : uint8
{
	NormalRed    UMETA(DisplayName="Normal Red"),
	StickyYellow UMETA(DisplayName="Sticky Yellow"),
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

	/** 弾の生成・命中・デカール生成の詳細ログ。通常はOFF。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomato|Debug")
	bool bDebugProjectileLog = false;

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

	/** 特殊トマト（黄色粘着）を混ぜるか */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomato|Special")
	bool bEnableSpecialTomato = true;

	/** 黄色粘着トマトの出現確率（プレイヤー命中時） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomato|Special", meta=(ClampMin="0.0", ClampMax="1.0"))
	float StickyYellowChance = 0.2f;

	/** 黄色粘着トマトのサイズ倍率 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomato|Special", meta=(ClampMin="0.5", ClampMax="3.0"))
	float StickySizeMultiplier = 1.15f;

	/** 黄色粘着トマトを剥がすのに必要な連続ダッシュ回数 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomato|Special", meta=(ClampMin="1", ClampMax="16"))
	int32 StickyRequiredDashCount = 4;

	/**
	 * このトマトはプレイヤーを狙ってる弾か。
	 * true  → プレイヤー Pawn と当たって OnHitCamera を実行
	 * false → Pawn は貫通、ワールドの Static Mesh で OnHitWorld を実行（デカール）
	 * Thrower 側で Initialize 後にセットする。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomato")
	bool bAimedAtPlayer = true;

	/** ワールド命中時に貼るデカール用マテリアル（M_TomatoDecal 等） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomato|WorldDecal")
	UMaterialInterface* WorldDecalMaterial = nullptr;

	/** デカールサイズの最小値（cm） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomato|WorldDecal")
	float WorldDecalSizeMin = 30.f;

	/** デカールサイズの最大値（cm） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomato|WorldDecal")
	float WorldDecalSizeMax = 60.f;

	/** デカールの寿命（秒）。0 以下 = 永続 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomato|WorldDecal")
	float WorldDecalLifetime = 0.f;

	// =========================================================================
	// 着弾サウンド
	// =========================================================================

	/** カメラ命中時（プレイヤーがトマトを浴びる）に鳴らす SE */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomato|Audio")
	USoundBase* ImpactSoundCamera = nullptr;

	/** ワールド（壁・床など）命中時に鳴らす SE */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomato|Audio")
	USoundBase* ImpactSoundWorld = nullptr;

	/** 黄色粘着トマトがカメラに命中したときに鳴らす SE（未設定なら ImpactSoundCamera を使用） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomato|Audio")
	USoundBase* ImpactSoundSticky = nullptr;

	/** 着弾 SE の音量倍率 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomato|Audio", meta=(ClampMin="0.0", ClampMax="4.0"))
	float ImpactSoundVolume = 1.0f;

	/** 着弾 SE のピッチ倍率 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomato|Audio", meta=(ClampMin="0.1", ClampMax="4.0"))
	float ImpactSoundPitch = 1.0f;

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

	/** ワールドの Static Mesh への着弾処理（デカール貼り → Destroy） */
	void OnHitWorld(UPrimitiveComponent* HitComp, const FHitResult& Hit);

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

	/** 命中時の汚れバリエーションを決定する */
	ETomatoImpactVariant PickImpactVariant() const;

	UPROPERTY()
	ATomatoDirtManager* CachedDirtManager;

	bool bWarnedMissingDirtManager = false;
};
