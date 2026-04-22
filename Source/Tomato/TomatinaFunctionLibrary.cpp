// Fill out your copyright notice in the Description page of Project Settings.

#include "TomatinaFunctionLibrary.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#include "TomatinaTargetBase.h"

// -----------------------------------------------------------------------------
// CheckVisibility
// NDC 判定 + ライントレース（仕様通りの実装）
// -----------------------------------------------------------------------------
bool UTomatinaFunctionLibrary::CheckVisibility(
	USceneCaptureComponent2D* ZoomCamera,
	AActor* Target,
	FVector CheckLoc,
	float ScreenWidth,
	float ScreenHeight)
{
	UE_LOG(LogTemp, Warning, TEXT("CheckVisibility 開始"));

	if (!ZoomCamera || !Target) { return false; }
	UWorld* World = ZoomCamera->GetWorld();
	if (!World) { return false; }

	const FVector  CamLoc = ZoomCamera->GetComponentLocation();
	const FRotator CamRot = ZoomCamera->GetComponentRotation();
	const float    FOV    = ZoomCamera->FOVAngle;

	const FVector ToTarget = CheckLoc - CamLoc;
	const FVector Forward  = CamRot.Vector();
	const FVector Right    = FRotationMatrix(CamRot).GetScaledAxis(EAxis::Y);
	const FVector Up       = FRotationMatrix(CamRot).GetScaledAxis(EAxis::Z);

	const float LocalX = FVector::DotProduct(ToTarget, Forward);
	const float LocalY = FVector::DotProduct(ToTarget, Right);
	const float LocalZ = FVector::DotProduct(ToTarget, Up);

	if (LocalX <= 1.0f) { return false; }

	const float Aspect   = (ScreenHeight > 0.f) ? (ScreenWidth / ScreenHeight) : 1.f;
	const float TanHalf  = FMath::Tan(FMath::DegreesToRadians(FOV * 0.5f));

	const float ScreenX = (LocalY / LocalX) / TanHalf;
	const float ScreenY = (LocalZ / LocalX) / (TanHalf / Aspect);

	if (FMath::Abs(ScreenX) > 1.0f || FMath::Abs(ScreenY) > 1.0f)
	{
		return false;
	}

	// ── 遮蔽判定：静的メッシュ（建造物）は無視、
	//    スケルタルメッシュ／他 Pawn／"HidingProp" タグ付きは遮蔽とみなす
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(ZoomCamera->GetOwner());
	Params.AddIgnoredActor(Target);           // ターゲット自身は除外
	Params.bTraceComplex = false;

	TArray<FHitResult> Hits;
	World->LineTraceMultiByChannel(
		Hits, CamLoc, CheckLoc, ECC_Visibility, Params);

	for (const FHitResult& H : Hits)
	{
		AActor* HitActor            = H.GetActor();
		UPrimitiveComponent* HitComp = H.GetComponent();
		if (!HitActor || !HitComp) { continue; }

		// スケルタルメッシュに当たった → 遮蔽
		if (HitComp->IsA<USkeletalMeshComponent>()) { return false; }

		// 他の Pawn に当たった → 遮蔽（ターゲット以外のキャラ）
		if (HitActor->IsA<APawn>()) { return false; }

		// 「HidingProp」タグ付きアクター → 遮蔽（隠れる用の置物）
		if (HitActor->ActorHasTag(TEXT("HidingProp"))) { return false; }

		// それ以外（ワールド上の建造物の静的メッシュ等） → 無視して次へ
	}

	return true; // 最後まで遮蔽なし＝見えている
}

// -----------------------------------------------------------------------------
// CalculatePhotoScore
// Head / Root の 2 点で CheckVisibility を呼ぶ仕様
// -----------------------------------------------------------------------------
FPhotoResult UTomatinaFunctionLibrary::CalculatePhotoScore(
	USceneCaptureComponent2D* ZoomCamera,
	const TArray<ATomatinaTargetBase*>& Targets,
	FName CurrentMission,
	float ScreenWidth,
	float ScreenHeight)
{
	UE_LOG(LogTemp, Warning, TEXT("CalculatePhotoScore 開始 Mission=%s Targets=%d"),
		*CurrentMission.ToString(), Targets.Num());

	FPhotoResult Result;
	Result.Score      = 0;
	Result.BestTarget = nullptr;
	Result.Comment    = TEXT("写ってない！");

	if (!ZoomCamera)
	{
		UE_LOG(LogTemp, Warning, TEXT("CalculatePhotoScore: ZoomCamera が null"));
		return Result;
	}

	int32 BestScore = -1;
	FString BestComment = TEXT("写ってない！");
	ATomatinaTargetBase* BestActor = nullptr;

	for (ATomatinaTargetBase* Target : Targets)
	{
		if (!IsValid(Target)) { continue; }
		if (Target->MyType != CurrentMission) { continue; }

		// Head / Root 位置を取得
		FVector HeadLoc = Target->GetActorLocation() + FVector(0.f, 0.f, 100.f);
		FVector RootLoc = Target->GetActorLocation();

		// SkeletalMesh にソケットがあればそちらを優先
		if (Target->MeshComp)
		{
			if (Target->MeshComp->DoesSocketExist(TEXT("HeadSocket")))
			{
				HeadLoc = Target->MeshComp->GetSocketLocation(TEXT("HeadSocket"));
			}
			if (Target->MeshComp->DoesSocketExist(TEXT("RootSocket")))
			{
				RootLoc = Target->MeshComp->GetSocketLocation(TEXT("RootSocket"));
			}
		}

		const bool bHeadVisible = CheckVisibility(ZoomCamera, Target, HeadLoc, ScreenWidth, ScreenHeight);
		const bool bRootVisible = CheckVisibility(ZoomCamera, Target, RootLoc, ScreenWidth, ScreenHeight);

		int32   ThisScore   = 0;
		FString ThisComment = TEXT("写ってない！");

		if (bHeadVisible && bRootVisible)      { ThisScore = 100; ThisComment = TEXT("全身バッチリ！"); }
		else if (bHeadVisible)                 { ThisScore =  50; ThisComment = TEXT("上半身だけ撮れた"); }
		else if (bRootVisible)                 { ThisScore =  10; ThisComment = TEXT("足だけ撮れた"); }
		else                                   { ThisScore =   0; ThisComment = TEXT("写ってない！"); }

		UE_LOG(LogTemp, Warning, TEXT(" Target=%s Head=%d Root=%d Score=%d"),
			*Target->GetName(), bHeadVisible ? 1 : 0, bRootVisible ? 1 : 0, ThisScore);

		if (ThisScore > BestScore)
		{
			BestScore   = ThisScore;
			BestComment = ThisComment;
			BestActor   = Target;
		}
	}

	if (BestScore < 0) { BestScore = 0; }

	// ── ポーズボーナス ────────────────────────────────────────────────
	// ポーズ中のターゲットを撮影できていたらスコアを倍率で加算
	if (BestActor && BestActor->bEnablePose && BestActor->bIsPosing && BestScore > 0)
	{
		const int32 BoostedScore = FMath::RoundToInt(BestScore * BestActor->PoseScoreMultiplier);
		UE_LOG(LogTemp, Warning, TEXT(" ポーズボーナス適用: %d → %d (x%.2f)"),
			BestScore, BoostedScore, BestActor->PoseScoreMultiplier);
		BestScore = BoostedScore;
		BestComment = FString::Printf(TEXT("%s ポーズボーナス！"), *BestComment);
	}

	Result.Score      = BestScore;
	Result.Comment    = BestComment;
	Result.BestTarget = (BestScore > 0) ? BestActor : nullptr;

	UE_LOG(LogTemp, Warning, TEXT("CalculatePhotoScore 結果: Score=%d BestTarget=%s"),
		Result.Score,
		Result.BestTarget ? *Result.BestTarget->GetName() : TEXT("none"));

	return Result;
}

// -----------------------------------------------------------------------------
// CalculateZoomOffset
// クリック位置をズームカメラ中心に持ってくるための相対オフセット
// -----------------------------------------------------------------------------
FVector UTomatinaFunctionLibrary::CalculateZoomOffset(
	APlayerController* PC,
	const FHitResult& HitResult,
	UCameraComponent* Camera,
	float CameraFOV)
{
	UE_LOG(LogTemp, Warning, TEXT("CalculateZoomOffset 開始"));

	if (!PC || !Camera)
	{
		UE_LOG(LogTemp, Warning, TEXT("CalculateZoomOffset: PC または Camera が null"));
		return FVector::ZeroVector;
	}

	FVector2D ScreenPos;
	if (!PC->ProjectWorldLocationToScreen(HitResult.ImpactPoint, ScreenPos))
	{
		UE_LOG(LogTemp, Warning, TEXT("CalculateZoomOffset: ProjectWorldLocationToScreen 失敗"));
		return FVector::ZeroVector;
	}

	int32 ViewportX = 1, ViewportY = 1;
	PC->GetViewportSize(ViewportX, ViewportY);

	const float NormX = (ScreenPos.X / static_cast<float>(ViewportX)) - 0.5f;
	const float NormY = (ScreenPos.Y / static_cast<float>(ViewportY)) - 0.5f;

	const float Distance   = FVector::Distance(Camera->GetComponentLocation(), HitResult.ImpactPoint);
	const float HalfFOVRad = FMath::DegreesToRadians(CameraFOV * 0.5f);
	const float Scale      = Distance * FMath::Tan(HalfFOVRad) * 2.0f;

	const float OffsetY =  NormX * Scale;
	const float OffsetZ = -NormY * Scale;

	return FVector(0.f, OffsetY, OffsetZ);
}

// -----------------------------------------------------------------------------
// GetZoomScreenCenter
// iPhone 画面がメイン画面の右側に連結されている前提で、
// iPhone 領域の中心（＝ズーム完了時にカーソルを飛ばす座標）を返す
// -----------------------------------------------------------------------------
FVector2D UTomatinaFunctionLibrary::GetZoomScreenCenter(
	float MainWidth,
	float PhoneWidth,
	float PhoneHeight)
{
	UE_LOG(LogTemp, Warning, TEXT("GetZoomScreenCenter 呼び出し"));
	return FVector2D(MainWidth + PhoneWidth * 0.5f, PhoneHeight * 0.5f);
}

// -----------------------------------------------------------------------------
// ProjectZoomToMainScreen
// SceneCapture の視線方向をメイン画面のピクセル座標に投影
// -----------------------------------------------------------------------------
FVector2D UTomatinaFunctionLibrary::ProjectZoomToMainScreen(
	APlayerController* PC,
	USceneCaptureComponent2D* SceneCapture,
	float MainWidth,
	float MainHeight)
{
	if (!PC || !SceneCapture)
	{
		UE_LOG(LogTemp, Warning, TEXT("ProjectZoomToMainScreen: 引数が null"));
		return FVector2D(MainWidth * 0.5f, MainHeight * 0.5f);
	}

	const FVector Loc  = SceneCapture->GetComponentLocation();
	const FVector Fwd  = SceneCapture->GetForwardVector();
	const FVector Far  = Loc + Fwd * 10000.f;

	FVector2D ScreenPos;
	if (!PC->ProjectWorldLocationToScreen(Far, ScreenPos))
	{
		return FVector2D(MainWidth * 0.5f, MainHeight * 0.5f);
	}
	return ScreenPos;
}

// -----------------------------------------------------------------------------
// CopyZoomToPhoto
// TextureTarget 差し替え方式（RHI 非使用）— 仕様通り
// -----------------------------------------------------------------------------
void UTomatinaFunctionLibrary::CopyZoomToPhoto(
	USceneCaptureComponent2D* ZoomCamera,
	UTextureRenderTarget2D* PhotoTarget)
{
	UE_LOG(LogTemp, Warning, TEXT("CopyZoomToPhoto 開始"));

	if (!ZoomCamera || !PhotoTarget)
	{
		UE_LOG(LogTemp, Warning, TEXT("CopyZoomToPhoto: 引数が null"));
		return;
	}

	UTextureRenderTarget2D* Original = ZoomCamera->TextureTarget;
	ZoomCamera->TextureTarget = PhotoTarget;
	ZoomCamera->CaptureScene();
	ZoomCamera->TextureTarget = Original;
}
