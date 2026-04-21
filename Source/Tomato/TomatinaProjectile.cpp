// Fill out your copyright notice in the Description page of Project Settings.

#include "TomatinaProjectile.h"

#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

#include "TomatoDirtManager.h"
#include "TomatinaPlayerPawn.h"

// =============================================================================
// コンストラクタ
// =============================================================================

ATomatinaProjectile::ATomatinaProjectile()
	: CachedDirtManager(nullptr)
{
	PrimaryActorTick.bCanEverTick = true;

	TomatoMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TomatoMesh"));
	SetRootComponent(TomatoMesh);

	// オーバーラップで判定する（プレイヤーポーンと当たれば OnMeshOverlap）
	TomatoMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TomatoMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	TomatoMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TomatoMesh->SetGenerateOverlapEvents(true);
}

// =============================================================================
// BeginPlay
// =============================================================================

void ATomatinaProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (TomatoMesh)
	{
		TomatoMesh->OnComponentBeginOverlap.AddDynamic(this, &ATomatinaProjectile::OnMeshOverlap);
	}
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

	if (bHasHit) { return; }

	// 寿命を超えたら（プレイヤーに当たらず的外れ）破棄。汚れは生成しない
	ElapsedTime += DeltaTime;
	if (ElapsedTime >= MaxLifetime)
	{
		UE_LOG(LogTemp, Log, TEXT("ATomatinaProjectile: 寿命切れで破棄（プレイヤー未命中）"));
		Destroy();
		return;
	}

	if (FlightDuration <= 0.f) { return; }

	// 軌道沿いに移動を続ける（命中判定は OnMeshOverlap が担当）
	FlightProgress += DeltaTime / FlightDuration;

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

	// Sweep を有効にして移動中のオーバーラップを確実に拾う
	SetActorLocation(NewLoc, /*bSweep=*/true);

	// 進行方向を向く
	const FVector Dir = (TargetLocation - NewLoc).GetSafeNormal();
	if (!Dir.IsNearlyZero())
	{
		SetActorRotation(Dir.Rotation());
	}
}

// =============================================================================
// OnMeshOverlap — プレイヤーポーンに当たったときだけ汚れ生成
// =============================================================================

void ATomatinaProjectile::OnMeshOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (bHasHit || !OtherActor || OtherActor == this) { return; }

	// プレイヤーポーン（ATomatinaPlayerPawn）以外は無視
	if (!Cast<ATomatinaPlayerPawn>(OtherActor)) { return; }

	bHasHit = true;

	UE_LOG(LogTemp, Log, TEXT("ATomatinaProjectile::OnMeshOverlap: プレイヤー命中 → 汚れ生成"));

	OnHitCamera();
	Destroy();
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
