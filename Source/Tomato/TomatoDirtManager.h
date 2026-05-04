// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TomatoDirtManager.generated.h"

UENUM(BlueprintType)
enum class EDirtType : uint8
{
	NormalRed        UMETA(DisplayName="Normal Red"),
	StickyYellowDash UMETA(DisplayName="Sticky Yellow Dash x4"),
};

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

	/** HUD の DirtTextures 配列から参照する画像 index。範囲外なら DirtTexture にフォールバック */
	UPROPERTY(BlueprintReadWrite, Category="Dirt")
	int32 TextureIndex = 0;

	/** 汚れタイプ（通常 / 特殊） */
	UPROPERTY(BlueprintReadWrite, Category="Dirt")
	EDirtType DirtType = EDirtType::NormalRed;

	/** 特殊トマト用: 必要な連続ダッシュ回数 */
	UPROPERTY(BlueprintReadWrite, Category="Dirt")
	int32 RequiredDashCount = 0;

	/** 特殊トマト用: 現在の連続ダッシュ回数 */
	UPROPERTY(BlueprintReadWrite, Category="Dirt")
	int32 CurrentDashCount = 0;

	/** 特殊トマト用: 最後にダッシュ判定が成立した時刻（RealTime 秒） */
	UPROPERTY(BlueprintReadWrite, Category="Dirt")
	float LastDashTime = -1000.0f;

	/** 特殊トマト用: 最後にダッシュ判定が成立した位置 */
	UPROPERTY(BlueprintReadWrite, Category="Dirt")
	FVector2D LastDashPos = FVector2D::ZeroVector;
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

	/** 汚れ生成/消去/HUD通知の詳細ログ。通常は OFF。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dirt|Debug")
	bool bDebugDirtLog = false;

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

	/** 汚れが出現する正規化範囲の最小値（メイン画面内の左上、0〜1）。
	 *  スマホ/写真は等倍で表示されるのでここで制限すると全体が狭まる。
	 *  メイン側だけの表示制限は HUD の MainDirtAreaRatio で別途行う。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dirt|Config", meta=(ClampMin="0.0", ClampMax="1.0"))
	FVector2D SpawnRangeMin = FVector2D(0.0f, 0.0f);

	/** 汚れが出現する正規化範囲の最大値（メイン画面内の右下、0〜1） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dirt|Config", meta=(ClampMin="0.0", ClampMax="1.0"))
	FVector2D SpawnRangeMax = FVector2D(1.0f, 1.0f);

	/** 汚れ画像のバリエーション数（HUD の DirtTextures 配列サイズに合わせる）。1 以下なら DirtTexture を使う */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dirt|Config", meta=(ClampMin="1"))
	int32 NumDirtVariants = 1;

	/** 特殊トマト（黄色）に使う TextureIndex。HUD の DirtTextures で黄色画像を割り当てること */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dirt|Special")
	int32 StickyTextureIndex = 1;

	/** 特殊トマトを剥がすのに必要な連続ダッシュ回数 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dirt|Special", meta=(ClampMin="1"))
	int32 StickyRequiredDashCount = 4;

	/** 特殊トマト: ダッシュとして認める最小 Amount（WipeDirtAt の第3引数。BP で DeltaSeconds 掛け運用を想定） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dirt|Special", meta=(ClampMin="0.01"))
	float StickyDashMinAmount = 0.03f;

	/** 特殊トマト: ダッシュとして認める最小移動距離（正規化） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dirt|Special", meta=(ClampMin="0.001"))
	float StickyDashMinMoveDistance = 0.06f;

	/** 特殊トマト: 連続扱いになる最大間隔（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dirt|Special", meta=(ClampMin="0.01"))
	float StickyDashMaxInterval = 0.28f;

	/** 特殊トマト: 途中段階での最小不透明度（進捗の視覚化用） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dirt|Special", meta=(ClampMin="0.0", ClampMax="1.0"))
	float StickyProgressMinOpacity = 0.35f;

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

	/** タイプ指定で汚れを追加する（特殊トマト用）。TextureIndex を -1 にすると自動選択。 */
	UFUNCTION(BlueprintCallable, Category="Dirt")
	void AddDirtWithType(FVector2D NormPos, float Size, EDirtType DirtType, int32 InRequiredDashCount = 0, int32 InTextureIndex = -1);

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

	/** 最後に WipeDirtAt/ClearDirtAt が有効に呼ばれた RealTime 秒（2P 活動判定用） */
	UPROPERTY(BlueprintReadOnly, Category="Dirt")
	float LastWipeRealTime = -1000.f;

	UFUNCTION(BlueprintCallable, Category="Dirt")
	float GetSecondsSinceLastWipe() const;

	/** 全汚れを即時削除（最終リザルト前の溜め演出などで使用） */
	UFUNCTION(BlueprintCallable, Category="Dirt")
	void ClearAllDirts();

private:
	/** SpawnDirt までの残り時間 */
	float SpawnTimer = 0.0f;

	/** HUD に汚れ更新を通知する内部ヘルパー */
	void NotifyHUD();
};
