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

// =============================================================================
// コンストラクタ
// =============================================================================

ATomatinaGameMode::ATomatinaGameMode()
{
	// デフォルト MissionList
	MissionList = { FName("Gorilla") };
}

// =============================================================================
// TakePhoto
// =============================================================================

void ATomatinaGameMode::TakePhoto(USceneCaptureComponent2D* ZoomCamera)
{
	// リザルト表示中は二重撮影を防止
	if (bIsShowingResult)
	{
		UE_LOG(LogTemp, Warning, TEXT("ATomatinaGameMode::TakePhoto: リザルト表示中のためスキップ"));
		return;
	}

	if (!ZoomCamera)
	{
		UE_LOG(LogTemp, Warning, TEXT("ATomatinaGameMode::TakePhoto: ZoomCamera が null"));
		return;
	}

	// ① ズームカメラの映像を RT_Photo にコピー
	UTomatinaFunctionLibrary::CopyZoomToPhoto(ZoomCamera, RT_Photo);

	// ② スコア計算（ATomatinaTargetBase::StaticClass() は内部で使用）
	const int32 Score = UTomatinaFunctionLibrary::CalculatePhotoScore(
		ZoomCamera,
		CurrentMission,
		PhoneWidth,
		PhoneHeight);

	// ── タオル映り込み減点 ────────────────────────────────────────────────────
	FString TowelPenaltyComment;
	AActor* TowelActor = UGameplayStatics::GetActorOfClass(GetWorld(), ATomatinaTowelSystem::StaticClass());
	if (ATomatinaTowelSystem* Towel = Cast<ATomatinaTowelSystem>(TowelActor))
	{
		if (Towel->bTowelInZoomView)
		{
			Score += Towel->TowelPenalty;          // デフォルト -20
			Score  = FMath::Max(Score, 0);          // 0 未満にしない
			TowelPenaltyComment = TEXT(" タオルが映っちゃった！");
			UE_LOG(LogTemp, Warning,
				TEXT("ATomatinaGameMode::TakePhoto: タオル映り込み減点 %d → %d"),
				Score - Towel->TowelPenalty, Score);
		}
	}

	CurrentScore  = Score;
	TotalScore   += Score;

	UE_LOG(LogTemp, Warning, TEXT("ATomatinaGameMode::TakePhoto: Score=%d TotalScore=%d"), CurrentScore, TotalScore);

	// ③ スローモーション（時間停止）
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.0f);

	// ④ リザルトフラグを立てる
	bIsShowingResult = true;

	// ⑤ HUD にリザルト表示を依頼
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (PC)
	{
		ATomatinaHUD* HUD = Cast<ATomatinaHUD>(PC->GetHUD());
		if (HUD)
		{
			const FString Comment = GetScoreComment(Score) + TowelPenaltyComment;
			HUD->ShowResult(Score, Comment);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ATomatinaGameMode::TakePhoto: ATomatinaHUD の取得に失敗"));
		}
	}

	// ⑥ リザルトタイマーをセット
	// TimeDilation=0 でもタイマーは実時間で動作させるため
	// FTimerManager::SetTimer の第5引数 bIgnorePause=false はデフォルト。
	// SetGlobalTimeDilation(0) はゲーム時間を止めるため TimerManager も停止する。
	// そのため、ここでは TimeDilation が 0 でも動作するよう
	// GetWorldSettings()->SetTimerForNextTick ではなく
	// FTimerManager の InRate を実時間ベースで管理する方針を取る。
	//
	// ★ 実運用では TimeDilation=0 のままだとタイマーが止まる場合がある。
	//    その場合は Tick 内で RealTimeSeconds を使って手動カウントするか、
	//    AWorldSettings::TimeDilation を 0 ではなく 0.001 等の極小値にして
	//    タイマーが完全停止しないよう調整すること。
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			ResultTimerHandle,
			this,
			&ATomatinaGameMode::OnResultTimerEnd,
			ResultDisplayTime,
			false);   // bLoop=false
	}
}

// =============================================================================
// OnResultTimerEnd
// =============================================================================

void ATomatinaGameMode::OnResultTimerEnd()
{
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaGameMode::OnResultTimerEnd: リザルト終了"));

	// ① 時間を元に戻す
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);

	// ② リザルトフラグを解除
	bIsShowingResult = false;

	// ③ HUD のリザルトを非表示
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (PC)
	{
		ATomatinaHUD* HUD = Cast<ATomatinaHUD>(PC->GetHUD());
		if (HUD)
		{
			HUD->HideResult();
		}
	}

	// ④ 次のミッションへ
	AdvanceMission();
}

// =============================================================================
// AdvanceMission
// =============================================================================

void ATomatinaGameMode::AdvanceMission()
{
	MissionIndex++;

	if (MissionList.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("ATomatinaGameMode::AdvanceMission: MissionList が空"));
		return;
	}

	if (MissionIndex >= MissionList.Num())
	{
		// 全ミッションクリア → 先頭に戻してループ
		// ゲーム終了処理が必要な場合はここで呼ぶ
		UE_LOG(LogTemp, Warning, TEXT("ATomatinaGameMode::AdvanceMission: 全ミッション完了 → 先頭に戻す (TotalScore=%d)"), TotalScore);
		MissionIndex = 0;
	}

	CurrentMission = MissionList[MissionIndex];
	UE_LOG(LogTemp, Warning, TEXT("ATomatinaGameMode::AdvanceMission: 次のミッション = %s (Index=%d)"),
		*CurrentMission.ToString(), MissionIndex);
}

// =============================================================================
// GetScoreComment
// =============================================================================

FString ATomatinaGameMode::GetScoreComment(int32 Score) const
{
	if (Score >= 100) return TEXT("全身バッチリ！");
	if (Score >= 50)  return TEXT("上半身のみ！");
	if (Score >= 10)  return TEXT("足だけ...");
	return TEXT("映ってない！");
}
