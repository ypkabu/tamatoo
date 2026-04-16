// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TomatinaTargetBase.generated.h"

class UStaticMeshComponent;

// ─────────────────────────────────────────────────────────────────────────────
// 移動パターン
// ─────────────────────────────────────────────────────────────────────────────
UENUM(BlueprintType)
enum class ETargetMovement : uint8
{
	DepthHideAndSeek  UMETA(DisplayName="Depth Hide And Seek"),  // 奥行き出入り
	RunAcross         UMETA(DisplayName="Run Across"),           // 画面端から端へ
	FlyArc            UMETA(DisplayName="Fly Arc"),              // 弧を描いて飛ぶ
	FloatErratic      UMETA(DisplayName="Float Erratic"),        // 不規則浮遊
	BlendWithCrowd    UMETA(DisplayName="Blend With Crowd"),     // 群衆に紛れて歩く
};

// ─────────────────────────────────────────────────────────────────────────────
// ATomatinaTargetBase
// ─────────────────────────────────────────────────────────────────────────────
UCLASS(Blueprintable)
class TOMATO_API ATomatinaTargetBase : public AActor
{
	GENERATED_BODY()

public:
	ATomatinaTargetBase();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// =========================================================================
	// 識別・ビジュアル
	// =========================================================================

	/** お題タグ。GameMode の CurrentMission と照合する */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target")
	FName MyType = FName("None");

	/** 被写体のビジュアルメッシュ */
	UPROPERTY(EditAnywhere, Category="Target")
	UStaticMeshComponent* MeshComp;

	// =========================================================================
	// 移動パターン共通
	// =========================================================================

	/** 移動パターン */
	UPROPERTY(EditAnywhere, Category="Movement")
	ETargetMovement MovementType = ETargetMovement::DepthHideAndSeek;

	/** 移動速度（パターンによって意味が変わる） */
	UPROPERTY(EditAnywhere, Category="Movement")
	float MoveSpeed = 200.0f;

	/** true: ループ動作　false: 1 回動いて Destroy */
	UPROPERTY(EditAnywhere, Category="Movement")
	bool bLoop = true;

	/** 出現までの待機秒数（レベル配置時のタイミングをバラけさせる） */
	UPROPERTY(EditAnywhere, Category="Movement")
	float StartDelay = 0.0f;

	// =========================================================================
	// DepthHideAndSeek 用
	// =========================================================================

	/** 群衆の奥に隠れるときの Y 軸オフセット */
	UPROPERTY(EditAnywhere, Category="Movement|Depth")
	float HiddenY = -300.0f;

	/** 群衆の前に出るときの Y 軸オフセット */
	UPROPERTY(EditAnywhere, Category="Movement|Depth")
	float ShownY = 300.0f;

	/** 前に出ている秒数 */
	UPROPERTY(EditAnywhere, Category="Movement|Depth")
	float ShowDuration = 2.0f;

	/** 奥に隠れている秒数 */
	UPROPERTY(EditAnywhere, Category="Movement|Depth")
	float HideDuration = 3.0f;

	/** 前後移動の補間速度 */
	UPROPERTY(EditAnywhere, Category="Movement|Depth")
	float DepthInterpSpeed = 5.0f;

	// =========================================================================
	// RunAcross 用
	// =========================================================================

	/** スタート地点（OriginLocation からの相対オフセット） */
	UPROPERTY(EditAnywhere, Category="Movement|Run")
	FVector RunStartOffset = FVector(0.f, -2000.f, 0.f);

	/** ゴール地点（OriginLocation からの相対オフセット） */
	UPROPERTY(EditAnywhere, Category="Movement|Run")
	FVector RunEndOffset = FVector(0.f, 2000.f, 0.f);

	/** 地面からの高さ（犬=0、鳥=高い値） */
	UPROPERTY(EditAnywhere, Category="Movement|Run")
	float RunHeight = 0.0f;

	// =========================================================================
	// FlyArc 用
	// =========================================================================

	/** 弧の横方向の半径 */
	UPROPERTY(EditAnywhere, Category="Movement|Fly")
	float ArcRadius = 1500.0f;

	/** 弧の最高点の高さ */
	UPROPERTY(EditAnywhere, Category="Movement|Fly")
	float ArcHeight = 500.0f;

	/** ホバリング（一時停止）の秒数。0 = ホバリングなし */
	UPROPERTY(EditAnywhere, Category="Movement|Fly")
	float HoverDuration = 1.5f;

	/** ホバリング発生確率（0〜1） */
	UPROPERTY(EditAnywhere, Category="Movement|Fly")
	float HoverChance = 0.3f;

	// =========================================================================
	// FloatErratic 用
	// =========================================================================

	/** ランダム移動の範囲半径 */
	UPROPERTY(EditAnywhere, Category="Movement|Float")
	float FloatRadius = 500.0f;

	/** ワープ抽選を行う間隔（秒） */
	UPROPERTY(EditAnywhere, Category="Movement|Float")
	float WarpInterval = 5.0f;

	/** ワープ発生確率（0〜1） */
	UPROPERTY(EditAnywhere, Category="Movement|Float")
	float WarpChance = 0.2f;

	// =========================================================================
	// BlendWithCrowd 用
	// =========================================================================

	/** 群衆と合わせた歩行速度 */
	UPROPERTY(EditAnywhere, Category="Movement|Blend")
	float WalkSpeed = 100.0f;

	/** 奥行き方向のゆらぎ量 */
	UPROPERTY(EditAnywhere, Category="Movement|Blend")
	float DepthSwayAmount = 200.0f;

	/** ゆらぎの周期（Hz） */
	UPROPERTY(EditAnywhere, Category="Movement|Blend")
	float DepthSwayFrequency = 0.5f;

	// =========================================================================
	// ソケット代替：頭・ルート位置
	// =========================================================================

	/** GetHeadLocation が使う「メッシュ高さ」 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target")
	float MeshHeight = 100.0f;

	/** 頭の位置を返す（ソケット未使用時のフォールバック） */
	UFUNCTION(BlueprintCallable, Category="Target")
	FVector GetHeadLocation() const;

	/** アクターのルート位置を返す */
	UFUNCTION(BlueprintCallable, Category="Target")
	FVector GetRootLocation() const;

private:
	// ── 内部状態 ─────────────────────────────────────────────────────────────
	FVector OriginLocation;

	float StateTimer    = 0.f;
	bool  bIsShowing    = false;  // DepthHideAndSeek

	float RunProgress   = 0.f;   // RunAcross（0〜1）

	float ArcAngle      = 0.f;   // FlyArc（度数）
	bool  bIsHovering   = false;
	float HoverTimer    = 0.f;

	FVector FloatTarget;          // FloatErratic
	float   WarpTimer   = 0.f;
	float   ElapsedTime = 0.f;   // FloatErratic / BlendWithCrowd 共用

	float DelayTimer    = 0.f;
	bool  bActive       = false;

	// ── Tick サブ関数 ────────────────────────────────────────────────────────
	void TickDepthHideAndSeek(float DeltaTime);
	void TickRunAcross(float DeltaTime);
	void TickFlyArc(float DeltaTime);
	void TickFloatErratic(float DeltaTime);
	void TickBlendWithCrowd(float DeltaTime);
};
