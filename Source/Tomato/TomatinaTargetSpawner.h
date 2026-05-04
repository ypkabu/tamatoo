// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TomatinaTargetBase.h"
#include "TomatinaGameMode.h"          // FMissionData
#include "TomatinaTargetSpawner.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// FSpawnProfile — ターゲット種類ごとのスポーン設定
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FSpawnProfile
{
	GENERATED_BODY()

	/** FMissionData.SpawnProfileName と一致させるプロファイル名 */
	UPROPERTY(EditAnywhere)
	FName ProfileName;

	/** スポーン位置の候補リスト（ワールド座標）。エディタ上で調整可能 */
	UPROPERTY(EditAnywhere)
	TArray<FVector> SpawnLocations;

	/** スポーン回転の候補（空のとき ZeroRotator を使用） */
	UPROPERTY(EditAnywhere)
	TArray<FRotator> SpawnRotations;

	/** RunAcross 用：走行開始点のリスト */
	UPROPERTY(EditAnywhere)
	TArray<FVector> RunStartPoints;

	/** RunAcross 用：走行終了点のリスト（RunStartPoints と同数にすること） */
	UPROPERTY(EditAnywhere)
	TArray<FVector> RunEndPoints;

	/** FlyArc 用：弧の中心点のリスト */
	UPROPERTY(EditAnywhere)
	TArray<FVector> ArcCenterPoints;
};

// ─────────────────────────────────────────────────────────────────────────────
// ATomatinaTargetSpawner — 全ターゲットのスポーンを管理（レベルに 1 個配置）
// ─────────────────────────────────────────────────────────────────────────────
UCLASS(Blueprintable)
class TOMATO_API ATomatinaTargetSpawner : public AActor
{
	GENERATED_BODY()

public:
	ATomatinaTargetSpawner();

#if WITH_EDITOR
	/** エディタ上でプロパティを変更するたびにデバッグ描画を更新する */
	virtual void OnConstruction(const FTransform& Transform) override;
#endif

	// =========================================================================
	// スポーン設定
	// =========================================================================

	/** ターゲット種類ごとのスポーン設定リスト。エディタで編集 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn")
	TArray<FSpawnProfile> SpawnProfiles;

	/**
	 * 現在アクティブなターゲット一覧。
	 * GameMode の TakePhoto で CalculatePhotoScore に渡すために公開している。
	 */
	UPROPERTY(BlueprintReadOnly, Category="Spawn")
	TArray<ATomatinaTargetBase*> ActiveTargets;

	/** ターゲットスポーン詳細ログ。通常はOFF。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn|Debug")
	bool bDebugSpawnerLog = false;

	// =========================================================================
	// 関数
	// =========================================================================

	/**
	 * 指定ミッションのターゲットをスポーンする。
	 * 既存の全ターゲットを消去してから新たにスポーン。
	 * GameMode::StartMission から呼ぶ。
	 */
	UFUNCTION(BlueprintCallable, Category="Spawn")
	void SpawnTargetsForMission(const FMissionData& Mission);

	/** 全ターゲットを消去する */
	UFUNCTION(BlueprintCallable, Category="Spawn")
	void DestroyAllTargets();

	/**
	 * 特定のターゲットを消去する。
	 * 撮影成功時に GameMode から呼ぶ。
	 */
	UFUNCTION(BlueprintCallable, Category="Spawn")
	void RemoveTarget(ATomatinaTargetBase* Target);

private:
#if WITH_EDITOR
	/**
	 * 各 SpawnProfile の SpawnLocations / RunStart / RunEnd / ArcCenter を
	 * エディタのビューポートにデバッグ描画する。
	 * プロファイルごとに色を変える。
	 */
	void DrawDebugVisualization();
#endif
};
