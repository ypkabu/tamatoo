// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TomatinaFunctionLibrary.generated.h"

class APlayerController;
class USceneCaptureComponent2D;
class UCameraComponent;
class UTextureRenderTarget2D;
class ATomatinaTargetBase;

/**
 * ゲーム全体で使うユーティリティ関数をまとめた Blueprint Function Library
 *
 * ① CheckVisibility
 * ② CalculatePhotoScore
 * ③ CalculateZoomOffset
 * ④ GetZoomScreenCenter
 * ⑤ WorldToZoomScreen
 * ⑥ ProjectZoomToMainScreen
 * ⑦ CopyZoomToPhoto
 * ⑧ NormalizeScreenPosition
 * ⑨ DenormalizeToMainScreen
 * ⑩ DenormalizeToPhoneScreen
 */
UCLASS()
class TOMATO_API UTomatinaFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	// =========================================================================
	// ① CheckVisibility
	// =========================================================================

	/**
	 * 指定アクターが SceneCapture の視野内に入っているかを判定する。
	 * バウンドボックスの 8 頂点のうち 1 つでも Phone 画面内に投影されれば true。
	 *
	 * @param ZoomCamera   ズームカメラ（SceneCapture_Zoom）
	 * @param TargetActor  判定対象アクター
	 * @param PhoneWidth   iPhone 画面の横幅（ピクセル）
	 * @param PhoneHeight  iPhone 画面の縦幅（ピクセル）
	 * @return             画面内に少しでも映っていれば true
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Tomatina|Visibility")
	static bool CheckVisibility(
		USceneCaptureComponent2D* ZoomCamera,
		AActor* TargetActor,
		float PhoneWidth,
		float PhoneHeight);

	// =========================================================================
	// ② CalculatePhotoScore
	// =========================================================================

	/**
	 * ZoomCamera に写っている被写体の画面占有率からスコアを計算して返す。
	 * ATomatinaTargetBase にキャストして MyType を直接読む方式。
	 *
	 * @param ZoomCamera   ズームカメラ（SceneCapture_Zoom）
	 * @param MissionTag   現在のお題（CurrentMission）
	 * @param PhoneWidth   iPhone 画面の横幅（ピクセル）
	 * @param PhoneHeight  iPhone 画面の縦幅（ピクセル）
	 * @return             スコア（0〜100）
	 */
	UFUNCTION(BlueprintCallable, Category="Tomatina|Photo")
	static int32 CalculatePhotoScore(
		USceneCaptureComponent2D* ZoomCamera,
		FName MissionTag,
		float PhoneWidth,
		float PhoneHeight);

	// =========================================================================
	// ③ CalculateZoomOffset
	// =========================================================================

	/**
	 * ヒット地点をズームカメラの中心に合わせるための相対オフセットを計算する。
	 * SceneCapture_Zoom->SetRelativeLocation に渡すローカル空間ベクター (Y, Z) を返す。
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
	 * iPhone 画面上のピクセル座標（0〜PhoneWidth, 0〜PhoneHeight）に変換する。
	 *
	 * @param ZoomCamera   ズームカメラ（SceneCapture_Zoom）
	 * @param WorldLocation 投影したいワールド座標
	 * @param PhoneWidth   iPhone 画面の横幅（ピクセル）
	 * @param PhoneHeight  iPhone 画面の縦幅（ピクセル）
	 * @return             Phone 画面上のピクセル座標。画面外の場合も値を返す（クランプしない）
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
	 * ズーム中のカーソル位置をメイン画面上に表示するために使用。
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
	 * リザルト画面に「撮った写真」を表示するために使用。
	 */
	UFUNCTION(BlueprintCallable, Category="Tomatina|Photo")
	static void CopyZoomToPhoto(
		USceneCaptureComponent2D* ZoomCamera,
		UTextureRenderTarget2D* RT_Photo);

	// =========================================================================
	// ⑧ NormalizeScreenPosition
	// =========================================================================

	/**
	 * スクリーン座標（ピクセル）を 0〜1 に正規化する。
	 * 汚れの座標管理に使用。画面解像度が変わっても同じ相対位置を保てる。
	 *
	 * @param ScreenPos    ピクセル座標
	 * @param ScreenWidth  画面横幅
	 * @param ScreenHeight 画面縦幅
	 * @return             正規化座標（0.0〜1.0）
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Tomatina|ScreenCoord")
	static FVector2D NormalizeScreenPosition(
		FVector2D ScreenPos,
		float ScreenWidth,
		float ScreenHeight);

	// =========================================================================
	// ⑨ DenormalizeToMainScreen
	// =========================================================================

	/**
	 * 正規化座標（0〜1）をメイン画面のピクセル座標に変換する。
	 *
	 * @param NormPos    正規化座標
	 * @param MainWidth  メイン画面の横幅
	 * @param MainHeight メイン画面の縦幅
	 * @return           メイン画面上のピクセル座標
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Tomatina|ScreenCoord")
	static FVector2D DenormalizeToMainScreen(
		FVector2D NormPos,
		float MainWidth,
		float MainHeight);

	// =========================================================================
	// ⑩ DenormalizeToPhoneScreen
	// =========================================================================

	/**
	 * 正規化座標（0〜1）を超横長ウィンドウ上の Phone 画面ピクセル座標に変換する。
	 * Phone 画面はメイン画面の右側（X = MainWidth〜）に配置されている想定。
	 *
	 * @param NormPos    正規化座標
	 * @param MainWidth  メイン画面の横幅（Phone 画面の X 開始位置）
	 * @param PhoneWidth Phone 画面の横幅
	 * @param PhoneHeight Phone 画面の縦幅
	 * @return           超横長ウィンドウ上の絶対ピクセル座標
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Tomatina|ScreenCoord")
	static FVector2D DenormalizeToPhoneScreen(
		FVector2D NormPos,
		float MainWidth,
		float PhoneWidth,
		float PhoneHeight);
};
