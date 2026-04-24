// Fill out your copyright notice in the Description page of Project Settings.

#include "TomatinaThrower.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetMathLibrary.h"

#include "TomatinaFunctionLibrary.h"
#include "TomatinaProjectile.h"

// =============================================================================
// コンストラクタ
// =============================================================================

ATomatinaThrower::ATomatinaThrower()
{
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	SetRootComponent(MeshComp);
}

// =============================================================================
// BeginPlay
// =============================================================================

void ATomatinaThrower::BeginPlay()
{
	Super::BeginPlay();

	StartDelayTimer   = StartDelay;
	bStartDelayActive = (StartDelay > 0.f);
	ThrowTimer        = ThrowInterval + FMath::RandRange(-ThrowIntervalVariance, ThrowIntervalVariance);

	// Spawner が BeginWalkIn を呼ばなければ即 Active 扱い（レベル直置きにも対応）
	if (!bHasDestination)
	{
		State      = EThrowerState::Active;
		bIsWalking = false;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("ATomatinaThrower [%s]: BeginPlay State=%d AimAtPlayerChance=%.2f Interval=%.2f"),
		*GetName(), static_cast<int32>(State), AimAtPlayerChance, ThrowInterval);
}

// =============================================================================
// Tick
// =============================================================================

void ATomatinaThrower::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	switch (State)
	{
	case EThrowerState::WalkingIn: TickWalk(DeltaTime);   break;
	case EThrowerState::Active:    TickActive(DeltaTime); break;
	}
}

// =============================================================================
// TickWalk — 目的地まで歩く
// =============================================================================

void ATomatinaThrower::TickWalk(float DeltaTime)
{
	const FVector Cur     = GetActorLocation();
	const FVector ToDest  = WalkDestination - Cur;
	const float   DistSqr = ToDest.SizeSquared2D();

	if (DistSqr <= ArriveTolerance * ArriveTolerance)
	{
		State      = EThrowerState::Active;
		bIsWalking = false;
		OnArrivedAtDestination();
		UE_LOG(LogTemp, Log, TEXT("ATomatinaThrower [%s]: 目的地到着"), *GetName());
		return;
	}

	const FVector Dir = ToDest.GetSafeNormal2D();
	if (Dir.IsNearlyZero()) { return; }

	// 進行方向を向く
	FRotator Look = Dir.Rotation();
	Look.Pitch = 0.f;
	Look.Roll  = 0.f;
	SetActorRotation(Look);

	SetActorLocation(Cur + Dir * WalkSpeed * DeltaTime, /*bSweep=*/false);
}

// =============================================================================
// TickActive — 通常時の投擲ループ
// =============================================================================

void ATomatinaThrower::TickActive(float DeltaTime)
{
	if (bStartDelayActive)
	{
		StartDelayTimer -= DeltaTime;
		if (StartDelayTimer <= 0.f) { bStartDelayActive = false; }
		return;
	}

	if (ReleaseTimer >= 0.f)
	{
		ReleaseTimer -= DeltaTime;
		if (ReleaseTimer <= 0.f)
		{
			ReleaseTimer = -1.f;
			ReleaseTomato();
		}
		return;
	}

	ThrowTimer -= DeltaTime;
	if (ThrowTimer <= 0.f)
	{
		ThrowTimer = ThrowInterval + FMath::RandRange(-ThrowIntervalVariance, ThrowIntervalVariance);
		StartThrow();
	}
}

// =============================================================================
// BeginWalkIn
// =============================================================================

void ATomatinaThrower::BeginWalkIn(FVector Destination)
{
	WalkDestination = Destination;
	bHasDestination = true;
	State           = EThrowerState::WalkingIn;
	bIsWalking      = true;

	UE_LOG(LogTemp, Log,
		TEXT("ATomatinaThrower [%s]: 歩行開始 → (%.0f,%.0f,%.0f)"),
		*GetName(), Destination.X, Destination.Y, Destination.Z);
}

// =============================================================================
// StartThrow
// =============================================================================

void ATomatinaThrower::StartThrow()
{
	// このフレームで狙い先を確定（投擲時とリリース時で対象を一致させるため）
	PendingAimLocation = PickAimLocation(bPendingAimAtPlayer);

	// 体を狙う方向に向ける
	FRotator LookRot = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), PendingAimLocation);
	LookRot.Pitch = 0.f;
	LookRot.Roll  = 0.f;
	SetActorRotation(LookRot);

	if (ThrowMontage && MeshComp)
	{
		if (UAnimInstance* AnimInst = MeshComp->GetAnimInstance())
		{
			AnimInst->Montage_Play(ThrowMontage, ThrowMontagePlayRate);
		}
		else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("ATomatinaThrower [%s]: AnimInstance が無い（AnimBP 未設定）"), *GetName());
		}
	}

	OnThrowStarted();

	// 投擲開始 SE（Thrower 位置から 3D 再生）
	UTomatinaFunctionLibrary::PlayTomatinaCueAtLocation(
		this, ThrowStartSound, GetActorLocation());

	if (ReleaseDelay > 0.f)
	{
		ReleaseTimer = ReleaseDelay;
	}
	else
	{
		ReleaseTomato();
	}
}

// =============================================================================
// ReleaseTomato
// =============================================================================

void ATomatinaThrower::ReleaseTomato()
{
	if (!ProjectileClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ATomatinaThrower [%s]: ProjectileClass 未設定"), *GetName());
		return;
	}

	FVector SpawnLoc;
	if (MeshComp && MeshComp->DoesSocketExist(HandSocketName))
	{
		SpawnLoc = MeshComp->GetSocketLocation(HandSocketName);
	}
	else
	{
		SpawnLoc = GetActorLocation() + GetActorForwardVector() * 50.f + FVector(0.f, 0.f, 100.f);
	}

	// StartThrow で確定済みの狙い先を使う（無ければここで再選択）
	bool bAimPlayer = bPendingAimAtPlayer;
	FVector AimLoc  = PendingAimLocation;
	if (AimLoc.IsZero())
	{
		AimLoc = PickAimLocation(bAimPlayer);
	}
	PendingAimLocation = FVector::ZeroVector;

	const ETomatoTrajectory Traj = PickTrajectory();

	UWorld* World = GetWorld();
	if (!World) { return; }

	ATomatinaProjectile* Tomato = World->SpawnActor<ATomatinaProjectile>(
		ProjectileClass, SpawnLoc, FRotator::ZeroRotator);
	if (!Tomato) { return; }

	// リリース SE（手から離れた位置で 3D 再生）
	UTomatinaFunctionLibrary::PlayTomatinaCueAtLocation(
		this, ThrowReleaseSound, SpawnLoc);

	// プレイヤー狙いか街狙いかを Projectile に伝える
	Tomato->bAimedAtPlayer = bAimPlayer;

	Tomato->Initialize(AimLoc, Traj);

	if (Traj == ETomatoTrajectory::Curve)
	{
		Tomato->CurveDirection = (FMath::FRand() > 0.5f) ? 1.f : -1.f;
	}

	UE_LOG(LogTemp, Log,
		TEXT("ATomatinaThrower [%s]: 発射 Traj=%d → (%.0f,%.0f,%.0f)"),
		*GetName(), static_cast<int32>(Traj), AimLoc.X, AimLoc.Y, AimLoc.Z);
}

// =============================================================================
// PickAimLocation — プレイヤー or 街中アクター/座標
// =============================================================================

FVector ATomatinaThrower::PickAimLocation(bool& bOutAimAtPlayer)
{
	const float Roll = FMath::FRand();
	bOutAimAtPlayer  = (Roll < AimAtPlayerChance);

	if (bOutAimAtPlayer)
	{
		return GetPlayerLocation();
	}

	// プレイヤー以外を狙う
	const int32 NumActors    = SceneryAimActors.Num();
	const int32 NumLocations = SceneryAimLocations.Num();
	const int32 Total        = NumActors + NumLocations;

	if (Total <= 0)
	{
		// シーナリ未設定の場合はランダム散布、それもオフならプレイヤー
		if (bUseRandomScatter)
		{
			return GetRandomScatterLocation();
		}
		bOutAimAtPlayer = true;
		return GetPlayerLocation();
	}

	const int32 Idx = FMath::RandRange(0, Total - 1);
	if (Idx < NumActors)
	{
		AActor* A = SceneryAimActors[Idx];
		if (IsValid(A)) { return A->GetActorLocation(); }
	}
	else
	{
		return SceneryAimLocations[Idx - NumActors];
	}
	bOutAimAtPlayer = true;
	return GetPlayerLocation();
}

// =============================================================================
// GetRandomScatterLocation — 自分の周辺ランダム位置
// =============================================================================

FVector ATomatinaThrower::GetRandomScatterLocation() const
{
	const float Yaw   = FMath::FRandRange(0.f, 360.f);
	const float Dist  = FMath::FRandRange(RandomScatterMinDistance, RandomScatterMaxDistance);
	const float Z     = FMath::FRandRange(-RandomScatterHeightRange, RandomScatterHeightRange);
	const FVector Dir = FRotator(0.f, Yaw, 0.f).Vector();
	return GetActorLocation() + Dir * Dist + FVector(0.f, 0.f, Z);
}

// =============================================================================
// PickTrajectory
// =============================================================================

ETomatoTrajectory ATomatinaThrower::PickTrajectory() const
{
	const float Roll = FMath::FRand();
	if (Roll < StraightChance)              { return ETomatoTrajectory::Straight; }
	if (Roll < StraightChance + ArcChance)  { return ETomatoTrajectory::Arc; }
	return ETomatoTrajectory::Curve;
}

// =============================================================================
// GetPlayerLocation
// =============================================================================

FVector ATomatinaThrower::GetPlayerLocation() const
{
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (APawn* Pawn = PC->GetPawn())
			{
				return Pawn->GetActorLocation();
			}
		}
	}
	return GetActorLocation() + GetActorForwardVector() * 1500.f;
}
