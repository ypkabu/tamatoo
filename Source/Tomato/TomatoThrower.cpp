// Fill out your copyright notice in the Description page of Project Settings.

#include "TomatoThrower.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

#include "TomatinaProjectile.h"

// =============================================================================
// コンストラクタ
// =============================================================================

ATomatoThrower::ATomatoThrower()
{
	PrimaryActorTick.bCanEverTick = true;

	ThrowerMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ThrowerMesh"));
	SetRootComponent(ThrowerMesh);

	ThrowPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ThrowPoint"));
	ThrowPoint->SetupAttachment(ThrowerMesh);
	// 手のあたりのデフォルト相対位置（エディタで微調整可能）
	ThrowPoint->SetRelativeLocation(FVector(30.f, 40.f, 120.f));
}

// =============================================================================
// BeginPlay
// =============================================================================

void ATomatoThrower::BeginPlay()
{
	Super::BeginPlay();

	// 最初の投てきタイミングをランダムにバラけさせる
	ThrowTimer = FMath::RandRange(0.0f, ThrowInterval);
}

// =============================================================================
// Tick
// =============================================================================

void ATomatoThrower::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// ── アニメーション再生中 ──────────────────────────────────────────────────
	if (bAnimPlaying)
	{
		AnimTimer += DeltaTime;

		// 振りかぶりタイミングでトマト発射
		if (AnimTimer >= AnimNotifyTime && !bTomatoLaunched)
		{
			bTomatoLaunched = true;
			LaunchTomato();
		}

		// アニメーション終了判定
		if (ThrowerMesh && !ThrowerMesh->IsPlaying())
		{
			bAnimPlaying    = false;
			AnimTimer       = 0.0f;
			bTomatoLaunched = false;
			ThrowTimer = ThrowInterval
			           + FMath::RandRange(-ThrowIntervalVariance, ThrowIntervalVariance);
		}
		return;
	}

	// ── 次の投てきまでカウントダウン ─────────────────────────────────────────
	ThrowTimer -= DeltaTime;
	if (ThrowTimer <= 0.f)
	{
		StartThrow();
	}
}

// =============================================================================
// StartThrow
// =============================================================================

void ATomatoThrower::StartThrow()
{
	if (ThrowerMesh && ThrowAnimation)
	{
		ThrowerMesh->PlayAnimation(ThrowAnimation, false);
		bAnimPlaying    = true;
		AnimTimer       = 0.0f;
		bTomatoLaunched = false;

		UE_LOG(LogTemp, Log, TEXT("ATomatoThrower [%s]: 投てきアニメーション開始"), *GetName());
	}
	else
	{
		// アニメーション未設定でもトマトは発射する
		LaunchTomato();
		ThrowTimer = ThrowInterval
		           + FMath::RandRange(-ThrowIntervalVariance, ThrowIntervalVariance);
	}
}

// =============================================================================
// LaunchTomato
// =============================================================================

void ATomatoThrower::LaunchTomato()
{
	if (!ProjectileClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ATomatoThrower [%s]: ProjectileClass 未設定"), *GetName());
		return;
	}

	const FVector SpawnLoc = ThrowPoint->GetComponentLocation();

	APlayerController* PC   = GetWorld()->GetFirstPlayerController();
	APawn*             Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ATomatoThrower [%s]: Pawn の取得に失敗"), *GetName());
		return;
	}
	const FVector CameraLoc = Pawn->GetActorLocation();

	// 軌道をランダム選択
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

	ATomatinaProjectile* Tomato = GetWorld()->SpawnActor<ATomatinaProjectile>(
		ProjectileClass, SpawnLoc, FRotator::ZeroRotator);

	if (!Tomato)
	{
		UE_LOG(LogTemp, Warning, TEXT("ATomatoThrower [%s]: SpawnActor に失敗"), *GetName());
		return;
	}

	Tomato->Initialize(CameraLoc, Traj);

	if (Traj == ETomatoTrajectory::Curve)
	{
		Tomato->CurveDirection = (FMath::FRand() > 0.5f) ? 1.0f : -1.0f;
	}

	UE_LOG(LogTemp, Log,
		TEXT("ATomatoThrower [%s]: トマト発射 Traj=%d Loc=(%.0f,%.0f,%.0f)"),
		*GetName(), static_cast<int32>(Traj),
		SpawnLoc.X, SpawnLoc.Y, SpawnLoc.Z);
}
