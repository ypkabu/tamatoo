// Fill out your copyright notice in the Description page of Project Settings.

#include "TomatinaCrowdMember.h"

#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

#include "TomatinaGameMode.h"

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
	MeshComp->PrimaryComponentTick.bTickEvenWhenPaused = true;
	MeshComp->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
}

// =============================================================================
// BeginPlay
// =============================================================================

void ATomatinaCrowdMember::BeginPlay()
{
	Super::BeginPlay();

	// 汚れオーバーレイを適用（DirtOverlayMaterial が設定されているときのみ）。
	// 既存マテリアルに DirtAmount Scalar Parameter を仕込まずに、
	// 1 個の汚れマテリアルで全キャラに対応するための仕組み。
	ApplyDirtOverlay();

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

	const ATomatinaGameMode* GameMode = Cast<ATomatinaGameMode>(
		UGameplayStatics::GetGameMode(this));
	const bool bUseRealDelta = (GameMode && (GameMode->bInCountdown || GameMode->bIsLoading));
	const float EffectiveDeltaTime = bUseRealDelta
		? FApp::GetDeltaTime()
		: DeltaTime;

	if (bUseRealDelta && MeshComp && EffectiveDeltaTime > 0.f)
	{
		// GlobalTimeDilation=0 中は SkeletalMeshComponent の通常アニメ Tick が 0 秒になりやすい。
		// 観客だけはカウントダウン中も動かしたいので、実時間でポーズを進める。
		MeshComp->TickAnimation(EffectiveDeltaTime, false);
		MeshComp->RefreshBoneTransforms();
	}

	// 汚れ累積（常に進行。待機中でも時間は経過する）
	if (DirtAccumulationTime > 0.f && CurrentDirtAmount < MaxDirtAmount && MeshComp)
	{
		const float Rate = MaxDirtAmount / DirtAccumulationTime;
		CurrentDirtAmount = FMath::Min(CurrentDirtAmount + Rate * EffectiveDeltaTime, MaxDirtAmount);

		// オーバーレイ方式（推奨）：DMI に直接書く
		if (DirtOverlayMID)
		{
			DirtOverlayMID->SetScalarParameterValue(DirtParameterName, CurrentDirtAmount);
		}
		else
		{
			// フォールバック：旧方式（各マテリアルに DirtAmount を仕込んでいる場合）
			MeshComp->SetScalarParameterValueOnMaterials(DirtParameterName, CurrentDirtAmount);
		}
	}

	if (StartJitterTimer > 0.f)
	{
		StartJitterTimer -= EffectiveDeltaTime;
		return;
	}

	// 左右ペーシングモードでは Burst/Cheer 抽選を完全に無効化。
	// ランダム要素ゼロで端から端を往復するだけ。
	if (!bMoveLeftRightOnly)
	{
		// ── 状態継続管理（Burst / Cheer）─────────────────────────
		if (CurrentAction == ECrowdAction::Burst || CurrentAction == ECrowdAction::Cheer)
		{
			ActionTimer -= EffectiveDeltaTime;
			if (ActionTimer <= 0.f)
			{
				EnterAction(ECrowdAction::Wander);
			}
		}

		// ── 徘徊中の行動切替抽選 ────────────────────────────────
		if (CurrentAction == ECrowdAction::Wander)
		{
			ActionCheckTimer -= EffectiveDeltaTime;
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

	TickMovement(EffectiveDeltaTime);
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

	FVector Dir;

	// 左右ペーシングモードでは符号だけ取って軸を完全固定。
	// bLeftRightUsesYAxis の値に関わらず、ToDest の該当軸の符号で +/- を決める。
	if (bMoveLeftRightOnly)
	{
		if (bLeftRightUsesYAxis)
		{
			const float S = FMath::Sign(ToDest.Y);
			if (FMath::IsNearlyZero(S)) { return; }
			Dir = FVector(0.f, S, 0.f);
		}
		else
		{
			const float S = FMath::Sign(ToDest.X);
			if (FMath::IsNearlyZero(S)) { return; }
			Dir = FVector(S, 0.f, 0.f);
		}
	}
	else
	{
		Dir = ToDest.GetSafeNormal2D();
		if (Dir.IsNearlyZero()) { return; }

		if (CurrentAction == ECrowdAction::Burst && BurstJitter > 0.f)
		{
			const FVector Jitter = FMath::VRand() * BurstJitter;
			Dir = (Dir + Jitter * 0.01f).GetSafeNormal2D();
		}
	}

	// 進行方向へ向く（メッシュ natural forward が +X でない場合 MeshYawOffset で補正）
	FRotator Look = Dir.Rotation();
	Look.Pitch = 0.f;
	Look.Roll  = 0.f;
	Look.Yaw  += MeshYawOffset;

	// MeshComp の相対回転に残値があると Actor 回転を上書きしても見た目が変わらないため、0 に戻す保険
	if (bResetMeshRelativeRotation && MeshComp)
	{
		MeshComp->SetRelativeRotation(FRotator::ZeroRotator);
	}

	// 通常は Actor 回転で全体を回すが、BP 派生で Root が変わっている／AnimBP に
	// 上書きされる等の場合は MeshComp の WorldRotation を直接強制できるように切替可能
	if (bRotateMeshComponentDirectly && MeshComp)
	{
		MeshComp->SetWorldRotation(Look);
	}
	else
	{
		SetActorRotation(Look);
	}

	// 診断用ログ（毎秒 1 回）
	if (bDebugLogFacing)
	{
		DebugLogTimer += DeltaTime;
		if (DebugLogTimer >= 1.f)
		{
			DebugLogTimer = 0.f;
			const FRotator ActorRot = GetActorRotation();
			const FRotator MeshRot  = MeshComp ? MeshComp->GetComponentRotation() : FRotator::ZeroRotator;
			UE_LOG(LogTemp, Warning,
				TEXT("Crowd[%s] Dir=(%.2f,%.2f) TargetYaw=%.1f ActorYaw=%.1f MeshYaw=%.1f Offset=%.1f"),
				*GetName(), Dir.X, Dir.Y, Look.Yaw, ActorRot.Yaw, MeshRot.Yaw, MeshYawOffset);
		}
	}

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

// =============================================================================
// ApplyDirtOverlay
// SkeletalMeshComponent::SetOverlayMaterial に汚れ用 DMI を流し込む。
// 元のキャラ用マテリアルには手を加えない。
// =============================================================================
void ATomatinaCrowdMember::ApplyDirtOverlay()
{
	if (!MeshComp || !DirtOverlayMaterial) { return; }

	DirtOverlayMID = UMaterialInstanceDynamic::Create(DirtOverlayMaterial, this);
	if (!DirtOverlayMID) { return; }

	// 初期値 0（汚れなし）
	DirtOverlayMID->SetScalarParameterValue(DirtParameterName, 0.f);

	MeshComp->SetOverlayMaterial(DirtOverlayMID);
}
