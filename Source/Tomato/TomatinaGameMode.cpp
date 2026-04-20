// Fill out your copyright notice in the Description page of Project Settings.

#include "TomatinaGameMode.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

#include "TomatinaFunctionLibrary.h"
#include "TomatinaHUD.h"
#include "TomatinaPlayerPawn.h"
#include "TomatinaTargetSpawner.h"
#include "TomatinaTowelSystem.h"
#include "TomatoDirtManager.h"

ATomatinaGameMode::ATomatinaGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
}

// =============================================================================
// BeginPlay — カウントダウンを開始
// =============================================================================
void ATomatinaGameMode::BeginPlay()
{
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaGameMode::BeginPlay 開始"));
	Super::BeginPlay();

	if (!GetWorld())
	{
		UE_LOG(LogTemp, Error, TEXT("ATomatinaGameMode::BeginPlay: GetWorld 失敗"));
		return;
	}

	// TargetSpawner 取得
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(), ATomatinaTargetSpawner::StaticClass(), Found);
	if (Found.Num() > 0)
	{
		TargetSpawner = Cast<ATomatinaTargetSpawner>(Found[0]);
		UE_LOG(LogTemp, Warning, TEXT("ATomatinaGameMode: TargetSpawner 取得"));
	}
	else
	{
		UE_LOG(LogTemp, Error,
			TEXT("ATomatinaGameMode: TargetSpawner がレベルに未配置"));
	}

	// ミッションシャッフル
	if (bRandomOrder) { ShuffleMissions(); }

	// 画面サイズを PlayerPawn から取得
	CachePlayerPawnSizes();

	// BGM 再生
	if (BGM)
	{
		UGameplayStatics::PlaySound2D(this, BGM);
	}

	// ── カウントダウン開始：世界停止 + HUD にカウントダウン数字を表示 ──
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.0f);

	bInCountdown        = true;
	CountdownRemaining  = 3.0f;
	LastCountdownSecond = 3;

	if (ATomatinaHUD* HUD = GetTomatinaHUD())
	{
		HUD->ShowCountdown(3);
	}

	UE_LOG(LogTemp, Warning,
		TEXT("ATomatinaGameMode::BeginPlay: カウントダウン開始 Missions=%d"),
		Missions.Num());
}

// =============================================================================
// Tick — カウントダウン・リザルト・制限時間を全て FApp 実時間で管理
// =============================================================================
void ATomatinaGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float RealDelta = FApp::GetDeltaTime();

	// ── カウントダウン中 ───────────────────────────────
	if (bInCountdown)
	{
		CountdownRemaining -= RealDelta;

		const int32 NewSecond = FMath::CeilToInt(CountdownRemaining);
		if (NewSecond != LastCountdownSecond && NewSecond > 0)
		{
			LastCountdownSecond = NewSecond;
			if (ATomatinaHUD* HUD = GetTomatinaHUD())
			{
				HUD->ShowCountdown(NewSecond);
			}
		}

		if (CountdownRemaining <= 0.f)
		{
			bInCountdown = false;
			UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);

			if (ATomatinaHUD* HUD = GetTomatinaHUD())
			{
				HUD->HideCountdown();
			}
			StartMission(0);
		}
		return;
	}

	// ── 撮影リザルト表示中（TimeDilation=0 なので FApp で計測） ──
	if (bIsShowingResult)
	{
		ResultElapsed += RealDelta;
		if (ResultElapsed >= ResultDisplayTime)
		{
			ResultElapsed    = 0.f;
			bIsShowingResult = false;

			UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
			if (ATomatinaHUD* HUD = GetTomatinaHUD())
			{
				HUD->HideResult();
			}

			StartMission(CurrentMissionIndex + 1);
		}
		return;
	}

	// ── ミッション結果（時間切れ）表示中 ─────────────────
	if (bIsShowingMissionResult)
	{
		MissionResultElapsed += RealDelta;
		if (MissionResultElapsed >= MissionResultDisplayTime)
		{
			MissionResultElapsed    = 0.f;
			bIsShowingMissionResult = false;

			if (ATomatinaHUD* HUD = GetTomatinaHUD())
			{
				HUD->HideMissionResult();
			}

			StartMission(CurrentMissionIndex + 1);
		}
		return;
	}

	// ── ミッション残り時間 ──────────────────────────────
	if (RemainingTime > 0.f)
	{
		RemainingTime -= DeltaSeconds; // 通常再生中なので DeltaSeconds で OK

		if (ATomatinaHUD* HUD = GetTomatinaHUD())
		{
			HUD->UpdateTimer(FMath::Max(0.f, RemainingTime));
		}

		if (RemainingTime <= 0.f)
		{
			RemainingTime = -1.f;

			UE_LOG(LogTemp, Warning, TEXT("ATomatinaGameMode: 時間切れ Mission=%d"),
				CurrentMissionIndex);

			if (ATomatinaHUD* HUD = GetTomatinaHUD())
			{
				HUD->ShowMissionResult(0, TEXT("時間切れ！"));
			}

			bIsShowingMissionResult = true;
			MissionResultElapsed    = 0.f;
		}
	}
}

// =============================================================================
// StartMission
// =============================================================================
void ATomatinaGameMode::StartMission(int32 Index)
{
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaGameMode::StartMission Index=%d"), Index);

	if (Index >= Missions.Num())
	{
		ShowFinalResult();
		return;
	}

	CurrentMissionIndex = Index;
	const FMissionData& Mission = Missions[Index];
	CurrentMission = Mission.TargetType;

	if (ATomatinaHUD* HUD = GetTomatinaHUD())
	{
		HUD->ShowMissionDisplay(Mission.DisplayText, Mission.TargetImage);
	}

	if (TargetSpawner)
	{
		TargetSpawner->SpawnTargetsForMission(Mission);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("StartMission: TargetSpawner が null"));
	}

	RemainingTime = (Mission.TimeLimit > 0.f) ? Mission.TimeLimit : -1.f;
}

// =============================================================================
// TakePhoto — 左クリック（ズーム中）で PlayerPawn から呼ばれる
// =============================================================================
void ATomatinaGameMode::TakePhoto(USceneCaptureComponent2D* ZoomCamera)
{
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaGameMode::TakePhoto 開始"));

	if (bIsShowingResult || bIsShowingMissionResult)
	{
		UE_LOG(LogTemp, Warning, TEXT("TakePhoto: リザルト表示中のためスキップ"));
		return;
	}
	if (!ZoomCamera || !GetWorld()) { return; }

	// ① ズーム映像 → RT_Photo
	UTomatinaFunctionLibrary::CopyZoomToPhoto(ZoomCamera, RT_Photo);

	// ② スコア計算
	static const TArray<ATomatinaTargetBase*> EmptyTargets;
	const TArray<ATomatinaTargetBase*>& Targets =
		TargetSpawner ? TargetSpawner->ActiveTargets : EmptyTargets;

	FPhotoResult Result = UTomatinaFunctionLibrary::CalculatePhotoScore(
		ZoomCamera, Targets, CurrentMission, PhoneWidth, PhoneHeight);

	int32 Score     = Result.Score;
	FString Comment = Result.Comment;

	// ③ タオル映り込み減点
	if (AActor* TowelActor = UGameplayStatics::GetActorOfClass(
			GetWorld(), ATomatinaTowelSystem::StaticClass()))
	{
		if (ATomatinaTowelSystem* Towel = Cast<ATomatinaTowelSystem>(TowelActor))
		{
			if (Towel->bTowelInZoomView)
			{
				Score = FMath::Max(0, Score + Towel->TowelPenalty);
				Comment += TEXT(" タオルが映っちゃった！");
				UE_LOG(LogTemp, Warning, TEXT("TakePhoto: タオル映り込みで減点 → %d"), Score);
			}
		}
	}

	// ③' 写真に写り込んだ汚れの個数分を減点（写真 = 撮影時の汚れ分布そのもの）
	TArray<FDirtSplat> ActiveDirts;
	if (AActor* DirtActor = UGameplayStatics::GetActorOfClass(
			GetWorld(), ATomatoDirtManager::StaticClass()))
	{
		if (ATomatoDirtManager* DirtMgr = Cast<ATomatoDirtManager>(DirtActor))
		{
			ActiveDirts = DirtMgr->GetActiveDirts();
			const int32 DirtCount = ActiveDirts.Num();
			if (DirtCount > 0 && DirtPenaltyPerSplat != 0)
			{
				const int32 Before = Score;
				Score = FMath::Max(0, Score + DirtCount * DirtPenaltyPerSplat);
				Comment += FString::Printf(TEXT(" 汚れ%d個で%d点減点"),
					DirtCount, Before - Score);
				UE_LOG(LogTemp, Warning,
					TEXT("TakePhoto: 汚れ %d 個で減点 %d → %d"),
					DirtCount, Before, Score);
			}
		}
	}

	CurrentScore = Score;
	TotalScore  += Score;

	UE_LOG(LogTemp, Warning, TEXT("TakePhoto: Score=%d Total=%d Dirts=%d"),
		Score, TotalScore, ActiveDirts.Num());

	// ④ 撮影成功なら BestTarget を Spawner から除去
	if (Score > 0 && TargetSpawner && Result.BestTarget)
	{
		TargetSpawner->RemoveTarget(Result.BestTarget);
	}

	// ⑤ シャッター音
	if (ShutterSound)
	{
		UGameplayStatics::PlaySound2D(this, ShutterSound);
	}

	// ⑥ ミッションタイマーを止める
	RemainingTime = -1.f;

	// ⑦ 世界停止 → HUD にフラッシュ・結果表示
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.0f);
	bIsShowingResult = true;
	ResultElapsed    = 0.f;

	if (ATomatinaHUD* HUD = GetTomatinaHUD())
	{
		HUD->PlayShutterFlash();
		HUD->ShowResult(Score, Comment, ActiveDirts);
		HUD->UpdateTotalScore(TotalScore);
	}
}

// =============================================================================
// ShowFinalResult — 全ミッション終了
// =============================================================================
void ATomatinaGameMode::ShowFinalResult()
{
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaGameMode::ShowFinalResult Total=%d"), TotalScore);

	if (ATomatinaHUD* HUD = GetTomatinaHUD())
	{
		HUD->ShowFinalResult(TotalScore, Missions.Num());
	}
}

// =============================================================================
// ShuffleMissions — Fisher-Yates + 同タイプ連続回避
// =============================================================================
void ATomatinaGameMode::ShuffleMissions()
{
	const int32 Num = Missions.Num();
	if (Num <= 1) { return; }

	for (int32 i = Num - 1; i > 0; --i)
	{
		const int32 j = FMath::RandRange(0, i);
		Missions.Swap(i, j);
	}

	for (int32 i = 0; i < Num - 1; ++i)
	{
		if (Missions[i].TargetType != Missions[i + 1].TargetType) { continue; }
		for (int32 k = i + 2; k < Num; ++k)
		{
			if (Missions[k].TargetType != Missions[i].TargetType)
			{
				Missions.Swap(i + 1, k);
				break;
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("ShuffleMissions: %d ミッションをシャッフル"), Num);
}

// =============================================================================
// GetTomatinaHUD — ヘルパー
// =============================================================================
ATomatinaHUD* ATomatinaGameMode::GetTomatinaHUD() const
{
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			return Cast<ATomatinaHUD>(PC->GetHUD());
		}
	}
	return nullptr;
}

// =============================================================================
// CachePlayerPawnSizes — PlayerPawn から 4 サイズを取得
// =============================================================================
void ATomatinaGameMode::CachePlayerPawnSizes()
{
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (ATomatinaPlayerPawn* Pawn = Cast<ATomatinaPlayerPawn>(PC->GetPawn()))
			{
				MainWidth   = Pawn->MainWidth;
				MainHeight  = Pawn->MainHeight;
				PhoneWidth  = Pawn->PhoneWidth;
				PhoneHeight = Pawn->PhoneHeight;
				UE_LOG(LogTemp, Warning,
					TEXT("ATomatinaGameMode: サイズ取得 Main=(%.0fx%.0f) Phone=(%.0fx%.0f)"),
					MainWidth, MainHeight, PhoneWidth, PhoneHeight);
			}
		}
	}
}
