// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TomatinaThrowerSpawner.generated.h"

class ATomatinaThrower;
class UBoxComponent;
class UMaterialInterface;

// ─────────────────────────────────────────────────────────────────────────────
// 投擲手のバリアント定義（強さプリセット）
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FThrowerVariant
{
	GENERATED_BODY()

	/** スポーンする投げ手 BP（強さの違いはこの BP の Details で調整） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Variant")
	TSubclassOf<ATomatinaThrower> ThrowerClass;

	/** 抽選の重み（大きいほど出やすい） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Variant", meta=(ClampMin="0.0"))
	float Weight = 1.0f;
};

// ─────────────────────────────────────────────────────────────────────────────
// 出現エッジ
// ─────────────────────────────────────────────────────────────────────────────
UENUM(BlueprintType)
enum class ESpawnEdge : uint8
{
	Left   UMETA(DisplayName="Left"),
	Right  UMETA(DisplayName="Right"),
	Back   UMETA(DisplayName="Back"),
};

// ─────────────────────────────────────────────────────────────────────────────
// ATomatinaThrowerSpawner
//
// 広場の周囲（左/右/奥）からランダムに投げ手を出現させ、
// 広場内のランダム地点に歩いていかせる。
// 時間経過で出現間隔がだんだん短くなり、最大 MaxThrowers 体まで増える。
// ─────────────────────────────────────────────────────────────────────────────
UCLASS(Blueprintable)
class TOMATO_API ATomatinaThrowerSpawner : public AActor
{
	GENERATED_BODY()

public:
	ATomatinaThrowerSpawner();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// =========================================================================
	// 広場（PlayArea）
	// 中心はこのアクターの位置。エディタで Box を可視化できる。
	// =========================================================================

	/** 広場の半径方向の半幅（cm）。X=横, Y=奥行き, Z=高さ */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner|Area")
	FVector PlayAreaExtent = FVector(1500.f, 1500.f, 100.f);

	/** 投げ手スポナーの詳細ログ。通常はOFF。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner|Debug")
	bool bDebugSpawnerLog = false;

	/** 出現位置を Extent からどれだけ外側に置くか（cm） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner|Area")
	float SpawnOutsideMargin = 200.f;

	/** Z 方向の出現／目的地高さオフセット */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner|Area")
	float GroundZOffset = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner|Area")
	bool bUseLeftEdge = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner|Area")
	bool bUseRightEdge = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner|Area")
	bool bUseBackEdge = true;

	// =========================================================================
	// 出現テンポ
	// =========================================================================

	/** 1 体目が出るまでの待機（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner|Tempo")
	float InitialDelay = 2.0f;

	/** スポーン開始時の出現間隔（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner|Tempo")
	float StartingInterval = 12.0f;

	/** 最終的に到達する最短出現間隔（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner|Tempo")
	float MinInterval = 3.0f;

	/** 1 回スポーンするたびに次の出現間隔を短くする量（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner|Tempo")
	float IntervalDecreasePerSpawn = 0.5f;

	/** 出現間隔のランダム揺らぎ（±秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner|Tempo")
	float IntervalVariance = 0.6f;

	/** 同時に存在できる投げ手の最大数。超えると新規スポーンを止める */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner|Tempo")
	int32 MaxThrowers = 8;

	/**
	 * true にすると開始から FrontLoadWindowSeconds 秒以内に MaxThrowers 体を
	 * 均等間隔で全員スポーンし、以降はスポーンしない。
	 * false なら従来の漸次増加挙動（StartingInterval → MinInterval）。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner|Tempo")
	bool bSpawnAllAtStart = true;

	/** bSpawnAllAtStart=true のときに全員配置し切るまでの時間（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner|Tempo", meta=(ClampMin="1.0", EditCondition="bSpawnAllAtStart"))
	float FrontLoadWindowSeconds = 20.f;

	// =========================================================================
	// バリアント（強さプリセット）
	// =========================================================================

	/**
	 * スポーン候補のリスト。
	 * 1 つしか入れなければ常にそれを使う。
	 * 複数入れると Weight に従って確率抽選で選ばれる（弱・普通・強の混成等）。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner|Variants")
	TArray<FThrowerVariant> ThrowerVariants;

	// =========================================================================
	// 共有狙い候補（街・他キャラ）
	// スポーン時に各 Thrower の SceneryAimActors / SceneryAimLocations にコピー
	// =========================================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner|Aim")
	TArray<AActor*> SceneryAimActors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner|Aim")
	TArray<FVector> SceneryAimLocations;

	// =========================================================================
	// 汚れオーバーレイ
	// =========================================================================
	/**
	 * 全スポーン Thrower に共通で適用する汚れオーバーレイマテリアル。
	 * Thrower 個体側で DirtOverlayMaterial が空のときだけスポナーの値が使われる。
	 * 推奨設定は ATomatinaCrowdManager::DirtOverlayMaterial のコメント参照。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner|Dirt")
	UMaterialInterface* DirtOverlayMaterial = nullptr;

	// =========================================================================
	// API
	// =========================================================================

	/** 即時に 1 体スポーン（デバッグ用） */
	UFUNCTION(BlueprintCallable, Category="Spawner")
	ATomatinaThrower* SpawnThrowerNow();

private:
	float NextSpawnTimer  = 0.f;
	float CurrentInterval = 0.f;
	int32 SpawnedCount    = 0;

	UPROPERTY()
	TArray<TObjectPtr<ATomatinaThrower>> LiveThrowers;

	TSubclassOf<ATomatinaThrower> PickThrowerClass() const;
	ESpawnEdge PickEdge() const;
	FVector ComputeSpawnLocation(ESpawnEdge Edge) const;
	FVector ComputeWalkDestination() const;
	void PruneDeadThrowers();
};
