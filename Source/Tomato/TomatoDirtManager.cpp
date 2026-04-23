// Fill out your copyright notice in the Description page of Project Settings.

#include "TomatoDirtManager.h"
#include "TomatinaHUD.h"
#include "GameFramework/PlayerController.h"

// ─────────────────────────────────────────────────────────────────────────────
ATomatoDirtManager::ATomatoDirtManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

// ─────────────────────────────────────────────────────────────────────────────
void ATomatoDirtManager::BeginPlay()
{
	Super::BeginPlay();

	// 最初のスポーンまでの時間を初期化
	SpawnTimer = SpawnInterval;
}

// ─────────────────────────────────────────────────────────────────────────────
void ATomatoDirtManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// ── 汚れスポーンタイマー（bEnableAutoSpawn が true のときだけ動く） ──
	if (bEnableAutoSpawn)
	{
		SpawnTimer -= DeltaTime;
		if (SpawnTimer <= 0.0f)
		{
			SpawnDirt();
			SpawnTimer = SpawnInterval + FMath::RandRange(-1.0f, 1.0f);
		}
	}

	// ── 自然減衰 ─────────────────────────────────────────────────────────
	bool bDecayChanged = false;
	for (FDirtSplat& Dirt : DirtSplats)
	{
		if (!Dirt.bActive) { continue; }

		if (Dirt.FadeSpeed > 0.0f)
		{
			Dirt.Opacity -= Dirt.FadeSpeed * DeltaTime;
			bDecayChanged = true;
			if (Dirt.Opacity <= 0.0f)
			{
				Dirt.Opacity  = 0.0f;
				Dirt.bActive  = false;
			}
		}
	}

	// ── 非アクティブなものを一括削除 ─────────────────────────────────────
	DirtSplats.RemoveAll([](const FDirtSplat& D) { return !D.bActive; });

	if (bDecayChanged)
	{
		NotifyHUD();
	}
}

// ─────────────────────────────────────────────────────────────────────────────
void ATomatoDirtManager::SpawnDirt()
{
	// MaxDirts に達していれば生成しない
	if (DirtSplats.Num() >= MaxDirts)
	{
		UE_LOG(LogTemp, Warning, TEXT("ATomatoDirtManager::SpawnDirt: MaxDirts (%d) に達しているためスキップ"), MaxDirts);
		return;
	}

	// SpawnRangeMin / SpawnRangeMax は 0〜1 の正規化範囲。メイン画面内の一部領域に限定可能
	const float MinX = FMath::Min(SpawnRangeMin.X, SpawnRangeMax.X);
	const float MaxX = FMath::Max(SpawnRangeMin.X, SpawnRangeMax.X);
	const float MinY = FMath::Min(SpawnRangeMin.Y, SpawnRangeMax.Y);
	const float MaxY = FMath::Max(SpawnRangeMin.Y, SpawnRangeMax.Y);

	FDirtSplat NewDirt;
	NewDirt.NormalizedPosition = FVector2D(FMath::RandRange(MinX, MaxX),
	                                       FMath::RandRange(MinY, MaxY));
	NewDirt.Opacity      = 1.0f;
	NewDirt.Size         = FMath::RandRange(SpawnSizeMin, SpawnSizeMax);
	NewDirt.FadeSpeed    = 0.0f;
	NewDirt.bActive      = true;
	NewDirt.TextureIndex = (NumDirtVariants > 1) ? FMath::RandRange(0, NumDirtVariants - 1) : 0;

	DirtSplats.Add(NewDirt);

	UE_LOG(LogTemp, Log, TEXT("ATomatoDirtManager::SpawnDirt: 汚れ生成 pos=(%.2f, %.2f) size=%.3f tex=%d 合計=%d"),
		NewDirt.NormalizedPosition.X, NewDirt.NormalizedPosition.Y,
		NewDirt.Size, NewDirt.TextureIndex, DirtSplats.Num());

	NotifyHUD();
}

// ─────────────────────────────────────────────────────────────────────────────
void ATomatoDirtManager::AddDirt(FVector2D NormPos, float Size)
{
	AddDirtWithType(NormPos, Size, EDirtType::NormalRed);
}

// ─────────────────────────────────────────────────────────────────────────────
void ATomatoDirtManager::AddDirtWithType(FVector2D NormPos, float Size, EDirtType InDirtType, int32 InRequiredDashCount, int32 InTextureIndex)
{
	// MaxDirts に達していたら追加しない（既存の汚れは消さない）
	if (DirtSplats.Num() >= MaxDirts)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("ATomatoDirtManager::AddDirtWithType: MaxDirts(%d) に達しているため追加スキップ"),
			MaxDirts);
		return;
	}

	// セーフティクランプ：暴走サイズが入ってきても画面が埋め尽くされないようにする
	float SafeSize = Size;
	if (MaxDirtSize > 0.f && SafeSize > MaxDirtSize)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("ATomatoDirtManager::AddDirt: Size=%.3f が MaxDirtSize=%.3f を超過 → クランプ"),
			SafeSize, MaxDirtSize);
		SafeSize = MaxDirtSize;
	}
	SafeSize = FMath::Max(SafeSize, 0.01f);

	// SpawnRange にトマト命中位置もクランプ
	const float RangeMinX = FMath::Min(SpawnRangeMin.X, SpawnRangeMax.X);
	const float RangeMaxX = FMath::Max(SpawnRangeMin.X, SpawnRangeMax.X);
	const float RangeMinY = FMath::Min(SpawnRangeMin.Y, SpawnRangeMax.Y);
	const float RangeMaxY = FMath::Max(SpawnRangeMin.Y, SpawnRangeMax.Y);
	const FVector2D ClampedPos(
		FMath::Clamp(NormPos.X, RangeMinX, RangeMaxX),
		FMath::Clamp(NormPos.Y, RangeMinY, RangeMaxY));

	const bool bIsSticky = (InDirtType == EDirtType::StickyYellowDash);
	int32 TextureIndex = 0;
	if (InTextureIndex >= 0)
	{
		TextureIndex = InTextureIndex;
	}
	else if (bIsSticky)
	{
		TextureIndex = StickyTextureIndex;
	}
	else
	{
		TextureIndex = (NumDirtVariants > 1) ? FMath::RandRange(0, NumDirtVariants - 1) : 0;
	}

	FDirtSplat NewDirt;
	NewDirt.NormalizedPosition = ClampedPos;
	NewDirt.Size               = SafeSize;
	NewDirt.Opacity            = 1.0f;
	NewDirt.FadeSpeed          = 0.0f;
	NewDirt.bActive            = true;
	NewDirt.TextureIndex       = TextureIndex;
	NewDirt.DirtType           = InDirtType;
	NewDirt.RequiredDashCount  = bIsSticky
		? FMath::Max(1, (InRequiredDashCount > 0) ? InRequiredDashCount : StickyRequiredDashCount)
		: 0;
	NewDirt.CurrentDashCount   = 0;
	NewDirt.LastDashTime       = -1000.0f;
	NewDirt.LastDashPos        = ClampedPos;

	DirtSplats.Add(NewDirt);

	UE_LOG(LogTemp, Log,
		TEXT("ATomatoDirtManager::AddDirtWithType: type=%d pos=(%.2f,%.2f) size=%.3f tex=%d reqDash=%d 合計=%d"),
		static_cast<int32>(InDirtType), NormPos.X, NormPos.Y, SafeSize,
		NewDirt.TextureIndex, NewDirt.RequiredDashCount, DirtSplats.Num());

	NotifyHUD();
}

// ─────────────────────────────────────────────────────────────────────────────
void ATomatoDirtManager::ClearDirtAt(FVector2D NormPos, float Radius)
{
	const float RadiusSq = Radius * Radius;
	int32 ClearedCount = 0;

	for (FDirtSplat& Dirt : DirtSplats)
	{
		if (!Dirt.bActive) { continue; }

		const float DistSq = FVector2D::DistSquared(Dirt.NormalizedPosition, NormPos);
		if (DistSq <= RadiusSq)
		{
			Dirt.Opacity = 0.0f;
			Dirt.bActive = false;
			++ClearedCount;
		}
	}

	if (ClearedCount > 0)
	{
		DirtSplats.RemoveAll([](const FDirtSplat& D) { return !D.bActive; });
		UE_LOG(LogTemp, Log, TEXT("ATomatoDirtManager::ClearDirtAt: %d 個の汚れを消去"), ClearedCount);
		NotifyHUD();
	}
}

// ─────────────────────────────────────────────────────────────────────────────
void ATomatoDirtManager::WipeDirtAt(FVector2D NormPos, float Radius, float Amount)
{
	bool bAnyChanged = false;
	const float NowRealTime = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.f;

	for (FDirtSplat& Dirt : DirtSplats)
	{
		if (!Dirt.bActive) { continue; }

		const float Dist        = FVector2D::Distance(Dirt.NormalizedPosition, NormPos);
		const float EffectRange = Radius + Dirt.Size * 0.5f;

		if (Dist <= EffectRange)
		{
			if (Dirt.DirtType == EDirtType::StickyYellowDash)
			{
				const bool bFastEnough = (Amount >= StickyDashMinAmount);
				const float MoveDist = FVector2D::Distance(NormPos, Dirt.LastDashPos);
				const bool bMovedEnough = (MoveDist >= StickyDashMinMoveDistance);

				if (bFastEnough && bMovedEnough)
				{
					const bool bChain = ((NowRealTime - Dirt.LastDashTime) <= StickyDashMaxInterval);
					Dirt.CurrentDashCount = bChain ? (Dirt.CurrentDashCount + 1) : 1;
					Dirt.LastDashTime = NowRealTime;
					Dirt.LastDashPos  = NormPos;
					bAnyChanged = true;

					const int32 NeedDashCount = FMath::Max(1, Dirt.RequiredDashCount);
					if (Dirt.CurrentDashCount >= NeedDashCount)
					{
						Dirt.Opacity = 0.0f;
						Dirt.bActive = false;
					}
					else
					{
						const float Progress = static_cast<float>(Dirt.CurrentDashCount) / static_cast<float>(NeedDashCount);
						Dirt.Opacity = FMath::Lerp(1.0f, StickyProgressMinOpacity, FMath::Clamp(Progress, 0.f, 1.f));
					}
				}
				else if (Dirt.CurrentDashCount > 0 && (NowRealTime - Dirt.LastDashTime) > StickyDashMaxInterval)
				{
					// テンポが切れたら連続回数をリセット
					Dirt.CurrentDashCount = 0;
					Dirt.Opacity = 1.0f;
					bAnyChanged = true;
				}
			}
			else
			{
				bAnyChanged = true;
				// 中心ほど効果大（距離に応じたフォールオフ）
				const float Falloff = 1.0f - (Dist / EffectRange);
				Dirt.Opacity -= Amount * Falloff;
				if (Dirt.Opacity <= 0.0f)
				{
					Dirt.Opacity = 0.0f;
					Dirt.bActive = false;
				}
			}
		}
		else if (Dirt.DirtType == EDirtType::StickyYellowDash
			&& Dirt.CurrentDashCount > 0
			&& (NowRealTime - Dirt.LastDashTime) > StickyDashMaxInterval)
		{
			// 離れている間にテンポ切れした場合もリセット
			Dirt.CurrentDashCount = 0;
			Dirt.Opacity = 1.0f;
			bAnyChanged = true;
		}
	}

	DirtSplats.RemoveAll([](const FDirtSplat& D) { return !D.bActive; });

	// 変化があれば毎フレーム HUD を更新（透明度がスムーズに下がって見える）
	if (bAnyChanged)
	{
		NotifyHUD();
	}
}

// ─────────────────────────────────────────────────────────────────────────────
TArray<FDirtSplat> ATomatoDirtManager::GetActiveDirts() const
{
	TArray<FDirtSplat> Result;
	for (const FDirtSplat& Dirt : DirtSplats)
	{
		if (Dirt.bActive)
		{
			Result.Add(Dirt);
		}
	}
	return Result;
}

// ─────────────────────────────────────────────────────────────────────────────
void ATomatoDirtManager::NotifyHUD()
{
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (ATomatinaHUD* HUD = Cast<ATomatinaHUD>(PC->GetHUD()))
		{
			HUD->UpdateDirtDisplay(DirtSplats);
			UE_LOG(LogTemp, Log, TEXT("ATomatoDirtManager::NotifyHUD: HUD に通知（汚れ数=%d）"), DirtSplats.Num());
		}
	}
}
