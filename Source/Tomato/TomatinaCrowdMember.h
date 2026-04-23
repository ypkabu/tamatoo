// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TomatinaCrowdMember.generated.h"

class USkeletalMeshComponent;

// ─────────────────────────────────────────────────────────────────────────────
// 行動状態
// ─────────────────────────────────────────────────────────────────────────────
UENUM(BlueprintType)
enum class ECrowdAction : uint8
{
	Wander   UMETA(DisplayName="Wander"),    // 通常徘徊
	Burst    UMETA(DisplayName="Burst"),     // 興奮バースト（ダッシュ・ジャンプ等）
	Cheer    UMETA(DisplayName="Cheer"),     // その場で踊る（位置固定）
};

// ─────────────────────────────────────────────────────────────────────────────
// ATomatinaCrowdMember
// トマト祭りの群衆 1 人。Manager から配置される想定。
// ─────────────────────────────────────────────────────────────────────────────
UCLASS(Blueprintable)
class TOMATO_API ATomatinaCrowdMember : public AActor
{
	GENERATED_BODY()

public:
	ATomatinaCrowdMember();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// =========================================================================
	// コンポーネント
	// =========================================================================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Crowd")
	USkeletalMeshComponent* MeshComp;

	// =========================================================================
	// 移動範囲（Manager がセットするが Details 直編集も可）
	// =========================================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crowd|Area")
	FVector AreaCenter = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crowd|Area")
	FVector AreaExtent = FVector(1500.f, 1500.f, 100.f);

	// =========================================================================
	// 速度・行動
	// =========================================================================

	/** 通常徘徊の速度（cm/s） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crowd|Movement")
	float WanderSpeed = 180.f;

	/** バースト（ダッシュ）速度（cm/s） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crowd|Movement")
	float BurstSpeed = 450.f;

	/** バースト中にランダム揺れする半径（cm） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crowd|Movement")
	float BurstJitter = 80.f;

	/** 行動切替を行う間隔（秒）の中央値 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crowd|Movement")
	float ActionCheckInterval = 2.5f;

	/** 行動切替インターバルの揺らぎ（±秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crowd|Movement")
	float ActionCheckVariance = 1.5f;

	/** 切替時に Burst（ダッシュ）に入る確率 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crowd|Movement", meta=(ClampMin="0.0", ClampMax="1.0"))
	float BurstChance = 0.25f;

	/** 切替時に Cheer（その場踊り）に入る確率 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crowd|Movement", meta=(ClampMin="0.0", ClampMax="1.0"))
	float CheerChance = 0.2f;

	/** Burst 継続秒数 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crowd|Movement")
	float BurstDuration = 1.5f;

	/** Cheer 継続秒数 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crowd|Movement")
	float CheerDuration = 2.5f;

	/** 目的地到着判定距離（cm） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crowd|Movement")
	float ArriveTolerance = 60.f;

	/** スタート時にランダムに少しだけ Tick 開始タイミングをずらす（同期見え防止） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crowd|Movement")
	float MaxStartJitter = 1.5f;

	/** true なら左右 1 軸のみ移動（スポーン位置の X は固定、Y だけ動く）。
	 *  2 軸ランダム徘徊が嫌な場合に有効化する。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crowd|Movement")
	bool bMoveLeftRightOnly = true;

	/** 左右移動に使う軸。true = Y (横)、false = X (前後)。
	 *  通常 Y=左右。レベル配置に応じて調整。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crowd|Movement", meta=(EditCondition="bMoveLeftRightOnly"))
	bool bLeftRightUsesYAxis = true;

	// =========================================================================
	// 状態（BP / AnimBP から参照可）
	// =========================================================================
	UPROPERTY(BlueprintReadOnly, Category="Crowd|State")
	ECrowdAction CurrentAction = ECrowdAction::Wander;

	UPROPERTY(BlueprintReadOnly, Category="Crowd|State")
	float CurrentSpeed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="Crowd|State")
	bool bIsMoving = false;

	// =========================================================================
	// BP イベント（アニメ切替に使う）
	// =========================================================================
	UFUNCTION(BlueprintImplementableEvent, Category="Crowd")
	void OnEnterWander();

	UFUNCTION(BlueprintImplementableEvent, Category="Crowd")
	void OnEnterBurst();

	UFUNCTION(BlueprintImplementableEvent, Category="Crowd")
	void OnEnterCheer();

	// =========================================================================
	// API
	// =========================================================================
	UFUNCTION(BlueprintCallable, Category="Crowd")
	void InitializeFromManager(FVector Center, FVector Extent);

private:
	FVector WanderTarget    = FVector::ZeroVector;
	float   ActionTimer     = 0.f;   // 状態継続時間カウント（Burst/Cheer 用）
	float   ActionCheckTimer = 0.f;  // 次の行動切替抽選までの時間
	float   StartJitterTimer = 0.f;

	/** 左右ペーシング用：次の目標が +側か -側か */
	bool bPacingHeadingPositive = true;

	void EnterAction(ECrowdAction NewAction);
	FVector PickRandomPointInArea();
	FVector PickNextPacingTarget();
	void TickMovement(float DeltaTime);
};
