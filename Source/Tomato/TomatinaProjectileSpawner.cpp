// Fill out your copyright notice in the Description page of Project Settings.

#include "TomatinaProjectileSpawner.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

#include "TomatinaProjectile.h"

// =============================================================================
// コンストラクタ
// =============================================================================

ATomatinaProjectileSpawner::ATomatinaProjectileSpawner()
{
	PrimaryActorTick.bCanEverTick = true;

	// 最初のスポーンはすぐ発生しないよう初期値を基準値で設定
	SpawnTimer = SpawnInterval;
}

// =============================================================================
// Tick
// =============================================================================

void ATomatinaProjectileSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SpawnTimer -= DeltaTime;
	if (SpawnTimer <= 0.f)
	{
		SpawnTomato();
		SpawnTimer = SpawnInterval
		           + FMath::RandRange(-SpawnIntervalVariance, SpawnIntervalVariance);
	}
}

// =============================================================================
// SpawnTomato
// =============================================================================

void ATomatinaProjectileSpawner::SpawnTomato()
{
	if (SpawnPoints.Num() == 0 || !ProjectileClass)
	{
		if (bDebugSpawnerLog)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("ATomatinaProjectileSpawner::SpawnTomato: "
				     "SpawnPoints が空 または ProjectileClass 未設定"));
		}
		return;
	}

	// スポーン位置をランダム選択
	const FVector SpawnLoc =
		SpawnPoints[FMath::RandRange(0, SpawnPoints.Num() - 1)];

	// プレイヤーカメラ位置をターゲットにする
	APlayerController* PC   = GetWorld()->GetFirstPlayerController();
	APawn*             Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn)
	{
		if (bDebugSpawnerLog)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("ATomatinaProjectileSpawner::SpawnTomato: Pawn の取得に失敗"));
		}
		return;
	}
	const FVector CameraLoc = Pawn->GetActorLocation();

	// 軌道を確率で決定
	const float Roll = FMath::FRand();
	ETomatoTrajectory Traj;
	if (Roll < StraightChance)
	{
		Traj = ETomatoTrajectory::Straight;
	}
	else if (Roll < StraightChance + ArcChance)
	{
		Traj = ETomatoTrajectory::Arc;
	}
	else
	{
		Traj = ETomatoTrajectory::Curve;
	}

	// スポーン
	ATomatinaProjectile* Tomato = GetWorld()->SpawnActor<ATomatinaProjectile>(
		ProjectileClass, SpawnLoc, FRotator::ZeroRotator);

	if (!Tomato)
	{
		if (bDebugSpawnerLog)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("ATomatinaProjectileSpawner::SpawnTomato: SpawnActor に失敗"));
		}
		return;
	}

	Tomato->Initialize(CameraLoc, Traj);

	// Curve の場合はカーブ方向をランダムに設定
	if (Traj == ETomatoTrajectory::Curve)
	{
		Tomato->CurveDirection = (FMath::FRand() > 0.5f) ? 1.0f : -1.0f;
	}

	if (bDebugSpawnerLog)
	{
		UE_LOG(LogTemp, Log,
			TEXT("ATomatinaProjectileSpawner::SpawnTomato: "
			     "スポーン完了 Traj=%d Loc=(%.0f,%.0f,%.0f)"),
			static_cast<int32>(Traj),
			SpawnLoc.X, SpawnLoc.Y, SpawnLoc.Z);
	}
}
