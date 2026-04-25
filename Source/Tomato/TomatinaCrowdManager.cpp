// Fill out your copyright notice in the Description page of Project Settings.

#include "TomatinaCrowdManager.h"

#include "Engine/World.h"

#include "TomatinaCrowdMember.h"

// =============================================================================
// コンストラクタ
// =============================================================================

ATomatinaCrowdManager::ATomatinaCrowdManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

// =============================================================================
// BeginPlay
// =============================================================================

void ATomatinaCrowdManager::BeginPlay()
{
	Super::BeginPlay();
	SpawnAll();
}

// =============================================================================
// Tick — 補充モードのみ定期チェック
// =============================================================================

void ATomatinaCrowdManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	MaintenanceTimer -= DeltaTime;
	if (MaintenanceTimer <= 0.f)
	{
		MaintenanceTimer = 2.0f;
		TickMaintenance(DeltaTime);
	}
}

// =============================================================================
// SpawnAll
// =============================================================================

void ATomatinaCrowdManager::SpawnAll()
{
	for (const FCrowdVariant& V : Variants)
	{
		if (!V.MemberClass) { continue; }
		for (int32 i = 0; i < V.Count; ++i)
		{
			SpawnOne(V.MemberClass);
		}
	}

	UE_LOG(LogTemp, Warning,
		TEXT("ATomatinaCrowdManager: スポーン完了 計%d 体"),
		LiveMembers.Num());
}

// =============================================================================
// DespawnAll
// =============================================================================

void ATomatinaCrowdManager::DespawnAll()
{
	for (TObjectPtr<ATomatinaCrowdMember>& M : LiveMembers)
	{
		if (M && IsValid(M.Get())) { M->Destroy(); }
	}
	LiveMembers.Empty();
}

// =============================================================================
// TickMaintenance — bMaintainCount が true の Variant を補充
// =============================================================================

void ATomatinaCrowdManager::TickMaintenance(float DeltaTime)
{
	// 死亡分を除外
	LiveMembers.RemoveAll([](const TObjectPtr<ATomatinaCrowdMember>& T)
	{
		return !T || !IsValid(T.Get());
	});

	for (const FCrowdVariant& V : Variants)
	{
		if (!V.MemberClass || !V.bMaintainCount) { continue; }

		int32 Alive = 0;
		for (const TObjectPtr<ATomatinaCrowdMember>& M : LiveMembers)
		{
			if (M && M->GetClass() == V.MemberClass) { ++Alive; }
		}

		const int32 Need = V.Count - Alive;
		for (int32 i = 0; i < Need; ++i)
		{
			SpawnOne(V.MemberClass);
		}
	}
}

// =============================================================================
// SpawnOne
// =============================================================================

ATomatinaCrowdMember* ATomatinaCrowdManager::SpawnOne(TSubclassOf<ATomatinaCrowdMember> Cls)
{
	UWorld* World = GetWorld();
	if (!World || !Cls) { return nullptr; }

	const FVector Loc = PickSpawnLocation();
	const FRotator Rot(0.f, FMath::FRandRange(0.f, 360.f), 0.f);

	// Deferred spawn にして BeginPlay 前に DirtOverlayMaterial を流し込む。
	// （メンバー側の ApplyDirtOverlay は BeginPlay で動くため、ここで先に値を渡しておく）
	FTransform SpawnTransform(Rot, Loc);
	ATomatinaCrowdMember* M = World->SpawnActorDeferred<ATomatinaCrowdMember>(
		Cls,
		SpawnTransform,
		this,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!M) { return nullptr; }

	// メンバー側が未指定ならマネージャーの値を使う（個体上書きを尊重）
	if (!M->DirtOverlayMaterial && DirtOverlayMaterial)
	{
		M->DirtOverlayMaterial = DirtOverlayMaterial;
	}

	M->FinishSpawning(SpawnTransform);

	const FVector Center = GetActorLocation() + FVector(0.f, 0.f, GroundZOffset);
	M->InitializeFromManager(Center, AreaExtent);

	LiveMembers.Add(M);
	return M;
}

// =============================================================================
// PickSpawnLocation — 範囲内のランダム位置
// =============================================================================

FVector ATomatinaCrowdManager::PickSpawnLocation() const
{
	const FVector C = GetActorLocation();
	return FVector(
		C.X + FMath::FRandRange(-AreaExtent.X, AreaExtent.X),
		C.Y + FMath::FRandRange(-AreaExtent.Y, AreaExtent.Y),
		C.Z + GroundZOffset);
}
