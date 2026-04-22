// Fill out your copyright notice in the Description page of Project Settings.

#include "TomatinaThrowerSpawner.h"

#include "Engine/World.h"

#include "TomatinaThrower.h"

// =============================================================================
// コンストラクタ
// =============================================================================

ATomatinaThrowerSpawner::ATomatinaThrowerSpawner()
{
	PrimaryActorTick.bCanEverTick = true;
}

// =============================================================================
// BeginPlay
// =============================================================================

void ATomatinaThrowerSpawner::BeginPlay()
{
	Super::BeginPlay();

	NextSpawnTimer  = InitialDelay;
	CurrentInterval = StartingInterval;
	SpawnedCount    = 0;

	UE_LOG(LogTemp, Warning,
		TEXT("ATomatinaThrowerSpawner: BeginPlay InitialDelay=%.1f Start=%.1f Min=%.1f Max=%d"),
		InitialDelay, StartingInterval, MinInterval, MaxThrowers);
}

// =============================================================================
// Tick
// =============================================================================

void ATomatinaThrowerSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	NextSpawnTimer -= DeltaTime;
	if (NextSpawnTimer > 0.f) { return; }

	PruneDeadThrowers();

	if (LiveThrowers.Num() >= MaxThrowers)
	{
		// 上限に達してたら少しだけ待って再判定
		NextSpawnTimer = 1.0f;
		return;
	}

	SpawnThrowerNow();

	// 次の出現間隔を短くする
	CurrentInterval = FMath::Max(MinInterval, CurrentInterval - IntervalDecreasePerSpawn);
	NextSpawnTimer  = CurrentInterval + FMath::RandRange(-IntervalVariance, IntervalVariance);
	NextSpawnTimer  = FMath::Max(NextSpawnTimer, 0.5f);
}

// =============================================================================
// SpawnThrowerNow
// =============================================================================

ATomatinaThrower* ATomatinaThrowerSpawner::SpawnThrowerNow()
{
	UWorld* World = GetWorld();
	if (!World) { return nullptr; }

	TSubclassOf<ATomatinaThrower> Cls = PickThrowerClass();
	if (!Cls)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("ATomatinaThrowerSpawner: ThrowerVariants が空 or クラス未設定"));
		return nullptr;
	}

	const ESpawnEdge Edge     = PickEdge();
	const FVector    SpawnLoc = ComputeSpawnLocation(Edge);
	const FVector    DestLoc  = ComputeWalkDestination();

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ATomatinaThrower* Thrower = World->SpawnActor<ATomatinaThrower>(
		Cls, SpawnLoc, FRotator::ZeroRotator, Params);

	if (!Thrower)
	{
		UE_LOG(LogTemp, Warning, TEXT("ATomatinaThrowerSpawner: SpawnActor 失敗"));
		return nullptr;
	}

	// 共有狙い候補を引き継ぐ
	Thrower->SceneryAimActors    = SceneryAimActors;
	Thrower->SceneryAimLocations = SceneryAimLocations;

	Thrower->BeginWalkIn(DestLoc);

	LiveThrowers.Add(Thrower);
	SpawnedCount++;

	UE_LOG(LogTemp, Log,
		TEXT("ATomatinaThrowerSpawner: スポーン #%d Edge=%d From=(%.0f,%.0f) → Dest=(%.0f,%.0f)"),
		SpawnedCount, static_cast<int32>(Edge),
		SpawnLoc.X, SpawnLoc.Y, DestLoc.X, DestLoc.Y);

	return Thrower;
}

// =============================================================================
// PickThrowerClass — 重み付きランダム
// =============================================================================

TSubclassOf<ATomatinaThrower> ATomatinaThrowerSpawner::PickThrowerClass() const
{
	if (ThrowerVariants.Num() == 0) { return nullptr; }

	float Total = 0.f;
	for (const FThrowerVariant& V : ThrowerVariants)
	{
		if (V.ThrowerClass) { Total += FMath::Max(0.f, V.Weight); }
	}
	if (Total <= 0.f) { return nullptr; }

	float Roll = FMath::FRandRange(0.f, Total);
	for (const FThrowerVariant& V : ThrowerVariants)
	{
		if (!V.ThrowerClass) { continue; }
		Roll -= FMath::Max(0.f, V.Weight);
		if (Roll <= 0.f) { return V.ThrowerClass; }
	}
	return ThrowerVariants.Last().ThrowerClass;
}

// =============================================================================
// PickEdge — 有効なエッジから 1 つ選ぶ
// =============================================================================

ESpawnEdge ATomatinaThrowerSpawner::PickEdge() const
{
	TArray<ESpawnEdge> Pool;
	if (bUseLeftEdge)  { Pool.Add(ESpawnEdge::Left); }
	if (bUseRightEdge) { Pool.Add(ESpawnEdge::Right); }
	if (bUseBackEdge)  { Pool.Add(ESpawnEdge::Back); }

	if (Pool.Num() == 0) { return ESpawnEdge::Back; }
	return Pool[FMath::RandRange(0, Pool.Num() - 1)];
}

// =============================================================================
// ComputeSpawnLocation
// =============================================================================

FVector ATomatinaThrowerSpawner::ComputeSpawnLocation(ESpawnEdge Edge) const
{
	const FVector Center = GetActorLocation();
	FVector Out          = Center;
	Out.Z += GroundZOffset;

	const float HalfX = PlayAreaExtent.X;
	const float HalfY = PlayAreaExtent.Y;

	switch (Edge)
	{
	case ESpawnEdge::Left:
		Out.X = Center.X - HalfX - SpawnOutsideMargin;
		Out.Y = Center.Y + FMath::FRandRange(-HalfY, HalfY);
		break;
	case ESpawnEdge::Right:
		Out.X = Center.X + HalfX + SpawnOutsideMargin;
		Out.Y = Center.Y + FMath::FRandRange(-HalfY, HalfY);
		break;
	case ESpawnEdge::Back:
		Out.X = Center.X + FMath::FRandRange(-HalfX, HalfX);
		Out.Y = Center.Y + HalfY + SpawnOutsideMargin;
		break;
	}
	return Out;
}

// =============================================================================
// ComputeWalkDestination — 広場内のランダム地点
// =============================================================================

FVector ATomatinaThrowerSpawner::ComputeWalkDestination() const
{
	const FVector Center = GetActorLocation();
	FVector Out          = Center;
	Out.Z += GroundZOffset;
	Out.X = Center.X + FMath::FRandRange(-PlayAreaExtent.X, PlayAreaExtent.X);
	Out.Y = Center.Y + FMath::FRandRange(-PlayAreaExtent.Y, PlayAreaExtent.Y);
	return Out;
}

// =============================================================================
// PruneDeadThrowers
// =============================================================================

void ATomatinaThrowerSpawner::PruneDeadThrowers()
{
	LiveThrowers.RemoveAll([](const TObjectPtr<ATomatinaThrower>& T)
	{
		return !T || !IsValid(T.Get());
	});
}
