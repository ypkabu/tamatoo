// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TomatinaTargetBase.h"          // FPhotoResult::BestTarget に必要（full definition）
#include "TomatinaFunctionLibrary.generated.h"

class APlayerController;
class USceneCaptureComponent2D;
class UCameraComponent;
class UTextureRenderTarget2D;

// ─────────────────────────────────────────────────────────────────────────────
// FPhotoResult — CalculatePhotoScore の戻り値
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FPhotoResult
{
	GENERATED_BODY()

	/** スコア（0〜100） */
	UPROPERTY(BlueprintReadOnly)
	int32 Score = 0;

	/**
	 * スコアに最も貢献したターゲット。
	 * Score > 0 のとき有効。GameMode から RemoveTarget に渡す。
	 */
	UPROPERTY(BlueprintReadOnly)
	ATomatinaTargetBase* BestTarget = nullptr;

	/** スコアに応じたコメント文字列 */
	UPROPERTY(BlueprintReadOnly)
	FString Comment;
};

// ─────────────────────────────────────────────────────────────────────────────
// UTomatinaFunctionLibrary
// ─────────────────────────────────────────────────────────────────────────────
UCLASS()
class TOMATO_API UTomatinaFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	// =========================================================================
	// ① CheckVisibility
	// =========================================================================

	/**
	 * CheckLocation が ZoomCamera の視野内に収まり、かつ遮蔽されていなければ true。
	 * FOV ハーフタン法で NDC 範囲チェック → ラインレースで遮蔽確認。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Tomatina|Visibility")
	static bool CheckVisibility(
		USceneCaptureComponent2D* ZoomCamera,
		AActor* TargetActor,
		FVector CheckLocation,
		float ScreenWidth,
		float ScreenHeight);

	// =========================================================================
	// ② CalculatePhotoScore
	// =========================================================================

	/**
	 * ZoomCamera に写っているターゲットの画面占有率からスコアを計算して返す。
	 * 渡された Targets を CurrentMission でフィルタし、
	 * 最も占有率の高いターゲットを BestTarget として FPhotoResult に含める。
	 *
	 * @param ZoomCamera     ズームカメラ（SceneCapture_Zoom）
	 * @param Targets        判定対象ターゲット一覧（Spawner の ActiveTargets）
	 * @param CurrentMission 現在のお題タグ（ATomatinaTargetBase::MyType と照合）
	 * @param ScreenWidth    Phone 画面の横幅（ピクセル）
	 * @param ScreenHeight   Phone 画面の縦幅（ピクセル）
	 * @return               スコア・最高貢献ターゲット・コメントを含む FPhotoResult
	 */
	UFUNCTION(BlueprintCallable, Category="Tomatina|Photo")
	static FPhotoResult CalculatePhotoScore(
		USceneCaptureComponent2D* ZoomCamera,
		const TArray<ATomatinaTargetBase*>& Targets,
		FName CurrentMission,
		float ScreenWidth,
		float ScreenHeight);

	// =========================================================================
	// ③ CalculateZoomOffset
	// =========================================================================

	/**
	 * ヒット地点をズームカメラの中心に合わせるための相対オフセットを計算する。
	 */
	UFUNCTION(BlueprintCallable, Category="Tomatina|Camera")
	static FVector CalculateZoomOffset(
		APlayerController* PC,
		const FHitResult& HitResult,
		UCameraComponent* Camera,
		float CameraFOV = 90.f);

	// =========================================================================
	// ④ GetZoomScreenCenter
	// =========================================================================

	/**
	 * iPhone 画面（PhoneWidth x PhoneHeight）がメイン画面上で表示される
	 * 中心座標（スクリーン座標系）を返す。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Tomatina|Camera")
	static FVector2D GetZoomScreenCenter(float MainWidth, float PhoneWidth, float PhoneHeight);

	// =========================================================================
	// ⑤ WorldToZoomScreen
	// =========================================================================

	/**
	 * ワールド座標を SceneCapture の視野に投影し、
	 * iPhone 画面上のピクセル座標に変換する。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Tomatina|Camera")
	static FVector2D WorldToZoomScreen(
		USceneCaptureComponent2D* ZoomCamera,
		FVector WorldLocation,
		float PhoneWidth,
		float PhoneHeight);

	// =========================================================================
	// ⑥ ProjectZoomToMainScreen
	// =========================================================================

	/**
	 * SceneCapture_Zoom が向いている方向をメインスクリーン座標に投影して返す。
	 */
	UFUNCTION(BlueprintCallable, Category="Tomatina|Camera")
	static FVector2D ProjectZoomToMainScreen(
		APlayerController* PC,
		USceneCaptureComponent2D* SceneCapture,
		float MainWidth,
		float MainHeight);

	// =========================================================================
	// ⑦ CopyZoomToPhoto
	// =========================================================================

	/**
	 * SceneCapture_Zoom のレンダーターゲットの内容を RT_Photo にコピーする。
	 */
	UFUNCTION(BlueprintCallable, Category="Tomatina|Photo")
	static void CopyZoomToPhoto(
		USceneCaptureComponent2D* ZoomCamera,
		UTextureRenderTarget2D* RT_Photo);

	// =========================================================================
	// ⑧ NormalizeScreenPosition
	// =========================================================================

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Tomatina|ScreenCoord")
	static FVector2D NormalizeScreenPosition(
		FVector2D ScreenPos,
		float ScreenWidth,
		float ScreenHeight);

	// =========================================================================
	// ⑨ DenormalizeToMainScreen
	// =========================================================================

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Tomatina|ScreenCoord")
	static FVector2D DenormalizeToMainScreen(
		FVector2D NormPos,
		float MainWidth,
		float MainHeight);

	// =========================================================================
	// ⑩ DenormalizeToPhoneScreen
	// =========================================================================

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Tomatina|ScreenCoord")
	static FVector2D DenormalizeToPhoneScreen(
		FVector2D NormPos,
		float MainWidth,
		float PhoneWidth,
		float PhoneHeight);
};
