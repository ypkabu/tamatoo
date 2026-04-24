// Fill out your copyright notice in the Description page of Project Settings.

#include "TomatinaGameMode.h"

#include "Components/AudioComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/UObjectIterator.h"

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

	// スタイリッシュ初期化
	StylishGauge = 0.f;
	StylishRank = EStylishRank::C;
	StylishComboCount = 0;
	DirtCoverage01 = 0.f;
	LastHighScoreShotTime = -1000.f;

	// BGM 再生（StopAllSoundsExceptBGM で判別するために AudioComponent を保持）
	if (BGM)
	{
		BGMAudioComp = UGameplayStatics::SpawnSound2D(this, BGM);
	}

	// 観客ざわめきループ（最終リザルト溜めで StopAllSoundsExceptBGM により停止される）
	if (CrowdAmbient)
	{
		CrowdAudioComp = UGameplayStatics::SpawnSound2D(
			this, CrowdAmbient, CrowdAmbientVolume);
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
	PushStylishStateToHUD();

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
			// カウントダウン各秒の SE
			UTomatinaFunctionLibrary::PlayTomatinaCue2D(this, CountdownTickSound);
		}

		if (CountdownRemaining <= 0.f)
		{
			bInCountdown = false;
			UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);

			if (ATomatinaHUD* HUD = GetTomatinaHUD())
			{
				HUD->HideCountdown();
			}
			// スタートの合図 SE
			UTomatinaFunctionLibrary::PlayTomatinaCue2D(this, CountdownGoSound);
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

			if (bLastPhotoSucceeded)
			{
				// 成功 → 次ミッションへ
				StartMission(CurrentMissionIndex + 1);
			}
			// 失敗時は同ミッション継続（ターゲットは生きたまま、残り時間も継続）
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

	// ── 最終リザルト前の溜め（BGM のみ残して静止中） ─────
	if (bIsBuildingUpFinalResult)
	{
		FinalBuildupElapsed += RealDelta;
		if (FinalBuildupElapsed >= FinalResultBuildupTime)
		{
			FinalBuildupElapsed       = 0.f;
			bIsBuildingUpFinalResult  = false;
			ShowFinalResult();
		}
		return;
	}

	// ── ミッション残り時間 ──────────────────────────────
	UpdateStylishGauge(RealDelta);

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

			// 時間切れ SE
			UTomatinaFunctionLibrary::PlayTomatinaCue2D(this, TimeUpSound);

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
		BeginFinalResultBuildup();
		return;
	}

	CurrentMissionIndex = Index;
	const FMissionData& Mission = Missions[Index];
	CurrentMission = Mission.TargetType;

	if (ATomatinaHUD* HUD = GetTomatinaHUD())
	{
		HUD->ShowMissionDisplay(Mission.DisplayText, Mission.TargetImage);
	}
	PushStylishStateToHUD();

	if (TargetSpawner)
	{
		TargetSpawner->SpawnTargetsForMission(Mission);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("StartMission: TargetSpawner が null"));
	}

	RemainingTime = (Mission.TimeLimit > 0.f) ? Mission.TimeLimit : -1.f;

	// ミッション開始 SE
	UTomatinaFunctionLibrary::PlayTomatinaCue2D(this, MissionStartSound);
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

	// ③'' 高ランク時の被写体ボーナス（隠し要素）
	const bool bWasHighRank = (static_cast<uint8>(StylishRank) >= static_cast<uint8>(HiddenActionMinRank));
	if (Score > 0 && Result.BestTarget && bWasHighRank && Result.BestTarget->HighRankBonusScore != 0)
	{
		const int32 Before = Score;
		Score = FMath::Max(0, Score + Result.BestTarget->HighRankBonusScore);
		Comment += FString::Printf(TEXT(" 高ランク被写体ボーナス %+d"), Score - Before);
	}

	// ③''' スタイリッシュゲージ更新（高得点をテンポ良く重ねるとランクアップ）
	AddStylishGaugeFromShot(Score);

	const bool bIsHighRankNow = (static_cast<uint8>(StylishRank) >= static_cast<uint8>(HiddenActionMinRank));
	if (Score > 0 && Result.BestTarget && bIsHighRankNow)
	{
		Result.BestTarget->OnHighRankShot(GetStylishRankName(StylishRank), Score);
		OnHighStylishShot(Result.BestTarget, StylishRank, Score);
	}

	CurrentScore = Score;
	TotalScore  += Score;

	UE_LOG(LogTemp, Warning, TEXT("TakePhoto: Score=%d Total=%d Dirts=%d"),
		Score, TotalScore, ActiveDirts.Num());

	// ── リザルト統計サンプリング ─────────────────────────────────
	// 撮影時点のスタイリッシュランクを平均集計、2P 直近活動があったかで同期集計。
	{
		StylishRankSum += static_cast<float>(StylishRank);
		StylishRankSampleCount++;
		TotalPhotoAttempts++;

		if (ATomatoDirtManager* Mgr = GetDirtManager())
		{
			if (Mgr->GetSecondsSinceLastWipe() <= SyncWindowSeconds)
			{
				SyncPhotoCount++;
			}
		}
	}

	// 成功/失敗フラグ（Tick 側の分岐で使う。失敗時は次ミッションへ進めない）
	bLastPhotoSucceeded = (Score > 0);

	// ④ 撮影成功なら BestTarget を Spawner から除去
	if (bLastPhotoSucceeded && TargetSpawner && Result.BestTarget)
	{
		TargetSpawner->RemoveTarget(Result.BestTarget);
	}

	// ⑤ シャッター音
	if (ShutterSound)
	{
		UGameplayStatics::PlaySound2D(this, ShutterSound);
	}

	// ⑤' ファンファーレ（今回の撮影スコアに応じて音を変える）
	PlayFanfareForShotScore(Score);

	// ⑥ ミッションタイマー：成功時のみ停止。失敗時は継続（同ミッションを続ける）
	if (bLastPhotoSucceeded)
	{
		RemainingTime = -1.f;
	}

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
	PushStylishStateToHUD();
}

// =============================================================================
// BeginFinalResultBuildup — 全ミッション終了後、リザルト表示前の「溜め」
// BGM 以外の音停止、汚れ全削除、ミッション UI を隠して FinalResultBuildupTime 秒待つ
// =============================================================================
void ATomatinaGameMode::BeginFinalResultBuildup()
{
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaGameMode::BeginFinalResultBuildup (%.2fs)"),
		FinalResultBuildupTime);

	bIsBuildingUpFinalResult = true;
	FinalBuildupElapsed      = 0.f;

	// 世界停止（BGM・HUD Tick は実時間で動く）
	if (GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.0f);
	}

	// BGM 以外のすべての音を停止
	StopAllSoundsExceptBGM();

	// 溜め開始の合図 SE（BGM のみ残った静寂に対して効果的）
	UTomatinaFunctionLibrary::PlayTomatinaCue2D(this, FinalBuildupSound);

	// 汚れ全削除
	if (ATomatoDirtManager* DirtMgr = GetDirtManager())
	{
		DirtMgr->ClearAllDirts();
	}

	// ミッション UI を隠す（タイマー・スコアが残らないように）
	if (ATomatinaHUD* HUD = GetTomatinaHUD())
	{
		HUD->HideMissionDisplay();
	}

	// 溜め時間が 0 以下なら即座にリザルト表示
	if (FinalResultBuildupTime <= 0.f)
	{
		bIsBuildingUpFinalResult = false;
		ShowFinalResult();
	}
}

// =============================================================================
// PlayFanfareForShotScore — 撮影スコアに応じたファンファーレを再生
// FanfareTiers から MinScore <= ShotScore を満たすものの中で最大 MinScore を採用
// =============================================================================
void ATomatinaGameMode::PlayFanfareForShotScore(int32 ShotScore)
{
	if (FanfareTiers.Num() == 0) { return; }

	const FFanfareTier* Chosen = nullptr;
	int32 BestMin = INT32_MIN;
	for (const FFanfareTier& Tier : FanfareTiers)
	{
		if (Tier.Sound == nullptr) { continue; }
		if (Tier.MinScore <= ShotScore && Tier.MinScore > BestMin)
		{
			BestMin = Tier.MinScore;
			Chosen  = &Tier;
		}
	}

	if (!Chosen || !Chosen->Sound) { return; }

	UE_LOG(LogTemp, Log,
		TEXT("ATomatinaGameMode::PlayFanfareForShotScore: Score=%d → MinScore=%d Sound=%s"),
		ShotScore, Chosen->MinScore, *GetNameSafe(Chosen->Sound));

	UGameplayStatics::PlaySound2D(this, Chosen->Sound,
		Chosen->VolumeMultiplier, Chosen->PitchMultiplier);
}

// =============================================================================
// StopAllSoundsExceptBGM
// BGMAudioComp を除く全 UAudioComponent を停止する
// =============================================================================
void ATomatinaGameMode::StopAllSoundsExceptBGM()
{
	UWorld* World = GetWorld();
	if (!World) { return; }

	int32 StoppedCount = 0;
	for (TObjectIterator<UAudioComponent> It; It; ++It)
	{
		UAudioComponent* Comp = *It;
		if (!IsValid(Comp))                 { continue; }
		if (Comp->GetWorld() != World)      { continue; }
		if (Comp == BGMAudioComp)           { continue; }
		if (!Comp->IsPlaying())             { continue; }

		Comp->Stop();
		++StoppedCount;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("ATomatinaGameMode::StopAllSoundsExceptBGM: %d 個のサウンドを停止"),
		StoppedCount);
}

// =============================================================================
// ShowFinalResult — 全ミッション終了
// =============================================================================
void ATomatinaGameMode::ShowFinalResult()
{
	// 平均スタイリッシュランクを集計値から決定 (float 平均を enum にスナップ)
	const float AvgRankF = (StylishRankSampleCount > 0)
		? StylishRankSum / StylishRankSampleCount
		: 0.f;
	const int32 RankIdx = FMath::Clamp(FMath::RoundToInt(AvgRankF), 0,
		static_cast<int32>(EStylishRank::SSS));
	const EStylishRank AvgRank = static_cast<EStylishRank>(RankIdx);
	const FString AvgRankStr   = GetStylishRankName(AvgRank).ToString();

	// 1P-2P シンクロ率 (0.0〜1.0)
	const float SyncRate = (TotalPhotoAttempts > 0)
		? static_cast<float>(SyncPhotoCount) / TotalPhotoAttempts
		: 0.f;

	UE_LOG(LogTemp, Warning,
		TEXT("ATomatinaGameMode::ShowFinalResult Total=%d AvgRank=%s (%.2f) Sync=%.1f%% (%d/%d)"),
		TotalScore, *AvgRankStr, AvgRankF, SyncRate * 100.f,
		SyncPhotoCount, TotalPhotoAttempts);

	if (ATomatinaHUD* HUD = GetTomatinaHUD())
	{
		HUD->ShowFinalResult(TotalScore, Missions.Num(), AvgRankStr, SyncRate);
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

// =============================================================================
// GetDirtManager
// =============================================================================
ATomatoDirtManager* ATomatinaGameMode::GetDirtManager()
{
	if (CachedDirtManager && IsValid(CachedDirtManager))
	{
		return CachedDirtManager;
	}

	if (!GetWorld())
	{
		return nullptr;
	}

	if (AActor* DirtActor = UGameplayStatics::GetActorOfClass(GetWorld(), ATomatoDirtManager::StaticClass()))
	{
		CachedDirtManager = Cast<ATomatoDirtManager>(DirtActor);
	}

	return CachedDirtManager;
}

// =============================================================================
// CalculateDirtCoverage01
// 汚れ占有率を 0〜1 で近似。高値なら「画面が塞がれている」扱い。
// =============================================================================
float ATomatinaGameMode::CalculateDirtCoverage01()
{
	ATomatoDirtManager* DirtMgr = GetDirtManager();
	if (!DirtMgr)
	{
		return 0.f;
	}

	const TArray<FDirtSplat> Dirts = DirtMgr->GetActiveDirts();
	if (Dirts.Num() == 0)
	{
		return 0.f;
	}

	const int32 GridX = 32;
	const int32 GridY = 20;
	TArray<uint8> Occupancy;
	Occupancy.Init(0, GridX * GridY);

	const float Aspect = (MainHeight > KINDA_SMALL_NUMBER) ? (MainWidth / MainHeight) : 1.6f;

	for (const FDirtSplat& Dirt : Dirts)
	{
		if (!Dirt.bActive || Dirt.Opacity <= 0.f) { continue; }

		const float RadiusX = FMath::Max(0.01f, Dirt.Size * 0.5f);
		const float RadiusY = RadiusX * Aspect;
		const float CX = FMath::Clamp(Dirt.NormalizedPosition.X, 0.f, 1.f);
		const float CY = FMath::Clamp(Dirt.NormalizedPosition.Y, 0.f, 1.f);

		for (int32 Y = 0; Y < GridY; ++Y)
		{
			const float SampleY = (static_cast<float>(Y) + 0.5f) / static_cast<float>(GridY);
			for (int32 X = 0; X < GridX; ++X)
			{
				const float SampleX = (static_cast<float>(X) + 0.5f) / static_cast<float>(GridX);
				const float NX = (SampleX - CX) / RadiusX;
				const float NY = (SampleY - CY) / RadiusY;
				if ((NX * NX + NY * NY) <= 1.0f)
				{
					Occupancy[Y * GridX + X] = 1;
				}
			}
		}
	}

	int32 Filled = 0;
	for (uint8 Cell : Occupancy)
	{
		Filled += (Cell != 0) ? 1 : 0;
	}

	return static_cast<float>(Filled) / static_cast<float>(GridX * GridY);
}

// =============================================================================
// UpdateStylishGauge
// =============================================================================
void ATomatinaGameMode::UpdateStylishGauge(float RealDelta)
{
	if (RealDelta <= 0.f)
	{
		return;
	}

	DirtCoverage01 = CalculateDirtCoverage01();

	float DecayPerSecond = StylishBaseDecayPerSecond;
	if (DirtCoverage01 >= StylishDirtCriticalThreshold)
	{
		DecayPerSecond += StylishDirtCriticalDecayPerSecond;
	}
	else if (DirtCoverage01 >= StylishDirtDangerThreshold)
	{
		DecayPerSecond += StylishDirtDangerDecayPerSecond;
	}

	StylishGauge = FMath::Clamp(StylishGauge - DecayPerSecond * RealDelta, 0.f, StylishGaugeMax);
	if (StylishGauge <= KINDA_SMALL_NUMBER)
	{
		StylishComboCount = 0;
	}

	ApplyStylishRankFromGauge();
	PushStylishStateToHUD();
}

// =============================================================================
// AddStylishGaugeFromShot
// =============================================================================
void ATomatinaGameMode::AddStylishGaugeFromShot(int32 ShotScore)
{
	if (ShotScore <= 0)
	{
		StylishGauge = FMath::Clamp(StylishGauge - StylishMissPenalty, 0.f, StylishGaugeMax);
		if (bResetComboOnMiss)
		{
			StylishComboCount = 0;
		}
		ApplyStylishRankFromGauge();
		PushStylishStateToHUD();
		return;
	}

	const bool bGoodShot = (ShotScore >= StylishGoodShotThreshold);
	const float NowRealTime = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.f;

	float GaugeGain = static_cast<float>(ShotScore) * StylishScoreToGaugeScale;
	if (bGoodShot)
	{
		const bool bInTempo = ((NowRealTime - LastHighScoreShotTime) <= StylishTempoWindow);
		if (bInTempo && StylishComboCount > 0)
		{
			GaugeGain += StylishTempoBonusGauge;
			++StylishComboCount;
		}
		else
		{
			StylishComboCount = 1;
		}
		LastHighScoreShotTime = NowRealTime;
	}
	else
	{
		StylishComboCount = 0;
	}

	StylishGauge = FMath::Clamp(StylishGauge + GaugeGain, 0.f, StylishGaugeMax);
	ApplyStylishRankFromGauge();
	PushStylishStateToHUD();
}

// =============================================================================
// ApplyStylishRankFromGauge
// =============================================================================
void ATomatinaGameMode::ApplyStylishRankFromGauge()
{
	const EStylishRank OldRank = StylishRank;

	if (StylishGauge >= StylishThresholdSSS)      { StylishRank = EStylishRank::SSS; }
	else if (StylishGauge >= StylishThresholdS)   { StylishRank = EStylishRank::S;   }
	else if (StylishGauge >= StylishThresholdA)   { StylishRank = EStylishRank::A;   }
	else if (StylishGauge >= StylishThresholdB)   { StylishRank = EStylishRank::B;   }
	else                                          { StylishRank = EStylishRank::C;   }

	if (StylishRank != OldRank)
	{
		const bool bRankUp = (static_cast<uint8>(StylishRank) > static_cast<uint8>(OldRank));
		OnStylishRankChanged(StylishRank, OldRank, bRankUp);

		if (TargetSpawner)
		{
			const FName RankName = GetStylishRankName(StylishRank);
			const bool bHighRank = (static_cast<uint8>(StylishRank) >= static_cast<uint8>(HiddenActionMinRank));
			for (ATomatinaTargetBase* Target : TargetSpawner->ActiveTargets)
			{
				if (IsValid(Target))
				{
					Target->OnStylishRankChanged(RankName, bHighRank);
				}
			}
		}

		UE_LOG(LogTemp, Warning, TEXT("StylishRank: %s -> %s (Gauge=%.1f)"),
			*GetStylishRankName(OldRank).ToString(),
			*GetStylishRankName(StylishRank).ToString(),
			StylishGauge);
	}
}

// =============================================================================
// GetStylishRankName
// =============================================================================
FName ATomatinaGameMode::GetStylishRankName(EStylishRank Rank) const
{
	switch (Rank)
	{
	case EStylishRank::B:   return FName(TEXT("B"));
	case EStylishRank::A:   return FName(TEXT("A"));
	case EStylishRank::S:   return FName(TEXT("S"));
	case EStylishRank::SSS: return FName(TEXT("SSS"));
	case EStylishRank::C:
	default:
		return FName(TEXT("C"));
	}
}

// =============================================================================
// PushStylishStateToHUD
// =============================================================================
void ATomatinaGameMode::PushStylishStateToHUD()
{
	if (ATomatinaHUD* HUD = GetTomatinaHUD())
	{
		const float GaugePercent = (StylishGaugeMax > KINDA_SMALL_NUMBER)
			? (StylishGauge / StylishGaugeMax)
			: 0.f;
		const bool bDanger = (DirtCoverage01 >= StylishDirtDangerThreshold);
		HUD->UpdateStylishDisplay(GetStylishRankName(StylishRank).ToString(), GaugePercent, StylishComboCount, bDanger);
	}
}
