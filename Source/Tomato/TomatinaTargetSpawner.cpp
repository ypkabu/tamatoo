// Fill out your copyright notice in the Description page of Project Settings.

#include "TomatinaTargetSpawner.h"

#include "Engine/World.h"

#if WITH_EDITOR
#include "DrawDebugHelpers.h"
#endif

// =============================================================================
// コンストラクタ
// =============================================================================

ATomatinaTargetSpawner::ATomatinaTargetSpawner()
{
	// スポーンはすべてミッション駆動なので Tick は不要
	PrimaryActorTick.bCanEverTick = false;
}

// =============================================================================
// OnConstruction（エディタ専用：デバッグ可視化）
// =============================================================================

#if WITH_EDITOR
void ATomatinaTargetSpawner::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	DrawDebugVisualization();
}

void ATomatinaTargetSpawner::DrawDebugVisualization()
{
	if (!GetWorld()) { return; }

	FlushPersistentDebugLines(GetWorld());

	// プロファイルごとに色を変える
	const TArray<FColor> Colors = {
		FColor::Red, FColor::Green, FColor::Blue,
		FColor::Yellow, FColor::Cyan, FColor::Magenta,
		FColor::Orange, FColor::Purple
	};

	for (int32 i = 0; i < SpawnProfiles.Num(); ++i)
	{
		const FSpawnProfile& Profile = SpawnProfiles[i];
		const FColor& Col = Colors[i % Colors.Num()];

		// SpawnLocations を球で描画
		for (const FVector& Loc : Profile.SpawnLocations)
		{
			DrawDebugSphere(GetWorld(), Loc, 50.0f, 12, Col, /*bPersistent=*/true);
		}

		// RunStart→RunEnd を線と球で描画
		const int32 RunPairs = FMath::Min(
			Profile.RunStartPoints.Num(),
			Profile.RunEndPoints.Num());
		for (int32 j = 0; j < RunPairs; ++j)
		{
			DrawDebugSphere(GetWorld(), Profile.RunStartPoints[j], 30.0f, 8, Col,          true);
			DrawDebugSphere(GetWorld(), Profile.RunEndPoints[j],   30.0f, 8, FColor::White, true);
			DrawDebugLine(GetWorld(),
				Profile.RunStartPoints[j], Profile.RunEndPoints[j],
				Col, /*bPersistent=*/true, -1.0f, 0, 3.0f);
		}

		// ArcCenterPoints を球で描画
		for (const FVector& Pt : Profile.ArcCenterPoints)
		{
			DrawDebugSphere(GetWorld(), Pt, 40.0f, 8, Col, true);
		}
	}
}
#endif

// =============================================================================
// SpawnTargetsForMission
// =============================================================================

void ATomatinaTargetSpawner::SpawnTargetsForMission(const FMissionData& Mission)
{
	// 既存ターゲットを全消去
	DestroyAllTargets();

	// 候補クラスリストを構築（TargetClassVariants 優先、無ければ TargetClass 単体）
	TArray<TSubclassOf<ATomatinaTargetBase>> Candidates;
	for (const TSubclassOf<ATomatinaTargetBase>& C : Mission.TargetClassVariants)
	{
		if (C) { Candidates.Add(C); }
	}
	if (Candidates.Num() == 0 && Mission.TargetClass)
	{
		Candidates.Add(Mission.TargetClass);
	}

	if (Candidates.Num() == 0)
	{
		UE_LOG(LogTemp, Error,
			TEXT("ATomatinaTargetSpawner::SpawnTargetsForMission: "
			     "TargetClass も TargetClassVariants も未設定"));
		return;
	}

	// プロファイルを検索
	const FSpawnProfile* Profile = nullptr;
	for (const FSpawnProfile& P : SpawnProfiles)
	{
		if (P.ProfileName == Mission.SpawnProfileName)
		{
			Profile = &P;
			break;
		}
	}

	if (!Profile || Profile->SpawnLocations.Num() == 0)
	{
		UE_LOG(LogTemp, Error,
			TEXT("ATomatinaTargetSpawner::SpawnTargetsForMission: "
			     "SpawnProfile '%s' が見つからない、または SpawnLocations が空"),
			*Mission.SpawnProfileName.ToString());
		return;
	}

	// SpawnCount 体スポーン
	for (int32 i = 0; i < Mission.SpawnCount; ++i)
	{
		// ランダムにスポーン位置を選択
		const int32 LocIdx = FMath::RandRange(0, Profile->SpawnLocations.Num() - 1);
		const FVector SpawnLoc = Profile->SpawnLocations[LocIdx];

		// 回転（候補があれば選択、なければ ZeroRotator）
		FRotator SpawnRot = FRotator::ZeroRotator;
		if (Profile->SpawnRotations.Num() > 0)
		{
			SpawnRot = Profile->SpawnRotations[
				FMath::RandRange(0, Profile->SpawnRotations.Num() - 1)];
		}

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		// 候補クラスから毎スポーンごとにランダム 1 つ選択
		TSubclassOf<ATomatinaTargetBase> PickedClass =
			Candidates[FMath::RandRange(0, Candidates.Num() - 1)];

		ATomatinaTargetBase* Target = GetWorld()->SpawnActor<ATomatinaTargetBase>(
			PickedClass, SpawnLoc, SpawnRot, Params);

		if (!Target)
		{
			if (bDebugSpawnerLog)
			{
				UE_LOG(LogTemp, Warning,
					TEXT("ATomatinaTargetSpawner: SpawnActor に失敗 (i=%d)"), i);
			}
			continue;
		}

		// お題タグを明示的に設定（BP 側のデフォルト値を上書き）
		Target->MyType = Mission.TargetType;

		// ── RunAcross：走行経路を設定 ───────────────────────────────────────────
		// BeginPlay で OriginLocation = SpawnLoc になっているため
		//   Offset = WorldPoint - SpawnLoc → Tick 内で OriginLocation + Offset = WorldPoint
		if (Target->MovementType == ETargetMovement::RunAcross
			&& Profile->RunStartPoints.Num() > 0)
		{
			const int32 Idx = FMath::RandRange(0, Profile->RunStartPoints.Num() - 1);
			Target->RunStartOffset = Profile->RunStartPoints[Idx] - SpawnLoc;
			Target->RunEndOffset   = Profile->RunEndPoints.IsValidIndex(Idx)
			                         ? Profile->RunEndPoints[Idx] - SpawnLoc
			                         : Target->RunEndOffset;
		}

		// ── FlyArc：弧の中心点へアクターを移動 ─────────────────────────────────
		// OriginLocation は BeginPlay 時の SpawnLoc に固定されており、
		// SetActorLocation は視覚的な初期配置のみ変更する
		if (Target->MovementType == ETargetMovement::FlyArc
			&& Profile->ArcCenterPoints.Num() > 0)
		{
			const int32 Idx = FMath::RandRange(0, Profile->ArcCenterPoints.Num() - 1);
			Target->SetActorLocation(Profile->ArcCenterPoints[Idx]);
		}

		ActiveTargets.Add(Target);

		if (bDebugSpawnerLog)
		{
			UE_LOG(LogTemp, Log,
				TEXT("ATomatinaTargetSpawner: スポーン完了 Type=%s Class=%s Loc=(%.0f,%.0f,%.0f)"),
				*Mission.TargetType.ToString(),
				*GetNameSafe(PickedClass),
				SpawnLoc.X, SpawnLoc.Y, SpawnLoc.Z);
		}
	}

	if (bDebugSpawnerLog)
	{
		UE_LOG(LogTemp, Log,
			TEXT("ATomatinaTargetSpawner::SpawnTargetsForMission: '%s' 計 %d 体"),
			*Mission.SpawnProfileName.ToString(), ActiveTargets.Num());
	}
}

// =============================================================================
// DestroyAllTargets
// =============================================================================

void ATomatinaTargetSpawner::DestroyAllTargets()
{
	for (ATomatinaTargetBase* Target : ActiveTargets)
	{
		if (IsValid(Target))
		{
			Target->Destroy();
		}
	}
	ActiveTargets.Empty();
}

// =============================================================================
// RemoveTarget
// =============================================================================

void ATomatinaTargetSpawner::RemoveTarget(ATomatinaTargetBase* Target)
{
	ActiveTargets.Remove(Target);
	if (IsValid(Target))
	{
		Target->Destroy();
	}
}
