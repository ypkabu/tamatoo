// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TomatinaSoundCue.h"     // FTomatinaSoundCue
#include "TomatinaProjectile.h"   // ETomatoTrajectory
#include "TomatinaThrower.generated.h"

class USkeletalMeshComponent;
class UAnimMontage;
class ATomatinaProjectile;

// ─────────────────────────────────────────────────────────────────────────────
// 移動・状態
// ─────────────────────────────────────────────────────────────────────────────
UENUM(BlueprintType)
enum class EThrowerState : uint8
{
	WalkingIn  UMETA(DisplayName="Walking In"),  // 出現位置→目的地へ歩行中
	Active     UMETA(DisplayName="Active"),      // 目的地到着後・投擲ループ
};

// ─────────────────────────────────────────────────────────────────────────────
// ATomatinaThrower
// ─────────────────────────────────────────────────────────────────────────────
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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Thrower")
	USkeletalMeshComponent* MeshComp;

	// =========================================================================
	// アニメーション
	// =========================================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Thrower|Animation")
	UAnimMontage* ThrowMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Thrower|Animation")
	float ThrowMontagePlayRate = 1.0f;

	/** Montage 再生開始から手から離れるまでの秒数。0 にして AnimNotify から ReleaseTomato を呼ぶ運用も可 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Thrower|Animation")
	float ReleaseDelay = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Thrower|Animation")
	FName HandSocketName = TEXT("HandSocket");

	// =========================================================================
	// 移動（出現〜目的地）
	// =========================================================================

	/** 歩行速度（cm/s） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Thrower|Walk")
	float WalkSpeed = 250.f;

	/** 目的地に到着したと判定する距離（cm） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Thrower|Walk")
	float ArriveTolerance = 80.f;

	/** 現在の状態（BP / AnimBP から参照可） */
	UPROPERTY(BlueprintReadOnly, Category="Thrower|Walk")
	EThrowerState State = EThrowerState::Active;

	/** AnimBP が「歩きアニメ再生中か」判定するためのフラグ */
	UPROPERTY(BlueprintReadOnly, Category="Thrower|Walk")
	bool bIsWalking = false;

	// =========================================================================
	// 投擲
	// =========================================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Thrower|Throw")
	TSubclassOf<ATomatinaProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Thrower|Throw")
	float ThrowInterval = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Thrower|Throw")
	float ThrowIntervalVariance = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Thrower|Throw")
	float StartDelay = 1.0f;

	/**
	 * プレイヤーを狙う確率（0〜1）。
	 * これ以下の Roll → プレイヤーを狙う＆体もプレイヤーへ向ける。
	 * それ以上 → SceneryAimActors / SceneryAimLocations から狙いを選ぶ。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Thrower|Throw", meta=(ClampMin="0.0", ClampMax="1.0"))
	float AimAtPlayerChance = 0.4f;

	/** プレイヤー以外の狙い候補（街・他キャラ等のアクター） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Thrower|Throw")
	TArray<AActor*> SceneryAimActors;

	/** プレイヤー以外の狙い候補（ワールド座標） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Thrower|Throw")
	TArray<FVector> SceneryAimLocations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Thrower|Throw")
	float StraightChance = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Thrower|Throw")
	float ArcChance = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Thrower|Throw")
	float CurveChance = 0.25f;

	/**
	 * プレイヤー以外の狙いがどこも設定されてない場合、ランダム方向に投げる。
	 * true → 自分の周辺ランダム位置に投げ散らす（街並みや群衆に当たる）
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Thrower|Throw")
	bool bUseRandomScatter = true;

	/** ランダム散布距離の最小値（cm） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Thrower|Throw", meta=(EditCondition="bUseRandomScatter"))
	float RandomScatterMinDistance = 800.f;

	/** ランダム散布距離の最大値（cm） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Thrower|Throw", meta=(EditCondition="bUseRandomScatter"))
	float RandomScatterMaxDistance = 3000.f;

	/** ランダム散布の高さオフセット（cm）。負の値で下方向 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Thrower|Throw", meta=(EditCondition="bUseRandomScatter"))
	float RandomScatterHeightRange = 200.f;

	// =========================================================================
	// サウンド
	// =========================================================================

	/** 投擲モーション開始時の SE（かけ声・うなり声等。Thrower 位置で 3D 再生） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Thrower|Audio")
	FTomatinaSoundCue ThrowStartSound;

	/** トマトリリース（手から離れる）瞬間の SE（スウッ系。Thrower 位置で 3D 再生） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Thrower|Audio")
	FTomatinaSoundCue ThrowReleaseSound;

	// =========================================================================
	// API
	// =========================================================================

	/** Spawner から呼ばれる。歩行モードに入れて目的地を設定 */
	UFUNCTION(BlueprintCallable, Category="Thrower")
	void BeginWalkIn(FVector Destination);

	UFUNCTION(BlueprintCallable, Category="Thrower")
	void StartThrow();

	UFUNCTION(BlueprintCallable, Category="Thrower")
	void ReleaseTomato();

	/** AnimBP / BP 用フック（歩行→停止時） */
	UFUNCTION(BlueprintImplementableEvent, Category="Thrower")
	void OnArrivedAtDestination();

	/** AnimBP / BP 用フック（投擲モーション開始時） */
	UFUNCTION(BlueprintImplementableEvent, Category="Thrower")
	void OnThrowStarted();

private:
	float ThrowTimer        = 0.f;
	float ReleaseTimer      = -1.f;
	bool  bStartDelayActive = true;
	float StartDelayTimer   = 0.f;

	FVector WalkDestination = FVector::ZeroVector;
	bool    bHasDestination = false;

	/** 直近の投擲がプレイヤー狙いだったか（ReleaseTomato の発射方向に使う） */
	FVector PendingAimLocation = FVector::ZeroVector;
	bool    bPendingAimAtPlayer = false;

	ETomatoTrajectory PickTrajectory() const;
	FVector PickAimLocation(bool& bOutAimAtPlayer);
	FVector GetPlayerLocation() const;
	FVector GetRandomScatterLocation() const;

	void TickWalk(float DeltaTime);
	void TickActive(float DeltaTime);
};
