// Fill out your copyright notice in the Description page of Project Settings.

#include "TomatinaProjectile.h"

#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

#include "TomatoDirtManager.h"

// =============================================================================
// コンストラクタ
// =============================================================================

ATomatinaProjectile::ATomatinaProjectile()
	: CachedDirtManager(nullptr)
{
	PrimaryActorTick.bCanEverTick = true;

	TomatoMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TomatoMesh"));
	SetRootComponent(TomatoMesh);
}

// =============================================================================
// BeginPlay
// =============================================================================

void ATomatinaProjectile::BeginPlay()
{
	Super::BeginPlay();
}

// =============================================================================
// Initialize
// =============================================================================

void ATomatinaProjectile::Initialize(FVector Target, ETomatoTrajectory InTrajectory)
{
	StartLocation  = GetActorLocation();
	TargetLocation = Target;
	Trajectory     = InTrajectory;

	const float Dist = FVector::Dist(StartLocation, TargetLocation);
	FlightDuration = (FlightSpeed > 0.f) ? Dist / FlightSpeed : 1.f;
	FlightProgress = 0.f;

	UE_LOG(LogTemp, Log,
		TEXT("ATomatinaProjectile::Initialize: Traj=%d Dist=%.0f Duration=%.2f"),
		static_cast<int32>(Trajectory), Dist, FlightDuration);
}

// =============================================================================
// Tick
// =============================================================================

void ATomatinaProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (FlightDuration <= 0.f) { return; }

	FlightProgress += DeltaTime / FlightDuration;

	if (FlightProgress >= 1.0f)
	{
		OnHitCamera();
		Destroy();
		return;
	}

	const float Alpha = FlightProgress;
	FVector NewLoc    = FMath::Lerp(StartLocation, TargetLocation, Alpha);

	switch (Trajectory)
	{
	case ETomatoTrajectory::Straight:
		// 補間のみ（追加オフセットなし）
		break;

	case ETomatoTrajectory::Arc:
	{
		// 放物線（0→最高点→0）
		const float ArcOffset = FMath::Sin(Alpha * PI) * ArcHeight;
		NewLoc.Z += ArcOffset;
		break;
	}

	case ETomatoTrajectory::Curve:
	{
		// 横カーブ（サイン波）
		const FVector FlightDir = (TargetLocation - StartLocation).GetSafeNormal();
		const FVector Right     = FVector::CrossProduct(FlightDir, FVector::UpVector).GetSafeNormal();
		const float CurveOffset = FMath::Sin(Alpha * PI) * CurveStrength * CurveDirection;
		NewLoc += Right * CurveOffset;
		break;
	}
	}

	SetActorLocation(NewLoc);

	// 進行方向を向く
	const FVector Dir = (TargetLocation - NewLoc).GetSafeNormal();
	if (!Dir.IsNearlyZero())
	{
		SetActorRotation(Dir.Rotation());
	}
}

// =============================================================================
// OnHitCamera
// =============================================================================

void ATomatinaProjectile::OnHitCamera()
{
	// 着弾位置はランダム（画面端を避けた 0.1〜0.9 の範囲）
	const FVector2D SplatPos(
		FMath::RandRange(0.1f, 0.9f),
		FMath::RandRange(0.1f, 0.9f));
	const float SplatSize = FMath::RandRange(SplatSizeMin, SplatSizeMax);

	if (ATomatoDirtManager* DirtMgr = GetDirtManager())
	{
		DirtMgr->AddDirt(SplatPos, SplatSize);
	}

	UE_LOG(LogTemp, Log,
		TEXT("ATomatinaProjectile::OnHitCamera: pos=(%.2f,%.2f) size=%.3f"),
		SplatPos.X, SplatPos.Y, SplatSize);

	// 着弾エフェクト・SE は BP 派生クラスで BlueprintNativeEvent として追加可能
}

// =============================================================================
// GetDirtManager（キャッシュ付き）
// =============================================================================

ATomatoDirtManager* ATomatinaProjectile::GetDirtManager()
{
	if (!CachedDirtManager)
	{
		AActor* Found = UGameplayStatics::GetActorOfClass(GetWorld(), ATomatoDirtManager::StaticClass());
		CachedDirtManager = Cast<ATomatoDirtManager>(Found);

		if (!CachedDirtManager)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("ATomatinaProjectile::GetDirtManager: ATomatoDirtManager がレベル上に見つかりません"));
		}
	}
	return CachedDirtManager;
}
