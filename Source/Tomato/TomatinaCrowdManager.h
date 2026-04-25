// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TomatinaCrowdManager.generated.h"

class ATomatinaCrowdMember;
class UMaterialInterface;

// ─────────────────────────────────────────────────────────────────────────────
// 群衆種類定義
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FCrowdVariant
{
	GENERATED_BODY()

	/** 群衆メンバーの BP（種類ごとに見た目・速度をプリセット） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crowd")
	TSubclassOf<ATomatinaCrowdMember> MemberClass;

	/** この種類を何体スポーンするか */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crowd", meta=(ClampMin="0"))
	int32 Count = 5;

	/** 死亡時に補充するか（trueなら常に Count を維持） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crowd")
	bool bMaintainCount = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// ATomatinaCrowdManager
//
// レベルに 1 つ配置して使う。Variants で複数種類の群衆を一括スポーンし、
// 全員に同じ AreaCenter/AreaExtent を渡してその範囲で動き回らせる。
// ─────────────────────────────────────────────────────────────────────────────
UCLASS(Blueprintable)
class TOMATO_API ATomatinaCrowdManager : public AActor
{
	GENERATED_BODY()

public:
	ATomatinaCrowdManager();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// =========================================================================
	// 範囲（このアクターの位置が中心）
	// =========================================================================

	/** 動き回る範囲の半幅（cm） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crowd|Area")
	FVector AreaExtent = FVector(2000.f, 2000.f, 0.f);

	/** スポーン Z オフセット（地面の高さに合わせる） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crowd|Area")
	float GroundZOffset = 0.f;

	// =========================================================================
	// 群衆種類
	// =========================================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crowd|Variants")
	TArray<FCrowdVariant> Variants;

	// =========================================================================
	// 汚れオーバーレイ
	// =========================================================================

	/**
	 * 全スポーン群衆に共通で適用する汚れオーバーレイマテリアル。
	 * これを 1 個割り当てるだけで、全キャラの元マテリアルを改造せず汚れが乗る。
	 * メンバー個体側で DirtOverlayMaterial が空のときだけマネージャーの値が使われる
	 * （個体側で別マテリアルを上書きしたい場合は個体の BP で指定）。
	 *
	 * 推奨マテリアル設定:
	 *   Material Domain   = Surface
	 *   Blend Mode        = Translucent
	 *   Shading Model     = Unlit
	 *   ScalarParameter   = "DirtAmount" (default 0)
	 *   Constant3Vector(茶色 例 0.18,0.10,0.05) → Emissive Color
	 *   DirtAmount → Opacity
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crowd|Dirt")
	UMaterialInterface* DirtOverlayMaterial = nullptr;

	// =========================================================================
	// API
	// =========================================================================
	UFUNCTION(BlueprintCallable, Category="Crowd")
	void SpawnAll();

	UFUNCTION(BlueprintCallable, Category="Crowd")
	void DespawnAll();

private:
	UPROPERTY()
	TArray<TObjectPtr<ATomatinaCrowdMember>> LiveMembers;

	float MaintenanceTimer = 0.f;

	ATomatinaCrowdMember* SpawnOne(TSubclassOf<ATomatinaCrowdMember> Cls);
	FVector PickSpawnLocation() const;
	void TickMaintenance(float DeltaTime);
};
