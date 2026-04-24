// Fill out your copyright notice in the Description page of Project Settings.

#include "TomatinaCrowdMember.h"

#include "Components/SkeletalMeshComponent.h"

// =============================================================================
// コンストラクタ
// =============================================================================

ATomatinaCrowdMember::ATomatinaCrowdMember()
{
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	SetRootComponent(MeshComp);

	// 撮影スコア判定の LineTrace（ECC_Visibility）に群衆が引っかかるように、
	// SkeletalMesh を QueryOnly + Visibility=Block で設定する。
	// これにより BP 側で Collision Profile を忘れていても遮蔽判定が機能する。
	MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MeshComp->SetCollisionObjectType(ECC_WorldDynamic);
	MeshComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	MeshComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	MeshComp->SetGenerateOverlapEvents(false);
}

// =============================================================================
// BeginPlay
// =============================================================================

void ATomatinaCrowdMember::BeginPlay()
{
	Super::BeginPlay();

	// 遮蔽判定で二重保険として HidingProp タグを付与
	// （CheckVisibility 側で HidingProp Actor は遮蔽とみなされる）
	Tags.AddUnique(TEXT("HidingProp"));

	// バラけ用：開始時に少しランダム待機
	StartJitterTimer = FMath::FRandRange(0.f, MaxStartJitter);

	// 左右ペーシングモードでは現在位置より遠い端を最初のターゲットにすると自然
	if (bMoveLeftRightOnly)
	{
		const FVector Cur = GetActorLocation();
		if (bLeftRightUsesYAxis)
		{
			bPacingHeadingPositive = (Cur.Y <= AreaCenter.Y);
		}
		else
		{
			bPacingHeadingPositive = (Cur.X <= AreaCenter.X);
		}
		WanderTarget = PickNextPacingTarget();
	}
	else
	{
		WanderTarget = PickRandomPointInArea();
	}

	CurrentAction    = ECrowdAction::Wander;
	CurrentSpeed     = WanderSpeed;
	ActionCheckTimer = ActionCheckInterval + FMath::FRandRange(-ActionCheckVariance, ActionCheckVariance);
	bIsMoving        = true;

	OnEnterWander();
}

// =============================================================================
// Tick
// =============================================================================

void ATomatinaCrowdMember::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (StartJitterTimer > 0.f)
	{
		StartJitterTimer -= DeltaTime;
		return;
	}

	// 左右ペーシングモードでは Burst/Cheer 抽選を完全に無効化。
	// ランダム要素ゼロで端から端を往復するだけ。
	if (!bMoveLeftRightOnly)
	{
		// ── 状態継続管理（Burst / Cheer）─────────────────────────
		if (CurrentAction == ECrowdAction::Burst || CurrentAction == ECrowdAction::Cheer)
		{
			ActionTimer -= DeltaTime;
			if (ActionTimer <= 0.f)
			{
				EnterAction(ECrowdAction::Wander);
			}
		}

		// ── 徘徊中の行動切替抽選 ────────────────────────────────
		if (CurrentAction == ECrowdAction::Wander)
		{
			ActionCheckTimer -= DeltaTime;
			if (ActionCheckTimer <= 0.f)
			{
				ActionCheckTimer = ActionCheckInterval + FMath::FRandRange(-ActionCheckVariance, ActionCheckVariance);

				const float Roll = FMath::FRand();
				if (Roll < BurstChance)
				{
					EnterAction(ECrowdAction::Burst);
				}
				else if (Roll < BurstChance + CheerChance)
				{
					EnterAction(ECrowdAction::Cheer);
				}
				else
				{
					WanderTarget = PickRandomPointInArea();
				}
			}
		}
	}

	TickMovement(DeltaTime);
}

// =============================================================================
// EnterAction
// =============================================================================

void ATomatinaCrowdMember::EnterAction(ECrowdAction NewAction)
{
	CurrentAction = NewAction;

	switch (NewAction)
	{
	case ECrowdAction::Wander:
		WanderTarget = PickRandomPointInArea();
		CurrentSpeed = WanderSpeed;
		bIsMoving    = true;
		OnEnterWander();
		break;

	case ECrowdAction::Burst:
		ActionTimer  = BurstDuration;
		WanderTarget = PickRandomPointInArea();   // 走り先もランダムに更新
		CurrentSpeed = BurstSpeed;
		bIsMoving    = true;
		OnEnterBurst();
		break;

	case ECrowdAction::Cheer:
		ActionTimer  = CheerDuration;
		CurrentSpeed = 0.f;
		bIsMoving    = false;
		OnEnterCheer();
		break;
	}
}

// =============================================================================
// TickMovement
// =============================================================================

void ATomatinaCrowdMember::TickMovement(float DeltaTime)
{
	if (CurrentAction == ECrowdAction::Cheer)
	{
		// その場で踊る（位置は動かさない、回転だけ少しさせて雰囲気出す）
		FRotator R = GetActorRotation();
		R.Yaw += 60.f * DeltaTime;   // ゆっくり回る
		SetActorRotation(R);
		return;
	}

	const FVector Cur     = GetActorLocation();
	const FVector ToDest  = WanderTarget - Cur;
	const float   DistSqr = ToDest.SizeSquared2D();

	if (DistSqr <= ArriveTolerance * ArriveTolerance)
	{
		// 到着 → 次の点を選ぶ
		if (bMoveLeftRightOnly)
		{
			// 反対側の端に切り替え (完全ペーシング)
			bPacingHeadingPositive = !bPacingHeadingPositive;
			WanderTarget = PickNextPacingTarget();
		}
		else
		{
			WanderTarget = PickRandomPointInArea();
		}
		return;
	}

	FVector Dir = ToDest.GetSafeNormal2D();
	if (Dir.IsNearlyZero()) { return; }

	// 左右ペーシングモードでは Burst ジッターを完全に無効化。軸を固定するだけ。
	if (bMoveLeftRightOnly)
	{
		if (bLeftRightUsesYAxis) { Dir.X = 0.f; }
		else                     { Dir.Y = 0.f; }
		Dir = Dir.GetSafeNormal2D();
		if (Dir.IsNearlyZero()) { return; }
	}
	else if (CurrentAction == ECrowdAction::Burst && BurstJitter > 0.f)
	{
		const FVector Jitter = FMath::VRand() * BurstJitter;
		Dir = (Dir + Jitter * 0.01f).GetSafeNormal2D();
	}

	// 進行方向へ向く（メッシュ natural forward が +X でない場合 MeshYawOffset で補正）
	FRotator Look = Dir.Rotation();
	Look.Pitch = 0.f;
	Look.Roll  = 0.f;
	Look.Yaw  += MeshYawOffset;
	SetActorRotation(Look);

	SetActorLocation(Cur + Dir * CurrentSpeed * DeltaTime, /*bSweep=*/false);
}

// =============================================================================
// PickRandomPointInArea
// =============================================================================

FVector ATomatinaCrowdMember::PickRandomPointInArea()
{
	// 左右ペーシングモードの呼び出しは PickNextPacingTarget に委譲
	if (bMoveLeftRightOnly)
	{
		return PickNextPacingTarget();
	}

	const float X = AreaCenter.X + FMath::FRandRange(-AreaExtent.X, AreaExtent.X);
	const float Y = AreaCenter.Y + FMath::FRandRange(-AreaExtent.Y, AreaExtent.Y);
	return FVector(X, Y, AreaCenter.Z);
}

// =============================================================================
// PickNextPacingTarget — 左右ペーシング用。bPacingHeadingPositive 方向の端を返す
// =============================================================================

FVector ATomatinaCrowdMember::PickNextPacingTarget()
{
	const FVector Cur = GetActorLocation();
	const float Sign = bPacingHeadingPositive ? 1.f : -1.f;

	if (bLeftRightUsesYAxis)
	{
		const float TargetY = AreaCenter.Y + Sign * AreaExtent.Y;
		return FVector(Cur.X, TargetY, AreaCenter.Z);
	}
	else
	{
		const float TargetX = AreaCenter.X + Sign * AreaExtent.X;
		return FVector(TargetX, Cur.Y, AreaCenter.Z);
	}
}

// =============================================================================
// InitializeFromManager
// =============================================================================

void ATomatinaCrowdMember::InitializeFromManager(FVector Center, FVector Extent)
{
	AreaCenter = Center;
	AreaExtent = Extent;

	if (bMoveLeftRightOnly)
	{
		// スポーン位置から遠い端をまず目指す
		const FVector Cur = GetActorLocation();
		if (bLeftRightUsesYAxis)
		{
			bPacingHeadingPositive = (Cur.Y <= AreaCenter.Y);
		}
		else
		{
			bPacingHeadingPositive = (Cur.X <= AreaCenter.X);
		}
		WanderTarget = PickNextPacingTarget();
	}
	else
	{
		WanderTarget = PickRandomPointInArea();
	}
}
