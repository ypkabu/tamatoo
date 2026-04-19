// Fill out your copyright notice in the Description page of Project Settings.

#include "TomatinaGameMode.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/PlayerController.h"

#include "TomatinaFunctionLibrary.h"
#include "TomatinaHUD.h"
#include "TomatinaTowelSystem.h"
#include "TomatinaTargetSpawner.h"

// =============================================================================
// コンストラクタ
// =============================================================================

ATomatinaGameMode::ATomatinaGameMode()
{
}

// =============================================================================
// BeginPlay
// =============================================================================

void ATomatinaGameMode::BeginPlay()
{
	Super::BeginPlay();

	// レベル上の Spawner を検索して取得
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(), ATomatinaTargetSpawner::StaticClass(), Found);

	if (Found.Num() > 0)
	{
		TargetSpawner = Cast<ATomatinaTargetSpawner>(Found[0]);
		UE_LOG(LogTemp, Log, TEXT("ATomatinaGameMode::BeginPlay: TargetSpawner 取得"));
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("ATomatinaGameMode::BeginPlay: ATomatinaTargetSpawner がレベル上に見つかりません"));
	}

	// ランダム順が有効ならシャッフル
	if (bRandomOrder)
	{
		ShuffleMissions();
	}

	// 最初のミッション開始
	StartMission(0);
}

// =============================================================================
// StartMission
// =============================================================================

void ATomatinaGameMode::StartMission(int32 Index)
{
	if (Index >= Missions.Num())
	{
		ShowFinalResult();
		return;
	}

	CurrentMissionIndex = Index;
	const FMissionData& Mission = Missions[Index];
	CurrentMission = Mission.TargetType;

	UE_LOG(LogTemp, Log,
		TEXT("ATomatinaGameMode::StartMission: Index=%d Type=%s"),
		Index, *CurrentMission.ToString());

	// HUD にお題テキストとプレビュー画像を表示
	if (ATomatinaHUD* HUD = GetTomatinaHUD())
	{
		HUD->ShowMissionDisplay(Mission.DisplayText, Mission.TargetImage);
	}

	// ターゲットをスポーン
	if (TargetSpawner)
	{
		TargetSpawner->SpawnTargetsForMission(Mission);
	}

	// 制限時間タイマー開始（前のタイマーをクリアしてから）
	GetWorld()->GetTimerManager().ClearTimer(MissionTimerHandle);
	if (Mission.TimeLimit > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(
			MissionTimerHandle,
			this,
			&ATomatinaGameMode::OnMissionTimeUp,
			Mission.TimeLimit,
			false);
	}
}

// =============================================================================
// OnMissionTimeUp（制限時間切れ）
// =============================================================================

void ATomatinaGameMode::OnMissionTimeUp()
{
	UE_LOG(LogTemp, Warning,
		TEXT("ATomatinaGameMode::OnMissionTimeUp: 時間切れ Index=%d"),
		CurrentMissionIndex);

	if (ATomatinaHUD* HUD = GetTomatinaHUD())
	{
		HUD->ShowMissionResult(0, TEXT("時間切れ！"));
	}

	// 2 秒後に次のミッションへ進む
	FTimerHandle TempHandle;
	FTimerDelegate Delegate;
	Delegate.BindLambda([this]()
	{
		if (ATomatinaHUD* H = GetTomatinaHUD()) { H->HideMissionResult(); }
		StartMission(CurrentMissionIndex + 1);
	});
	GetWorld()->GetTimerManager().SetTimer(TempHandle, Delegate, 2.0f, false);
}

// =============================================================================
// TakePhoto
// =============================================================================

void ATomatinaGameMode::TakePhoto(USceneCaptureComponent2D* ZoomCamera)
{
	if (bIsShowingResult)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("ATomatinaGameMode::TakePhoto: リザルト表示中のためスキップ"));
		return;
	}

	if (!ZoomCamera)
	{
		UE_LOG(LogTemp, Warning, TEXT("ATomatinaGameMode::TakePhoto: ZoomCamera が null"));
		return;
	}

	// ① ズームカメラの映像を RT_Photo にコピー
	UTomatinaFunctionLibrary::CopyZoomToPhoto(ZoomCamera, RT_Photo);

	// ② スコア計算（Spawner のアクティブターゲットを渡す）
	static const TArray<ATomatinaTargetBase*> EmptyTargets;
	const TArray<ATomatinaTargetBase*>& Targets =
		TargetSpawner ? TargetSpawner->ActiveTargets : EmptyTargets;

	FPhotoResult Result = UTomatinaFunctionLibrary::CalculatePhotoScore(
		ZoomCamera, Targets, CurrentMission, PhoneWidth, PhoneHeight);

	int32 Score      = Result.Score;
	FString Comment  = Result.Comment;

	// ── タオル映り込み減点 ─────────────────────────────────────────────────────
	AActor* TowelActor = UGameplayStatics::GetActorOfClass(
		GetWorld(), ATomatinaTowelSystem::StaticClass());
	if (ATomatinaTowelSystem* Towel = Cast<ATomatinaTowelSystem>(TowelActor))
	{
		if (Towel->bTowelInZoomView)
		{
			Score  += Towel->TowelPenalty;   // デフォルト -20
			Score   = FMath::Max(Score, 0);
			Comment += TEXT(" タオルが映っちゃった！");
			UE_LOG(LogTemp, Warning,
				TEXT("ATomatinaGameMode::TakePhoto: タオル映り込み減点 → %d"), Score);
		}
	}

	CurrentScore  = Score;
	TotalScore   += Score;

	UE_LOG(LogTemp, Warning,
		TEXT("ATomatinaGameMode::TakePhoto: Score=%d TotalScore=%d"), Score, TotalScore);

	// ③ 制限時間タイマーを止める
	GetWorld()->GetTimerManager().ClearTimer(MissionTimerHandle);

	// ④ 撮影成功（Score > 0）なら最高貢献ターゲットを消す
	if (Score > 0 && TargetSpawner && Result.BestTarget)
	{
		TargetSpawner->RemoveTarget(Result.BestTarget);
	}

	// ⑤ スローモーション（時間停止）
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.0f);

	// ⑥ リザルトフラグを立てる
	bIsShowingResult = true;

	// ⑦ HUD にリザルト表示を依頼
	if (ATomatinaHUD* HUD = GetTomatinaHUD())
	{
		HUD->ShowResult(Score, Comment);
	}

	// ⑧ リザルトタイマーをセット
	GetWorld()->GetTimerManager().SetTimer(
		ResultTimerHandle,
		this,
		&ATomatinaGameMode::OnResultTimerEnd,
		ResultDisplayTime,
		false);
}

// =============================================================================
// OnResultTimerEnd
// =============================================================================

void ATomatinaGameMode::OnResultTimerEnd()
{
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaGameMode::OnResultTimerEnd"));

	// 時間を元に戻す
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
	bIsShowingResult = false;

	if (ATomatinaHUD* HUD = GetTomatinaHUD())
	{
		HUD->HideResult();
	}

	// 次のミッションへ
	StartMission(CurrentMissionIndex + 1);
}

// =============================================================================
// ShowFinalResult
// =============================================================================

void ATomatinaGameMode::ShowFinalResult()
{
	UE_LOG(LogTemp, Warning,
		TEXT("ATomatinaGameMode::ShowFinalResult: 全ミッション完了 TotalScore=%d"),
		TotalScore);

	if (ATomatinaHUD* HUD = GetTomatinaHUD())
	{
		const FString Comment = FString::Printf(
			TEXT("ゲーム終了！合計スコア: %d"), TotalScore);
		HUD->ShowResult(TotalScore, Comment);
	}
}

// =============================================================================
// ShuffleMissions
// =============================================================================

void ATomatinaGameMode::ShuffleMissions()
{
	const int32 Num = Missions.Num();
	if (Num <= 1) { return; }

	// Fisher-Yates シャッフル
	for (int32 i = Num - 1; i > 0; --i)
	{
		const int32 j = FMath::RandRange(0, i);
		Missions.Swap(i, j);
	}

	// 同じ TargetType が連続しないように調整
	// 隣り合うペアを見つけたら後ろのものをさらに後ろの異なる要素と入れ替える
	for (int32 i = 0; i < Num - 1; ++i)
	{
		if (Missions[i].TargetType != Missions[i + 1].TargetType) { continue; }

		// i+1 と入れ替えられる候補（TargetType が Missions[i] と異なる）を探す
		bool bSwapped = false;
		for (int32 k = i + 2; k < Num; ++k)
		{
			if (Missions[k].TargetType != Missions[i].TargetType)
			{
				Missions.Swap(i + 1, k);
				bSwapped = true;
				break;
			}
		}

		if (!bSwapped)
		{
			// 後ろに候補がなければ前方向を探す
			for (int32 k = 0; k < i; ++k)
			{
				if (Missions[k].TargetType != Missions[i].TargetType
					&& (k == 0 || Missions[k - 1].TargetType != Missions[i + 1].TargetType))
				{
					Missions.Swap(i + 1, k);
					break;
				}
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("ATomatinaGameMode::ShuffleMissions: シャッフル完了 (%d ミッション)"), Num);
}

// =============================================================================
// GetTomatinaHUD（ヘルパー）
// =============================================================================

ATomatinaHUD* ATomatinaGameMode::GetTomatinaHUD() const
{
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	return PC ? Cast<ATomatinaHUD>(PC->GetHUD()) : nullptr;
}
