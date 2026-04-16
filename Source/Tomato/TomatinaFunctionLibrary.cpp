// Fill out your copyright notice in the Description page of Project Settings.

#include "TomatinaFunctionLibrary.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"

#include "TomatinaTargetBase.h"

// =============================================================================
// 共通ヘルパー：SceneCapture のビュー行列と投影パラメータを計算
// =============================================================================
namespace TomatinaInternal
{
	/**
	 * ZoomCamera の位置・回転・FOV からビュー行列と投影係数を計算する。
	 * @param ZoomCamera    対象の SceneCapture
	 * @param PhoneWidth    Phone 画面の横幅
	 * @param PhoneHeight   Phone 画面の縦幅
	 * @param OutViewMatrix カメラ空間へ変換する行列（出力）
	 * @param OutCotHalfFOV 1 / tan(FOV/2) （出力）
	 * @param OutAspect     アスペクト比 W/H （出力）
	 */
	static void BuildViewParams(
		USceneCaptureComponent2D* ZoomCamera,
		float PhoneWidth,
		float PhoneHeight,
		FMatrix& OutViewMatrix,
		float& OutCotHalfFOV,
		float& OutAspect)
	{
		const FRotator Rot = ZoomCamera->GetComponentRotation();
		const FVector  Loc = ZoomCamera->GetComponentLocation();

		OutViewMatrix  = FRotationTranslationMatrix(Rot, Loc).InverseFast();
		OutAspect      = (PhoneHeight > 0.f) ? (PhoneWidth / PhoneHeight) : 1.f;
		const float HalfFOV = FMath::DegreesToRadians(ZoomCamera->FOVAngle * 0.5f);
		OutCotHalfFOV  = 1.f / FMath::Tan(HalfFOV);
	}

	/**
	 * カメラローカル座標を NDC（-1〜1）に変換する。
	 * @param LocalPos    カメラ空間の座標（X = 奥行き）
	 * @param CotHalfFOV  1 / tan(FOV/2)
	 * @param Aspect      アスペクト比 W/H
	 * @param OutNDC      NDC 座標（出力）
	 * @return            カメラの前方にある（X > 0）なら true
	 */
	static bool LocalToNDC(
		const FVector& LocalPos,
		float CotHalfFOV,
		float Aspect,
		FVector2D& OutNDC)
	{
		if (LocalPos.X <= 0.f) return false;
		OutNDC.X =  (LocalPos.Y / LocalPos.X) * CotHalfFOV / Aspect;
		OutNDC.Y = -(LocalPos.Z / LocalPos.X) * CotHalfFOV;
		return true;
	}
}

// =============================================================================
// ① CheckVisibility
// =============================================================================

bool UTomatinaFunctionLibrary::CheckVisibility(
	USceneCaptureComponent2D* ZoomCamera,
	AActor* TargetActor,
	float PhoneWidth,
	float PhoneHeight)
{
	if (!ZoomCamera || !TargetActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("CheckVisibility: ZoomCamera または TargetActor が null"));
		return false;
	}

	FMatrix ViewMatrix;
	float CotHalfFOV, Aspect;
	TomatinaInternal::BuildViewParams(ZoomCamera, PhoneWidth, PhoneHeight, ViewMatrix, CotHalfFOV, Aspect);

	// バウンドボックスの 8 頂点を取得
	FVector Origin, Extent;
	TargetActor->GetActorBounds(false, Origin, Extent);

	const FVector Corners[8] =
	{
		Origin + FVector( Extent.X,  Extent.Y,  Extent.Z),
		Origin + FVector(-Extent.X,  Extent.Y,  Extent.Z),
		Origin + FVector( Extent.X, -Extent.Y,  Extent.Z),
		Origin + FVector(-Extent.X, -Extent.Y,  Extent.Z),
		Origin + FVector( Extent.X,  Extent.Y, -Extent.Z),
		Origin + FVector(-Extent.X,  Extent.Y, -Extent.Z),
		Origin + FVector( Extent.X, -Extent.Y, -Extent.Z),
		Origin + FVector(-Extent.X, -Extent.Y, -Extent.Z),
	};

	// 1 頂点でも NDC（-1〜1）の内側に入れば可視
	for (const FVector& Corner : Corners)
	{
		const FVector LocalPos = ViewMatrix.TransformPosition(Corner);
		FVector2D NDC;
		if (!TomatinaInternal::LocalToNDC(LocalPos, CotHalfFOV, Aspect, NDC)) continue;

		if (NDC.X >= -1.f && NDC.X <= 1.f &&
			NDC.Y >= -1.f && NDC.Y <= 1.f)
		{
			return true;
		}
	}

	return false;
}

// =============================================================================
// ② CalculatePhotoScore
//    渡された Targets を CurrentMission でフィルタし、
//    最も画面占有率が高いターゲットを BestTarget として FPhotoResult で返す
// =============================================================================

FPhotoResult UTomatinaFunctionLibrary::CalculatePhotoScore(
	USceneCaptureComponent2D* ZoomCamera,
	const TArray<ATomatinaTargetBase*>& Targets,
	FName CurrentMission,
	float ScreenWidth,
	float ScreenHeight)
{
	FPhotoResult Result;
	Result.Comment = TEXT("映ってない！");

	if (!ZoomCamera)
	{
		UE_LOG(LogTemp, Warning, TEXT("CalculatePhotoScore: ZoomCamera が null"));
		return Result;
	}

	FMatrix ViewMatrix;
	float CotHalfFOV, Aspect;
	TomatinaInternal::BuildViewParams(ZoomCamera, ScreenWidth, ScreenHeight,
	                                  ViewMatrix, CotHalfFOV, Aspect);

	float BestCoverage = 0.0f;

	for (ATomatinaTargetBase* Target : Targets)
	{
		if (!IsValid(Target))                    { continue; }
		if (Target->MyType != CurrentMission)    { continue; }
		if (!CheckVisibility(ZoomCamera, Target, ScreenWidth, ScreenHeight)) { continue; }

		// バウンドボックス 8 頂点を NDC に投影して最小・最大矩形を求める
		FVector Origin, Extent;
		Target->GetActorBounds(false, Origin, Extent);

		const FVector Corners[8] =
		{
			Origin + FVector( Extent.X,  Extent.Y,  Extent.Z),
			Origin + FVector(-Extent.X,  Extent.Y,  Extent.Z),
			Origin + FVector( Extent.X, -Extent.Y,  Extent.Z),
			Origin + FVector(-Extent.X, -Extent.Y,  Extent.Z),
			Origin + FVector( Extent.X,  Extent.Y, -Extent.Z),
			Origin + FVector(-Extent.X,  Extent.Y, -Extent.Z),
			Origin + FVector( Extent.X, -Extent.Y, -Extent.Z),
			Origin + FVector(-Extent.X, -Extent.Y, -Extent.Z),
		};

		float MinU = FLT_MAX, MaxU = -FLT_MAX;
		float MinV = FLT_MAX, MaxV = -FLT_MAX;
		int32 VisibleCount = 0;

		for (const FVector& Corner : Corners)
		{
			const FVector LocalPos = ViewMatrix.TransformPosition(Corner);
			FVector2D NDC;
			if (!TomatinaInternal::LocalToNDC(LocalPos, CotHalfFOV, Aspect, NDC)) { continue; }

			MinU = FMath::Min(MinU, NDC.X);
			MaxU = FMath::Max(MaxU, NDC.X);
			MinV = FMath::Min(MinV, NDC.Y);
			MaxV = FMath::Max(MaxV, NDC.Y);
			++VisibleCount;
		}

		if (VisibleCount == 0) { continue; }

		// 画面内にクランプして占有面積を算出（NDC 全画面 = 2×2 = 4）
		const float CU0 = FMath::Clamp(MinU, -1.f, 1.f);
		const float CU1 = FMath::Clamp(MaxU, -1.f, 1.f);
		const float CV0 = FMath::Clamp(MinV, -1.f, 1.f);
		const float CV1 = FMath::Clamp(MaxV, -1.f, 1.f);

		const float Coverage = FMath::Clamp((CU1 - CU0) * (CV1 - CV0) / 4.f, 0.f, 1.f);

		if (Coverage > BestCoverage)
		{
			BestCoverage      = Coverage;
			Result.BestTarget = Target;
		}
	}

	Result.Score = FMath::RoundToInt(BestCoverage * 100.f);

	// コメント生成
	if      (Result.Score >= 100) { Result.Comment = TEXT("全身バッチリ！"); }
	else if (Result.Score >= 50)  { Result.Comment = TEXT("上半身のみ！");   }
	else if (Result.Score >= 10)  { Result.Comment = TEXT("足だけ...");       }
	else                          { Result.Comment = TEXT("映ってない！");     }

	UE_LOG(LogTemp, Warning,
		TEXT("CalculatePhotoScore: Mission='%s' Coverage=%.2f Score=%d"),
		*CurrentMission.ToString(), BestCoverage, Result.Score);

	return Result;
}

// =============================================================================
// ③ CalculateZoomOffset
// =============================================================================

FVector UTomatinaFunctionLibrary::CalculateZoomOffset(
	APlayerController* PC,
	const FHitResult& HitResult,
	UCameraComponent* Camera,
	float CameraFOV)
{
	if (!PC || !Camera)
	{
		UE_LOG(LogTemp, Warning, TEXT("CalculateZoomOffset: PC または Camera が null"));
		return FVector::ZeroVector;
	}

	FVector2D ScreenPos;
	if (!PC->ProjectWorldLocationToScreen(HitResult.ImpactPoint, ScreenPos))
	{
		UE_LOG(LogTemp, Warning, TEXT("CalculateZoomOffset: ProjectWorldLocationToScreen に失敗"));
		return FVector::ZeroVector;
	}

	int32 ViewportX = 1, ViewportY = 1;
	PC->GetViewportSize(ViewportX, ViewportY);

	// 画面中心からの正規化オフセット（-0.5 〜 0.5）
	const float NormX = (ScreenPos.X / static_cast<float>(ViewportX)) - 0.5f;
	const float NormY = (ScreenPos.Y / static_cast<float>(ViewportY)) - 0.5f;

	const float Distance    = FVector::Distance(Camera->GetComponentLocation(), HitResult.ImpactPoint);
	const float HalfFOVRad  = FMath::DegreesToRadians(CameraFOV * 0.5f);
	const float Scale       = Distance * FMath::Tan(HalfFOVRad) * 2.0f;

	// ローカル空間オフセット（Y = 右、Z = 上）
	const float OffsetY =  NormX * Scale;
	const float OffsetZ = -NormY * Scale;

	UE_LOG(LogTemp, Warning, TEXT("CalculateZoomOffset: Offset Y=%.1f Z=%.1f"), OffsetY, OffsetZ);
	return FVector(0.f, OffsetY, OffsetZ);
}

// =============================================================================
// ④ GetZoomScreenCenter
// =============================================================================

FVector2D UTomatinaFunctionLibrary::GetZoomScreenCenter(
	float MainWidth,
	float PhoneWidth,
	float PhoneHeight)
{
	// Phone ビューが水平中央に配置されている場合の中心座標
	const float CenterX = MainWidth * 0.5f;
	const float CenterY = PhoneHeight * 0.5f;
	return FVector2D(CenterX, CenterY);
}

// =============================================================================
// ⑤ WorldToZoomScreen
// =============================================================================

FVector2D UTomatinaFunctionLibrary::WorldToZoomScreen(
	USceneCaptureComponent2D* ZoomCamera,
	FVector WorldLocation,
	float PhoneWidth,
	float PhoneHeight)
{
	if (!ZoomCamera)
	{
		UE_LOG(LogTemp, Warning, TEXT("WorldToZoomScreen: ZoomCamera が null"));
		return FVector2D::ZeroVector;
	}

	FMatrix ViewMatrix;
	float CotHalfFOV, Aspect;
	TomatinaInternal::BuildViewParams(ZoomCamera, PhoneWidth, PhoneHeight, ViewMatrix, CotHalfFOV, Aspect);

	const FVector LocalPos = ViewMatrix.TransformPosition(WorldLocation);
	FVector2D NDC;
	if (!TomatinaInternal::LocalToNDC(LocalPos, CotHalfFOV, Aspect, NDC))
	{
		// カメラの後方：画面外を示す大きな値を返す
		return FVector2D(-PhoneWidth, -PhoneHeight);
	}

	// NDC（-1〜1）→ Phone 画面ピクセル座標（0〜PhoneWidth/PhoneHeight）
	const float PixelX = (NDC.X * 0.5f + 0.5f) * PhoneWidth;
	const float PixelY = (NDC.Y * 0.5f + 0.5f) * PhoneHeight;

	return FVector2D(PixelX, PixelY);
}

// =============================================================================
// ⑥ ProjectZoomToMainScreen
// =============================================================================

FVector2D UTomatinaFunctionLibrary::ProjectZoomToMainScreen(
	APlayerController* PC,
	USceneCaptureComponent2D* SceneCapture,
	float MainWidth,
	float MainHeight)
{
	if (!PC || !SceneCapture)
	{
		UE_LOG(LogTemp, Warning, TEXT("ProjectZoomToMainScreen: PC または SceneCapture が null"));
		return FVector2D::ZeroVector;
	}

	// SceneCapture が向く遠点をメインカメラのスクリーン座標に投影
	const FVector CaptureLocation = SceneCapture->GetComponentLocation();
	const FVector CaptureForward  = SceneCapture->GetForwardVector();
	const FVector TargetPoint     = CaptureLocation + CaptureForward * 10000.f;

	FVector2D ScreenPos;
	if (!PC->ProjectWorldLocationToScreen(TargetPoint, ScreenPos))
	{
		return FVector2D(MainWidth * 0.5f, MainHeight * 0.5f);
	}

	return ScreenPos;
}

// =============================================================================
// ⑦ CopyZoomToPhoto
// =============================================================================

void UTomatinaFunctionLibrary::CopyZoomToPhoto(
	USceneCaptureComponent2D* ZoomCamera,
	UTextureRenderTarget2D* RT_Photo)
{
	if (!ZoomCamera)
	{
		UE_LOG(LogTemp, Warning, TEXT("CopyZoomToPhoto: ZoomCamera が null"));
		return;
	}
	if (!RT_Photo)
	{
		UE_LOG(LogTemp, Warning, TEXT("CopyZoomToPhoto: RT_Photo が null"));
		return;
	}

	// ZoomCamera の TextureTarget を RT_Photo に一時差し替えてキャプチャし、
	// 完了後に元の RT_Zoom へ戻す。RHI 操作は一切使わない。
	UTextureRenderTarget2D* OriginalTarget = ZoomCamera->TextureTarget;
	ZoomCamera->TextureTarget = RT_Photo;
	ZoomCamera->CaptureScene();
	ZoomCamera->TextureTarget = OriginalTarget;

	UE_LOG(LogTemp, Warning, TEXT("CopyZoomToPhoto: RT_Photo へキャプチャ完了"));
}

// =============================================================================
// ⑧ NormalizeScreenPosition
// =============================================================================

FVector2D UTomatinaFunctionLibrary::NormalizeScreenPosition(
	FVector2D ScreenPos,
	float ScreenWidth,
	float ScreenHeight)
{
	if (ScreenWidth <= 0.f || ScreenHeight <= 0.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("NormalizeScreenPosition: ScreenWidth/Height がゼロ以下"));
		return FVector2D::ZeroVector;
	}

	return FVector2D(ScreenPos.X / ScreenWidth, ScreenPos.Y / ScreenHeight);
}

// =============================================================================
// ⑨ DenormalizeToMainScreen
// =============================================================================

FVector2D UTomatinaFunctionLibrary::DenormalizeToMainScreen(
	FVector2D NormPos,
	float MainWidth,
	float MainHeight)
{
	return FVector2D(NormPos.X * MainWidth, NormPos.Y * MainHeight);
}

// =============================================================================
// ⑩ DenormalizeToPhoneScreen
// =============================================================================

FVector2D UTomatinaFunctionLibrary::DenormalizeToPhoneScreen(
	FVector2D NormPos,
	float MainWidth,
	float PhoneWidth,
	float PhoneHeight)
{
	// Phone 画面はメイン画面の右側（X 開始 = MainWidth）に配置されている想定
	return FVector2D(MainWidth + NormPos.X * PhoneWidth, NormPos.Y * PhoneHeight);
}
