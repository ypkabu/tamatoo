// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TomatinaTargetBase.h"
#include "TomatinaGameMode.generated.h"

class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class UTexture2D;
class USoundBase;
class ATomatinaTowelSystem;
class ATomatinaTargetSpawner;
class ATomatinaHUD;

// ─────────────────────────────────────────────────────────────────────────────
// FMissionData
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FMissionData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FName TargetType;

	UPROPERTY(EditAnywhere)
	FText DisplayText;

	UPROPERTY(EditAnywhere)
	float TimeLimit = 15.0f;

	UPROPERTY(EditAnywhere)
	TSubclassOf<ATomatinaTargetBase> TargetClass;

	UPROPERTY(EditAnywhere)
	int32 SpawnCount = 1;

	UPROPERTY(EditAnywhere)
	FName SpawnProfileName;

	UPROPERTY(EditAnywhere, Category="Mission")
	UTexture2D* TargetImage = nullptr;
};

// ─────────────────────────────────────────────────────────────────────────────
// ATomatinaGameMode
// ─────────────────────────────────────────────────────────────────────────────
UCLASS()
class TOMATO_API ATomatinaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATomatinaGameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// =========================================================================
	// ミッション
	// =========================================================================

	UPROPERTY(EditAnywhere, Category="Tomatina|Mission")
	TArray<FMissionData> Missions;

	UPROPERTY(EditAnywhere, Category="Tomatina|Mission")
	bool bRandomOrder = true;

	UPROPERTY(BlueprintReadOnly, Category="Tomatina|Mission")
	int32 CurrentMissionIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category="Tomatina|Mission")
	FName CurrentMission;

	UPROPERTY(BlueprintReadOnly, Category="Tomatina|Mission")
	float RemainingTime = 0.f;

	// =========================================================================
	// スコア
	// =========================================================================

	UPROPERTY(BlueprintReadOnly, Category="Tomatina|Score")
	int32 CurrentScore = 0;

	UPROPERTY(BlueprintReadOnly, Category="Tomatina|Score")
	int32 TotalScore = 0;

	// =========================================================================
	// レンダーターゲット・画面サイズ
	// =========================================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Photo")
	UTextureRenderTarget2D* RT_Photo = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Photo")
	float PhoneWidth = 1024.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Photo")
	float PhoneHeight = 768.f;

	// =========================================================================
	// サウンド
	// =========================================================================

	UPROPERTY(EditAnywhere, Category="Sound")
	USoundBase* ShutterSound;

	UPROPERTY(EditAnywhere, Category="Sound")
	USoundBase* BGM;

	// =========================================================================
	// リザルト
	// =========================================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tomatina|Result")
	float ResultDisplayTime = 2.0f;

	UPROPERTY(BlueprintReadOnly, Category="Tomatina|Result")
	bool bIsShowingResult = false;

	// =========================================================================
	// 関数
	// =========================================================================

	UFUNCTION(BlueprintCallable, Category="Tomatina|Photo")
	void TakePhoto(USceneCaptureComponent2D* ZoomCamera);

	UFUNCTION(BlueprintCallable, Category="Tomatina|Mission")
	void StartMission(int32 Index);

	UFUNCTION(BlueprintCallable, Category="Tomatina|Mission")
	void AdvanceMission();

protected:
	UFUNCTION()
	void OnResultTimerEnd();

	UFUNCTION()
	void OnMissionTimeUp();

private:
	FTimerHandle ResultTimerHandle;
	FTimerHandle MissionTimerHandle;

	double ResultStartRealTime = 0.0;

	UPROPERTY()
	ATomatinaTargetSpawner* TargetSpawner = nullptr;

	void ShowFinalResult();
	void ShuffleMissions();
	ATomatinaHUD* GetTomatinaHUD() const;
};
