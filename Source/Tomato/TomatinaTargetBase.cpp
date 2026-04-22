// Fill out your copyright notice in the Description page of Project Settings.

#include "TomatinaTargetBase.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"

// =============================================================================
// コンストラクタ
// =============================================================================

ATomatinaTargetBase::ATomatinaTargetBase()
{
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	SetRootComponent(MeshComp);

	MarkerWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("MarkerWidgetComp"));
	MarkerWidgetComp->SetupAttachment(MeshComp);
	MarkerWidgetComp->SetWidgetSpace(EWidgetSpace::Screen); // 常にカメラを向く
	MarkerWidgetComp->SetDrawSize(FVector2D(128.f, 128.f));
	MarkerWidgetComp->SetRelativeLocation(FVector(0.f, 0.f, 160.f));
}

// =============================================================================
// BeginPlay
// =============================================================================

void ATomatinaTargetBase::BeginPlay()
{
	Super::BeginPlay();

	OriginLocation = GetActorLocation();
	DelayTimer     = StartDelay;
	bActive        = (StartDelay <= 0.f);

	switch (MovementType)
	{
	case ETargetMovement::DepthHideAndSeek:
	{
		// 初期状態は奥に隠れた位置
		FVector Loc = OriginLocation;
		Loc.Y += HiddenY;
		SetActorLocation(Loc);
		StateTimer  = HideDuration + FMath::RandRange(-1.0f, 1.0f);
		bIsShowing  = false;
		break;
	}
	case ETargetMovement::RunAcross:
	{
		SetActorLocation(OriginLocation + RunStartOffset);
		RunProgress = 0.f;
		break;
	}
	case ETargetMovement::FlyArc:
	{
		ArcAngle = 0.f;
		break;
	}
	case ETargetMovement::FloatErratic:
	{
		FloatTarget = OriginLocation + FMath::VRand() * FloatRadius;
		break;
	}
	case ETargetMovement::BlendWithCrowd:
	{
		ElapsedTime = 0.f;
		break;
	}
	}

	// StartDelay 中はメッシュを非表示にする
	if (!bActive && MeshComp)
	{
		MeshComp->SetVisibility(false);
	}

	// 目印ウィジェットの初期設定
	if (MarkerWidgetComp)
	{
		if (MarkerWidgetClass)
		{
			MarkerWidgetComp->SetWidgetClass(MarkerWidgetClass);
		}
		MarkerWidgetComp->SetDrawSize(MarkerDrawSize);
		MarkerWidgetComp->SetRelativeLocation(FVector(0.f, 0.f, MeshHeight + MarkerHeightOffset));
		MarkerWidgetComp->SetVisibility(bShowMarker && MarkerWidgetClass != nullptr);
	}

	// アウトライン用の CustomDepth Stencil を有効化（BP のポストプロセスマテリアルで参照）
	if (MeshComp && bEnableOutlineStencil)
	{
		MeshComp->SetRenderCustomDepth(true);
		MeshComp->SetCustomDepthStencilValue(OutlineStencilValue);
	}

	UE_LOG(LogTemp, Warning, TEXT("ATomatinaTargetBase [%s]: BeginPlay Pattern=%d Delay=%.1f"),
		*GetName(), static_cast<int32>(MovementType), StartDelay);
}

// =============================================================================
// Tick
// =============================================================================

void ATomatinaTargetBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// ── StartDelay ─────────────────────────────────────────────────────────
	if (!bActive)
	{
		DelayTimer -= DeltaTime;
		if (DelayTimer <= 0.f)
		{
			bActive = true;
			if (MeshComp) { MeshComp->SetVisibility(true); }
			UE_LOG(LogTemp, Warning, TEXT("ATomatinaTargetBase [%s]: Delay 終了・アクティブ化"), *GetName());
		}
		return;
	}

	// ── ポーズ管理（true: 凍結中なので movement Tick をスキップ） ─────────────
	if (TickPose(DeltaTime))
	{
		return;
	}

	// ── 各パターンへ委譲 ────────────────────────────────────────────────────
	switch (MovementType)
	{
	case ETargetMovement::DepthHideAndSeek: TickDepthHideAndSeek(DeltaTime); break;
	case ETargetMovement::RunAcross:        TickRunAcross(DeltaTime);        break;
	case ETargetMovement::FlyArc:           TickFlyArc(DeltaTime);           break;
	case ETargetMovement::FloatErratic:     TickFloatErratic(DeltaTime);     break;
	case ETargetMovement::BlendWithCrowd:   TickBlendWithCrowd(DeltaTime);   break;
	}
}

// =============================================================================
// TickPose
// 抽選で一時停止 → 一定時間経過で復帰。ポーズ中は movement Tick をスキップさせる
// =============================================================================

bool ATomatinaTargetBase::TickPose(float DeltaTime)
{
	if (!bEnablePose)
	{
		return false;
	}

	// すでにポーズ中：継続時間を消費して終了判定
	if (bIsPosing)
	{
		PoseTimer -= DeltaTime;
		if (PoseTimer <= 0.f)
		{
			bIsPosing      = false;
			PoseCheckTimer = 0.f;
			OnPoseEnded();
			UE_LOG(LogTemp, Warning, TEXT("ATomatinaTargetBase [%s]: ポーズ終了"), *GetName());
		}
		return true; // ポーズ中は movement を凍結
	}

	// 抽選インターバル
	PoseCheckTimer += DeltaTime;
	if (PoseCheckTimer >= PoseCheckInterval)
	{
		PoseCheckTimer = 0.f;
		if (FMath::FRand() < PoseChance)
		{
			bIsPosing = true;
			PoseTimer = PoseDuration;
			OnPoseStarted();
			UE_LOG(LogTemp, Warning, TEXT("ATomatinaTargetBase [%s]: ポーズ開始 (%.2f 秒・倍率 x%.2f)"),
				*GetName(), PoseDuration, PoseScoreMultiplier);
			return true;
		}
	}

	return false;
}

// =============================================================================
// TickDepthHideAndSeek
// =============================================================================

void ATomatinaTargetBase::TickDepthHideAndSeek(float DeltaTime)
{
	StateTimer -= DeltaTime;

	const float TargetLocalY = bIsShowing ? ShownY : HiddenY;
	FVector TargetLoc = OriginLocation;
	TargetLoc.Y += TargetLocalY;

	const FVector NewLoc = FMath::VInterpTo(GetActorLocation(), TargetLoc, DeltaTime, DepthInterpSpeed);
	SetActorLocation(NewLoc);

	if (StateTimer <= 0.f)
	{
		bIsShowing = !bIsShowing;
		StateTimer = bIsShowing
			? ShowDuration + FMath::RandRange(-0.5f, 0.5f)
			: HideDuration + FMath::RandRange(-1.0f, 1.0f);

		UE_LOG(LogTemp, Warning, TEXT("ATomatinaTargetBase [%s]: DepthHideAndSeek → %s (%.2f 秒)"),
			*GetName(), bIsShowing ? TEXT("表示") : TEXT("隠れ"), StateTimer);
	}
}

// =============================================================================
// TickRunAcross
// =============================================================================

void ATomatinaTargetBase::TickRunAcross(float DeltaTime)
{
	const float Distance = (RunEndOffset - RunStartOffset).Size();
	if (Distance < KINDA_SMALL_NUMBER) { return; }

	RunProgress += (MoveSpeed / Distance) * DeltaTime;

	if (RunProgress >= 1.0f)
	{
		if (bLoop)
		{
			RunProgress = 0.f;
			UE_LOG(LogTemp, Warning, TEXT("ATomatinaTargetBase [%s]: RunAcross ループ"), *GetName());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ATomatinaTargetBase [%s]: RunAcross 終了 → Destroy"), *GetName());
			Destroy();
			return;
		}
	}

	FVector NewLoc = OriginLocation + FMath::Lerp(RunStartOffset, RunEndOffset, RunProgress);
	NewLoc.Z += RunHeight;
	SetActorLocation(NewLoc);

	// 進行方向を向く
	const FVector Dir = (RunEndOffset - RunStartOffset).GetSafeNormal();
	if (!Dir.IsNearlyZero())
	{
		SetActorRotation(Dir.Rotation());
	}
}

// =============================================================================
// TickFlyArc
// =============================================================================

void ATomatinaTargetBase::TickFlyArc(float DeltaTime)
{
	// ホバリング中
	if (bIsHovering)
	{
		HoverTimer -= DeltaTime;
		if (HoverTimer <= 0.f)
		{
			bIsHovering = false;
		}
		return;
	}

	ArcAngle += MoveSpeed * DeltaTime;

	if (ArcAngle >= 180.0f)
	{
		if (bLoop)
		{
			ArcAngle = 0.f;

			// ホバリング抽選
			if (HoverDuration > 0.f && FMath::FRand() < HoverChance)
			{
				bIsHovering = true;
				HoverTimer  = HoverDuration;
				UE_LOG(LogTemp, Warning, TEXT("ATomatinaTargetBase [%s]: FlyArc ホバリング開始 (%.2f 秒)"),
					*GetName(), HoverDuration);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ATomatinaTargetBase [%s]: FlyArc 終了 → Destroy"), *GetName());
			Destroy();
			return;
		}
	}

	const float Rad = FMath::DegreesToRadians(ArcAngle);
	const float X   = FMath::Cos(Rad) * ArcRadius;
	const float Z   = FMath::Sin(Rad) * ArcHeight;
	// X: 横移動、Z: 高さ。Y はプロジェクト座標系に応じてエディタで調整
	SetActorLocation(OriginLocation + FVector(X, 0.f, Z));
}

// =============================================================================
// TickFloatErratic
// =============================================================================

void ATomatinaTargetBase::TickFloatErratic(float DeltaTime)
{
	// 目標へ補間しながら浮遊
	FVector NewLoc = FMath::VInterpTo(GetActorLocation(), FloatTarget, DeltaTime, 2.0f);

	// 微小な揺れを重ねる
	NewLoc += FVector(
		FMath::Sin(ElapsedTime * 3.0f) * 10.0f,
		FMath::Cos(ElapsedTime * 2.5f) * 10.0f,
		FMath::Sin(ElapsedTime * 2.0f) * 5.0f);

	SetActorLocation(NewLoc);
	ElapsedTime += DeltaTime;

	// 目標に近づいたら次の目標を設定
	if (FVector::Dist(NewLoc, FloatTarget) < 50.0f)
	{
		FloatTarget = OriginLocation + FMath::VRand() * FloatRadius;
	}

	// ワープ抽選
	WarpTimer += DeltaTime;
	if (WarpTimer >= WarpInterval)
	{
		WarpTimer = 0.f;
		if (FMath::FRand() < WarpChance)
		{
			FloatTarget = OriginLocation + FMath::VRand() * FloatRadius;
			SetActorLocation(FloatTarget);
			UE_LOG(LogTemp, Warning, TEXT("ATomatinaTargetBase [%s]: FloatErratic ワープ"), *GetName());
		}
	}
}

// =============================================================================
// TickBlendWithCrowd
// =============================================================================

void ATomatinaTargetBase::TickBlendWithCrowd(float DeltaTime)
{
	ElapsedTime += DeltaTime;

	FVector NewLoc = GetActorLocation();

	// 前方向へ歩く
	NewLoc += GetActorForwardVector() * WalkSpeed * DeltaTime;

	// 奥行きをサイン波でゆらす
	const float DepthSway = FMath::Sin(ElapsedTime * DepthSwayFrequency * 2.0f * PI)
	                        * DepthSwayAmount;
	NewLoc.Y = OriginLocation.Y + DepthSway;

	SetActorLocation(NewLoc);

	// 走行範囲（RunStartOffset / RunEndOffset を流用）を超えたらループ
	const FVector Delta = NewLoc - OriginLocation;
	const FVector EndWorld = OriginLocation + RunEndOffset;
	if (FVector::Dist(NewLoc, OriginLocation) > (RunEndOffset - RunStartOffset).Size())
	{
		// 反対端（RunStartOffset 側）へ瞬間移動
		SetActorLocation(OriginLocation + RunStartOffset);
		UE_LOG(LogTemp, Warning, TEXT("ATomatinaTargetBase [%s]: BlendWithCrowd ループ"), *GetName());
	}
}

// =============================================================================
// GetHeadLocation / GetRootLocation
// =============================================================================

FVector ATomatinaTargetBase::GetHeadLocation() const
{
	return GetActorLocation() + FVector(0.f, 0.f, MeshHeight);
}

FVector ATomatinaTargetBase::GetRootLocation() const
{
	return GetActorLocation();
}
