// Fill out your copyright notice in the Description page of Project Settings.

#include "TomatinaGameMode.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/PlayerController.h"
#include "Framework/Application/SlateApplication.h"

#include "TomatinaFunctionLibrary.h"
#include "TomatinaHUD.h"
#include "TomatinaTowelSystem.h"
#include "TomatinaTargetSpawner.h"

// =============================================================================
// コンストラクタ
// =============================================================================

ATomatinaGameMode::ATomatinaGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
}

// =============================================================================
// BeginPlay
// =============================================================================

void ATomatinaGameMode::BeginPlay()
{
	Super::BeginPlay();

	// セカンダリモニターから PhoneWidth/PhoneHeight を取得
	FDisplayMetrics DM;
	FSlateApplication::Get().GetDisplayMetrics(DM);
	for (const FMonitorInfo& Monitor : DM.MonitorInfo)
	{
		if (!Monitor.bIsPrimary)
		{
			PhoneWidth  = static_cast<float>(Monitor.NativeWidth);
			PhoneHeight = static_cast<float>(Monitor.NativeHeight);
		}
	}

	// Spawner を検索
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(), ATomatinaTargetSpawner::StaticClass(), Found);
	if (Found.Num() > 0)
	{
		TargetSpawner = Cast<ATomatinaTargetSpawner>(Found[0]);
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("ATomatinaGameMode::BeginPlay: ATomatinaTargetSpawner がレベル上にありません"));
	}

	if (bRandomOrder)
	{
		ShuffleMissions();
	}

	if (BGM)
	{
		UGameplayStatics::PlaySound2D(this, BGM);
	}

	UE_LOG(LogTemp, Warning,
		TEXT("ATomatinaGameMode::BeginPlay: GameMode BeginPlay, Missions=%d"),
		Missions.Num());

	// カウントダウンなし、直接スタート
	StartMission(0);
}

// =============================================================================
// Tick
// =============================================================================

void ATomatinaGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// リザルト表示中：リアルタイムで経過判定
	if (bIsShowingResult)
	{
		const double Elapsed = GetWorld()->GetRealTimeSeconds() - ResultStartRealTime;
		if (Elapsed >= static_cast<double>(ResultDisplayTime))
		{
			OnResultTimerEnd();
		}
		return;
	}

	// ミッション残り時間
	if (Missions.IsValidIndex(CurrentMissionIndex)
		&& Missions[CurrentMissionIndex].TimeLimit > 0.f)
	{
		RemainingTime -= DeltaTime;
		if (ATomatinaHUD* HUD = GetTomatinaHUD())
		{
			HUD->UpdateTimer(FMath::Max(0.f, RemainingTime));
		}
		if (RemainingTime <= 0.f)
		{
			RemainingTime = 0.f;
			OnMissionTimeUp();
		}
	}
}

// =============================================================================
// AdvanceMission
// =============================================================================

void ATomatinaGameMode::AdvanceMission()
{
	StartMission(CurrentMissionIndex + 1);
}

// =============================================================================
// StartMission
// =============================================================================

void ATomatinaGameMode::StartMission(int32 Index)
{
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaGameMode: StartMission %d"), Index);

	if (Index >= Missions.Num())
	{
		ShowFinalResult();
		return;
	}

	CurrentMissionIndex = Index;
	const FMissionData& Mission = Missions[Index];
	CurrentMission = Mission.TargetType;
	RemainingTime  = Mission.TimeLimit;

	if (ATomatinaHUD* HUD = GetTomatinaHUD())
	{
		HUD->ShowMissionDisplay(Mission.DisplayText, Mission.TargetImage);
		HUD->UpdateTotalScore(TotalScore);
	}

	if (TargetSpawner)
	{
		TargetSpawner->SpawnTargetsForMission(Mission);
	}

	// 旧 MissionTimerHandle は Tick 方式に切り替えたのでクリアのみ
	GetWorld()->GetTimerManager().ClearTimer(MissionTimerHandle);
}

// =============================================================================
// OnMissionTimeUp
// =============================================================================

void ATomatinaGameMode::OnMissionTimeUp()
{
	UE_LOG(LogTemp, Warning,
		TEXT("ATomatinaGameMode::OnMissionTimeUp: Index=%d"), CurrentMissionIndex);

	if (ATomatinaHUD* HUD = GetTomatinaHUD())
	{
		HUD->ShowMissionResult(0, TEXT("時間切れ！"));
	}

	// 2 秒後に次ミッション
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
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaGameMode: TakePhoto Entry"));

	if (bIsShowingResult)
	{
		UE_LOG(LogTemp, Warning, TEXT("TakePhoto: リザルト中のためスキップ"));
		return;
	}
	if (!ZoomCamera)
	{
		UE_LOG(LogTemp, Warning, TEXT("TakePhoto: ZoomCamera が null"));
		return;
	}

	// ズーム映像を RT_Photo にコピー（先にキャプチャしてから UI を出す）
	UTomatinaFunctionLibrary::CopyZoomToPhoto(ZoomCamera, RT_Photo);

	// スコア計算
	static const TArray<ATomatinaTargetBase*> EmptyTargets;
	const TArray<ATomatinaTargetBase*>& Targets =
		TargetSpawner ? TargetSpawner->ActiveTargets : EmptyTargets;

	FPhotoResult Result = UTomatinaFunctionLibrary::CalculatePhotoScore(
		ZoomCamera, Targets, CurrentMission, PhoneWidth, PhoneHeight);

	int32   Score   = Result.Score;
	FString Comment = Result.Comment;

	// タオル映り込み減点
	AActor* TowelActor = UGameplayStatics::GetActorOfClass(
		GetWorld(), ATomatinaTowelSystem::StaticClass());
	if (ATomatinaTowelSystem* Towel = Cast<ATomatinaTowelSystem>(TowelActor))
	{
		if (Towel->bTowelInZoomView)
		{
			Score   += Towel->TowelPenalty;
			Score    = FMath::Max(Score, 0);
			Comment += TEXT(" タオルが映っちゃった！");
		}
	}

	CurrentScore = Score;
	TotalScore  += Score;

	// 撮影成功ならターゲットを消す
	if (Score > 0 && TargetSpawner && Result.BestTarget)
	{
		TargetSpawner->RemoveTarget(Result.BestTarget);
	}

	// シャッター音・フラッシュ
	if (ShutterSound)
	{
		UGameplayStatics::PlaySound2D(this, ShutterSound);
	}
	if (ATomatinaHUD* HUD = GetTomatinaHUD())
	{
		HUD->PlayShutterFlash();
	}

	// 時間停止
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.0f);

	// リザルト表示
	if (ATomatinaHUD* HUD = GetTomatinaHUD())
	{
		HUD->ShowResult(Score, Comment);
		HUD->UpdateTotalScore(TotalScore);
	}

	bIsShowingResult    = true;
	ResultStartRealTime = GetWorld()->GetRealTimeSeconds();

	UE_LOG(LogTemp, Warning,
		TEXT("ATomatinaGameMode: TakePhoto End, Score=%d Total=%d"), Score, TotalScore);
}

// =============================================================================
// OnResultTimerEnd
// =============================================================================

void ATomatinaGameMode::OnResultTimerEnd()
{
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaGameMode::OnResultTimerEnd"));

	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
	bIsShowingResult = false;

	if (ATomatinaHUD* HUD = GetTomatinaHUD())
	{
		HUD->HideResult();
	}

	StartMission(CurrentMissionIndex + 1);
}

// =============================================================================
// ShowFinalResult
// =============================================================================

void ATomatinaGameMode::ShowFinalResult()
{
	UE_LOG(LogTemp, Warning,
		TEXT("ATomatinaGameMode::ShowFinalResult: Total=%d"), TotalScore);

	if (ATomatinaHUD* HUD = GetTomatinaHUD())
	{
		HUD->ShowFinalResult(TotalScore, Missions.Num());
	}
}

// =============================================================================
// ShuffleMissions
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

	// 同じ TargetType が連続しないよう調整
	for (int32 i = 0; i < Num - 1; ++i)
	{
		if (Missions[i].TargetType != Missions[i + 1].TargetType) { continue; }

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
}

// =============================================================================
// GetTomatinaHUD
// =============================================================================

ATomatinaHUD* ATomatinaGameMode::GetTomatinaHUD() const
{
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	return PC ? Cast<ATomatinaHUD>(PC->GetHUD()) : nullptr;
}
