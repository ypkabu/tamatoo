// Fill out your copyright notice in the Description page of Project Settings.

#include "TomatinaThrower.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetMathLibrary.h"

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

	UE_LOG(LogTemp, Warning,
		TEXT("ATomatinaThrower [%s]: BeginPlay StartDelay=%.2f Interval=%.2f"),
		*GetName(), StartDelay, ThrowInterval);
}

// =============================================================================
// Tick
// =============================================================================

void ATomatinaThrower::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// ── StartDelay ─────────────────────────────────────────────────────────
	if (bStartDelayActive)
	{
		StartDelayTimer -= DeltaTime;
		if (StartDelayTimer <= 0.f)
		{
			bStartDelayActive = false;
		}
		return;
	}

	// ── 振りかぶり中（モンタージュ再生済み・発射待ち） ────────────────────
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

	// ── 次の投擲までのタイマー ─────────────────────────────────────────────
	ThrowTimer -= DeltaTime;
	if (ThrowTimer <= 0.f)
	{
		ThrowTimer = ThrowInterval + FMath::RandRange(-ThrowIntervalVariance, ThrowIntervalVariance);
		StartThrow();
	}
}

// =============================================================================
// StartThrow
// =============================================================================

void ATomatinaThrower::StartThrow()
{
	// プレイヤーの方を向く
	if (bRotateToPlayerOnThrow)
	{
		const FVector AimLoc = GetAimTargetLocation();
		FRotator LookRot     = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), AimLoc);
		LookRot.Pitch = 0.f;
		LookRot.Roll  = 0.f;
		SetActorRotation(LookRot);
	}

	// モンタージュ再生
	if (ThrowMontage && MeshComp)
	{
		if (UAnimInstance* AnimInst = MeshComp->GetAnimInstance())
		{
			AnimInst->Montage_Play(ThrowMontage, ThrowMontagePlayRate);
			UE_LOG(LogTemp, Log, TEXT("ATomatinaThrower [%s]: ThrowMontage 再生"), *GetName());
		}
		else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("ATomatinaThrower [%s]: AnimInstance が無い（AnimBP 未設定）"),
				*GetName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("ATomatinaThrower [%s]: ThrowMontage 未設定 → モーション無しで発射"),
			*GetName());
	}

	// ReleaseDelay 後に発射するためのタイマー設定
	// 0 以下なら即発射（Anim Notify を使う場合は手動で ReleaseTomato を呼ぶ運用）
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
		UE_LOG(LogTemp, Warning,
			TEXT("ATomatinaThrower [%s]: ProjectileClass が未設定"), *GetName());
		return;
	}

	// 手のソケットからスポーン位置を取得
	FVector SpawnLoc;
	if (MeshComp && MeshComp->DoesSocketExist(HandSocketName))
	{
		SpawnLoc = MeshComp->GetSocketLocation(HandSocketName);
	}
	else
	{
		SpawnLoc = GetActorLocation() + GetActorForwardVector() * 50.f + FVector(0.f, 0.f, 100.f);
		UE_LOG(LogTemp, Warning,
			TEXT("ATomatinaThrower [%s]: ソケット '%s' が見つからない → アクター位置から発射"),
			*GetName(), *HandSocketName.ToString());
	}

	const FVector AimLoc = bAimAtPlayer
		? GetAimTargetLocation()
		: SpawnLoc + GetActorForwardVector() * 1500.f;

	const ETomatoTrajectory Traj = PickTrajectory();

	UWorld* World = GetWorld();
	if (!World) { return; }

	ATomatinaProjectile* Tomato = World->SpawnActor<ATomatinaProjectile>(
		ProjectileClass, SpawnLoc, FRotator::ZeroRotator);

	if (!Tomato)
	{
		UE_LOG(LogTemp, Warning, TEXT("ATomatinaThrower [%s]: SpawnActor 失敗"), *GetName());
		return;
	}

	Tomato->Initialize(AimLoc, Traj);

	if (Traj == ETomatoTrajectory::Curve)
	{
		Tomato->CurveDirection = (FMath::FRand() > 0.5f) ? 1.0f : -1.0f;
	}

	UE_LOG(LogTemp, Log,
		TEXT("ATomatinaThrower [%s]: 発射 Traj=%d From=(%.0f,%.0f,%.0f) → (%.0f,%.0f,%.0f)"),
		*GetName(), static_cast<int32>(Traj),
		SpawnLoc.X, SpawnLoc.Y, SpawnLoc.Z,
		AimLoc.X, AimLoc.Y, AimLoc.Z);
}

// =============================================================================
// PickTrajectory
// =============================================================================

ETomatoTrajectory ATomatinaThrower::PickTrajectory() const
{
	const float Roll = FMath::FRand();
	if (Roll < StraightChance)
	{
		return ETomatoTrajectory::Straight;
	}
	if (Roll < StraightChance + ArcChance)
	{
		return ETomatoTrajectory::Arc;
	}
	return ETomatoTrajectory::Curve;
}

// =============================================================================
// GetAimTargetLocation
// =============================================================================

FVector ATomatinaThrower::GetAimTargetLocation() const
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
